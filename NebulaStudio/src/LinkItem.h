// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QCoreApplication>
#include <QGraphicsItem>
#include <QPainterPath>

class InterfaceItem;

// An interface a component consumes bound to the one another component provides, in the edition zone: a curve between
// their dots, which follows them. Clicking it selects it, which removing it needs, see EditionScene; the rubber band of
// a view does not: a link that crosses it would go with the components it selects. Hovered, it is drawn thicker, and
// its tooltip tells what it binds; selected, in the color of the selection, as a component is.
class LinkItem : public QGraphicsItem
{
    Q_DECLARE_TR_FUNCTIONS(LinkItem)

  public:
    // For qgraphicsitem_cast.
    enum
    {
        Type = UserType + 3
    };

    // Not a child of them, but deleted with the first one deleted.
    LinkItem(InterfaceItem& provided, InterfaceItem& consumed);
    ~LinkItem() override;

    // The curve of a link from a provided interface to a consumed one, their dots being there in the scene.
    [[nodiscard]] static QPainterPath curve(const QPointF& provided, const QPointF& consumed);

    [[nodiscard]] int type() const override;
    // The one it is drawn from.
    [[nodiscard]] InterfaceItem& provided() const;
    // The one it is drawn to, whose component is bound to the other one.
    [[nodiscard]] InterfaceItem& consumed() const;

    // Draws it between its interfaces again, where they are now.
    void follow();

    [[nodiscard]] QRectF boundingRect() const override;
    // Around its curve, wide enough to be clicked.
    [[nodiscard]] QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    // Refuses to be selected by the rubber band of a view.
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

  private:
    void setHovered(bool hovered);

    InterfaceItem& m_provided;
    InterfaceItem& m_consumed;
    QPainterPath m_curve; // In the scene: the item stays at its origin
    QPainterPath m_shape; // Around m_curve
    bool m_hovered = false;
};
