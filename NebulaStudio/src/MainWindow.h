// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QJsonObject>
#include <QList>
#include <QMainWindow>
#include <QString>

#include "Nebula/Application/Universe.h"

class ComponentModel;
class ComponentViewer;
class EditionScene;
class EditionView;
class GalaxyModel;
class GalaxyViewer;
class PluginModel;
class QAction;
class PluginViewer;
class PropertyEditor;
class QCommandLineParser;
class QLineEdit;
class QTreeView;

// Lists the plugins of <application directory>/plugins, and of the directories given on its command line, and the
// components of the loaded ones, each with a viewer of the one selected; a plugin is loaded from its viewer, or opened
// from anywhere on the disk, and the components are searched for by a part of their name, of their description or of
// their category (Ctrl+F). Next to the plugins, in another tab, the galaxies of a directory, scene/ until another one
// is chosen: the project files there, each telling whether the application knows its plugins, opened from its viewer
// when it does. Between them, the edition zone, where a component dropped from its list is instantiated, and a galaxy
// dropped from its own is placed, above the editor of the properties of the one selected there, and the list of what
// keeps components from working, see ProblemList. A galaxy placed there opens in its place, under a path back to the
// application, where it is edited as the application is: Save galaxy writes it to its file, and every instance of that
// file is read again from it; leaving a galaxy changed since asks whether to save it. Its File menu saves what is in
// the edition zone to a project file, with the description of its galaxy, and opens it again; its View menu zooms the
// edition zone in and out, at the zoom level its status bar tells; its Run menu starts the application assembled there,
// which is not edited while it runs, and stops it.
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
    // Loads pluginId and shows its components; tells why when it cannot be loaded.
    void load(const QString& pluginId);
    // Unloads pluginId and no longer shows its components. Those of the application that are its own are removed
    // with it, once that is confirmed: nothing is done otherwise. Tells why when it cannot be unloaded.
    void unload(const QString& pluginId);
    // Asks for a plugin file, then selects and loads it.
    void openPlugin();

    // filePath is the file of the application, none when empty, which holds it as it is now.
    void setProjectFile(const QString& filePath);
    // Asks, when the galaxies shown changed since they were, then when the application changed since its file holds
    // it, whether to save them, see leaveGalaxies(). Returns false when what was about to replace it is given up: the
    // answer was to cancel, or it could not be saved.
    bool saveChanges();
    // Empties the application, once its changes are saved or given up.
    void newProject();
    // Asks for a project file and opens it, once the changes of the application are saved or given up.
    void openProject();
    // Saves the application to its file, asked for when it has none; false when it is not saved.
    bool saveProject();
    // Asks for the file to save the application to; false when it is not saved.
    bool saveProjectAs();
    // Starts the application; tells why when it cannot be.
    void start();
    // Asks for the description of the application, the one its galaxy has once it is saved.
    void editDescription();

    // Lists the galaxies of directory, and tells which one above them.
    void setGalaxyDirectory(const QString& directory);
    // Asks for the directory whose galaxies are listed.
    void chooseGalaxyDirectory();
    // Opens the galaxy of the file filePath, once the changes of the application are saved or given up. Does nothing
    // when it cannot be loaded, as its viewer tells, and while the application runs.
    void openGalaxy(const QString& filePath);

    // Shows galaxy, one placed in what the edition zone shows, in its place, where it is edited.
    void enterGalaxy(Nebula::Galaxy& galaxy);
    // Shows in the edition zone what is depth galaxies down the path: the application at 0. Those further down are
    // closed.
    void showDepth(qsizetype depth);
    // Closes the galaxies further than depth down the path, the last one first, asking for each one that changed since
    // it was shown whether to save it: saved, every instance of its file is read again from it; its changes given up,
    // it is read again itself. Then shows what is depth galaxies down. Returns false, leaving shown the one whose
    // question was canceled, or that could not be saved, when one was; while the application runs, one that changed
    // is not left, unless closing tells that NebulaStudio closes: it is saved without anything read again then.
    bool leaveGalaxies(qsizetype depth, bool closing = false);
    // Saves the galaxy shown to its file, then reads every instance of that file again from it.
    void saveGalaxy();
    // Writes project, a galaxy as its scene saves it, to its file filePath; tells why and returns false when it cannot
    // be written.
    bool writeGalaxyFile(const QString& filePath, const QJsonObject& project);
    // Reads every instance of the galaxy file filePath again from it, and tells what they could not keep.
    void readInstancesAgain(const QString& filePath);
    // Rebuilds the path above the edition zone: a button for the application and each galaxy above the one shown;
    // hidden while the application is.
    void refreshPath();
    // Tells message in the status bar, for a while.
    void tell(const QString& message);

    Nebula::Universe m_application; // What is assembled in the edition zone, with the plugins it is made from
    QString m_openDirectory;        // Where openPlugin() starts browsing: the directory of the last plugin opened
    QString m_projectFile;          // Empty until the application is saved to a file, or opened from one
    QJsonObject m_savedProject;     // What m_scene saved when m_projectFile was set
    PluginModel* m_pluginModel = nullptr;
    QTreeView* m_pluginView = nullptr;
    EditionScene* m_scene = nullptr;       // Of m_application
    QList<EditionScene*> m_galaxyScenes;   // Opened, each of a galaxy of the one before: m_scene, then them
    QWidget* m_path = nullptr;             // Above the edition zone, while a galaxy is shown
    QAction* m_saveGalaxyAction = nullptr; // Enabled while a galaxy is shown and the application does not run
    EditionView* m_editionView = nullptr;  // Shows m_scene, or the last of m_galaxyScenes
    PluginViewer* m_pluginViewer = nullptr;
    ComponentModel* m_componentModel = nullptr;
    ComponentViewer* m_componentViewer = nullptr;
    PropertyEditor* m_propertyEditor = nullptr; // Of the component selected in m_editionView
    GalaxyModel* m_galaxyModel = nullptr;
    QTreeView* m_galaxyView = nullptr;
    GalaxyViewer* m_galaxyViewer = nullptr;
    QLineEdit* m_galaxyDirectory = nullptr; // Tells the directory of m_galaxyModel
};
