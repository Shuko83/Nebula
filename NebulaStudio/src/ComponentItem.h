// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QColor>
#include <QGraphicsObject>
#include <QIcon>
#include <QList>
#include <QString>
#include <QStringList>
#include <map>
#include <utility>

#include "InterfaceItem.h"

class NameEditor;
class QFontMetricsF;
class QGraphicsScene;
class QPainter;
class QPainterPath;
class QPalette;

namespace Nebula
{
class Component;
}

// A component instantiated in the edition zone: its name above the name of its component, then its interfaces, one
// InterfaceItem per row: those it consumes, their dots on its left edge, then those it provides, their dots on its
// right edge, each until it is put on the other one or its row swapped with another, and rows of text under them when
// it is given some. A triangle in
// its top right corner tells that it runs. An exclamation mark in its top left corner tells that it does not work: an
// interface it cannot do without is bound to nothing, see Component::unboundMandatoryInterfaces(); its tooltip names
// them and tells how to bind them. It can be selected, and, while it is editable, moved, and its name edited in place
// by double-clicking its names. It can be given a color, which its names are written over.
class ComponentItem : public QGraphicsObject
{
    Q_OBJECT

  public:
    // A row of text under its interfaces: a name, and what it is, written fainter on the right.
    using Row = std::pair<QString, QString>;

    // component, owned by the application, outlives the item.
    ComponentItem(Nebula::Component& component, QString componentName);

    // Where the top left corner of an item is from its position, whatever its interfaces: they are under its names,
    // which are around it.
    [[nodiscard]] static QPointF corner();

    [[nodiscard]] Nebula::Component& component() const;
    // The name of what it is an instance of, under its own name: its component, its galaxy, or a stargate.
    [[nodiscard]] const QString& componentName() const;
    // Its interfaces, in the order of Component::consumedInterfaces() and Component::providedInterfaces().
    [[nodiscard]] const QList<InterfaceItem*>& consumedInterfaces() const;
    [[nodiscard]] const QList<InterfaceItem*>& providedInterfaces() const;
    // Its interface of kind named name; nullptr when it has none.
    [[nodiscard]] InterfaceItem* interfaceItem(InterfaceItem::Kind kind, const QString& name) const;
    // Puts the dot of its interface of kind named name on side, its name next to it, inside: its links go out of that
    // edge. It stays there while the component has that interface, shown again or not, renamed in place too, and goes
    // with it; nothing when it has none so named.
    void setInterfaceSide(InterfaceItem::Kind kind, const QString& name, InterfaceItem::Side side);
    // Its interfaces, in the order of their rows, from the top: those it consumes, then those it provides, until rows
    // are swapped; one it has from then on comes last.
    [[nodiscard]] QList<InterfaceItem*> interfaceRows() const;
    // Swaps the rows of two of its interfaces, which keep them as setInterfaceSide() keeps an edge; nothing when it has
    // not both.
    void swapInterfaces(const InterfaceItem::Key& one, const InterfaceItem::Key& other);
    // Shows its interfaces in the order of keys, from the top, those it does not tell after them, in the order of their
    // rows; a key of an interface it does not have is left out.
    void setInterfaceOrder(const QList<InterfaceItem::Key>& keys);
    // Not valid until it is given one: it has the colors of the style. As opaque as it was chosen.
    [[nodiscard]] const QColor& color() const;
    void setColor(const QColor& color);
    // Edits its name in place, until Enter is pressed or anything else is clicked; Escape gives the edition up. It
    // must be in a scene. See nameEdited().
    void editName();
    // Whether it can be moved, and its name edited by double-clicking it: it is until told otherwise.
    void setEditable(bool editable);
    // The icon before the name of what it is an instance of; none, a null icon, until it is given one.
    [[nodiscard]] const QIcon& componentIcon() const;
    void setIcon(const QIcon& icon);
    // Shows it as something that opens, a galaxy say: about at the start of its tooltip, in place of the id of its
    // component, then howToOpen. Double-clicking it below its name, or anywhere while it is not editable, asks for it
    // to be opened: see openRequested().
    void setOpenable(const QString& about, const QString& howToOpen);
    // The rows under its interfaces, as a stargate lists the properties it exposes; none until it is given some.
    void setRows(const QList<Row>& rows);
    // Whether the names of its interfaces are edited in place by double-clicking them, as those of a stargate are, see
    // interfaceNameEdited(): they are not until told otherwise.
    void setInterfacesRenamable(bool renamable);
    // Edits the name of interfaceItem, one of its own, in place, as editName() does its name.
    void editInterfaceName(const InterfaceItem& interfaceItem);

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    // Whether the name of interfaceItem, one of its own, is being edited in place: it does not paint it meanwhile.
    [[nodiscard]] bool isNameEdited(const InterfaceItem& interfaceItem) const;

  signals:
    // name was typed in place of its name, on one line: its component is still to be named so.
    void nameEdited(const QString& name);
    // newName was typed in place of name, an interface of it of kind, on one line: it is still to be named so.
    void interfaceNameEdited(InterfaceItem::Kind kind, const QString& name, const QString& newName);
    // It was double-clicked to be opened, see setOpenable(): it does not open by itself.
    void openRequested();
    // Its interfaces were shown again, without their links, as one of them was renamed or removed.
    void interfacesRebuilt();

  protected:
    // Not moved while its name is edited: the double-click that edits it still holds it, and the view scrolling to
    // show what is typed would drag it along.
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

  private:
    // Whether its name, or the one of an interface, is being edited in place.
    [[nodiscard]] bool isNameEdited() const;
    // The interface whose name is at position; nullptr when there is none.
    [[nodiscard]] const InterfaceItem* interfaceNameAt(const QPointF& position) const;
    // Shows the interfaces of the component as they are now, again from the start when one of those it shows was
    // renamed or removed. Then whether it works.
    void refreshInterfaces();
    // Deletes its interface items, with their links, unless they show the first of consumed and provided: one was
    // renamed or removed, as those of a stargate are. Returns whether it deleted them.
    bool clearIfChanged(const QList<Nebula::InterfaceInfo>& consumed, const QList<Nebula::InterfaceInfo>& provided);
    // Shows consumed, then provided, under its names, adding the items they miss, each dot on the edge it is put on.
    void showInterfaces(const QList<Nebula::InterfaceInfo>& consumed, const QList<Nebula::InterfaceInfo>& provided);
    // Has items show interfaces, of kind, adding those they miss.
    void showInterfaces(QList<InterfaceItem*>& items, const QList<Nebula::InterfaceInfo>& interfaces,
                        InterfaceItem::Kind kind);
    // Puts its interfaces on their rows, under its names, each dot on the edge it is put on.
    void placeInterfaces();
    // Puts item, one of its interfaces, on the row at y, its dot on the edge it is put on.
    void place(InterfaceItem& item, qreal y);
    // The edge the dot of its interface of kind named name is put on: the one of its kind until it is put on another.
    [[nodiscard]] InterfaceItem::Side interfaceSide(InterfaceItem::Kind kind, const QString& name) const;
    // Keeps side as the edge the dot of its interface of kind named name is put on, without moving it.
    void keepSide(InterfaceItem::Kind kind, const QString& name, InterfaceItem::Side side);
    // The rows m_order tells, from the top, of the interfaces it has.
    [[nodiscard]] QList<InterfaceItem*> orderedRows() const;
    // Whether it has the interface key tells.
    [[nodiscard]] bool has(const InterfaceItem::Key& key) const;
    // Tells that the interface whose name was edited is to be named newName, see interfaceNameEdited(): its dot stays
    // on its edge, and on its row, once it is.
    void nameEditedInterface(const QString& newName);
    // Keeps the edge and the row of the dot of its interface of kind named name for it once it is named newName, as
    // well as for it now.
    void keepPlace(InterfaceItem::Kind kind, const QString& name, const QString& newName);
    // Forgets the edges and the rows of the interfaces it no longer has.
    void forgetLost();
    // Whether it works: what it misses is drawn in the warning color, and told by its mark and its tooltip.
    void refreshUnbound();
    // What it is, how it opens, and what it misses, unbound, when it does not work.
    [[nodiscard]] QString toolTipText(const QStringList& unbound) const;
    // Where its rows start: under its interfaces.
    [[nodiscard]] qreal rowsTop() const;
    // The editor of the names of interfaces, created when first asked for.
    NameEditor& interfaceEditor();
    // Where a double-click edits its name: one that opens keeps the line of its name to be renamed, the rest of it to
    // be opened.
    [[nodiscard]] QRectF nameArea() const;
    // Edits its name or the one of an interface at position, or asks for it to be opened; false when there is nothing
    // to do there.
    bool openOrEditAt(const QPointF& position);
    // Edits the name of the interface at position, when there is one and it can be; returns whether it does.
    bool editInterfaceNameAt(const QPointF& position);
    // Asks for it to be opened, when it opens; returns whether it does.
    bool open();

    // Its background and its outline, highlighted when selected.
    void paintBody(QPainter& painter, const QPalette& palette, bool selected) const;
    // Its color, behind its names only: its interfaces keep the colors of the style, which their dots are drawn with.
    void paintColor(QPainter& painter, const QPainterPath& outline) const;
    // A shape, as the dots of its interfaces, in ink: a triangle while it runs, nothing while it is edited.
    void paintStateMark(QPainter& painter, const QColor& ink) const;
    // In the corner across, while it does not work: an exclamation mark, which tells it without color, in a disc of
    // the warning color, outlined in the one of the item to stand apart from the color it was given.
    void paintWarningMark(QPainter& painter, const QPalette& palette, const QFont& font) const;
    // Its name, in ink, unless it is the text being edited.
    void paintName(QPainter& painter, const QColor& ink, const QFont& font) const;
    // Its rows, under its interfaces.
    void paintRows(QPainter& painter, const QPalette& palette, const QFont& font) const;
    // row in rect: the name on the left, what it is on the right, both elided to share the width.
    static void paintRow(QPainter& painter, const QPalette& palette, const QRectF& rect, const Row& row);
    // The name of its component, in ink, after its icon when it has one.
    void paintComponentName(QPainter& painter, const QColor& ink, const QFont& font) const;
    // The icon before the name of its component, both in the middle.
    void paintIconAndComponentName(QPainter& painter, const QFontMetricsF& metrics) const;

    Nebula::Component& m_component;
    QString m_componentName;
    QStringList m_unbound; // Component::unboundMandatoryInterfaces(), as refreshInterfaces() last read them
    QColor m_color;
    bool m_editable = true;
    QIcon m_icon; // Before the name of its component
    bool m_openable = false;
    QString m_about;     // The start of its tooltip, when it opens
    QString m_howToOpen; // In its tooltip, when it opens
    QList<Row> m_rows;
    bool m_interfacesRenamable = false;
    // Its children.
    QList<InterfaceItem*> m_consumedInterfaces;
    QList<InterfaceItem*> m_providedInterfaces;
    // The edges the dots of its interfaces are put on, by kind and name, when it is not the one of their kind: the
    // items are deleted when an interface is renamed or removed.
    std::map<InterfaceItem::Key, InterfaceItem::Side> m_sides;
    // The order of the rows of its interfaces, from the top, once some were swapped; empty while they are in the one of
    // consumedInterfaces() then providedInterfaces().
    QList<InterfaceItem::Key> m_order;
    NameEditor* m_nameEditor = nullptr;      // Over its name while it is edited; created when first so
    NameEditor* m_interfaceEditor = nullptr; // Over the name of an interface while it is edited, the same way
    InterfaceItem::Kind m_editedKind = InterfaceItem::Kind::Provided; // Of the interface whose name is edited
    QString m_editedName;                                             // Its name before
};

// The items of components among those of scene, the topmost first.
[[nodiscard]] QList<ComponentItem*> componentItems(const QGraphicsScene& scene);
