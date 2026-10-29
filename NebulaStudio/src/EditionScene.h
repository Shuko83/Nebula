// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QColor>
#include <QGraphicsScene>
#include <QJsonObject>
#include <QList>
#include <QStringList>
#include <cstddef>
#include <memory>
#include <vector>

#include "InterfaceItem.h"
#include "LinkAnchors.h"
#include "LinkStyle.h"

class AnchorItem;
class ComponentItem;
class ContextMenu;
class LinkItem;
class QAction;
class QGraphicsPathItem;
class QJsonArray;
class QMenu;
class QMimeData;
class SettingsItem;
class StargateItem;
class ZoneItem;

namespace Nebula
{
class Component;
class ComponentFactory;
class Galaxy;
class Stargate;
class Universe;
class SettingsFile;
} // namespace Nebula

// The zone where the application is assembled: a component dragged from a ComponentModel and dropped here is
// instantiated in the application, and shown where it was dropped. Dragging from the dot of an interface to the dot of
// one it can be bound to, a provided and a consumed one of the same type, binds them and draws a link between them;
// meanwhile, the interfaces it cannot be bound to are dimmed. Its context menu adds a zone, which groups components,
// see ZoneItem, unless it is opened on a component, and renames, recolors or removes the one under the cursor; it
// renames and recolors the component under the cursor too, see ComponentItem. A component keeps a name no other one of
// the application has: see refused().
// The same menu removes the component under the cursor from the application, with its links, and the Delete key
// removes the selected ones. A link, selected by clicking it, is removed the same way, which unbinds its interfaces: by
// the Delete key, or from the menu opened on it.
// The menu adds a settings file of the application as well, see SettingsItem, whose properties are chosen at once, and
// again when it is double-clicked or from its own menu, which writes its default values or removes it.
// The menu adds a stargate too, see StargateItem, which exposes what the galaxy the project is has once placed: what
// it exposes is chosen at once, and again when it is double-clicked or from its own menu, see StargateDialog.
// Expose..., in the menu of the dot of an interface, adds one that exposes that interface, bound to it, and edits the
// name it exposes it as at once.
// Bind to, in the menu of the dot of an interface consumed, binds it to one of the interfaces shown it can be bound to,
// those it is bound to checked: choosing one of these unbinds it.
// Double-clicking a link, or Add anchor point in its menu, has it go through that point, an anchor, see AnchorItem,
// which the mouse drags; double-clicking an anchor, or Remove anchor point in its menu, removes it.
// None of this is done while the application runs, see Universe: its components can still be selected, but nothing is
// dropped, moved, linked, renamed, recolored, added or removed until it is stopped.
// A galaxy dragged from a GalaxyModel is placed in the application the same way, see Universe::instantiateGalaxy();
// double-clicking it below its name asks for it to be opened, see galaxyOpened(), in another scene that shows it, and
// where it is edited as the application is.
// The links it shows are those of the bindings of the components, drawn again whenever they change from here, in the
// style it is given: curves until another one is chosen. A link drawn again keeps its anchors, see LinkAnchors.
class EditionScene : public QGraphicsScene
{
    Q_OBJECT

  public:
    EditionScene(Nebula::Universe& application, const Nebula::ComponentFactory& factory, QObject* parent = nullptr);
    // Shows galaxy, one of the galaxies of application or of those they hold, where its file shows them, and edits it
    // as the scene of the application edits the application: what is dropped, added, linked, renamed or removed here
    // is in galaxy, which save() writes as its file is to hold it. Its galaxies open as those of the application do.
    // Shown again when galaxy is read again from its file, see Nebula::Universe::reloadGalaxy(). galaxy outlives the
    // scene.
    EditionScene(Nebula::Universe& application, const Nebula::ComponentFactory& factory, Nebula::Galaxy& galaxy,
                 QObject* parent = nullptr);

    // The galaxy it shows; nullptr when it shows the application.
    [[nodiscard]] Nebula::Galaxy* galaxy() const;
    // Whether what save() returns changed since the galaxy was shown, or read again from its file; false for the
    // application, whose changes are followed by who saves it.
    [[nodiscard]] bool isModified() const;
    // The component selected, when a single one is; nullptr otherwise. See selectionChanged().
    [[nodiscard]] Nebula::Component* selectedComponent() const;
    // Selects component alone, one of the application, and returns what shows it; nullptr, selecting nothing, when it
    // is not shown.
    ComponentItem* select(const Nebula::Component& component);

    // The application as a project file holds it, see README.md: what Universe::save() writes, with where its
    // components and its settings files are in the edition zone, the color its components were given, and its zones.
    // The galaxy, as its file is to hold it, when it shows one: see Universe::saveGalaxy().
    [[nodiscard]] QJsonObject save() const;
    // Replaces the application with the one project holds, as save() wrote it, see Universe::load(), and shows
    // it as it was. Returns what could not be restored, a sentence each: none when everything was.
    QStringList load(const QJsonObject& project);
    // Empties the edition zone, and the application of its components.
    void reset();
    // Removes components, some of the application, from it and from the edition zone, with their links.
    void remove(const QList<Nebula::Component*>& components);

    // How its links are drawn, and the one being drawn.
    [[nodiscard]] LinkStyle linkStyle() const;
    // Draws its links in style from now on, those it shows already included. Only what is seen changes: it is not
    // saved, see save().
    void setLinkStyle(LinkStyle style);

  signals:
    // What was just asked of it is not done; why is a sentence to show.
    void refused(const QString& why);
    // What was just asked of it is done, which is worth telling; what is a sentence to show.
    void told(const QString& what);
    // galaxy, one it shows, was double-clicked to be opened: the scene does not open it by itself.
    void galaxyOpened(Nebula::Galaxy& galaxy);
    // galaxy was dropped and placed in the application, which loaded the plugins it needs.
    void galaxyPlaced(Nebula::Galaxy& galaxy);

  protected:
    void dragEnterEvent(QGraphicsSceneDragDropEvent* event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent* event) override;
    void dropEvent(QGraphicsSceneDragDropEvent* event) override;
    // Pressing the dot of an interface starts a link, which follows the cursor until the button is released.
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    // Double-clicking a settings file edits it, an anchor removes it, and a link anchors it there; a stargate edits
    // what it exposes by itself, see StargateItem.
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    // Not while it is busy, see isBusy().
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    // Delete removes the selected components, settings files and links, unless a name is edited: the key is for what is
    // typed then. Not while it is busy either, see isBusy().
    void keyPressEvent(QKeyEvent* event) override;
    // A grid, and a hint while nothing is in the edition zone.
    void drawBackground(QPainter* painter, const QRectF& rect) override;

  private:
    // Whether what it shows is edited: while the application does not run.
    [[nodiscard]] bool isEditable() const;
    // Whether it is editable; tells why it is not through refused() when it is not.
    bool checkEditable();
    // Lets the mouse move its items, or keeps it from doing so, see isEditable().
    void setEditable(bool editable);
    // Tells through refused() why what was asked is not done: the application runs.
    void refuseEdition();
    // Whether a link is drawn or an item is dragged: what is dragged could be removed from under the cursor.
    [[nodiscard]] bool isBusy() const;

    // Shows the galaxy as its file holds it now, and takes that as saved, see isModified().
    void showGalaxy();
    // Removes its items before the galaxy is read again from its file, keeping the names of the components selected.
    void hideGalaxy();
    // Shows the galaxy again, once it was read again from its file, with the same components selected.
    void showGalaxyAgain();
    // The names of the components selected.
    [[nodiscard]] QStringList selectedNames() const;
    // Selects the components named one of names.
    void selectNamed(const QStringList& names);
    // Selects item alone.
    void selectOnly(QGraphicsItem& item);
    // What shows component; nullptr when nothing does.
    [[nodiscard]] ComponentItem* itemOf(const Nebula::Component& component) const;
    // The settings files of what it shows: those of the galaxy, or of the application.
    [[nodiscard]] const std::vector<std::unique_ptr<Nebula::SettingsFile>>& shownFiles() const;
    // Writes where items, the settings files shown, are in project, in the order of shownFiles().
    void placeSettings(QJsonObject& project, const QList<SettingsItem*>& items) const;

    // Shows components, with their links, files and the zones of project, which tells where they are.
    void show(const std::vector<std::unique_ptr<Nebula::Component>>& components,
              const std::vector<std::unique_ptr<Nebula::SettingsFile>>& files, const QJsonObject& project);
    // Shows component where json, what a project holds of it, tells, in the color it tells.
    void showSaved(Nebula::Component& component, const QJsonObject& json);
    // Draws the links of what components, those shown, bind, through the anchors project holds, and the lines of the
    // stargates.
    void showLinks(const std::vector<std::unique_ptr<Nebula::Component>>& components, const QJsonObject& project);
    // Has the stargates shown draw the lines to the components of the properties they expose.
    void refreshStargateLinks();
    // Shows files where settings, what a project holds of them, tells: one each, in their order.
    void showSettingsFiles(const std::vector<std::unique_ptr<Nebula::SettingsFile>>& files, const QJsonArray& settings);
    // Adds the zones a project holds.
    void showZones(const QJsonArray& zones);
    // Shows item at position, a new item of the edition zone.
    void place(QGraphicsItem& item, const QPointF& position);

    // Instantiates what data, dropped, carries in what it shows: a galaxy or a component. nullptr when it cannot be.
    Nebula::Component* dropped(const QMimeData& data);
    // Places the galaxy of filePath in what it shows, and tells what it misses; tells why, and returns nullptr, when
    // it cannot be.
    Nebula::Galaxy* placeGalaxy(const QString& filePath);
    // Tells what galaxy, just placed, misses of its file, when it misses anything.
    void tellProblems(const Nebula::Galaxy& galaxy);

    // Starts drawing a link from source.
    void startLink(InterfaceItem& source);
    // Shows what hovered, the dot under the cursor where event is, is: no tooltip shows by itself while a button is
    // pressed, and its type is what tells whether the link being drawn can end there.
    void showTypeOf(const InterfaceItem* hovered, const QGraphicsSceneMouseEvent& event) const;
    // Draws the link being drawn to hovered, the dot under the cursor, when it would be bound to it, which tells that
    // releasing there binds them; to cursor otherwise.
    void drawPendingLink(const InterfaceItem* hovered, const QPointF& cursor);
    // Ends the link being drawn, and binds its interfaces when it ends at position on one it can be bound to.
    void finishLink(const QPointF& position);
    // Stops drawing a link.
    void endLink();
    // The interface whose dot is at position; nullptr when there is none.
    [[nodiscard]] InterfaceItem* interfaceAt(const QPointF& position) const;
    // The interface the link being drawn would be bound to if released at position; nullptr when there is none.
    [[nodiscard]] InterfaceItem* linkTargetAt(const QPointF& position) const;
    // Dims the interfaces a link drawn from source cannot be bound to; none when source is nullptr.
    void dimLinkTargets(const InterfaceItem* source);
    // The interfaces shown consumed can be bound to: those of its type other components provide, from the first
    // component shown, each in the order of its interfaces.
    [[nodiscard]] QList<InterfaceItem*> linkTargets(const InterfaceItem& consumed) const;

    // Shows the menu of where event is: about the component, the settings file or the link there, or about the edition
    // zone when there is none, then about the zone there.
    void showMenu(const QGraphicsSceneContextMenuEvent& event);
    // Adds to menu what is done to component, at position, with dialogs of parent; false when component is nullptr.
    bool addComponentEntries(ContextMenu& menu, ComponentItem* component, const QPointF& position, QWidget* parent);
    // Adds to menu binding consumed, an interface shown, in a submenu: to one of the interfaces shown it can be bound
    // to, or unbinding it from one it is bound to. Nothing when consumed is nullptr, or provided.
    void addBindEntries(ContextMenu& menu, InterfaceItem* consumed);
    // Adds to bindMenu, a submenu of menu, an entry for each of the interfaces shown consumed can be bound to, checked
    // when it is bound to it; one that tells there is none otherwise.
    void addBindTargets(ContextMenu& menu, QMenu& bindMenu, InterfaceItem& consumed);
    // Adds to bindMenu, a submenu of menu, the entry that binds consumed to provided, or unbinds them, see
    // toggleLink().
    QAction* addBindTarget(ContextMenu& menu, QMenu& bindMenu, InterfaceItem& consumed, InterfaceItem& provided);
    // Adds to menu what is done to component as what it is: exposing the interface at position, opening a galaxy,
    // editing a stargate.
    void addKindEntries(ContextMenu& menu, ComponentItem& component, const QPointF& position, QWidget* parent);
    // Adds to menu exposing exposed, an interface of a component that is not a stargate; nothing when it is nullptr.
    void addExposeEntry(ContextMenu& menu, const InterfaceItem* exposed);
    // Adds to menu opening galaxy; nothing when it is nullptr.
    void addOpenEntry(ContextMenu& menu, Nebula::Galaxy* galaxy);
    // Adds to menu editing what stargate exposes in a dialog of parent; nothing when it is nullptr.
    void addEditEntry(ContextMenu& menu, StargateItem* stargate, QWidget* parent);
    // Adds to menu renaming, recoloring and removing component.
    void addComponentActions(ContextMenu& menu, ComponentItem& component, QWidget* parent);
    // Adds to menu what is done to settings; false when it is nullptr.
    bool addSettingsEntries(ContextMenu& menu, SettingsItem* settings, QWidget* parent);
    // Adds to menu editing settings, writing its default values and removing it.
    void addSettingsActions(ContextMenu& menu, SettingsItem& settings, QWidget* parent);
    // Adds to menu removing anchor; false when it is nullptr.
    static bool addAnchorEntries(ContextMenu& menu, AnchorItem* anchor);
    // Adds to menu anchoring link at position, removing its anchors, and removing it; false when it is nullptr.
    bool addLinkEntries(ContextMenu& menu, LinkItem* link, const QPointF& position);
    // Adds to menu anchoring link at position and removing its anchors.
    static void addAnchoringEntries(ContextMenu& menu, LinkItem& link, const QPointF& position);
    // Adds to menu what is added at position: a zone, a settings file or a stargate.
    void addSceneEntries(ContextMenu& menu, const QPointF& position, QWidget* parent);
    // Adds to menu what is done to the zone at position; nothing when there is none.
    void addZoneEntries(ContextMenu& menu, const QPointF& position, QWidget* parent);
    // Asks in a dialog of parent for the color of component, its opacity too, from the one it has: the one of the
    // style until it is given one.
    void recolor(ComponentItem& component, QWidget* parent);
    // Asks in a dialog of parent for the color of zone.
    static void recolor(ZoneItem& zone, QWidget* parent);
    // Removes zone, not what is in it.
    void removeZone(ZoneItem& zone);
    // Adds a settings file to what it shows, at position, and chooses what it holds at once in a dialog of parent.
    void addSettingsFileAt(const QPointF& position, QWidget* parent);
    // Adds a stargate to what it shows, at position, and chooses what it exposes at once in a dialog of parent.
    void addStargateAt(const QPointF& position, QWidget* parent);

    // Removes what is selected: its links, its settings files and its components, with their links.
    void removeSelected();
    // Removes each of removed, see remove().
    template <typename Item> void removeAll(const QList<Item*>& removed);
    // Shows component, one of the application, at position.
    ComponentItem* showComponent(Nebula::Component& component, const QPointF& position);
    // A new item that shows component as what it is: a stargate, a galaxy or a component of a plugin.
    [[nodiscard]] ComponentItem* newItem(Nebula::Component& component);
    [[nodiscard]] ComponentItem* newStargateItem(Nebula::Stargate& stargate);
    [[nodiscard]] ComponentItem* newGalaxyItem(Nebula::Galaxy& galaxy);
    // The component shown at position, or whose interface is; nullptr when there is none.
    [[nodiscard]] ComponentItem* componentAt(const QPointF& position) const;
    // Names component name, as typed in place of its name; tells why through refused() when it cannot be.
    void rename(Nebula::Component& component, const QString& name);
    // Removes item, with the links of its interfaces, and its component from the application.
    void remove(ComponentItem& item);
    // Empties the edition zone, while the components it shows still exist.
    void removeItems();
    // Binds the consumed one to the provided one, whichever is which, and draws their link, in place of the one the
    // consumed one had, or next to those of a list; does nothing when they cannot be bound.
    void link(InterfaceItem& source, InterfaceItem& target);
    // The link at position, unless something is shown over it there; nullptr when there is none.
    [[nodiscard]] LinkItem* linkAt(const QPointF& position) const;
    // The anchor of a link at position, unless something is shown over it there; nullptr when there is none.
    [[nodiscard]] AnchorItem* anchorAt(const QPointF& position) const;
    // Does what double-clicking at position does, see mouseDoubleClickEvent(), with dialogs of parent; false when
    // nothing is there that it does something to.
    bool doubleClick(const QPointF& position, QWidget* parent);
    // Edits the settings file at position in a dialog of parent; false when there is none.
    bool editSettingsAt(const QPointF& position, QWidget* parent);
    // Removes the anchor at position from its link; false when there is none.
    bool removeAnchorAt(const QPointF& position);
    // Has the link at position go through it; false when there is none.
    bool anchorLinkAt(const QPointF& position);
    // Unbinds the interfaces links are drawn between, each from the one it is drawn to, and removes them from the
    // edition zone: a list keeps the others.
    void unlink(const QList<LinkItem*>& links);
    // Unbinds consumed from provided when they are linked, see unlink(); binds them otherwise, see link().
    void toggleLink(InterfaceItem& consumed, InterfaceItem& provided);
    // The zone position is in, the smallest one when several are; nullptr when there is none.
    [[nodiscard]] ZoneItem* zoneAt(const QPointF& position) const;
    // Adds a zone, its top left corner at position; of the next color when color is not valid.
    ZoneItem* addZone(const QPointF& position, const QSizeF& size, const QString& title, const QColor& color = {});
    // Shows file, one of the application, at position.
    SettingsItem* showSettings(Nebula::SettingsFile& file, const QPointF& position);
    // The settings file at position, unless something is shown over it there; nullptr when there is none.
    [[nodiscard]] SettingsItem* settingsAt(const QPointF& position) const;
    // Edits the file of item in a dialog of parent, see SettingsDialog, which is left open once this returns.
    void editSettings(const SettingsItem& item, QWidget* parent);
    // Writes the default values of the file of item to it, and tells whether it could.
    void writeDefaults(const SettingsItem& item);
    // Removes item, with its links, and its file from the application.
    void remove(SettingsItem& item);
    // Draws a link for each binding of the components shown, and none other: those already drawn stay. Forgets the
    // anchors of those no longer drawn.
    void refreshLinks();
    // Draws a link for each binding of the components shown, and none other.
    void drawBindings();
    // Draws the links of consumed to provided, the interfaces shown it is bound to, and none other: those already
    // drawn stay.
    void drawLinks(InterfaceItem& consumed, const QList<InterfaceItem*>& provided);
    // Draws a new link from provided to consumed, through the anchors the one it replaces had, see m_anchors.
    void drawLink(InterfaceItem& provided, InterfaceItem& consumed);
    // Edits what the stargate of item exposes in a dialog of parent, see StargateDialog, which is left open once this
    // returns; tells why it is not when the scene is not editable.
    void editStargate(const StargateItem& item, QWidget* parent);
    // Adds a stargate that exposes interfaceItem, an interface of a component, bound to it, next to it, and edits the
    // name it exposes it as.
    void expose(const InterfaceItem& interfaceItem);
    // Shows stargate at position, which exposes as name an interface of kind, and edits that name at once: the dot of
    // the stargate is on the edge across from the one of the interface.
    void showExposing(Nebula::Stargate& stargate, const QString& name, InterfaceItem::Kind kind,
                      const QPointF& position);
    // Names name, an interface of stargate of kind, as its item shows it, newName, as typed in place of its name;
    // tells why through refused() when it cannot be.
    void renameExposed(Nebula::Stargate& stargate, InterfaceItem::Kind kind, const QString& name,
                       const QString& newName);
    // Draws the grid of the edition zone across rect.
    void drawGrid(QPainter& painter, const QRectF& rect) const;
    // Draws how to fill the edition zone, while nothing is in it.
    void drawHint(QPainter& painter) const;

    Nebula::Universe& m_application;
    const Nebula::ComponentFactory& m_factory;
    Nebula::Galaxy* m_galaxy = nullptr;         // The one shown; nullptr when it is the application
    QJsonObject m_saved;                        // What save() returned once m_galaxy was shown: see isModified()
    QStringList m_selected;                     // The components selected while m_galaxy is read again, by name
    InterfaceItem* m_linkSource = nullptr;      // Where the link being drawn starts; nullptr when none is
    QGraphicsPathItem* m_pendingLink = nullptr; // The link being drawn
    LinkStyle m_linkStyle = LinkStyle::Curved;  // Of its links, and of m_pendingLink
    LinkAnchors m_anchors;                      // Of its links, which they outlive
    std::size_t m_zoneCount = 0;                // The zones added so far: each takes the next color
};
