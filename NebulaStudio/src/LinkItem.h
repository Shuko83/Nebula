// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QCoreApplication>
#include <QGraphicsItem>
#include <QPainterPath>

#include "LinkStyle.h"

class InterfaceItem;

// An interface a component consumes bound to the one another component provides, in the edition zone: a line between
// their dots, drawn in the style it is given, which follows them. Clicking it selects it, which removing it needs, see
// EditionScene; the rubber band of a view does not: a link that crosses it would go with the components it selects.
// Hovered, it is drawn thicker, and its tooltip tells what it binds; selected, in the color of the selection, as a
// component is.
class LinkItem : public QGraphicsItem
{
    Q_DECLARE_TR_FUNCTIONS(LinkItem)

  public:
    // For qgraphicsitem_cast.
    enum
    {
        Type = UserType + 3
    };

    // Not a child of them, but deleted with the first one deleted. Drawn in style until it is given another one.
    LinkItem(InterfaceItem& provided, InterfaceItem& consumed, LinkStyle style);
    ~LinkItem() override;

    // The line of a link in style from a provided interface to a consumed one, their dots being there in the scene.
    [[nodiscard]] static QPainterPath route(LinkStyle style, const QPointF& provided, const QPointF& consumed);

    [[nodiscard]] int type() const override;
    // The one it is drawn from.
    [[nodiscard]] InterfaceItem& provided() const;
    // The one it is drawn to, whose component is bound to the other one.
    [[nodiscard]] InterfaceItem& consumed() const;

    [[nodiscard]] LinkStyle style() const;
    // Draws it in style from now on.
    void setStyle(LinkStyle style);
    // What it draws, in the scene: see route().
    [[nodiscard]] const QPainterPath& path() const;

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
    // Refuses to be selected by the rubber band of a view.
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

  private:
    void setHovered(bool hovered);

    InterfaceItem& m_provided;
    InterfaceItem& m_consumed;
    LinkStyle m_style;
    QPainterPath m_path;  // In the scene: the item stays at its origin
    QPainterPath m_shape; // Around m_path
    bool m_hovered = false;
};
