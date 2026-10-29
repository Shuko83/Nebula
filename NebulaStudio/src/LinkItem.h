// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QCoreApplication>
#include <QGraphicsItem>
#include <QList>
#include <QPainterPath>
#include <QPointF>
#include <QString>
#include <functional>

#include "LinkStyle.h"

class AnchorItem;
class InterfaceItem;

namespace Nebula
{
class Component;
} // namespace Nebula

// An interface a component consumes bound to the one another component provides, in the edition zone: a line between
// their dots, drawn in the style it is given, which follows them. Clicking it selects it, which removing it needs, see
// EditionScene; the rubber band of a view does not: a link that crosses it would go with the components it selects.
// Hovered, it is drawn thicker, and its tooltip tells what it binds; selected, in the color of the selection, as a
// component is.
// It can be made to go through points of the edition zone, its anchors, in their order, wherever its components are:
// see AnchorItem. Its line goes from each of them to the next one in its style, as it goes from one dot to the other.
class LinkItem : public QGraphicsItem
{
    Q_DECLARE_TR_FUNCTIONS(LinkItem)

  public:
    // For qgraphicsitem_cast.
    enum
    {
        Type = UserType + 3
    };

    // What a link binds: consumed, an interface of consumer, to provided, one of provider. It tells the link a link
    // drawn again replaces, as the interfaces of their components are shown again.
    struct Ends
    {
        const Nebula::Component* consumer = nullptr;
        QString consumed;
        const Nebula::Component* provider = nullptr;
        QString provided;

        // For a std::map.
        [[nodiscard]] bool operator<(const Ends& other) const;
    };

    // Not a child of them, but deleted with the first one deleted. Drawn in style until it is given another one.
    // anchored is called with it whenever its anchors change, once it is built: given, added, moved or removed.
    LinkItem(InterfaceItem& provided, InterfaceItem& consumed, LinkStyle style,
             std::function<void(const LinkItem&)> anchored = {});
    ~LinkItem() override;

    // The line of a link in style from a provided interface to a consumed one, their dots being there in the scene.
    [[nodiscard]] static QPainterPath route(LinkStyle style, const QPointF& provided, const QPointF& consumed);

    [[nodiscard]] int type() const override;
    // The one it is drawn from.
    [[nodiscard]] InterfaceItem& provided() const;
    // The one it is drawn to, whose component is bound to the other one.
    [[nodiscard]] InterfaceItem& consumed() const;
    // What it binds, as its interfaces are named now.
    [[nodiscard]] Ends ends() const;

    [[nodiscard]] LinkStyle style() const;
    // Draws it in style from now on.
    void setStyle(LinkStyle style);
    // What it draws, in the scene: see route(), from each of the points it goes through to the next one.
    [[nodiscard]] const QPainterPath& path() const;

    // The points it goes through between its interfaces, in their order, in the scene.
    [[nodiscard]] QList<QPointF> anchors() const;
    // Has it go through anchors, in their order, in place of those it went through.
    void setAnchors(const QList<QPointF>& anchors);
    // Has it go through position as well, between the two points of its line position is closest to: its interfaces,
    // or the anchors next to each other there.
    void addAnchor(const QPointF& position);
    // Has it no longer go through anchor, one of its own.
    void removeAnchor(const AnchorItem& anchor);
    // Draws it again through its anchors, and tells who it was built with that they changed: one of them was moved by
    // the mouse, say.
    void anchorsChanged();

    // Whether the mouse drags its anchors.
    [[nodiscard]] bool isEditable() const;
    void setEditable(bool editable);

    // Draws it between its interfaces again, where they are now.
    void follow();

    [[nodiscard]] QRectF boundingRect() const override;
    // Around its line, wide enough to be clicked.
    [[nodiscard]] QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    // Refuses to be selected by the rubber band of a view; has its anchors painted again once it is selected or no
    // longer is, in its color.
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

  private:
    void setHovered(bool hovered);
    // Paints its anchors again, in its color.
    void updateAnchors();
    // Where its line goes through: the dot of its provided interface, its anchors, then the dot of its consumed one.
    [[nodiscard]] QList<QPointF> stops() const;
    // Where position is inserted among its anchors: the index of the stretch of its line, between two of its stops,
    // position is the closest to.
    [[nodiscard]] qsizetype nearestStretch(const QPointF& position) const;

    InterfaceItem& m_provided;
    InterfaceItem& m_consumed;
    LinkStyle m_style;
    QPainterPath m_path;  // In the scene: the item stays at its origin
    QPainterPath m_shape; // Around m_path
    bool m_hovered = false;
    QList<AnchorItem*> m_anchors; // Its children, in their order along its line
    std::function<void(const LinkItem&)> m_anchored;
    bool m_editable = true;
};
