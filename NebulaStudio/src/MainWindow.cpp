// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "MainWindow.h"

#include <QAction>
#include <QActionGroup>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsView>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>
#include <QShortcut>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>

#include "ComponentFinder.h"
#include "ComponentItem.h"
#include "ComponentModel.h"
#include "ComponentViewer.h"
#include "EditionScene.h"
#include "EditionView.h"
#include "Galaxy/GalaxyModel.h"
#include "Galaxy/GalaxyViewer.h"
#include "Nebula/Component/ComponentFactory.h"
#include "NebulaStudio_info.h"
#include "PluginModel.h"
#include "PluginViewer.h"
#include "ProblemList.h"
#include "PropertyEditor.h"
#include "Theme.h"

namespace
{

// How long the status bar tells what the edition zone refused or did, in milliseconds.
constexpr int MessageDuration = 8000;

// What the components can be searched by: a column of ComponentModel, and the role of its data the text is looked
// for in. The description of a component is its tool tip, in every column.
struct SearchField
{
    const char* name; // Translated in the context of MainWindow
    int column;
    int role;
};

constexpr std::array SearchFields{
    SearchField{QT_TRANSLATE_NOOP("MainWindow", "Name"), ComponentModel::Name, Qt::DisplayRole},
    SearchField{QT_TRANSLATE_NOOP("MainWindow", "Description"), ComponentModel::Name, Qt::ToolTipRole},
    SearchField{QT_TRANSLATE_NOOP("MainWindow", "Category"), ComponentModel::Category, Qt::DisplayRole},
};

// Gives field the keys with its text selected, even when it already has them: what is typed replaces the last search.
void focusToReplace(QLineEdit& field)
{
    field.setFocus(Qt::ShortcutFocusReason);
    field.selectAll();
}

// Has filter look for the text it is given where the one of SearchFields at index tells.
void searchBy(QSortFilterProxyModel& filter, int index)
{
    const SearchField& field = SearchFields.at(static_cast<std::size_t>(index));
    filter.setFilterKeyColumn(field.column);
    filter.setFilterRole(field.role);
}

// A way of drawing the links, as the View menu offers it.
struct LinkStyleEntry
{
    const char* name; // Translated in the context of MainWindow
    LinkStyle style;
};

constexpr std::array LinkStyles{
    LinkStyleEntry{.name = QT_TRANSLATE_NOOP("MainWindow", "&Curves"), .style = LinkStyle::Curved},
    LinkStyleEntry{.name = QT_TRANSLATE_NOOP("MainWindow", "&Straight Lines"), .style = LinkStyle::Straight},
    LinkStyleEntry{.name = QT_TRANSLATE_NOOP("MainWindow", "&Right Angles"), .style = LinkStyle::RightAngled},
};

// Adds to group the action that draws the links as entry tells, its style in its data; checked when it is current, the
// style they are drawn in.
void addLinkStyleAction(QActionGroup& group, const LinkStyleEntry& entry, LinkStyle current)
{
    QAction* action = group.addAction(MainWindow::tr(entry.name));
    action->setData(QVariant::fromValue(entry.style));
    action->setCheckable(true);
    action->setChecked(entry.style == current);
}

// Where the project files are saved and looked for unless told otherwise: scene/ next to NebulaStudio.
QString defaultProjectDirectory()
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/scene");
}

// Disables action while application runs.
void disableWhileRunning(const Nebula::Universe& application, QAction& action)
{
    QObject::connect(&application, &Nebula::Universe::runningChanged, &action,
                     [disabled = &action](bool running) { disabled->setEnabled(!running); });
}

// A list of the rows of model, one selected at a time.
QTreeView* createView(QAbstractItemModel& model)
{
    auto* view = new QTreeView;
    view->setModel(&model);
    view->setRootIsDecorated(false);
    view->setUniformRowHeights(true);
    view->setAllColumnsShowFocus(true);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->header()->setStretchLastSection(false);
    view->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    view->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    return view;
}

// A list of the rows of model, which are dragged from it.
QTreeView* createDraggableView(QAbstractItemModel& model)
{
    QTreeView* view = createView(model);
    view->setDragEnabled(true);
    view->setDragDropMode(QAbstractItemView::DragOnly);
    return view;
}

// Has show, a member of viewer, called with the data of role of each row made current in view.
template <typename Viewer>
void showCurrentIn(const QAbstractItemView& view, int role, Viewer* viewer, void (Viewer::*show)(const QString&))
{
    QObject::connect(view.selectionModel(), &QItemSelectionModel::currentRowChanged, viewer,
                     [role, viewer, show](const QModelIndex& current)
                     { (viewer->*show)(current.data(role).toString()); });
}

// view above viewer.
QSplitter* createSplitter(QWidget* view, QWidget* viewer)
{
    auto* splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(view);
    splitter->addWidget(viewer);
    splitter->setStretchFactor(1, 1);
    return splitter;
}

// content under title.
QWidget* createGroup(const QString& title, QWidget* content)
{
    auto* group = new QGroupBox(title);
    auto* layout = new QVBoxLayout(group);
    layout->addWidget(content);
    return group;
}

// top above below, without margins; top aligned as alignment tells, across the width when it tells nothing.
QWidget* createColumn(QWidget* top, QWidget* below, Qt::Alignment alignment = {})
{
    auto* column = new QWidget;
    auto* layout = new QVBoxLayout(column);
    layout->setContentsMargins({});
    layout->addWidget(top, 0, alignment);
    layout->addWidget(below);
    return column;
}

// A widget whose children are side by side, without margins around them.
QWidget* createRow()
{
    auto* row = new QWidget;
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins({});
    return row;
}

// stretched, which takes the room left, next to other.
QWidget* createBar(QWidget* stretched, QWidget* other)
{
    QWidget* bar = createRow();
    auto* layout = static_cast<QHBoxLayout*>(bar->layout());
    layout->addWidget(stretched, 1);
    layout->addWidget(other);
    return bar;
}

// A button telling text, and what it does as its tool tip.
QPushButton* createButton(const QString& text, const QString& toolTip)
{
    auto* button = new QPushButton(text);
    button->setToolTip(toolTip);
    return button;
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

QStringList namesOf(const QList<Nebula::Component*>& components)
{
    QStringList names;
    for(const Nebula::Component* component : components)
    {
        names.append(component->objectName());
    }
    return names;
}

} // namespace

MainWindow::MainWindow(const QCommandLineParser* commandLine, QWidget* parent)
    : QMainWindow(parent), m_pluginModel(new PluginModel(m_application.plugins(), this)),
      m_pluginViewer(new PluginViewer(m_application.plugins())),
      m_componentModel(new ComponentModel(Nebula::ComponentFactory::instance(), this)),
      m_componentViewer(new ComponentViewer(Nebula::ComponentFactory::instance())),
      m_propertyEditor(new PropertyEditor(m_application, Nebula::ComponentFactory::instance())),
      m_galaxyModel(new GalaxyModel(m_application, this)), m_galaxyViewer(new GalaxyViewer(*m_galaxyModel)),
      m_preferences(QString::fromUtf8(NebulaStudioInfo::organization), QString::fromUtf8(NebulaStudioInfo::name))
{
    createFileMenu();
    createViewMenu();
    createRunMenu();
    followApplication();
    readCommandLine(commandLine);
    setCentralWidget(createPanels());
    createStatusBar();
    setGalaxyDirectory(defaultProjectDirectory()); // Where the projects are saved unless told otherwise
    setProjectFile({});
    resize(1440, 800);
}

MainWindow::~MainWindow()
{
    // Its items, and the property editor, refer to the components of m_application, destroyed after this body, before
    // the children of the window. The scenes are no longer listened to: deleting a selected item changes their
    // selection. Neither is the application, stopped when it is destroyed.
    m_application.disconnect(this);
    m_propertyEditor->setComponent(nullptr);
    closeGalaxyScenes(0); // They show the components of the galaxies of m_application
    closeScene(*m_scene);
}

void MainWindow::createFileMenu()
{
    QMenu* menu = menuBar()->addMenu(tr("&File"));
    addProjectActions(*menu);
    addSaveGalaxyAction(*menu);
    menu->addSeparator();
    // The description of the galaxy its file is.
    disableWhileRunning(m_application, *menu->addAction(icon(GlossaryTerm::Galaxy), tr("&Description..."), this,
                                                        &MainWindow::editDescription));
}

void MainWindow::addProjectActions(QMenu& menu)
{
    // An empty application in its place.
    disableWhileRunning(m_application, *menu.addAction(icon(GlossaryTerm::Universe), tr("&New"), QKeySequence::New,
                                                       this, &MainWindow::newProject));
    disableWhileRunning(m_application,
                        *menu.addAction(tr("&Open..."), QKeySequence::Open, this, &MainWindow::openProject));
    menu.addAction(tr("&Save"), QKeySequence::Save, this, &MainWindow::saveProject);
    menu.addAction(tr("Save &As..."), QKeySequence::SaveAs, this, &MainWindow::saveProjectAs);
}

void MainWindow::addSaveGalaxyAction(QMenu& menu)
{
    m_saveGalaxyAction = menu.addAction(icon(GlossaryTerm::Galaxy), tr("Save &Galaxy"),
                                        QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_S), this, &MainWindow::saveGalaxy);
    m_saveGalaxyAction->setToolTip(
        tr("Save the galaxy shown to its file: each of its instances has what it holds then"));
    m_saveGalaxyAction->setEnabled(false); // Until a galaxy is shown
}

void MainWindow::createViewMenu()
{
    QMenu* menu = menuBar()->addMenu(tr("&View"));
    menu->addAction(tr("&Find Instances"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F), this,
                    [this] { focusToReplace(*m_finder); });
    menu->addSeparator();
    menu->addAction(tr("Zoom &In"), QKeySequence::ZoomIn, this, [this] { m_editionView->zoomIn(); });
    menu->addAction(tr("Zoom &Out"), QKeySequence::ZoomOut, this, [this] { m_editionView->zoomOut(); });
    menu->addAction(tr("&Actual Size"), QKeySequence(Qt::CTRL | Qt::Key_0), this,
                    [this] { m_editionView->resetZoom(); });
    menu->addSeparator();
    addLinkStyleMenu(*menu);
}

void MainWindow::addLinkStyleMenu(QMenu& menu)
{
    auto* group = new QActionGroup(&menu); // Exclusive: one style at a time
    connect(group, &QActionGroup::triggered, this,
            [this](const QAction* action) { setLinkStyle(action->data().value<LinkStyle>()); });
    for(const LinkStyleEntry& entry : LinkStyles)
    {
        addLinkStyleAction(*group, entry, m_linkStyle);
    }
    menu.addMenu(tr("&Links"))->addActions(group->actions());
}

void MainWindow::setLinkStyle(LinkStyle style)
{
    m_linkStyle = style;
    writeLinkStyle(m_preferences, style);
    m_scene->setLinkStyle(style);
    for(EditionScene* scene : std::as_const(m_galaxyScenes))
    {
        scene->setLinkStyle(style);
    }
}

void MainWindow::createRunMenu()
{
    QMenu* menu = menuBar()->addMenu(tr("&Run"));
    disableWhileRunning(m_application,
                        *menu->addAction(tr("&Start"), QKeySequence(Qt::Key_F5), this, &MainWindow::start));
    QAction* stop =
        menu->addAction(tr("S&top"), QKeySequence(Qt::SHIFT | Qt::Key_F5), &m_application, &Nebula::Universe::stop);
    stop->setEnabled(false);
    connect(&m_application, &Nebula::Universe::runningChanged, stop, &QAction::setEnabled);
}

void MainWindow::followApplication()
{
    connect(&m_application, &Nebula::Universe::runningChanged, this, &MainWindow::followRunning);
    // In the status bar, as what the edition zone tells, nor in the middle of a run, which goes on without the file.
    connect(&m_application, &Nebula::Universe::settingsFileFailed, this, &MainWindow::tell);
    connect(&m_application, &Nebula::Universe::componentsChanged, this, &MainWindow::leaveRemovedGalaxy);
}

void MainWindow::followRunning(bool running)
{
    // Its instances are read again, which is not done while it runs.
    m_saveGalaxyAction->setEnabled(!running && !m_galaxyScenes.isEmpty());
    m_pluginViewer->setUnloadAllowed(!running); // What runs keeps its plugin
    m_galaxyViewer->setOpenAllowed(!running);
}

void MainWindow::leaveRemovedGalaxy()
{
    const Nebula::Galaxy* shown = m_galaxyScenes.isEmpty() ? nullptr : m_galaxyScenes.first()->galaxy();
    if(shown != nullptr &&
       std::ranges::none_of(m_application.components(), [shown](const std::unique_ptr<Nebula::Component>& component)
                            { return component.get() == shown; }))
    {
        showDepth(0);
    }
}

void MainWindow::readCommandLine(const QCommandLineParser* commandLine)
{
    if(commandLine != nullptr)
    {
        m_application.applyCommandLine(*commandLine);
    }
    m_openDirectory = Nebula::PluginManager::defaultDirectory();
    refreshPlugins();
}

void MainWindow::refreshPlugins()
{
    m_pluginModel->refresh();
    m_pluginViewer->refresh();
    m_componentModel->refresh();
}

void MainWindow::refreshSources()
{
    refreshPlugins();
    m_galaxyModel->refresh();
}

QWidget* MainWindow::createPanels()
{
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(createSources());
    splitter->addWidget(createEdition());
    splitter->addWidget(createGroup(tr("Components"), createSplitter(createComponentList(), m_componentViewer)));
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({320, 760, 360});
    return splitter;
}

QWidget* MainWindow::createSources()
{
    auto* sources = new QTabWidget;
    sources->setDocumentMode(true); // Without a frame, on the background of the window, as the other panels are
    sources->addTab(createSplitter(createPluginList(), m_pluginViewer), tr("Plugins"));
    sources->addTab(createSplitter(createGalaxyList(), m_galaxyViewer), icon(GlossaryTerm::Galaxy), tr("Galaxies"));
    return sources;
}

QWidget* MainWindow::createPluginList()
{
    m_pluginView = createView(*m_pluginModel);
    showCurrentIn(*m_pluginView, PluginModel::IdRole, m_pluginViewer, &PluginViewer::setPluginId);
    m_pluginView->setCurrentIndex(m_pluginModel->index(0, 0));
    connect(m_pluginViewer, &PluginViewer::loadRequested, this, &MainWindow::load);
    connect(m_pluginViewer, &PluginViewer::unloadRequested, this, &MainWindow::unload);
    QPushButton* open = createButton(tr("Open plugin..."), tr("Load a plugin from anywhere on the disk"));
    connect(open, &QPushButton::clicked, this, &MainWindow::openPlugin);
    return createColumn(open, m_pluginView, Qt::AlignLeft);
}

QWidget* MainWindow::createGalaxyList()
{
    m_galaxyView = createDraggableView(*m_galaxyModel);
    showCurrentIn(*m_galaxyView, GalaxyModel::FilePathRole, m_galaxyViewer, &GalaxyViewer::setFilePath);
    connect(m_galaxyView, &QAbstractItemView::activated, this,
            [this](const QModelIndex& index) { openGalaxy(index.data(GalaxyModel::FilePathRole).toString()); });
    connect(m_galaxyViewer, &GalaxyViewer::openRequested, this, &MainWindow::openGalaxy);
    return createColumn(createGalaxyBar(), m_galaxyView);
}

QWidget* MainWindow::createGalaxyBar()
{
    m_galaxyDirectory = new QLineEdit;
    m_galaxyDirectory->setReadOnly(true);
    m_galaxyDirectory->setToolTip(tr("The directory whose galaxies are listed"));
    QPushButton* choose = createButton(tr("Choose directory..."), tr("List the galaxies of another directory"));
    connect(choose, &QPushButton::clicked, this, &MainWindow::chooseGalaxyDirectory);
    return createBar(m_galaxyDirectory, choose);
}

QWidget* MainWindow::createComponentList()
{
    auto* filter = new QSortFilterProxyModel(this);
    filter->setSourceModel(m_componentModel);
    filter->setFilterCaseSensitivity(Qt::CaseInsensitive);
    QTreeView* view = createDraggableView(*filter);
    // Also when the search hides the selected component: the view selects one of those left, none when none is.
    showCurrentIn(*view, ComponentModel::IdRole, m_componentViewer, &ComponentViewer::setComponentId);
    return createColumn(createSearchBar(*filter), view);
}

QWidget* MainWindow::createSearchBar(QSortFilterProxyModel& filter)
{
    auto* search = new QLineEdit;
    search->setPlaceholderText(tr("Search components"));
    search->setClearButtonEnabled(true);
    connect(search, &QLineEdit::textChanged, &filter, &QSortFilterProxyModel::setFilterFixedString);
    addFindShortcut(*search);
    return createBar(search, createSearchFields(filter));
}

QComboBox* MainWindow::createSearchFields(QSortFilterProxyModel& filter)
{
    auto* fields = new QComboBox;
    for(const SearchField& field : SearchFields)
    {
        fields->addItem(tr(field.name));
    }
    fields->setToolTip(tr("Where the text is looked for"));
    searchBy(filter, fields->currentIndex());
    connect(fields, &QComboBox::currentIndexChanged, &filter,
            [searched = &filter](int index) { searchBy(*searched, index); });
    return fields;
}

void MainWindow::addFindShortcut(QLineEdit& search)
{
    auto* shortcut = new QShortcut(QKeySequence::Find, this);
    connect(shortcut, &QShortcut::activated, &search, [field = &search] { focusToReplace(*field); });
}

QWidget* MainWindow::createEdition()
{
    auto* splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(createZone());
    splitter->addWidget(m_propertyEditor);
    splitter->addWidget(createProblemList());
    splitter->setStretchFactor(0, 1);
    splitter->setSizes({480, 200, 120});
    return createGroup(tr("Application"), splitter);
}

QWidget* MainWindow::createZone()
{
    m_scene = new EditionScene(m_application, Nebula::ComponentFactory::instance(), this);
    followScene(*m_scene);
    m_editionView = createEditionView(*m_scene);
    return createColumn(createZoneBar(), m_editionView);
}

QWidget* MainWindow::createZoneBar()
{
    // Where the galaxy shown is: the application, then the galaxies it is in, each a button that shows it again.
    m_path = createRow();
    m_path->hide();
    m_finder = new ComponentFinder(*m_editionView);
    connect(m_finder, &ComponentFinder::told, this, &MainWindow::tell);
    QWidget* bar = createRow();
    auto* layout = static_cast<QHBoxLayout*>(bar->layout());
    layout->addWidget(m_path, 1);
    layout->addWidget(m_finder, 0, Qt::AlignRight); // On the right, alone there while the application is shown
    return bar;
}

void MainWindow::followScene(EditionScene& scene)
{
    scene.setLinkStyle(m_linkStyle);
    connect(&scene, &QGraphicsScene::selectionChanged, this, [this, shown = &scene] { showSelected(*shown); });
    // In the status bar rather than in a box, which would open in the middle of the click that ends an edition.
    connect(&scene, &EditionScene::refused, this, &MainWindow::tell);
    connect(&scene, &EditionScene::told, this, &MainWindow::tell);
    connect(&scene, &EditionScene::galaxyOpened, this, &MainWindow::enterGalaxy);
    // The plugins it loaded, and their components.
    connect(&scene, &EditionScene::galaxyPlaced, this, &MainWindow::refreshPlugins);
}

void MainWindow::showSelected(const EditionScene& scene)
{
    if(m_editionView->scene() == &scene)
    {
        m_propertyEditor->setComponent(scene.selectedComponent());
    }
}

QWidget* MainWindow::createProblemList()
{
    auto* problems = new ProblemList(m_application);
    connect(problems, &ProblemList::componentActivated, this, &MainWindow::showComponent);
    return problems;
}

void MainWindow::showComponent(const QString& name)
{
    // Where the components of the application are.
    if(leaveGalaxies(0))
    {
        const Nebula::Component* component = m_application.component(name);
        centerOnItem(component != nullptr ? m_scene->select(*component) : nullptr);
    }
}

void MainWindow::centerOnItem(const ComponentItem* item)
{
    if(item != nullptr)
    {
        m_editionView->centerOn(item);
        m_editionView->setFocus();
    }
}

void MainWindow::createStatusBar()
{
    statusBar()->addWidget(
        new QLabel(tr("Plugins of %1").arg(QDir::toNativeSeparators(Nebula::PluginManager::defaultDirectory()))));
    auto* zoomLabel = new QLabel;
    const auto showZoom = [zoomLabel](qreal zoom) { zoomLabel->setText(tr("Zoom %1%").arg(qRound(zoom * 100))); };
    showZoom(m_editionView->zoom());
    connect(m_editionView, &EditionView::zoomChanged, zoomLabel, showZoom);
    statusBar()->addPermanentWidget(zoomLabel);
}

bool MainWindow::openProjectFile(const QString& filePath)
{
    QString error;
    const std::optional<QJsonObject> project = Nebula::Universe::read(filePath, error);
    if(!project)
    {
        // Told before anything is changed.
        warn(tr("Open project"), tr("%1 cannot be opened:\n%2").arg(QDir::toNativeSeparators(filePath), error));
        return false;
    }
    showProject(filePath, *project);
    return true;
}

void MainWindow::showProject(const QString& filePath, const QJsonObject& project)
{
    m_application.setFilePath(filePath); // Its galaxies are relative to it
    const QStringList problems = m_scene->load(project);
    refreshSources();
    // No longer its file when part of it is missing: saving would write over what could not be restored.
    setProjectFile(problems.isEmpty() ? filePath : QString());
    if(!problems.isEmpty())
    {
        warn(tr("Open project"),
             tr("%1 could not be entirely opened:\n%2").arg(QDir::toNativeSeparators(filePath), problems.join(u'\n')));
    }
}

bool MainWindow::saveProjectFile(const QString& filePath)
{
    m_scene->setFocusItem(nullptr); // What was typed in a name is taken before it is saved
    if(!writeProject(filePath))
    {
        return false;
    }
    setProjectFile(filePath);
    m_galaxyModel->refresh(); // Its directory may have just been made, which nothing followed yet
    return true;
}

bool MainWindow::writeProject(const QString& filePath)
{
    const QString previousFile = m_application.filePath();
    m_application.setFilePath(filePath);
    const bool written = writeFile(tr("Save project"), filePath, m_scene->save());
    if(!written)
    {
        m_application.setFilePath(previousFile);
    }
    return written;
}

bool MainWindow::writeFile(const QString& title, const QString& filePath, const QJsonObject& json)
{
    QSaveFile file(filePath);
    if(!file.open(QIODevice::WriteOnly) || file.write(QJsonDocument(json).toJson()) < 0 || !file.commit())
    {
        warn(title, tr("%1 cannot be saved:\n%2").arg(QDir::toNativeSeparators(filePath), file.errorString()));
        return false;
    }
    return true;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    event->setAccepted(saveChanges());
}

void MainWindow::load(const QString& pluginId)
{
    if(!m_application.plugins().load(pluginId))
    {
        warn(tr("Load plugin"), tr("%1 cannot be loaded:\n%2").arg(pluginId, m_application.plugins().errorString()));
        return;
    }
    refreshPlugins();
}

void MainWindow::unload(const QString& pluginId)
{
    const QList<Nebula::Component*> used = m_application.componentsOf(pluginId);
    if(used.isEmpty() || confirmUnload(pluginId, used))
    {
        m_scene->remove(used); // With what shows them, before the application would remove them itself
        unloadPlugin(pluginId);
    }
}

bool MainWindow::confirmUnload(const QString& pluginId, const QList<Nebula::Component*>& used)
{
    const QMessageBox::StandardButton answer =
        QMessageBox::warning(this, tr("Unload plugin"),
                             tr("The application has components of %1:\n%2\n\n"
                                "Unloading the plugin removes them.\nDo you want to unload it?")
                                 .arg(pluginId, namesOf(used).join(QStringLiteral(", "))),
                             QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    return answer == QMessageBox::Yes;
}

void MainWindow::unloadPlugin(const QString& pluginId)
{
    if(!m_application.unloadPlugin(pluginId))
    {
        warn(tr("Unload plugin"), tr("%1 cannot be unloaded:\n%2").arg(pluginId, m_application.errorString()));
    }
    refreshPlugins();
}

void MainWindow::openPlugin()
{
    const QString filePath = QFileDialog::getOpenFileName(this, tr("Open plugin"), m_openDirectory,
                                                          tr("Nebula plugins (*.nbdl);;All files (*)"));
    if(!filePath.isEmpty())
    {
        m_openDirectory = QFileInfo(filePath).absolutePath();
        addPlugin(filePath);
    }
}

void MainWindow::addPlugin(const QString& filePath)
{
    const std::optional<Nebula::PluginInfo> info = m_application.plugins().addFile(filePath);
    if(!info)
    {
        warn(tr("Open plugin"), tr("%1 cannot be opened:\n%2")
                                    .arg(QDir::toNativeSeparators(filePath), m_application.plugins().errorString()));
        return;
    }
    m_galaxyModel->refresh(); // Those that missed it are loadable now
    selectAndLoad(info->id);
}

void MainWindow::selectAndLoad(const QString& pluginId)
{
    m_pluginModel->refresh();
    m_pluginView->setCurrentIndex(m_pluginModel->indexOf(pluginId));
    if(!m_application.plugins().isLoaded(pluginId))
    {
        load(pluginId);
    }
}

void MainWindow::setProjectFile(const QString& filePath)
{
    m_projectFile = filePath;
    m_application.setFilePath(filePath); // Before it is saved: its galaxies are relative to it
    m_savedProject = m_scene->save();
    setWindowTitle(filePath.isEmpty() ? tr("NebulaStudio")
                                      : tr("%1 - NebulaStudio").arg(QFileInfo(filePath).fileName()));
}

bool MainWindow::saveChanges()
{
    return leaveGalaxies(0, true) && (m_scene->save() == m_savedProject || saveOrDiscardProject());
}

bool MainWindow::saveOrDiscardProject()
{
    const QMessageBox::StandardButton answer =
        askToSave(tr("The application has been modified.\nDo you want to save it?"));
    return answer == QMessageBox::Save ? saveProject() : answer == QMessageBox::Discard;
}

QMessageBox::StandardButton MainWindow::askToSave(const QString& text)
{
    return QMessageBox::warning(this, tr("NebulaStudio"), text,
                                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
}

void MainWindow::newProject()
{
    if(saveChanges())
    {
        m_scene->reset();
        setProjectFile({});
    }
}

void MainWindow::openProject()
{
    const QString filePath = saveChanges() ? askForProjectFile() : QString();
    if(!filePath.isEmpty())
    {
        openProjectFile(filePath);
    }
}

QString MainWindow::askForProjectFile()
{
    return QFileDialog::getOpenFileName(this, tr("Open project"), projectDialogPath(),
                                        tr("Nebula projects (*.nbp);;All files (*)"));
}

QString MainWindow::projectDialogPath() const
{
    return m_projectFile.isEmpty() ? defaultProjectDirectory() : m_projectFile;
}

bool MainWindow::saveProject()
{
    return m_projectFile.isEmpty() ? saveProjectAs() : saveProjectFile(m_projectFile);
}

bool MainWindow::saveProjectAs()
{
    if(m_projectFile.isEmpty())
    {
        QDir().mkpath(
            defaultProjectDirectory()); // There the first time a project is saved: the dialog would start elsewhere
    }
    const QString filePath =
        QFileDialog::getSaveFileName(this, tr("Save project"), projectDialogPath(), tr("Nebula projects (*.nbp)"));
    return !filePath.isEmpty() && saveProjectFile(filePath);
}

void MainWindow::start()
{
    // The title of a zone, or the name of a component, is no longer edited: what was typed is taken before the
    // application is no longer edited.
    m_editionView->scene()->setFocusItem(nullptr);
    if(!m_application.start())
    {
        warn(tr("Start"), tr("The application cannot be started:\n%1").arg(m_application.errorString()));
    }
}

void MainWindow::editDescription()
{
    bool accepted = false;
    const QString description = QInputDialog::getMultiLineText(
        this, tr("Description"), tr("What the application is for, shown with its galaxy in the Galaxies tab:"),
        m_application.description(), &accepted);
    if(accepted)
    {
        m_application.setDescription(description.trimmed()); // None when only blanks are left
    }
}

void MainWindow::setGalaxyDirectory(const QString& directory)
{
    m_galaxyModel->setDirectory(directory);
    m_galaxyDirectory->setText(QDir::toNativeSeparators(m_galaxyModel->directory()));
    m_galaxyView->setCurrentIndex(m_galaxyModel->index(0, 0));
}

void MainWindow::chooseGalaxyDirectory()
{
    const QString directory =
        QFileDialog::getExistingDirectory(this, tr("Galaxies directory"), m_galaxyModel->directory());
    if(!directory.isEmpty())
    {
        setGalaxyDirectory(directory);
    }
}

void MainWindow::openGalaxy(const QString& filePath)
{
    const std::optional<GalaxyModel::Galaxy> galaxy = m_galaxyModel->galaxy(filePath);
    if(!galaxy || !galaxy->isLoadable() || m_application.isRunning() || !saveChanges())
    {
        return;
    }
    openProjectFile(filePath);
}

void MainWindow::enterGalaxy(Nebula::Galaxy& galaxy)
{
    auto* scene = new EditionScene(m_application, Nebula::ComponentFactory::instance(), galaxy, this);
    followScene(*scene);
    m_galaxyScenes.append(scene);
    showDepth(m_galaxyScenes.size());
}

void MainWindow::showDepth(qsizetype depth)
{
    EditionScene* shown = depth == 0 ? m_scene : m_galaxyScenes.at(depth - 1);
    m_editionView->setScene(shown); // Before those further down go
    closeGalaxyScenes(depth);
    m_propertyEditor->setComponent(shown->selectedComponent());
    m_saveGalaxyAction->setEnabled(depth > 0 && !m_application.isRunning());
    refreshPath();
}

void MainWindow::closeGalaxyScenes(qsizetype depth)
{
    while(m_galaxyScenes.size() > depth)
    {
        closeScene(*m_galaxyScenes.takeLast());
    }
}

void MainWindow::closeScene(EditionScene& scene)
{
    scene.disconnect(this);
    delete &scene;
}

bool MainWindow::leaveGalaxies(qsizetype depth, bool closing)
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

bool MainWindow::leaveLastGalaxy(bool closing)
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

QMessageBox::StandardButton MainWindow::leaveChoice(const EditionScene& scene, bool closing)
{
    if(!scene.isModified())
    {
        return QMessageBox::NoButton;
    }
    const QMessageBox::StandardButton answer = askToLeave(*scene.galaxy(), closing);
    return answer != QMessageBox::Save || writeGalaxy(scene) ? answer : QMessageBox::Cancel;
}

QMessageBox::StandardButton MainWindow::askToLeave(const Nebula::Galaxy& galaxy, bool closing)
{
    showDepth(m_galaxyScenes.size()); // The one asked about
    if(m_application.isRunning() && !closing)
    {
        tell(tr("%1 has been modified: stop the application to save it, or to give its changes up")
                 .arg(galaxy.objectName()));
        return QMessageBox::Cancel;
    }
    return askToSave(tr("The galaxy %1 has been modified.\nDo you want to save it to %2? Each of its instances is read "
                        "again from it then.")
                         .arg(galaxy.objectName(), QDir::toNativeSeparators(galaxy.filePath())));
}

void MainWindow::readAgain(Nebula::Galaxy& galaxy, QMessageBox::StandardButton choice)
{
    if(choice == QMessageBox::Save && !m_application.isRunning())
    {
        readInstancesAgain(galaxy.filePath());
    }
    else if(choice == QMessageBox::Discard && !m_application.isRunning())
    {
        readGalaxyAgain(galaxy);
    }
}

void MainWindow::readGalaxyAgain(Nebula::Galaxy& galaxy)
{
    const QStringList problems = m_application.reloadGalaxy(galaxy);
    if(!problems.isEmpty())
    {
        warn(tr("NebulaStudio"),
             tr("%1 could not be read again as its file holds it:\n%2").arg(galaxy.objectName(), problems.join(u'\n')));
    }
}

void MainWindow::saveGalaxy()
{
    if(m_galaxyScenes.isEmpty() || m_application.isRunning())
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

bool MainWindow::writeGalaxy(const EditionScene& scene)
{
    return writeFile(tr("Save galaxy"), scene.galaxy()->filePath(), scene.save());
}

void MainWindow::readInstancesAgain(const QString& filePath)
{
    const QStringList problems = reloadInstances(filePath);
    m_galaxyModel->refresh(); // What it tells of the galaxy, at once
    if(problems.isEmpty())
    {
        tell(tr("%1 is saved: its instances have what it holds now").arg(QDir::toNativeSeparators(filePath)));
        return;
    }
    warn(tr("Save galaxy"), tr("%1 is saved, but its instances could not keep everything they had:\n%2")
                                .arg(QDir::toNativeSeparators(filePath), problems.join(u'\n')));
}

QStringList MainWindow::reloadInstances(const QString& filePath)
{
    QStringList problems;
    const QList<Nebula::Galaxy*> instances = m_application.galaxies(filePath);
    for(Nebula::Galaxy* instance : instances)
    {
        problems.append(m_application.reloadGalaxy(*instance));
    }
    return problems;
}

void MainWindow::refreshPath()
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

void MainWindow::addPathLevels()
{
    addPathLevel(0, icon(GlossaryTerm::Universe), tr("Application"));
    for(qsizetype depth = 1; depth <= m_galaxyScenes.size(); ++depth)
    {
        addPathLevel(depth, icon(GlossaryTerm::Galaxy), m_galaxyScenes.at(depth - 1)->galaxy()->objectName());
    }
    static_cast<QHBoxLayout*>(m_path->layout())->addStretch();
}

void MainWindow::addPathLevel(qsizetype depth, const QIcon& levelIcon, const QString& name)
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

QToolButton* MainWindow::createLevelButton(qsizetype depth, const QIcon& levelIcon, const QString& name)
{
    QToolButton* button = createFlatButton();
    button->setText(name);
    button->setIcon(levelIcon);
    connect(button, &QToolButton::clicked, this, [this, depth] { leaveGalaxies(depth); });
    return button;
}

void MainWindow::tell(const QString& message)
{
    statusBar()->showMessage(message, MessageDuration);
}

void MainWindow::warn(const QString& title, const QString& text)
{
    QMessageBox::warning(this, title, text);
}
