// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QMainWindow>
#include <QString>

#include "Studio.h"

class EditionZone;
class Project;
class PropertyEditor;
class QCommandLineParser;

// The window of NebulaStudio, which assembles its parts around a Studio. On the left, in tabs, the plugins of
// <application directory>/plugins, and of the directories given on its command line, see PluginPanel, and the
// galaxies of a directory, see GalaxyPanel; on the right, the components of the loaded plugins, see ComponentPanel.
// Between them, the edition zone, see EditionZone, where a component dropped from its list is instantiated, and a
// galaxy dropped from its own is placed, above the editor of the properties of the one selected there, and the list of
// what keeps components from working, see ProblemList: activating one shows it, selected, in the middle of the zone.
// Its File menu saves what is in the edition zone to a project file, see Project, and opens it again; its View menu
// gives the keys to the field above the edition zone that finds the instances it shows (Ctrl+Shift+F), zooms the
// edition zone in and out, at the zoom level its status bar tells, and chooses how its links are drawn; its Run menu
// starts the application assembled there, and stops it. The galaxy of NebulaStudio, in StudioPlugin/, assembles the
// same parts from components.
class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    // commandLine, when there is one, is the command line of NebulaStudio, processed by a parser it was given to
    // Universe::addCommandLineOptions(): the plugins of its directories are listed with those of plugins/,
    // and its plugin root is the one of the project files.
    explicit MainWindow(const QCommandLineParser* commandLine = nullptr, QWidget* parent = nullptr);
    ~MainWindow() override;

    // Replaces the application with the one of the project file filePath, whose plugins it loads, and tells what
    // could not be restored. Tells why, changes nothing and returns false when the file cannot be read.
    bool openProjectFile(const QString& filePath);
    // Writes the application to the project file filePath, from then on its file; tells why and returns false when it
    // cannot be written.
    bool saveProjectFile(const QString& filePath);

  protected:
    // Not closed when the changes to save are neither saved nor given up.
    void closeEvent(QCloseEvent* event) override;

  private:
    // File, View and Run.
    void createMenus();
    // The edition zone between the plugins and the components, next to the list a component is dragged from.
    QWidget* createPanels();
    // What the application is made from, on the disk: the plugins, and the galaxies in another tab.
    QWidget* createSources();
    // The edition zone, above the properties of the component selected there, then the problems; the zone takes the
    // room.
    QWidget* createEdition();
    // What keeps components from working; activating one shows it, selected, in the middle of the edition zone.
    QWidget* createProblemList();
    // The directory of the plugins, what the parts tell, and the zoom of the edition zone.
    void createStatusBar();

    Studio m_studio; // The application assembled in the edition zone, with the plugins it is made from
    EditionZone* m_zone = nullptr;
    Project* m_project = nullptr;               // Of the application of m_studio, as m_zone shows it
    PropertyEditor* m_propertyEditor = nullptr; // Of the component selected in m_zone
};
