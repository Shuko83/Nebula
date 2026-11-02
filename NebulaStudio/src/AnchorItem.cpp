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
#include <algorithm>

#include "LinkItem.h"

namespace
{

// From its center to each of its corners.
constexpr qreal Radius = 5;
// Of the disc of a junction.
constexpr qreal JunctionRadius = 4;
// Around it, where it is still grabbed.
constexpr qreal GrabMargin = 3;
constexpr qreal Outline = 1.5;
// Over the links, which it is a point of, under the components.
constexpr qreal ZValue = -0.5;

// Its diamond, around its position.
QPolygonF diamond(qreal radius)
{
    return QPolygonF({{0, -radius}, {radius, 0}, {0, radius}, {-radius, 0}});
}

// The diamond of an anchor, outlined in color, filled as background is: a point of the line, which a dot of an
// interface is not.
void paintAnchor(QPainter& painter, const QColor& color, const QColor& background)
{
    painter.setPen(QPen(color, Outline));
    painter.setBrush(background);
    painter.drawPolygon(diamond(Radius));
}

// The disc of a junction, filled in color, as the line is.
void paintJunction(QPainter& painter, const QColor& color)
{
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(QPointF(), JunctionRadius, JunctionRadius);
}

} // namespace

AnchorItem::AnchorItem(const QPointF& position)
{
    setPos(position);
    setZValue(ZValue);
    setEditable(true);
}

AnchorItem::~AnchorItem()
{
    for(LinkItem* link : std::as_const(m_links))
    {
        link->forgetAnchor(*this);
    }
}

int AnchorItem::type() const
{
    return Type;
}

const QList<LinkItem*>& AnchorItem::links() const
{
    return m_links;
}

bool AnchorItem::isJunction() const
{
    if(m_links.size() < 2)
    {
        return false;
    }
    // No two links go to the same consumed interface: one that goes there next parts from the others.
    const AnchorItem* next = m_links.constFirst()->after(*this);
    return next == nullptr ||
           std::ranges::any_of(m_links, [this, next](const LinkItem* link) { return link->after(*this) != next; });
}

void AnchorItem::addLink(LinkItem& link)
{
    if(!m_links.contains(&link))
    {
        m_links.append(&link);
    }
}

void AnchorItem::removeLink(LinkItem& link)
{
    m_links.removeOne(&link);
}

void AnchorItem::setEditable(bool editable)
{
    m_editable = editable;
    setCursor(editable ? Qt::SizeAllCursor : Qt::ArrowCursor);
}

void AnchorItem::refresh()
{
    update();
    setToolTip(isJunction()
                   ? tr("A junction: the links through it share their line up to it, then part\nDrag it to move it; "
                        "double-click it, or right-click it, to remove it with the line they share: each goes from "
                        "its provided interface again\nShift+drag from it to bind another interface through it")
                   : tr("A point this line goes through\nDrag it to move it; double-click it, or right-click it, to "
                        "remove it\nShift+drag from it to bind another interface through it"));
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
    // In the color of its links.
    const QPalette palette = widget != nullptr ? widget->palette() : QApplication::palette();
    const QColor color = palette.color(isSelectedLink() ? QPalette::Highlight : QPalette::ButtonText);
    painter->setRenderHint(QPainter::Antialiasing);
    if(isJunction())
    {
        paintJunction(*painter, color);
        return;
    }
    paintAnchor(*painter, color, palette.color(QPalette::Base));
}

void AnchorItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    event->setAccepted(m_editable && event->button() == Qt::LeftButton);
}

void AnchorItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    setPos(pos() + event->scenePos() - event->lastScenePos());
    for(LinkItem* link : std::as_const(m_links))
    {
        link->anchorsChanged();
    }
}

bool AnchorItem::isSelectedLink() const
{
    return std::ranges::any_of(m_links, [](const LinkItem* link) { return link->isSelected(); });
}
