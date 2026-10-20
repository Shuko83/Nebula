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

class ComponentItem;
class LinkItem;
class QGraphicsPathItem;
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
// name it exposes it as at once. None of this is done while the application runs, see Universe: its components can
// still be selected, but nothing is dropped, moved, linked, renamed, recolored, added or removed until it is stopped.
// A galaxy dragged from a GalaxyModel is placed in the application the same way, see Universe::instantiateGalaxy();
// double-clicking it below its name asks for it to be opened, see galaxyOpened(), in another scene that shows it, and
// where it is edited as the application is.
// The links it shows are those of the bindings of the components, drawn again whenever they change from here.
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
    // Double-clicking a settings file edits it; a stargate edits what it exposes by itself, see StargateItem.
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    // Delete removes the selected components, settings files and links, unless a name is edited: the key is for what is
    // typed then.
    void keyPressEvent(QKeyEvent* event) override;
    // A grid, and a hint while nothing is in the edition zone.
    void drawBackground(QPainter* painter, const QRectF& rect) override;

  private:
    // Whether what it shows is edited: while the application does not run.
    [[nodiscard]] bool isEditable() const;
    // Lets the mouse move its items, or keeps it from doing so, see isEditable().
    void setEditable(bool editable);
    // Tells through refused() why what was asked is not done: the application runs.
    void refuseEdition();
    // Shows the galaxy again, once it was read again from its file, with the same components selected.
    void showGalaxyAgain();
    // Shows components, with their links, files and the zones of project, which tells where they are.
    void show(const std::vector<std::unique_ptr<Nebula::Component>>& components,
              const std::vector<std::unique_ptr<Nebula::SettingsFile>>& files, const QJsonObject& project);
    // The interface whose dot is at position; nullptr when there is none.
    [[nodiscard]] InterfaceItem* interfaceAt(const QPointF& position) const;
    // The interface the link being drawn would be bound to if released at position; nullptr when there is none.
    [[nodiscard]] InterfaceItem* linkTargetAt(const QPointF& position) const;
    // Dims the interfaces a link drawn from source cannot be bound to; none when source is nullptr.
    void dimLinkTargets(const InterfaceItem* source);
    // Shows component, one of the application, at position.
    ComponentItem* showComponent(Nebula::Component& component, const QPointF& position);
    // The component shown at position, or whose interface is; nullptr when there is none.
    [[nodiscard]] ComponentItem* componentAt(const QPointF& position) const;
    // Names component name, as typed in place of its name; tells why through refused() when it cannot be.
    void rename(Nebula::Component& component, const QString& name);
    // Removes item, with the links of its interfaces, and its component from the application.
    void remove(ComponentItem& item);
    // Empties the edition zone, while the components it shows still exist.
    void removeItems();
    // Binds the consumed one to the provided one, whichever is which, and draws their link, in place of the one the
    // consumed one had; does nothing when they cannot be bound.
    void link(InterfaceItem& source, InterfaceItem& target);
    // The link at position, unless something is shown over it there; nullptr when there is none.
    [[nodiscard]] LinkItem* linkAt(const QPointF& position) const;
    // Unbinds the interfaces links are drawn between, and removes them from the edition zone.
    void unlink(const QList<LinkItem*>& links);
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
    // Draws a link for each binding of the components shown, and none other: those already drawn stay.
    void refreshLinks();
    // Edits what the stargate of item exposes in a dialog of parent, see StargateDialog, which is left open once this
    // returns; tells why it is not when the scene is not editable.
    void editStargate(const StargateItem& item, QWidget* parent);
    // Adds a stargate that exposes interfaceItem, an interface of a component, bound to it, next to it, and edits the
    // name it exposes it as.
    void expose(const InterfaceItem& interfaceItem);
    // Names name, an interface of stargate of kind, as its item shows it, newName, as typed in place of its name;
    // tells why through refused() when it cannot be.
    void renameExposed(Nebula::Stargate& stargate, InterfaceItem::Kind kind, const QString& name,
                       const QString& newName);

    Nebula::Universe& m_application;
    const Nebula::ComponentFactory& m_factory;
    Nebula::Galaxy* m_galaxy = nullptr;         // The one shown; nullptr when it is the application
    QJsonObject m_saved;                        // What save() returned once m_galaxy was shown: see isModified()
    QStringList m_selected;                     // The components selected while m_galaxy is read again, by name
    InterfaceItem* m_linkSource = nullptr;      // Where the link being drawn starts; nullptr when none is
    QGraphicsPathItem* m_pendingLink = nullptr; // The link being drawn
    std::size_t m_zoneCount = 0;                // The zones added so far: each takes the next color
};
