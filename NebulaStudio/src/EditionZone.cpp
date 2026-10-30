// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "EditionZone.h"

#include <QAction>
#include <QActionGroup>
#include <QDir>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QToolButton>
#include <QVBoxLayout>
#include <algorithm>
#include <array>
#include <memory>
#include <utility>

#include "ComponentFinder.h"
#include "ComponentItem.h"
#include "EditionScene.h"
#include "EditionView.h"
#include "Messages.h"
#include "Nebula/Component/ComponentFactory.h"
#include "PanelWidgets.h"
#include "ProjectFiles.h"
#include "Studio.h"
#include "Theme.h"

namespace
{

// A way of drawing the links, as the View menu offers it.
struct LinkStyleEntry
{
    const char* name; // Translated in the context of EditionZone
    LinkStyle style;
};

constexpr std::array LinkStyles{
    LinkStyleEntry{.name = QT_TRANSLATE_NOOP("EditionZone", "&Curves"), .style = LinkStyle::Curved},
    LinkStyleEntry{.name = QT_TRANSLATE_NOOP("EditionZone", "&Straight Lines"), .style = LinkStyle::Straight},
    LinkStyleEntry{.name = QT_TRANSLATE_NOOP("EditionZone", "&Right Angles"), .style = LinkStyle::RightAngled},
};

// Adds to group the action that draws the links as entry tells, its style in its data; checked when it is current, the
// style they are drawn in.
void addLinkStyleAction(QActionGroup& group, const LinkStyleEntry& entry, LinkStyle current)
{
    QAction* action = group.addAction(EditionZone::tr(entry.name));
    action->setData(QVariant::fromValue(entry.style));
    action->setCheckable(true);
    action->setChecked(entry.style == current);
}

// The view of scene, the edition zone.
EditionView* createEditionView(EditionScene& scene)
{
    auto* view = new EditionView(&scene);
    view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    view->setRenderHint(QPainter::Antialiasing);
    view->setDragMode(QGraphicsView::RubberBandDrag);
    view->setAcceptDrops(true);
    return view;
}

// A flat button that tells its text beside its icon.
QToolButton* createFlatButton()
{
    auto* button = new QToolButton;
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    button->setAutoRaise(true);
    return button;
}

// text, in the font of the titles.
QLabel* createTitle(const QString& text)
{
    auto* title = new QLabel(text);
    title->setFont(titleFont(title->font()));
    return title;
}

// Empties layout. Its widgets go later: one of them may be the button being clicked.
void clearLayout(QLayout& layout)
{
    while(QLayoutItem* item = layout.takeAt(0))
    {
        if(QWidget* widget = item->widget())
        {
            widget->hide();
            widget->deleteLater();
        }
        delete item;
    }
}

} // namespace

EditionZone::EditionZone(Studio& studio, QWidget* parent)
    : QWidget(parent), m_studio(studio),
      m_scene(new EditionScene(studio.application(), Nebula::ComponentFactory::instance(), this)), m_path(createRow()),
      m_view(createEditionView(*m_scene)), m_finder(new ComponentFinder(*m_view))
{
    createSaveGalaxyAction();
    followScene(*m_scene);
    followStudio();
    layOut();
}

EditionZone::~EditionZone()
{
    // The scenes are no longer listened to: deleting a selected item changes their selection.
    closeGalaxyScenes(0); // They show the components of the galaxies of the application
    closeScene(*m_scene);
}

void EditionZone::layOut()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->addWidget(createBar());
    layout->addWidget(m_view);
}

QWidget* EditionZone::createBar()
{
    // Where the galaxy shown is: the application, then the galaxies it is in, each a button that shows it again.
    m_path->hide();
    connect(m_finder, &ComponentFinder::told, this, &EditionZone::tell);
    QWidget* bar = createRow();
    auto* layout = static_cast<QHBoxLayout*>(bar->layout());
    layout->addWidget(m_path, 1);
    layout->addWidget(m_finder, 0, Qt::AlignRight); // On the right, alone there while the application is shown
    return bar;
}

void EditionZone::createSaveGalaxyAction()
{
    m_saveGalaxyAction = new QAction(icon(GlossaryTerm::Galaxy), tr("Save &Galaxy"), this);
    m_saveGalaxyAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_S));
    m_saveGalaxyAction->setToolTip(
        tr("Save the galaxy shown to its file: each of its instances has what it holds then"));
    m_saveGalaxyAction->setEnabled(false); // Until a galaxy is shown
    connect(m_saveGalaxyAction, &QAction::triggered, this, &EditionZone::saveGalaxy);
    addAction(m_saveGalaxyAction); // Its shortcut works in the window of the zone, with or without a menu
}

EditionScene& EditionZone::applicationScene() const
{
    return *m_scene;
}

Nebula::Component* EditionZone::selectedComponent() const
{
    return shownScene().selectedComponent();
}

QAction* EditionZone::saveGalaxyAction() const
{
    return m_saveGalaxyAction;
}

void EditionZone::addViewActions(QMenu& menu)
{
    menu.addAction(tr("&Find Instances"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F), this,
                   [this] { focusToReplace(*m_finder); });
    menu.addSeparator();
    menu.addAction(tr("Zoom &In"), QKeySequence::ZoomIn, this, [this] { m_view->zoomIn(); });
    menu.addAction(tr("Zoom &Out"), QKeySequence::ZoomOut, this, [this] { m_view->zoomOut(); });
    menu.addAction(tr("&Actual Size"), QKeySequence(Qt::CTRL | Qt::Key_0), this, [this] { m_view->resetZoom(); });
    menu.addSeparator();
    addLinkStyleMenu(menu);
}

void EditionZone::addLinkStyleMenu(QMenu& menu)
{
    auto* group = new QActionGroup(&menu); // Exclusive: one style at a time
    connect(group, &QActionGroup::triggered, this,
            [this](const QAction* action) { m_studio.setLinkStyle(action->data().value<LinkStyle>()); });
    for(const LinkStyleEntry& entry : LinkStyles)
    {
        addLinkStyleAction(*group, entry, m_studio.linkStyle());
    }
    menu.addMenu(tr("&Links"))->addActions(group->actions());
}

void EditionZone::addRunActions(QMenu& menu)
{
    Nebula::Universe& application = m_studio.application();
    disableWhileRunning(application,
                        *menu.addAction(tr("&Start"), QKeySequence(Qt::Key_F5), this, &EditionZone::start));
    QAction* stop =
        menu.addAction(tr("S&top"), QKeySequence(Qt::SHIFT | Qt::Key_F5), &application, &Nebula::Universe::stop);
    stop->setEnabled(application.isRunning());
    connect(&application, &Nebula::Universe::runningChanged, stop, &QAction::setEnabled);
}

QLabel* EditionZone::createZoomLabel() const
{
    auto* label = new QLabel;
    const auto showZoom = [label](qreal zoom) { label->setText(tr("Zoom %1%").arg(qRound(zoom * 100))); };
    showZoom(m_view->zoom());
    connect(m_view, &EditionView::zoomChanged, label, showZoom);
    return label;
}

void EditionZone::followStudio()
{
    const Nebula::Universe& application = m_studio.application();
    connect(&application, &Nebula::Universe::runningChanged, this, &EditionZone::followRunning);
    connect(&application, &Nebula::Universe::componentsChanged, this, &EditionZone::leaveRemovedGalaxy);
    connect(&m_studio, &Studio::linkStyleChanged, this, &EditionZone::drawLinks);
}

void EditionZone::followRunning(bool running)
{
    m_saveGalaxyAction->setEnabled(!running && !m_galaxyScenes.isEmpty());
}

void EditionZone::leaveRemovedGalaxy()
{
    const Nebula::Galaxy* shown = m_galaxyScenes.isEmpty() ? nullptr : m_galaxyScenes.first()->galaxy();
    if(shown != nullptr && std::ranges::none_of(m_studio.application().components(),
                                                [shown](const std::unique_ptr<Nebula::Component>& component)
                                                { return component.get() == shown; }))
    {
        showDepth(0);
    }
}

void EditionZone::drawLinks(LinkStyle style)
{
    m_scene->setLinkStyle(style);
    for(EditionScene* scene : std::as_const(m_galaxyScenes))
    {
        scene->setLinkStyle(style);
    }
}

void EditionZone::followScene(EditionScene& scene)
{
    scene.setLinkStyle(m_studio.linkStyle());
    connect(&scene, &QGraphicsScene::selectionChanged, this, [this, shown = &scene] { showSelected(*shown); });
    // In the status bar rather than in a box, which would open in the middle of the click that ends an edition.
    connect(&scene, &EditionScene::refused, this, &EditionZone::tell);
    connect(&scene, &EditionScene::told, this, &EditionZone::tell);
    connect(&scene, &EditionScene::galaxyOpened, this, &EditionZone::enterGalaxy);
    // The plugins it loaded, and their components.
    connect(&scene, &EditionScene::galaxyPlaced, this, [this] { m_studio.refreshPlugins(); });
}

void EditionZone::tell(const QString& message)
{
    m_studio.tell(message);
}

void EditionZone::showSelected(const EditionScene& scene)
{
    if(m_view->scene() == &scene)
    {
        emit selectionChanged(scene.selectedComponent());
    }
}

void EditionZone::showComponent(const QString& name)
{
    // Where the components of the application are.
    if(leaveGalaxies(0))
    {
        const Nebula::Component* component = m_studio.application().component(name);
        centerOnItem(component != nullptr ? m_scene->select(*component) : nullptr);
    }
}

void EditionZone::centerOnItem(const ComponentItem* item)
{
    if(item != nullptr)
    {
        m_view->centerOn(item);
        m_view->setFocus();
    }
}

void EditionZone::remove(const QList<Nebula::Component*>& components)
{
    m_scene->remove(components);
}

void EditionZone::start()
{
    // The title of a zone, or the name of a component, is no longer edited: what was typed is taken before the
    // application is no longer edited.
    shownScene().setFocusItem(nullptr);
    if(!m_studio.application().start())
    {
        warn(this, tr("Start"), tr("The application cannot be started:\n%1").arg(m_studio.application().errorString()));
    }
}

EditionScene& EditionZone::shownScene() const
{
    return m_galaxyScenes.isEmpty() ? *m_scene : *m_galaxyScenes.constLast();
}

void EditionZone::enterGalaxy(Nebula::Galaxy& galaxy)
{
    auto* scene = new EditionScene(m_studio.application(), Nebula::ComponentFactory::instance(), galaxy, this);
    followScene(*scene);
    m_galaxyScenes.append(scene);
    showDepth(m_galaxyScenes.size());
}

void EditionZone::showDepth(qsizetype depth)
{
    EditionScene* shown = depth == 0 ? m_scene : m_galaxyScenes.at(depth - 1);
    m_view->setScene(shown); // Before those further down go
    closeGalaxyScenes(depth);
    emit selectionChanged(shown->selectedComponent());
    m_saveGalaxyAction->setEnabled(depth > 0 && !m_studio.application().isRunning());
    refreshPath();
}

void EditionZone::closeGalaxyScenes(qsizetype depth)
{
    while(m_galaxyScenes.size() > depth)
    {
        closeScene(*m_galaxyScenes.takeLast());
    }
}

void EditionZone::closeScene(EditionScene& scene)
{
    scene.disconnect(this);
    delete &scene;
}

bool EditionZone::leaveGalaxies(qsizetype depth, bool closing)
{
    while(m_galaxyScenes.size() > depth)
    {
        if(!leaveLastGalaxy(closing))
        {
            return false;
        }
    }
    showDepth(depth);
    return true;
}

bool EditionZone::leaveLastGalaxy(bool closing)
{
    EditionScene& scene = *m_galaxyScenes.constLast();
    scene.setFocusItem(nullptr); // What was typed in a name is taken before what changed is told
    const QMessageBox::StandardButton choice = leaveChoice(scene, closing);
    if(choice != QMessageBox::Cancel)
    {
        Nebula::Galaxy& galaxy = *scene.galaxy();
        showDepth(m_galaxyScenes.size() - 1); // Before it is read again: its scene shows what goes then
        readAgain(galaxy, choice);
    }
    return choice != QMessageBox::Cancel;
}

QMessageBox::StandardButton EditionZone::leaveChoice(const EditionScene& scene, bool closing)
{
    if(!scene.isModified())
    {
        return QMessageBox::NoButton;
    }
    const QMessageBox::StandardButton answer = askToLeave(*scene.galaxy(), closing);
    return answer != QMessageBox::Save || writeGalaxy(scene) ? answer : QMessageBox::Cancel;
}

QMessageBox::StandardButton EditionZone::askToLeave(const Nebula::Galaxy& galaxy, bool closing)
{
    showDepth(m_galaxyScenes.size()); // The one asked about
    if(m_studio.application().isRunning() && !closing)
    {
        tell(tr("%1 has been modified: stop the application to save it, or to give its changes up")
                 .arg(galaxy.objectName()));
        return QMessageBox::Cancel;
    }
    return askToSave(this, tr("The galaxy %1 has been modified.\nDo you want to save it to %2? Each of its instances "
                              "is read again from it then.")
                               .arg(galaxy.objectName(), QDir::toNativeSeparators(galaxy.filePath())));
}

void EditionZone::readAgain(Nebula::Galaxy& galaxy, QMessageBox::StandardButton choice)
{
    if(choice == QMessageBox::Save && !m_studio.application().isRunning())
    {
        readInstancesAgain(galaxy.filePath());
    }
    else if(choice == QMessageBox::Discard && !m_studio.application().isRunning())
    {
        readGalaxyAgain(galaxy);
    }
}

void EditionZone::readGalaxyAgain(Nebula::Galaxy& galaxy)
{
    const QStringList problems = m_studio.application().reloadGalaxy(galaxy);
    if(!problems.isEmpty())
    {
        warn(this, tr("NebulaStudio"),
             tr("%1 could not be read again as its file holds it:\n%2").arg(galaxy.objectName(), problems.join(u'\n')));
    }
}

void EditionZone::saveGalaxy()
{
    if(m_galaxyScenes.isEmpty() || m_studio.application().isRunning())
    {
        return;
    }
    EditionScene& scene = *m_galaxyScenes.constLast();
    scene.setFocusItem(nullptr); // What was typed in a name is taken before it is saved
    if(writeGalaxy(scene))
    {
        readInstancesAgain(scene.galaxy()->filePath());
    }
}

bool EditionZone::writeGalaxy(const EditionScene& scene)
{
    return writeProjectFile(this, tr("Save galaxy"), scene.galaxy()->filePath(), scene.save());
}

void EditionZone::readInstancesAgain(const QString& filePath)
{
    const QStringList problems = reloadInstances(filePath);
    m_studio.refreshGalaxies(); // What the list of galaxies tells of it, at once
    if(problems.isEmpty())
    {
        tell(tr("%1 is saved: its instances have what it holds now").arg(QDir::toNativeSeparators(filePath)));
        return;
    }
    warn(this, tr("Save galaxy"),
         tr("%1 is saved, but its instances could not keep everything they had:\n%2")
             .arg(QDir::toNativeSeparators(filePath), problems.join(u'\n')));
}

QStringList EditionZone::reloadInstances(const QString& filePath)
{
    QStringList problems;
    const QList<Nebula::Galaxy*> instances = m_studio.application().galaxies(filePath);
    for(Nebula::Galaxy* instance : instances)
    {
        problems.append(m_studio.application().reloadGalaxy(*instance));
    }
    return problems;
}

void EditionZone::refreshPath()
{
    clearLayout(*m_path->layout());
    m_path->setVisible(!m_galaxyScenes.isEmpty());
    if(m_galaxyScenes.isEmpty())
    {
        return;
    }
    addPathLevels();
    QToolButton* save = createFlatButton(); // Next to the path to the galaxy it saves
    save->setDefaultAction(m_saveGalaxyAction);
    m_path->layout()->addWidget(save);
}

void EditionZone::addPathLevels()
{
    addPathLevel(0, icon(GlossaryTerm::Universe), tr("Application"));
    for(qsizetype depth = 1; depth <= m_galaxyScenes.size(); ++depth)
    {
        addPathLevel(depth, icon(GlossaryTerm::Galaxy), m_galaxyScenes.at(depth - 1)->galaxy()->objectName());
    }
    static_cast<QHBoxLayout*>(m_path->layout())->addStretch();
}

void EditionZone::addPathLevel(qsizetype depth, const QIcon& levelIcon, const QString& name)
{
    QLayout* path = m_path->layout();
    if(depth == m_galaxyScenes.size())
    {
        path->addWidget(createTitle(name)); // The one shown
        return;
    }
    path->addWidget(createLevelButton(depth, levelIcon, name));
    path->addWidget(new QLabel(QStringLiteral("›"))); // ›
}

QToolButton* EditionZone::createLevelButton(qsizetype depth, const QIcon& levelIcon, const QString& name)
{
    QToolButton* button = createFlatButton();
    button->setText(name);
    button->setIcon(levelIcon);
    connect(button, &QToolButton::clicked, this, [this, depth] { leaveGalaxies(depth); });
    return button;
}
