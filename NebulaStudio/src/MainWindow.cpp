// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "MainWindow.h"

#include <QCloseEvent>
#include <QGroupBox>
#include <QLabel>
#include <QMenuBar>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QVBoxLayout>

#include "ComponentPanel.h"
#include "EditionZone.h"
#include "Galaxy/GalaxyPanel.h"
#include "Messages.h"
#include "Nebula/Component/ComponentFactory.h"
#include "NebulaStudio_info.h"
#include "PanelWidgets.h"
#include "PluginPanel.h"
#include "ProblemList.h"
#include "Project.h"
#include "PropertyEditor.h"

namespace
{

// content under title.
QWidget* createGroup(const QString& title, QWidget* content)
{
    auto* group = new QGroupBox(title);
    auto* layout = new QVBoxLayout(group);
    layout->addWidget(content);
    return group;
}

} // namespace

MainWindow::MainWindow(const QCommandLineParser* commandLine, QWidget* parent)
    : QMainWindow(parent),
      m_studio(QString::fromUtf8(NebulaStudioInfo::organization), QString::fromUtf8(NebulaStudioInfo::name)),
      m_zone(new EditionZone(m_studio)), m_project(new Project(m_studio, *m_zone, this)),
      m_propertyEditor(new PropertyEditor(m_studio.application(), Nebula::ComponentFactory::instance()))
{
    if(commandLine != nullptr)
    {
        m_studio.applyCommandLine(*commandLine); // Before the plugins and the galaxies are listed
    }
    createMenus();
    setCentralWidget(createPanels());
    createStatusBar();
    setWindowTitle(m_project->title());
    connect(m_project, &Project::titleChanged, this, &QWidget::setWindowTitle);
    resize(1440, 800);
}

MainWindow::~MainWindow()
{
    // The edition zone, its items, and the property editor refer to the components of the application of m_studio,
    // destroyed after this body, before the children of the window: they let them go first. Nothing listens to the
    // application any longer, stopped when it is destroyed.
    m_studio.application().disconnect();
    m_propertyEditor->setComponent(nullptr);
    delete m_zone;
}

void MainWindow::createMenus()
{
    m_project->addFileActions(*menuBar()->addMenu(tr("&File")));
    m_zone->addViewActions(*menuBar()->addMenu(tr("&View")));
    m_zone->addRunActions(*menuBar()->addMenu(tr("&Run")));
}

QWidget* MainWindow::createPanels()
{
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(createSources());
    splitter->addWidget(createEdition());
    splitter->addWidget(createGroup(tr("Components"), new ComponentPanel(m_studio)));
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({320, 760, 360});
    return splitter;
}

QWidget* MainWindow::createSources()
{
    auto* sources = new QTabWidget;
    sources->setDocumentMode(true); // Without a frame, on the background of the window, as the other panels are
    auto* plugins = new PluginPanel(m_studio);
    connect(plugins, &PluginPanel::removing, m_zone, &EditionZone::remove);
    auto* galaxies = new GalaxyPanel(m_studio);
    connect(galaxies, &GalaxyPanel::openRequested, m_project, &Project::openGalaxy);
    insertPage(*sources, *plugins);
    insertPage(*sources, *galaxies);
    return sources;
}

QWidget* MainWindow::createEdition()
{
    connect(m_zone, &EditionZone::selectionChanged, m_propertyEditor, &PropertyEditor::setComponent);
    auto* splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(m_zone);
    splitter->addWidget(m_propertyEditor);
    splitter->addWidget(createProblemList());
    splitter->setStretchFactor(0, 1);
    splitter->setSizes({480, 200, 120});
    return createGroup(tr("Application"), splitter);
}

QWidget* MainWindow::createProblemList()
{
    auto* problems = new ProblemList(m_studio.application());
    connect(problems, &ProblemList::componentActivated, m_zone, &EditionZone::showComponent);
    return problems;
}

void MainWindow::createStatusBar()
{
    statusBar()->addWidget(createPluginDirectoryLabel());
    statusBar()->addPermanentWidget(m_zone->createZoomLabel());
    connect(&m_studio, &Studio::told, this, [this](const QString& message) { tell(*statusBar(), message); });
}

bool MainWindow::openProjectFile(const QString& filePath)
{
    return m_project->openProjectFile(filePath);
}

bool MainWindow::saveProjectFile(const QString& filePath)
{
    return m_project->saveProjectFile(filePath);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    event->setAccepted(m_project->saveChanges());
}
