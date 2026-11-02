// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QCoreApplication>
#include <QGraphicsItem>
#include <QList>

class LinkItem;

// A point links are made to go through, on their line: a small diamond, which the mouse drags while they are editable,
// and the links follow. The links of a same provided interface that go through it go through the same points before
// it, see LinkItem: up to it, their line is one. Where they part, it is a junction, a filled disc, as on the drawing of
// a circuit. Its position is the one of the point, in the scene: it is not a child of its links, and goes with the last
// one that leaves it.
class AnchorItem : public QGraphicsItem
{
    Q_DECLARE_TR_FUNCTIONS(AnchorItem)

  public:
    // For qgraphicsitem_cast.
    enum
    {
        Type = UserType + 4
    };

    // At position, which no link goes through yet.
    explicit AnchorItem(const QPointF& position);
    // Has the links that still go through it forget it: the scene can delete it before them, as it is cleared.
    ~AnchorItem() override;

    [[nodiscard]] int type() const override;
    // The links that go through it, in the order they came: the first one draws the line they share up to it.
    [[nodiscard]] const QList<LinkItem*>& links() const;
    // Whether the links that go through it part there: they do not all go to the same point next. A point a single link
    // goes through is an anchor of its own.
    [[nodiscard]] bool isJunction() const;
    // A link goes through it, or no longer does: LinkItem adds and removes itself.
    void addLink(LinkItem& link);
    void removeLink(LinkItem& link);
    // Whether the mouse drags it, which its cursor tells.
    void setEditable(bool editable);
    // Paints it again, and tells what it is in its tooltip: its links, or the points after it, changed.
    void refresh();

    [[nodiscard]] QRectF boundingRect() const override;
    // Around its diamond, wide enough to be grabbed.
    [[nodiscard]] QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  protected:
    // Takes the press that drags it, while it is editable; otherwise leaves it to the link under it, which it selects.
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    // Moves it by itself, and has its links follow: the items selected stay where they are, which they would not if it
    // moved as a movable item does.
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;

  private:
    // Whether one of its links is selected: it is painted in the color of the selection, as their line is.
    [[nodiscard]] bool isSelectedLink() const;

    QList<LinkItem*> m_links;
    bool m_editable = true;
};
