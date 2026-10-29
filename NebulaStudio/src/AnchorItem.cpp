// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "AnchorItem.h"

#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPolygonF>
#include <QWidget>

#include "LinkItem.h"

namespace
{

// From its center to each of its corners.
constexpr qreal Radius = 5;
// Around it, where it is still grabbed.
constexpr qreal GrabMargin = 3;
constexpr qreal Outline = 1.5;

// Its diamond, around its position.
QPolygonF diamond(qreal radius)
{
    return QPolygonF({{0, -radius}, {radius, 0}, {0, radius}, {-radius, 0}});
}

} // namespace

AnchorItem::AnchorItem(const QPointF& position, LinkItem& link) : QGraphicsItem(&link), m_link(link)
{
    setPos(position);
    setToolTip(
        tr("A point this link goes through\nDrag it to move it; double-click it, or right-click it, to remove it"));
    setEditable(true);
}

int AnchorItem::type() const
{
    return Type;
}

LinkItem& AnchorItem::link() const
{
    return m_link;
}

void AnchorItem::setEditable(bool editable)
{
    m_editable = editable;
    setCursor(editable ? Qt::SizeAllCursor : Qt::ArrowCursor);
}

QRectF AnchorItem::boundingRect() const
{
    const qreal radius = Radius + GrabMargin;
    return {-radius, -radius, 2 * radius, 2 * radius};
}

QPainterPath AnchorItem::shape() const
{
    QPainterPath path;
    path.addPolygon(diamond(Radius + GrabMargin));
    return path;
}

void AnchorItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* widget)
{
    // In the color of its link, filled as the background is: a point of the line, which a dot of an interface is not.
    const QPalette palette = widget != nullptr ? widget->palette() : QApplication::palette();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(palette.color(m_link.isSelected() ? QPalette::Highlight : QPalette::ButtonText), Outline));
    painter->setBrush(palette.color(QPalette::Base));
    painter->drawPolygon(diamond(Radius));
}

void AnchorItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    event->setAccepted(m_editable && event->button() == Qt::LeftButton);
}

void AnchorItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    setPos(pos() + event->scenePos() - event->lastScenePos());
    m_link.anchorsChanged();
}
