// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QCoreApplication>
#include <QGraphicsItem>

class LinkItem;

// A point a link is made to go through, on its line: a small diamond, which the mouse drags while the link is editable,
// and the link follows. Its position is the one of the point, in the scene: its link stays at the origin.
class AnchorItem : public QGraphicsItem
{
    Q_DECLARE_TR_FUNCTIONS(AnchorItem)

  public:
    // For qgraphicsitem_cast.
    enum
    {
        Type = UserType + 4
    };

    // A child of link, at position.
    AnchorItem(const QPointF& position, LinkItem& link);

    [[nodiscard]] int type() const override;
    // The link that goes through it.
    [[nodiscard]] LinkItem& link() const;
    // Whether the mouse drags it, which its cursor tells.
    void setEditable(bool editable);

    [[nodiscard]] QRectF boundingRect() const override;
    // Around its diamond, wide enough to be grabbed.
    [[nodiscard]] QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  protected:
    // Takes the press that drags it, while it is editable; otherwise leaves it to its link, which it selects.
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    // Moves it by itself, and has its link follow: the items selected stay where they are, which they would not if it
    // moved as a movable item does.
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;

  private:
    LinkItem& m_link;
    bool m_editable = true;
};
