// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QColor>
#include <QGraphicsObject>
#include <QIcon>
#include <QList>
#include <QString>
#include <QStringList>
#include <utility>

#include "InterfaceItem.h"

class NameEditor;

namespace Nebula
{
class Component;
}

// A component instantiated in the edition zone: its name above the name of its component, then its interfaces, one
// InterfaceItem per row: those it consumes on its left edge, then those it provides on its right edge, and rows of
// text under them when it is given some. A triangle in its top right corner tells that it runs. An exclamation mark in
// its top left corner tells that it does not work: an interface it cannot do without is bound to nothing, see
// Component::unboundMandatoryInterfaces(); its tooltip names them and tells how to bind them. It can be selected, and,
// while it is editable, moved, and its name edited in place by double-clicking its names. It can be given a color,
// which its names are written over.
class ComponentItem : public QGraphicsObject
{
    Q_OBJECT

  public:
    // A row of text under its interfaces: a name, and what it is, written fainter on the right.
    using Row = std::pair<QString, QString>;

    // component, owned by the application, outlives the item.
    ComponentItem(Nebula::Component& component, QString componentName);

    [[nodiscard]] Nebula::Component& component() const;
    // Its interfaces, in the order of Component::consumedInterfaces() and Component::providedInterfaces().
    [[nodiscard]] const QList<InterfaceItem*>& consumedInterfaces() const;
    [[nodiscard]] const QList<InterfaceItem*>& providedInterfaces() const;
    // Not valid until it is given one: it has the colors of the style. As opaque as it was chosen.
    [[nodiscard]] const QColor& color() const;
    void setColor(const QColor& color);
    // Edits its name in place, until Enter is pressed or anything else is clicked; Escape gives the edition up. It
    // must be in a scene. See nameEdited().
    void editName();
    // Whether it can be moved, and its name edited by double-clicking it: it is until told otherwise.
    void setEditable(bool editable);
    // The icon before the name of what it is an instance of; none until it is given one.
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
    // Where its rows start: under its interfaces.
    [[nodiscard]] qreal rowsTop() const;

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
    NameEditor* m_nameEditor = nullptr;      // Over its name while it is edited; created when first so
    NameEditor* m_interfaceEditor = nullptr; // Over the name of an interface while it is edited, the same way
    InterfaceItem::Kind m_editedKind = InterfaceItem::Kind::Provided; // Of the interface whose name is edited
    QString m_editedName;                                             // Its name before
};
