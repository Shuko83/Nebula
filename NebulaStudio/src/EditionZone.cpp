// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "EditionZone.h"

#include <QAbstractSpinBox>
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QToolButton>
#include <QVBoxLayout>
#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <utility>

#include "ComponentFinder.h"
#include "ComponentItem.h"
#include "CopiedInstances.h"
#include "EditionScene.h"
#include "EditionView.h"
#include "Messages.h"
#include "Nebula/Component/ComponentFactory.h"
#include "Nebula/Standard/IHistory.h"
#include "PanelWidgets.h"
#include "ProjectFiles.h"
#include "SceneOverview.h"
#include "Studio.h"
#include "Theme.h"

namespace
{

// One of the choices of a submenu of the View menu, and what it chooses.
template <typename Value> struct Choice
{
    const char* name; // Translated in the context of EditionZone
    Value value;
};

// The ways of drawing the links.
constexpr std::array LinkStyles{
    Choice<LinkStyle>{.name = QT_TRANSLATE_NOOP("EditionZone", "&Curves"), .value = LinkStyle::Curved},
    Choice<LinkStyle>{.name = QT_TRANSLATE_NOOP("EditionZone", "&Straight Lines"), .value = LinkStyle::Straight},
    Choice<LinkStyle>{.name = QT_TRANSLATE_NOOP("EditionZone", "&Right Angles"), .value = LinkStyle::RightAngled},
};

// The color schemes of the theme: the one of the system first.
constexpr std::array ColorSchemes{
    Choice<Qt::ColorScheme>{.name = QT_TRANSLATE_NOOP("EditionZone", "&System"), .value = Qt::ColorScheme::Unknown},
    Choice<Qt::ColorScheme>{.name = QT_TRANSLATE_NOOP("EditionZone", "&Light"), .value = Qt::ColorScheme::Light},
    Choice<Qt::ColorScheme>{.name = QT_TRANSLATE_NOOP("EditionZone", "&Dark"), .value = Qt::ColorScheme::Dark},
};

// Adds to group the action of choice, what it chooses in its data; checked when it is current.
template <typename Value> void addChoice(QActionGroup& group, const Choice<Value>& choice, Value current)
{
    QAction* action = group.addAction(EditionZone::tr(choice.name));
    action->setData(QVariant::fromValue(choice.value));
    action->setCheckable(true);
    action->setChecked(choice.value == current);
}

// Adds to menu the submenu title of choices, see addChoice(), and returns their group, which tells the one triggered:
// one of them is checked at a time.
template <typename Value, std::size_t Count>
const QActionGroup& addChoiceMenu(QMenu& menu, const QString& title, const std::array<Choice<Value>, Count>& choices,
                                  Value current)
{
    QMenu* submenu = menu.addMenu(title);
    auto* group = new QActionGroup(submenu); // Exclusive
    for(const Choice<Value>& choice : choices)
    {
        addChoice(*group, choice, current);
    }
    submenu->addActions(group->actions());
    return *group;
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

// The shortcuts of Undo: those of the platform, Ctrl+Z first.
QList<QKeySequence> undoShortcuts()
{
    return QKeySequence::keyBindings(QKeySequence::Undo);
}

// The shortcuts of Redo: Ctrl+Shift+Z first, whatever the platform, then its own, Ctrl+Y under Windows say.
QList<QKeySequence> redoShortcuts()
{
    QList<QKeySequence> shortcuts{QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z)};
    for(const QKeySequence& shortcut : QKeySequence::keyBindings(QKeySequence::Redo))
    {
        if(!shortcuts.contains(shortcut))
        {
            shortcuts.append(shortcut);
        }
    }
    return shortcuts;
}

// Whether widget is where text is typed: a field, a spin box, or a text.
bool takesText(const QWidget* widget)
{
    return qobject_cast<const QLineEdit*>(widget) != nullptr ||
           qobject_cast<const QAbstractSpinBox*>(widget) != nullptr ||
           qobject_cast<const QTextEdit*>(widget) != nullptr || qobject_cast<const QPlainTextEdit*>(widget) != nullptr;
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
    createEditActions();
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

void EditionZone::createEditActions()
{
    m_undoAction = createEditAction(tr("&Undo"), undoShortcuts(), &EditionZone::undo);
    m_undoAction->setToolTip(tr("Undo the last edit of the application, or of a galaxy opened"));
    m_redoAction = createEditAction(tr("&Redo"), redoShortcuts(), &EditionZone::redo);
    m_redoAction->setToolTip(tr("Do again the last edit undone"));
    createClipboardActions();
    refreshEditActions(); // Undo and Redo disabled until it has a history
}

void EditionZone::createClipboardActions()
{
    m_cutAction = createSceneAction(tr("Cu&t"), QKeySequence::Cut, &EditionScene::cut);
    m_copyAction = createSceneAction(tr("&Copy"), QKeySequence::Copy, &EditionScene::copy);
    m_pasteAction = createSceneAction(tr("&Paste"), QKeySequence::Paste, &EditionScene::paste);
    m_duplicateAction = createSceneAction(tr("&Duplicate"), QKeySequence::UnknownKey, &EditionScene::duplicate);
    m_duplicateAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D)); // None of the platform
    describeClipboardActions();
    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, this, &EditionZone::refreshClipboardActions);
}

void EditionZone::describeClipboardActions()
{
    m_cutAction->setToolTip(tr("Copy the instances selected, then remove what is selected"));
    m_copyAction->setToolTip(tr("Copy the instances selected, with the links between them"));
    m_pasteAction->setToolTip(tr("Place the instances copied, a little further than where they were copied"));
    m_duplicateAction->setToolTip(tr("Place a copy of the instances selected, without changing what is copied"));
}

QAction* EditionZone::createEditAction(const QString& text, const QList<QKeySequence>& shortcuts,
                                       void (EditionZone::*step)())
{
    auto* action = new QAction(text, this);
    action->setShortcuts(shortcuts);
    connect(action, &QAction::triggered, this, step);
    addAction(action); // Its shortcuts work in the window of the zone, with or without a menu
    return action;
}

QAction* EditionZone::createSceneAction(const QString& text, QKeySequence::StandardKey key, void (EditionScene::*act)())
{
    auto* action = new QAction(text, this);
    action->setShortcuts(key);
    connect(action, &QAction::triggered, this, [this, act] { editShownScene(act); });
    addAction(action); // Its shortcuts work in the window of the zone, with or without a menu
    return action;
}

void EditionZone::editShownScene(void (EditionScene::*act)())
{
    // Not while a field takes the keys: Ctrl+D is none of its own, as Ctrl+C and Ctrl+V are.
    if(canEdit())
    {
        (shownScene().*act)();
    }
}

void EditionZone::addEditActions(QMenu& menu)
{
    menu.addAction(m_undoAction);
    menu.addAction(m_redoAction);
    menu.addSeparator();
    menu.addActions({m_cutAction, m_copyAction, m_pasteAction, m_duplicateAction});
    // Whether the history does them is not told: it is asked each time they are shown.
    connect(&menu, &QMenu::aboutToShow, this, &EditionZone::refreshEditActions);
}

void EditionZone::setHistory(Nebula::IHistory* history)
{
    m_history = history;
    m_scene->setHistory(history);
    for(EditionScene* scene : std::as_const(m_galaxyScenes))
    {
        scene->setHistory(history);
    }
    refreshEditActions();
}

void EditionZone::edit(const QString& what, const std::function<void()>& change)
{
    shownScene().edit(what, change);
}

void EditionZone::undo()
{
    if(canStep())
    {
        m_history->undo(); // Nothing when there is nothing to undo
    }
}

void EditionZone::redo()
{
    if(canStep())
    {
        m_history->redo();
    }
}

void EditionZone::clearHistory()
{
    if(m_history != nullptr)
    {
        m_history->clear();
    }
}

void EditionZone::refreshEditActions()
{
    const bool editable = m_history != nullptr && !m_studio.application().isRunning();
    m_undoAction->setEnabled(editable && m_history->isUndoActive());
    m_redoAction->setEnabled(editable && m_history->isRedoActive());
    refreshClipboardActions();
}

void EditionZone::refreshClipboardActions()
{
    const bool editable = !m_studio.application().isRunning();
    const bool copiable = editable && shownScene().canCopy();
    m_cutAction->setEnabled(copiable);
    m_copyAction->setEnabled(copiable);
    m_duplicateAction->setEnabled(copiable);
    m_pasteAction->setEnabled(editable && CopiedInstances::isInClipboard());
}

bool EditionZone::canStep() const
{
    return m_history != nullptr && canEdit();
}

bool EditionZone::canEdit() const
{
    return !m_studio.application().isRunning() && !shownScene().isBusy() && !isTyping();
}

bool EditionZone::isTyping() const
{
    // A field takes Ctrl+Z, but not Ctrl+Shift+Z under Windows, where its Redo is Ctrl+Y.
    return takesText(QApplication::focusWidget()) || shownScene().focusItem() != nullptr;
}

EditionScene& EditionZone::applicationScene() const
{
    return *m_scene;
}

Nebula::Component* EditionZone::selectedComponent() const
{
    return shownScene().selectedComponent();
}

Nebula::SettingsFile* EditionZone::selectedSettingsFile() const
{
    return shownScene().selectedSettingsFile();
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
    addThemeMenu(menu);
}

void EditionZone::addLinkStyleMenu(QMenu& menu)
{
    const QActionGroup& styles = addChoiceMenu(menu, tr("&Links"), LinkStyles, m_studio.linkStyle());
    connect(&styles, &QActionGroup::triggered, this,
            [this](const QAction* action) { m_studio.setLinkStyle(action->data().value<LinkStyle>()); });
}

void EditionZone::addThemeMenu(QMenu& menu)
{
    const QActionGroup& schemes = addChoiceMenu(menu, tr("&Theme"), ColorSchemes, m_studio.colorScheme());
    connect(&schemes, &QActionGroup::triggered, this,
            [this](const QAction* action) { m_studio.setColorScheme(action->data().value<Qt::ColorScheme>()); });
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

SceneOverview* EditionZone::createOverview() const
{
    return new SceneOverview(*m_view);
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
    refreshEditActions();
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
    connect(&scene, &EditionScene::instancesPlaced, this, [this] { m_studio.refreshPlugins(); });
}

void EditionZone::tell(const QString& message)
{
    m_studio.tell(message);
}

void EditionZone::showSelected(const EditionScene& scene)
{
    if(m_view->scene() == &scene)
    {
        emit selectionChanged(scene.selectedComponent(), scene.selectedSettingsFile());
        refreshClipboardActions(); // What is copied
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
    clearHistory(); // Its steps could no longer be undone without them, nor done again
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
    scene->setHistory(m_history);
    // After the scene itself, which lets go of what goes then: for as long as it is open.
    connect(&galaxy, &Nebula::Galaxy::aboutToBeReloaded, scene, [this, scene] { leaveBelow(*scene); });
    m_galaxyScenes.append(scene);
    showDepth(m_galaxyScenes.size());
}

void EditionZone::leaveBelow(const EditionScene& scene)
{
    const qsizetype depth = m_galaxyScenes.indexOf(&scene) + 1;
    if(depth > 0 && depth < m_galaxyScenes.size())
    {
        showDepth(depth);
    }
}

void EditionZone::showDepth(qsizetype depth)
{
    EditionScene* shown = depth == 0 ? m_scene : m_galaxyScenes.at(depth - 1);
    m_view->showScene(shown); // Before those further down go, its overview with it
    closeGalaxyScenes(depth);
    emit selectionChanged(shown->selectedComponent(), shown->selectedSettingsFile());
    m_saveGalaxyAction->setEnabled(depth > 0 && !m_studio.application().isRunning());
    refreshClipboardActions();
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
