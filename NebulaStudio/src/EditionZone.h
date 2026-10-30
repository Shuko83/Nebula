// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QWidget>

#include "LinkStyle.h"

class ComponentFinder;
class ComponentItem;
class EditionScene;
class EditionView;
class QAction;
class QIcon;
class QLabel;
class QMenu;
class QToolButton;
class Studio;

namespace Nebula
{
class Component;
class Galaxy;
} // namespace Nebula

// The edition zone of NebulaStudio: the scene of the application of a Studio, see EditionScene, in its view, see
// EditionView, under the path to the galaxy shown in its place. A galaxy placed there, when it is opened, is shown in
// a scene of its own, where it is edited as the application is, under a path back to the application, each level a
// button that shows it again once those further down are left: leaving a galaxy changed since it was shown asks
// whether to save it, see leaveGalaxies(). Save Galaxy writes the galaxy shown to its file, then reads every instance
// of that file again from it. On the right of the path, a field finds the instances the view shows by their name or
// the one of their component, see ComponentFinder, and shows the one found in its middle. Its View menu gives the keys
// to that field, zooms the view and chooses how the links of every scene are drawn, as its studio keeps it; its Run
// menu starts the application, which is not edited while it runs, and stops it. What its scenes and its field refuse
// or tell, it tells through its studio, as the plugins a galaxy placed loaded.
class EditionZone : public QWidget
{
    Q_OBJECT

  public:
    explicit EditionZone(Studio& studio, QWidget* parent = nullptr);
    // Its items show the components of the application of its studio: it is deleted before the application is.
    ~EditionZone() override;

    // The scene of the application, which its project file holds.
    [[nodiscard]] EditionScene& applicationScene() const;
    // The component selected in the scene shown, when a single one is; nullptr otherwise. See selectionChanged().
    [[nodiscard]] Nebula::Component* selectedComponent() const;
    // Saves the galaxy shown to its file: enabled while one is shown and the application does not run. It is the
    // zone's, which shows it at the end of the path; a menu shows it too.
    [[nodiscard]] QAction* saveGalaxyAction() const;
    // Adds to menu Find Instances, which gives the keys to the field that finds them (Ctrl+Shift+F), what zooms the
    // view in and out, also done with + and -, or with Ctrl and the mouse wheel, then Links: the styles the links of
    // every scene can be drawn in, the one they are drawn in checked.
    void addViewActions(QMenu& menu);
    // Adds to menu Start and Stop, which start the application and stop it, each enabled when it can be done.
    void addRunActions(QMenu& menu);
    // A label that tells the zoom level of the view, as it changes; who takes it owns it.
    [[nodiscard]] QLabel* createZoomLabel() const;

    // Shows the component of the application named name, selected, in the middle of the view, once the galaxies
    // shown are left; nothing when one of them is not left, or when no component has that name.
    void showComponent(const QString& name);
    // Removes components, some of the application, from it and from its scene, with their links: those of a plugin
    // about to be unloaded, say.
    void remove(const QList<Nebula::Component*>& components);
    // Closes the galaxies further than depth down the path, the last one first, asking for each one that changed since
    // it was shown whether to save it: saved, every instance of its file is read again from it; its changes given up,
    // it is read again itself. Then shows what is depth galaxies down: the application at 0. Returns false, leaving
    // shown the one whose question was canceled, or that could not be saved, when one was; while the application runs,
    // one that changed is not left, unless closing tells that NebulaStudio closes: it is saved without anything read
    // again then.
    bool leaveGalaxies(qsizetype depth, bool closing = false);

  signals:
    // The component selected in the scene shown changed, or another scene is shown: component is the one selected
    // there, when a single one is; nullptr otherwise.
    void selectionChanged(Nebula::Component* component);

  private:
    // The bar above the view: the path, hidden while the application is shown, then the field that finds instances.
    void layOut();
    // The path to the galaxy shown, then on the right the field that finds the instances the view shows.
    QWidget* createBar();
    // Save Galaxy, which works by its shortcut wherever it is shown.
    void createSaveGalaxyAction();
    // Links: the styles of LinkStyles, one of them checked, the one the links are drawn in.
    void addLinkStyleMenu(QMenu& menu);
    // Follows what the application of its studio tells, whether it runs and its components, and the style of the
    // links.
    void followStudio();
    // Enables Save Galaxy as running, whether the application runs, tells: its instances are read again, which is not
    // done while it runs.
    void followRunning(bool running);
    // Shows the application again when the first galaxy shown is no longer one of its components: before it goes,
    // with those it holds.
    void leaveRemovedGalaxy();
    // Draws the links of every scene in style: those of the application, and of the galaxies opened.
    void drawLinks(LinkStyle style);
    // Follows what scene tells, the one of the application or of a galaxy, and draws its links in the style chosen.
    void followScene(EditionScene& scene);
    // Tells message through the studio.
    void tell(const QString& message);
    // Tells which component is selected in scene, when it is the one shown.
    void showSelected(const EditionScene& scene);
    // Shows item in the middle of the view, and gives it the keys, as Delete is for it; nothing when item is nullptr.
    void centerOnItem(const ComponentItem* item);
    // Starts the application; tells why when it cannot be.
    void start();

    // The scene shown: the one of the application, or of the last galaxy down the path.
    [[nodiscard]] EditionScene& shownScene() const;
    // Shows galaxy, one placed in what the zone shows, in its place, where it is edited.
    void enterGalaxy(Nebula::Galaxy& galaxy);
    // Shows what is depth galaxies down the path: the application at 0. Those further down are closed.
    void showDepth(qsizetype depth);
    // Closes the scenes of the galaxies further than depth down the path, the last one first.
    void closeGalaxyScenes(qsizetype depth);
    // Deletes scene, no longer listened to.
    void closeScene(EditionScene& scene);
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

    // Rebuilds the path above the view, again from the start, as the names may have changed since: a button for the
    // application and each galaxy above the one shown, then the button of Save Galaxy; hidden while the application
    // is.
    void refreshPath();
    // The application, then the galaxies down to the one shown.
    void addPathLevels();
    // The level of the path depth galaxies down, named name: a button that shows it again, or its name when it is the
    // one shown.
    void addPathLevel(qsizetype depth, const QIcon& levelIcon, const QString& name);
    // A button that shows what is depth galaxies down again, once the changes of those further down are saved or
    // given up.
    QToolButton* createLevelButton(qsizetype depth, const QIcon& levelIcon, const QString& name);

    Studio& m_studio;
    EditionScene* m_scene = nullptr;       // Of the application of m_studio
    QList<EditionScene*> m_galaxyScenes;   // Opened, each of a galaxy of the one before: m_scene, then them
    QWidget* m_path = nullptr;             // Above the view, while a galaxy is shown
    QAction* m_saveGalaxyAction = nullptr; // Enabled while a galaxy is shown and the application does not run
    EditionView* m_view = nullptr;         // Shows m_scene, or the last of m_galaxyScenes
    ComponentFinder* m_finder = nullptr;   // Above the view, on the right, of what it shows
};
