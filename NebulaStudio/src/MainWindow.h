// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QJsonObject>
#include <QList>
#include <QMainWindow>
#include <QMessageBox>
#include <QString>

#include "LinkStyle.h"
#include "Nebula/Application/Universe.h"

class ComponentItem;
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
class QComboBox;
class QCommandLineParser;
class QLineEdit;
class QMenu;
class QSortFilterProxyModel;
class QToolButton;
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
// edition zone in and out, at the zoom level its status bar tells, and chooses how its links are drawn, in the
// application as in the galaxies; its Run menu starts the application assembled there, which is not edited while it
// runs, and stops it.
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
    // While the application runs, what would replace it is disabled, as is what edits it: see EditionScene. It is
    // saved with the values of the edition.
    void createFileMenu();
    // New, Open..., Save and Save As....
    void addProjectActions(QMenu& menu);
    // Save Galaxy: the galaxy shown, to its own file, which its other instances are read again from.
    void addSaveGalaxyAction(QMenu& menu);
    // The zoom of the edition zone, also changed there with + and -, or with Ctrl and the mouse wheel, then how its
    // links are drawn.
    void createViewMenu();
    // Links: the styles of LinkStyles, one of them checked, the one the links are drawn in.
    void addLinkStyleMenu(QMenu& menu);
    // Draws the links of every scene in style from now on: those of the application, and of the galaxies opened.
    void setLinkStyle(LinkStyle style);
    void createRunMenu();
    // Follows what the application tells: whether it runs, its settings files that failed, its components.
    void followApplication();
    // Enables what can be done while the application runs, or not, as running tells.
    void followRunning(bool running);
    // Shows the application again when the first galaxy shown is no longer one of its components: before it goes,
    // with those it holds.
    void leaveRemovedGalaxy();
    // Lists the plugins of the directories of commandLine, see MainWindow(), with those of plugins/.
    void readCommandLine(const QCommandLineParser* commandLine);
    // Lists the plugins again, as they are loaded or not, and the components of those loaded.
    void refreshPlugins();
    // Lists the plugins, their components and the galaxies again: those a project loaded, or opened from their file,
    // are known from now on, and so are the galaxies they were missing from.
    void refreshSources();

    // The edition zone between the plugins and the components, next to the list a component is dragged from.
    QWidget* createPanels();
    // What the application is made from, on the disk: the plugins, and the galaxies in another tab.
    QWidget* createSources();
    // The plugins, under the button that opens one from anywhere on the disk; the first one selected.
    QWidget* createPluginList();
    // The galaxies of a directory, those the application knows the plugins of opened from their viewer, or activated,
    // and dragged into the edition zone; under the directory, and the button that chooses another one.
    QWidget* createGalaxyList();
    QWidget* createGalaxyBar();
    // The components whose name, description or category, as chosen next to the search bar, holds what is typed there,
    // whatever its case: through a filter, which hands what is dragged over to the model, which makes the data.
    QWidget* createComponentList();
    // What filter, of the components, looks for, and where.
    QWidget* createSearchBar(QSortFilterProxyModel& filter);
    // Where filter, of the components, looks for the text, among SearchFields.
    static QComboBox* createSearchFields(QSortFilterProxyModel& filter);
    // Ctrl+F selects what search, the search bar, holds, for what is typed to replace it.
    void addFindShortcut(QLineEdit& search);
    // The edition zone, above the properties of the component selected there, then the problems; the zone takes the
    // room.
    QWidget* createEdition();
    // The view of the scene of the application, under the path to the galaxy shown.
    QWidget* createZone();
    // Follows what scene tells, the one of the application or of a galaxy, and draws its links in the style chosen.
    void followScene(EditionScene& scene);
    // Edits the properties of the component selected in scene, when it is the one shown.
    void showSelected(const EditionScene& scene);
    // What keeps components from working; activating one shows it, selected, in the middle of the edition zone.
    QWidget* createProblemList();
    // Shows the component of the application named name, selected, in the middle of the edition zone, once the
    // galaxies shown are left.
    void showComponent(const QString& name);
    // Shows item in the middle of the edition zone, and gives it the keys, as Delete is for it; nothing when item is
    // nullptr.
    void centerOnItem(const ComponentItem* item);
    // The directory of the plugins, and the zoom of the edition zone.
    void createStatusBar();

    // Loads pluginId and shows its components; tells why when it cannot be loaded.
    void load(const QString& pluginId);
    // Unloads pluginId and no longer shows its components. Those of the application that are its own are removed
    // with it, once that is confirmed: nothing is done otherwise. Tells why when it cannot be unloaded.
    void unload(const QString& pluginId);
    // Whether to unload pluginId, which removes used, the components of the application that are its own.
    bool confirmUnload(const QString& pluginId, const QList<Nebula::Component*>& used);
    // Unloads pluginId, whose components are no longer in the application; tells why when it cannot be.
    void unloadPlugin(const QString& pluginId);
    // Asks for a plugin file, then selects and loads it.
    void openPlugin();
    // Lists the plugin of the file filePath, then selects and loads it; tells why when it cannot be read.
    void addPlugin(const QString& filePath);
    // Selects pluginId, just listed, and loads it unless it is.
    void selectAndLoad(const QString& pluginId);

    // filePath is the file of the application, none when empty, which holds it as it is now.
    void setProjectFile(const QString& filePath);
    // Asks, when the galaxies shown changed since they were, then when the application changed since its file holds
    // it, whether to save them, see leaveGalaxies(). Returns false when what was about to replace it is given up: the
    // answer was to cancel, or it could not be saved.
    bool saveChanges();
    // Asks whether to save the application, which changed; false when it is not saved, nor its changes given up.
    bool saveOrDiscardProject();
    // Asks, with text, whether to save what changed: Save, Discard, or Cancel to do neither.
    QMessageBox::StandardButton askToSave(const QString& text);
    // Empties the application, once its changes are saved or given up.
    void newProject();
    // Asks for a project file and opens it, once the changes of the application are saved or given up.
    void openProject();
    // Asks for the project file to open; none when none is chosen.
    QString askForProjectFile();
    // Where the dialogs of the project files start: the file of the application, when it has one; the default
    // directory otherwise.
    [[nodiscard]] QString projectDialogPath() const;
    // Shows project, read from the file filePath, in place of the application, and tells what could not be restored.
    void showProject(const QString& filePath, const QJsonObject& project);
    // Saves the application to its file, asked for when it has none; false when it is not saved.
    bool saveProject();
    // Asks for the file to save the application to; false when it is not saved.
    bool saveProjectAs();
    // Writes the application to the file filePath, which its galaxies are written relative to; tells why and returns
    // false when it cannot be written.
    bool writeProject(const QString& filePath);
    // Writes json to the file filePath; tells why in a box titled title, and returns false, when it cannot be written.
    bool writeFile(const QString& title, const QString& filePath, const QJsonObject& json);
    // Starts the application; tells why when it cannot be.
    void start();
    // Asks for the description of the application, the one its galaxy has once it is saved.
    void editDescription();

    // Lists the galaxies of directory, tells which one above them, and shows the first one in its viewer.
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
    // Closes the scenes of the galaxies further than depth down the path, the last one first.
    void closeGalaxyScenes(qsizetype depth);
    // Deletes scene, no longer listened to.
    void closeScene(EditionScene& scene);
    // Closes the galaxies further than depth down the path, the last one first, asking for each one that changed since
    // it was shown whether to save it: saved, every instance of its file is read again from it; its changes given up,
    // it is read again itself. Then shows what is depth galaxies down. Returns false, leaving shown the one whose
    // question was canceled, or that could not be saved, when one was; while the application runs, one that changed
    // is not left, unless closing tells that NebulaStudio closes: it is saved without anything read again then.
    bool leaveGalaxies(qsizetype depth, bool closing = false);
    // Closes the last galaxy down the path, as leaveGalaxies() does; false when it is not.
    bool leaveLastGalaxy(bool closing);
    // What is done with the galaxy of scene, the last one down the path, before it is left: NoButton when it did not
    // change, Save once it is saved, Discard when its changes are given up, Cancel when it is not left.
    QMessageBox::StandardButton leaveChoice(const EditionScene& scene, bool closing);
    // Shows galaxy, which changed, and asks whether to save it, see leaveGalaxies(): Cancel while the application
    // runs, unless closing.
    QMessageBox::StandardButton askToLeave(const Nebula::Galaxy& galaxy, bool closing);
    // Reads galaxy again, just left, as choice tells: every instance of its file once it is saved, itself when its
    // changes are given up. Not while the application runs, which it does only when NebulaStudio closes.
    void readAgain(Nebula::Galaxy& galaxy, QMessageBox::StandardButton choice);
    // Reads galaxy again from its file, its changes given up, and tells what it could not keep.
    void readGalaxyAgain(Nebula::Galaxy& galaxy);
    // Saves the galaxy shown to its file, then reads every instance of that file again from it.
    void saveGalaxy();
    // Writes the galaxy scene shows to its file; tells why and returns false when it cannot be written.
    bool writeGalaxy(const EditionScene& scene);
    // Reads every instance of the galaxy file filePath again from it, and tells what they could not keep.
    void readInstancesAgain(const QString& filePath);
    // Reads every instance of the galaxy file filePath again from it, and returns what they could not keep.
    QStringList reloadInstances(const QString& filePath);
    // Rebuilds the path above the edition zone, again from the start, as the names may have changed since: a button
    // for the application and each galaxy above the one shown, then the button of Save Galaxy; hidden while the
    // application is.
    void refreshPath();
    // The application, then the galaxies down to the one shown.
    void addPathLevels();
    // The level of the path depth galaxies down, named name: a button that shows it again, or its name when it is the
    // one shown.
    void addPathLevel(qsizetype depth, const QIcon& levelIcon, const QString& name);
    // A button that shows what is depth galaxies down again, once the changes of those further down are saved or
    // given up.
    QToolButton* createLevelButton(qsizetype depth, const QIcon& levelIcon, const QString& name);
    // Tells message in the status bar, for a while.
    void tell(const QString& message);
    // Tells text in a box titled title, which waits for the user.
    void warn(const QString& title, const QString& text);

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

    LinkStyle m_linkStyle = LinkStyle::Curved; // Of the links of m_scene and m_galaxyScenes, until another is chosen
};
