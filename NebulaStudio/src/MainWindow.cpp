// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "MainWindow.h"

#include <QAction>
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

#include "ComponentItem.h"
#include "ComponentModel.h"
#include "ComponentViewer.h"
#include "EditionScene.h"
#include "EditionView.h"
#include "Galaxy/GalaxyModel.h"
#include "Galaxy/GalaxyViewer.h"
#include "Nebula/Component/ComponentFactory.h"
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

// Where the project files are saved and looked for unless told otherwise: scene/ next to NebulaStudio.
QString defaultProjectDirectory()
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/scene");
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

// view above viewer.
QSplitter* createSplitter(QWidget* view, QWidget* viewer)
{
    auto* splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(view);
    splitter->addWidget(viewer);
    splitter->setStretchFactor(1, 1);
    return splitter;
}

// view above viewer, under title.
QWidget* createPanel(const QString& title, QWidget* view, QWidget* viewer)
{
    auto* panel = new QGroupBox(title);
    auto* layout = new QVBoxLayout(panel);
    layout->addWidget(createSplitter(view, viewer));
    return panel;
}

} // namespace

MainWindow::MainWindow(const QCommandLineParser* commandLine, QWidget* parent)
    : QMainWindow(parent), m_pluginModel(new PluginModel(m_application.plugins(), this)),
      m_pluginViewer(new PluginViewer(m_application.plugins())),
      m_componentModel(new ComponentModel(Nebula::ComponentFactory::instance(), this)),
      m_componentViewer(new ComponentViewer(Nebula::ComponentFactory::instance())),
      m_propertyEditor(new PropertyEditor(m_application, Nebula::ComponentFactory::instance()))
{
    // While the application runs, what would replace it is disabled, as is what edits it: see EditionScene. It is
    // saved with the values of the edition.
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
    QAction* newAction = fileMenu->addAction(tr("&New"), QKeySequence::New, this, &MainWindow::newProject);
    newAction->setIcon(icon(GlossaryTerm::Universe)); // An empty application in its place
    QAction* openAction = fileMenu->addAction(tr("&Open..."), QKeySequence::Open, this, &MainWindow::openProject);
    fileMenu->addAction(tr("&Save"), QKeySequence::Save, this, &MainWindow::saveProject);
    fileMenu->addAction(tr("Save &As..."), QKeySequence::SaveAs, this, &MainWindow::saveProjectAs);
    // The galaxy shown, to its own file, which its other instances are read again from.
    m_saveGalaxyAction =
        fileMenu->addAction(icon(GlossaryTerm::Galaxy), tr("Save &Galaxy"),
                            QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_S), this, &MainWindow::saveGalaxy);
    m_saveGalaxyAction->setToolTip(
        tr("Save the galaxy shown to its file: each of its instances has what it holds then"));
    m_saveGalaxyAction->setEnabled(false); // Until a galaxy is shown
    fileMenu->addSeparator();
    QAction* descriptionAction = fileMenu->addAction(tr("&Description..."), this, &MainWindow::editDescription);
    descriptionAction->setIcon(icon(GlossaryTerm::Galaxy)); // The one of the galaxy its file is

    // The zoom of the edition zone, also changed there with + and -, or with Ctrl and the mouse wheel.
    QMenu* viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(tr("Zoom &In"), QKeySequence::ZoomIn, this, [this] { m_editionView->zoomIn(); });
    viewMenu->addAction(tr("Zoom &Out"), QKeySequence::ZoomOut, this, [this] { m_editionView->zoomOut(); });
    viewMenu->addAction(tr("&Actual Size"), QKeySequence(Qt::CTRL | Qt::Key_0), this,
                        [this] { m_editionView->resetZoom(); });

    QMenu* runMenu = menuBar()->addMenu(tr("&Run"));
    QAction* startAction = runMenu->addAction(tr("&Start"), QKeySequence(Qt::Key_F5), this, &MainWindow::start);
    QAction* stopAction =
        runMenu->addAction(tr("S&top"), QKeySequence(Qt::SHIFT | Qt::Key_F5), &m_application, &Nebula::Universe::stop);
    stopAction->setEnabled(false);
    connect(&m_application, &Nebula::Universe::runningChanged, this,
            [this, newAction, openAction, descriptionAction, startAction, stopAction](bool running)
            {
                newAction->setEnabled(!running);
                openAction->setEnabled(!running);
                descriptionAction->setEnabled(!running);
                startAction->setEnabled(!running);
                stopAction->setEnabled(running);
                // Its instances are read again, which is not done while it runs.
                m_saveGalaxyAction->setEnabled(!running && !m_galaxyScenes.isEmpty());
                m_pluginViewer->setUnloadAllowed(!running); // What runs keeps its plugin
                m_galaxyViewer->setOpenAllowed(!running);
            });

    if(commandLine != nullptr)
    {
        m_application.applyCommandLine(*commandLine);
    }
    m_openDirectory = Nebula::PluginManager::defaultDirectory();
    m_pluginModel->refresh();
    m_componentModel->refresh();

    m_pluginView = createView(*m_pluginModel);
    connect(m_pluginView->selectionModel(), &QItemSelectionModel::currentRowChanged, this,
            [this](const QModelIndex& current)
            { m_pluginViewer->setPluginId(current.data(PluginModel::IdRole).toString()); });
    connect(m_pluginViewer, &PluginViewer::loadRequested, this, &MainWindow::load);
    connect(m_pluginViewer, &PluginViewer::unloadRequested, this, &MainWindow::unload);

    auto* openButton = new QPushButton(tr("Open plugin..."));
    openButton->setToolTip(tr("Load a plugin from anywhere on the disk"));
    connect(openButton, &QPushButton::clicked, this, &MainWindow::openPlugin);
    auto* pluginList = new QWidget;
    auto* pluginListLayout = new QVBoxLayout(pluginList);
    pluginListLayout->setContentsMargins({});
    pluginListLayout->addWidget(openButton, 0, Qt::AlignLeft);
    pluginListLayout->addWidget(m_pluginView);

    // The galaxies of a directory, those the application knows the plugins of opened from their viewer, or activated,
    // and dragged into the edition zone.
    m_galaxyModel = new GalaxyModel(m_application, this);
    m_galaxyViewer = new GalaxyViewer(*m_galaxyModel);
    m_galaxyView = createView(*m_galaxyModel);
    m_galaxyView->setDragEnabled(true);
    m_galaxyView->setDragDropMode(QAbstractItemView::DragOnly);
    connect(m_galaxyView->selectionModel(), &QItemSelectionModel::currentRowChanged, this,
            [this](const QModelIndex& current)
            { m_galaxyViewer->setFilePath(current.data(GalaxyModel::FilePathRole).toString()); });
    connect(m_galaxyView, &QAbstractItemView::activated, this,
            [this](const QModelIndex& index) { openGalaxy(index.data(GalaxyModel::FilePathRole).toString()); });
    connect(m_galaxyViewer, &GalaxyViewer::openRequested, this, &MainWindow::openGalaxy);

    m_galaxyDirectory = new QLineEdit;
    m_galaxyDirectory->setReadOnly(true);
    m_galaxyDirectory->setToolTip(tr("The directory whose galaxies are listed"));
    auto* chooseButton = new QPushButton(tr("Choose directory..."));
    chooseButton->setToolTip(tr("List the galaxies of another directory"));
    connect(chooseButton, &QPushButton::clicked, this, &MainWindow::chooseGalaxyDirectory);
    auto* directoryBar = new QHBoxLayout;
    directoryBar->addWidget(m_galaxyDirectory, 1);
    directoryBar->addWidget(chooseButton);
    auto* galaxyList = new QWidget;
    auto* galaxyListLayout = new QVBoxLayout(galaxyList);
    galaxyListLayout->setContentsMargins({});
    galaxyListLayout->addLayout(directoryBar);
    galaxyListLayout->addWidget(m_galaxyView);

    // The components whose name, description or category, as chosen next to the search bar, holds what is typed there,
    // whatever its case. The proxy hands what is dragged over to the model, which makes the data.
    auto* componentFilter = new QSortFilterProxyModel(this);
    componentFilter->setSourceModel(m_componentModel);
    componentFilter->setFilterCaseSensitivity(Qt::CaseInsensitive);
    auto* search = new QLineEdit;
    search->setPlaceholderText(tr("Search components"));
    search->setClearButtonEnabled(true);
    connect(search, &QLineEdit::textChanged, componentFilter, &QSortFilterProxyModel::setFilterFixedString);
    auto* searchField = new QComboBox;
    for(const SearchField& field : SearchFields)
    {
        searchField->addItem(tr(field.name));
    }
    searchField->setToolTip(tr("Where the text is looked for"));
    const auto searchBy = [componentFilter](int index)
    {
        const SearchField& field = SearchFields.at(static_cast<std::size_t>(index));
        componentFilter->setFilterKeyColumn(field.column);
        componentFilter->setFilterRole(field.role);
    };
    searchBy(searchField->currentIndex());
    connect(searchField, &QComboBox::currentIndexChanged, componentFilter, searchBy);
    auto* findShortcut = new QShortcut(QKeySequence::Find, this);
    connect(findShortcut, &QShortcut::activated, search,
            [search]
            {
                search->setFocus(Qt::ShortcutFocusReason);
                search->selectAll(); // Even when it already had the focus: what is typed replaces the last search
            });

    QTreeView* componentView = createView(*componentFilter);
    componentView->setDragEnabled(true);
    componentView->setDragDropMode(QAbstractItemView::DragOnly);
    // Also when the search hides the selected component: the view selects one of those left, none when none is.
    connect(componentView->selectionModel(), &QItemSelectionModel::currentRowChanged, this,
            [this](const QModelIndex& current)
            { m_componentViewer->setComponentId(current.data(ComponentModel::IdRole).toString()); });

    auto* searchBar = new QHBoxLayout;
    searchBar->addWidget(search, 1);
    searchBar->addWidget(searchField);
    auto* componentList = new QWidget;
    auto* componentListLayout = new QVBoxLayout(componentList);
    componentListLayout->setContentsMargins({});
    componentListLayout->addLayout(searchBar);
    componentListLayout->addWidget(componentView);

    m_scene = new EditionScene(m_application, Nebula::ComponentFactory::instance(), this);
    m_editionView = new EditionView(m_scene);
    m_editionView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_editionView->setRenderHint(QPainter::Antialiasing);
    m_editionView->setDragMode(QGraphicsView::RubberBandDrag);
    m_editionView->setAcceptDrops(true);
    connect(m_scene, &QGraphicsScene::selectionChanged, this,
            [this]
            {
                if(m_editionView->scene() == m_scene)
                {
                    m_propertyEditor->setComponent(m_scene->selectedComponent());
                }
            });
    // In the status bar rather than in a box, which would open in the middle of the click that ends an edition.
    connect(m_scene, &EditionScene::refused, this, &MainWindow::tell);
    connect(m_scene, &EditionScene::told, this, &MainWindow::tell);
    connect(m_scene, &EditionScene::galaxyOpened, this, &MainWindow::enterGalaxy);
    // The plugins it loaded, and their components.
    connect(m_scene, &EditionScene::galaxyPlaced, this,
            [this]
            {
                m_pluginModel->refresh();
                m_pluginViewer->refresh();
                m_componentModel->refresh();
            });
    // Nor in the middle of a run, which goes on without the file.
    connect(&m_application, &Nebula::Universe::settingsFileFailed, this, &MainWindow::tell);
    // The galaxy shown is closed before it goes, with those it holds.
    connect(&m_application, &Nebula::Universe::componentsChanged, this,
            [this]
            {
                const Nebula::Galaxy* shown = m_galaxyScenes.isEmpty() ? nullptr : m_galaxyScenes.first()->galaxy();
                if(shown != nullptr && std::ranges::none_of(m_application.components(),
                                                            [shown](const std::unique_ptr<Nebula::Component>& component)
                                                            { return component.get() == shown; }))
                {
                    showDepth(0);
                }
            });

    // Where the galaxy shown is: the application, then the galaxies it is in, each a button that shows it again.
    m_path = new QWidget;
    auto* pathLayout = new QHBoxLayout(m_path);
    pathLayout->setContentsMargins({});
    m_path->hide();
    auto* zone = new QWidget;
    auto* zoneLayout = new QVBoxLayout(zone);
    zoneLayout->setContentsMargins({});
    zoneLayout->addWidget(m_path);
    zoneLayout->addWidget(m_editionView);

    // What keeps components from working; activating one shows it, selected, in the middle of the edition zone.
    auto* problemList = new ProblemList(m_application);
    connect(problemList, &ProblemList::componentActivated, this,
            [this](const QString& name)
            {
                // Where the components of the application are.
                if(!leaveGalaxies(0))
                {
                    return;
                }
                const Nebula::Component* component = m_application.component(name);
                const ComponentItem* item = component != nullptr ? m_scene->select(*component) : nullptr;
                if(item != nullptr)
                {
                    m_editionView->centerOn(item);
                    m_editionView->setFocus(); // The keys are for it, as Delete is
                }
            });

    // The properties of the component selected in the edition zone, under it, then the problems; the zone takes the
    // room.
    auto* editionSplitter = new QSplitter(Qt::Vertical);
    editionSplitter->addWidget(zone);
    editionSplitter->addWidget(m_propertyEditor);
    editionSplitter->addWidget(problemList);
    editionSplitter->setStretchFactor(0, 1);
    editionSplitter->setSizes({480, 200, 120});
    auto* edition = new QGroupBox(tr("Application"));
    auto* editionLayout = new QVBoxLayout(edition);
    editionLayout->addWidget(editionSplitter);

    // What the application is made from, on the disk: the plugins, and the galaxies in another tab.
    auto* sources = new QTabWidget;
    sources->setDocumentMode(true); // Without a frame, on the background of the window, as the other panels are
    sources->addTab(createSplitter(pluginList, m_pluginViewer), tr("Plugins"));
    sources->addTab(createSplitter(galaxyList, m_galaxyViewer), icon(GlossaryTerm::Galaxy), tr("Galaxies"));

    // The edition zone between the plugins and the components, next to the list a component is dragged from.
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(sources);
    splitter->addWidget(edition);
    splitter->addWidget(createPanel(tr("Components"), componentList, m_componentViewer));
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({320, 760, 360});
    setCentralWidget(splitter);

    statusBar()->addWidget(
        new QLabel(tr("Plugins of %1").arg(QDir::toNativeSeparators(Nebula::PluginManager::defaultDirectory()))));
    auto* zoomLabel = new QLabel;
    const auto showZoom = [zoomLabel](qreal zoom) { zoomLabel->setText(tr("Zoom %1%").arg(qRound(zoom * 100))); };
    showZoom(m_editionView->zoom());
    connect(m_editionView, &EditionView::zoomChanged, zoomLabel, showZoom);
    statusBar()->addPermanentWidget(zoomLabel);
    m_pluginView->setCurrentIndex(m_pluginModel->index(0, 0));
    setGalaxyDirectory(defaultProjectDirectory()); // Where the projects are saved unless told otherwise
    m_galaxyView->setCurrentIndex(m_galaxyModel->index(0, 0));
    setProjectFile({});
    resize(1440, 800);
}

MainWindow::~MainWindow()
{
    // Its items, and the property editor, refer to the components of m_application, destroyed after this body, before
    // the children of the window. The scene is no longer listened to: deleting a selected item changes its selection.
    // Neither is the application, stopped when it is destroyed.
    m_application.disconnect(this);
    m_scene->disconnect(this);
    m_propertyEditor->setComponent(nullptr);
    for(EditionScene* scene : std::as_const(m_galaxyScenes))
    {
        scene->disconnect(this);
    }
    qDeleteAll(m_galaxyScenes); // They show the components of the galaxies of m_application
    delete m_scene;
}

bool MainWindow::openProjectFile(const QString& filePath)
{
    QString error;
    const std::optional<QJsonObject> project = Nebula::Universe::read(filePath, error);
    if(!project)
    {
        // Told before anything is changed.
        QMessageBox::warning(this, tr("Open project"),
                             tr("%1 cannot be opened:\n%2").arg(QDir::toNativeSeparators(filePath), error));
        return false;
    }
    m_application.setFilePath(filePath); // Its galaxies are relative to it
    const QStringList problems = m_scene->load(*project);
    // The plugins of the project, loaded with it, and their components; those opened from their file are known from
    // now on, and so are the galaxies they were missing from.
    m_pluginModel->refresh();
    m_pluginViewer->refresh();
    m_componentModel->refresh();
    m_galaxyModel->refresh();
    // No longer its file when part of it is missing: saving would write over what could not be restored.
    setProjectFile(problems.isEmpty() ? filePath : QString());
    if(!problems.isEmpty())
    {
        QMessageBox::warning(this, tr("Open project"),
                             tr("%1 could not be entirely opened:\n%2")
                                 .arg(QDir::toNativeSeparators(filePath), problems.join(u'\n')));
    }
    return true;
}

bool MainWindow::saveProjectFile(const QString& filePath)
{
    // The title of a zone, or the name of a component, is no longer edited: what was typed is taken before it is saved.
    m_scene->setFocusItem(nullptr);
    const QString previousFile = m_application.filePath();
    m_application.setFilePath(filePath); // Its galaxies are written relative to it
    QSaveFile file(filePath);
    if(!file.open(QIODevice::WriteOnly) || file.write(QJsonDocument(m_scene->save()).toJson()) < 0 || !file.commit())
    {
        m_application.setFilePath(previousFile);
        QMessageBox::warning(this, tr("Save project"),
                             tr("%1 cannot be saved:\n%2").arg(QDir::toNativeSeparators(filePath), file.errorString()));
        return false;
    }
    setProjectFile(filePath);
    m_galaxyModel->refresh(); // Its directory may have just been made, which nothing followed yet
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
        QMessageBox::warning(this, tr("Load plugin"),
                             tr("%1 cannot be loaded:\n%2").arg(pluginId, m_application.plugins().errorString()));
        return;
    }
    m_pluginModel->refresh();
    m_pluginViewer->refresh();
    m_componentModel->refresh();
}

void MainWindow::unload(const QString& pluginId)
{
    const QList<Nebula::Component*> used = m_application.componentsOf(pluginId);
    if(!used.isEmpty())
    {
        QStringList names;
        for(const Nebula::Component* component : used)
        {
            names.append(component->objectName());
        }
        const QMessageBox::StandardButton answer =
            QMessageBox::warning(this, tr("Unload plugin"),
                                 tr("The application has components of %1:\n%2\n\n"
                                    "Unloading the plugin removes them.\nDo you want to unload it?")
                                     .arg(pluginId, names.join(QStringLiteral(", "))),
                                 QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if(answer != QMessageBox::Yes)
        {
            return;
        }
        m_scene->remove(used); // With what shows them, before the application would remove them itself
    }
    if(!m_application.unloadPlugin(pluginId))
    {
        QMessageBox::warning(this, tr("Unload plugin"),
                             tr("%1 cannot be unloaded:\n%2").arg(pluginId, m_application.errorString()));
    }
    m_pluginModel->refresh();
    m_pluginViewer->refresh();
    m_componentModel->refresh();
}

void MainWindow::openPlugin()
{
    const QString filePath = QFileDialog::getOpenFileName(this, tr("Open plugin"), m_openDirectory,
                                                          tr("Nebula plugins (*.nbdl);;All files (*)"));
    if(filePath.isEmpty())
    {
        return;
    }
    m_openDirectory = QFileInfo(filePath).absolutePath();

    const std::optional<Nebula::PluginInfo> info = m_application.plugins().addFile(filePath);
    if(!info)
    {
        QMessageBox::warning(this, tr("Open plugin"),
                             tr("%1 cannot be opened:\n%2")
                                 .arg(QDir::toNativeSeparators(filePath), m_application.plugins().errorString()));
        return;
    }
    m_pluginModel->refresh();
    m_galaxyModel->refresh(); // Those that missed it are loadable now
    m_pluginView->setCurrentIndex(m_pluginModel->indexOf(info->id));
    if(!m_application.plugins().isLoaded(info->id))
    {
        load(info->id);
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
    if(!leaveGalaxies(0, true))
    {
        return false;
    }
    if(m_scene->save() == m_savedProject)
    {
        return true;
    }
    const QMessageBox::StandardButton answer = QMessageBox::warning(
        this, tr("NebulaStudio"), tr("The application has been modified.\nDo you want to save it?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    return answer == QMessageBox::Save ? saveProject() : answer == QMessageBox::Discard;
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
    if(!saveChanges())
    {
        return;
    }
    // From the directory of its file, when it has one; from the default one otherwise.
    const QString filePath = QFileDialog::getOpenFileName(
        this, tr("Open project"), m_projectFile.isEmpty() ? defaultProjectDirectory() : m_projectFile,
        tr("Nebula projects (*.nbp);;All files (*)"));
    if(!filePath.isEmpty())
    {
        openProjectFile(filePath);
    }
}

bool MainWindow::saveProject()
{
    return m_projectFile.isEmpty() ? saveProjectAs() : saveProjectFile(m_projectFile);
}

bool MainWindow::saveProjectAs()
{
    QString proposed = m_projectFile;
    if(proposed.isEmpty())
    {
        proposed = defaultProjectDirectory();
        QDir().mkpath(proposed); // There the first time a project is saved: the dialog would start elsewhere
    }
    const QString filePath =
        QFileDialog::getSaveFileName(this, tr("Save project"), proposed, tr("Nebula projects (*.nbp)"));
    return !filePath.isEmpty() && saveProjectFile(filePath);
}

void MainWindow::start()
{
    // The title of a zone, or the name of a component, is no longer edited: what was typed is taken before the
    // application is no longer edited.
    m_editionView->scene()->setFocusItem(nullptr);
    if(!m_application.start())
    {
        QMessageBox::warning(this, tr("Start"),
                             tr("The application cannot be started:\n%1").arg(m_application.errorString()));
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
}

void MainWindow::chooseGalaxyDirectory()
{
    const QString directory =
        QFileDialog::getExistingDirectory(this, tr("Galaxies directory"), m_galaxyModel->directory());
    if(!directory.isEmpty())
    {
        setGalaxyDirectory(directory);
        m_galaxyView->setCurrentIndex(m_galaxyModel->index(0, 0));
    }
}

void MainWindow::enterGalaxy(Nebula::Galaxy& galaxy)
{
    auto* scene = new EditionScene(m_application, Nebula::ComponentFactory::instance(), galaxy, this);
    connect(scene, &QGraphicsScene::selectionChanged, this,
            [this, scene]
            {
                if(m_editionView->scene() == scene)
                {
                    m_propertyEditor->setComponent(scene->selectedComponent());
                }
            });
    connect(scene, &EditionScene::refused, this, &MainWindow::tell);
    connect(scene, &EditionScene::told, this, &MainWindow::tell);
    connect(scene, &EditionScene::galaxyOpened, this, &MainWindow::enterGalaxy);
    // The plugins it loaded, and their components, as in the application.
    connect(scene, &EditionScene::galaxyPlaced, this,
            [this]
            {
                m_pluginModel->refresh();
                m_pluginViewer->refresh();
                m_componentModel->refresh();
            });
    m_galaxyScenes.append(scene);
    showDepth(m_galaxyScenes.size());
}

void MainWindow::showDepth(qsizetype depth)
{
    EditionScene* shown = depth == 0 ? m_scene : m_galaxyScenes.at(depth - 1);
    m_editionView->setScene(shown); // Before those further down go
    while(m_galaxyScenes.size() > depth)
    {
        EditionScene* closed = m_galaxyScenes.takeLast();
        closed->disconnect(this);
        delete closed;
    }
    m_propertyEditor->setComponent(shown->selectedComponent());
    m_saveGalaxyAction->setEnabled(depth > 0 && !m_application.isRunning());
    refreshPath();
}

bool MainWindow::leaveGalaxies(qsizetype depth, bool closing)
{
    while(m_galaxyScenes.size() > depth)
    {
        EditionScene* scene = m_galaxyScenes.constLast();
        // What was typed in a name is taken before what changed is told.
        scene->setFocusItem(nullptr);
        Nebula::Galaxy* galaxy = scene->galaxy();
        const QString filePath = galaxy->filePath();
        const bool modified = scene->isModified();
        QMessageBox::StandardButton answer = QMessageBox::Discard;
        if(modified)
        {
            showDepth(m_galaxyScenes.size()); // The one asked about
            if(m_application.isRunning() && !closing)
            {
                tell(tr("%1 has been modified: stop the application to save it, or to give its changes up")
                         .arg(galaxy->objectName()));
                return false;
            }
            answer = QMessageBox::warning(
                this, tr("NebulaStudio"),
                tr("The galaxy %1 has been modified.\nDo you want to save it to %2? Each of its instances is read "
                   "again from it then.")
                    .arg(galaxy->objectName(), QDir::toNativeSeparators(filePath)),
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
            if(answer == QMessageBox::Cancel ||
               (answer == QMessageBox::Save && !writeGalaxyFile(filePath, scene->save())))
            {
                return false;
            }
        }
        showDepth(m_galaxyScenes.size() - 1); // Before it is read again: its scene shows what goes then
        if(!modified || m_application.isRunning())
        {
            continue; // Nothing to read again, or NebulaStudio closes
        }
        if(answer == QMessageBox::Save)
        {
            readInstancesAgain(filePath);
        }
        else if(const QStringList problems = m_application.reloadGalaxy(*galaxy); !problems.isEmpty())
        {
            QMessageBox::warning(this, tr("NebulaStudio"),
                                 tr("%1 could not be read again as its file holds it:\n%2")
                                     .arg(galaxy->objectName(), problems.join(u'\n')));
        }
    }
    showDepth(depth);
    return true;
}

void MainWindow::saveGalaxy()
{
    if(m_galaxyScenes.isEmpty() || m_application.isRunning())
    {
        return;
    }
    EditionScene* scene = m_galaxyScenes.constLast();
    // What was typed in a name is taken before it is saved.
    scene->setFocusItem(nullptr);
    const QString filePath = scene->galaxy()->filePath();
    if(writeGalaxyFile(filePath, scene->save()))
    {
        readInstancesAgain(filePath);
    }
}

bool MainWindow::writeGalaxyFile(const QString& filePath, const QJsonObject& project)
{
    QSaveFile file(filePath);
    if(!file.open(QIODevice::WriteOnly) || file.write(QJsonDocument(project).toJson()) < 0 || !file.commit())
    {
        QMessageBox::warning(this, tr("Save galaxy"),
                             tr("%1 cannot be saved:\n%2").arg(QDir::toNativeSeparators(filePath), file.errorString()));
        return false;
    }
    return true;
}

void MainWindow::readInstancesAgain(const QString& filePath)
{
    QStringList problems;
    const QList<Nebula::Galaxy*> instances = m_application.galaxies(filePath);
    for(Nebula::Galaxy* instance : instances)
    {
        problems.append(m_application.reloadGalaxy(*instance));
    }
    m_galaxyModel->refresh(); // What it tells of the galaxy, at once
    if(problems.isEmpty())
    {
        tell(tr("%1 is saved: its instances have what it holds now").arg(QDir::toNativeSeparators(filePath)));
        return;
    }
    QMessageBox::warning(this, tr("Save galaxy"),
                         tr("%1 is saved, but its instances could not keep everything they had:\n%2")
                             .arg(QDir::toNativeSeparators(filePath), problems.join(u'\n')));
}

void MainWindow::refreshPath()
{
    // Again from the start: the names may have changed since. Later: one of its buttons may be the one being clicked.
    QLayout* path = m_path->layout();
    while(QLayoutItem* item = path->takeAt(0))
    {
        if(QWidget* widget = item->widget())
        {
            widget->hide();
            widget->deleteLater();
        }
        delete item;
    }
    m_path->setVisible(!m_galaxyScenes.isEmpty());
    if(m_galaxyScenes.isEmpty())
    {
        return;
    }
    const auto addLevel = [this, path](qsizetype depth, const QIcon& levelIcon, const QString& name)
    {
        if(depth == m_galaxyScenes.size())
        {
            auto* current = new QLabel(name); // The one shown
            current->setFont(titleFont(current->font()));
            path->addWidget(current);
            return;
        }
        auto* button = new QToolButton;
        button->setText(name);
        button->setIcon(levelIcon);
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        button->setAutoRaise(true);
        // Once the changes of those further down are saved or given up.
        connect(button, &QToolButton::clicked, this, [this, depth] { leaveGalaxies(depth); });
        path->addWidget(button);
        path->addWidget(new QLabel(QStringLiteral("›"))); // ›
    };
    addLevel(0, icon(GlossaryTerm::Universe), tr("Application"));
    for(qsizetype depth = 1; depth <= m_galaxyScenes.size(); ++depth)
    {
        addLevel(depth, icon(GlossaryTerm::Galaxy), m_galaxyScenes.at(depth - 1)->galaxy()->objectName());
    }
    static_cast<QHBoxLayout*>(path)->addStretch();
    auto* save = new QToolButton; // Next to the path to the galaxy it saves
    save->setDefaultAction(m_saveGalaxyAction);
    save->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    save->setAutoRaise(true);
    path->addWidget(save);
}

void MainWindow::tell(const QString& message)
{
    statusBar()->showMessage(message, MessageDuration);
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
