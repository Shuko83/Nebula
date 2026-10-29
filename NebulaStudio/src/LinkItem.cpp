// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "LinkItem.h"

#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsView>
#include <QList>
#include <QPainter>
#include <QPainterPathStroker>
#include <QPalette>
#include <QWidget>
#include <algorithm>
#include <cmath>

#include "InterfaceItem.h"
#include "Nebula/Component/Component.h"

namespace
{

constexpr qreal LineWidth = 1.5;
// Hovered or selected.
constexpr qreal ThickLineWidth = 3;
// How wide, around its line, a link is hovered and clicked: much wider than it is drawn.
constexpr qreal PickWidth = 10;
// How far, at least, a curved or right-angled link goes straight out of its interfaces before turning to the other one.
constexpr qreal MinimumReach = 40;

// Out of the right edge of the provider, into the left edge of the consumer, wherever they are.
QPainterPath curved(const QPointF& provided, const QPointF& consumed)
{
    const QPointF reach(std::max(MinimumReach, std::abs(consumed.x() - provided.x()) / 2), 0);
    QPainterPath path(provided);
    path.cubicTo(provided + reach, consumed - reach, consumed);
    return path;
}

QPainterPath straight(const QPointF& provided, const QPointF& consumed)
{
    QPainterPath path(provided);
    path.lineTo(consumed);
    return path;
}

// Where a right-angled link turns: halfway between its interfaces when the consumer is far enough to the right of the
// provider; otherwise out of the provider, back across halfway between their heights, and into the consumer.
QList<QPointF> corners(const QPointF& provided, const QPointF& consumed)
{
    if(consumed.x() - provided.x() >= 2 * MinimumReach)
    {
        const qreal across = (provided.x() + consumed.x()) / 2;
        return {{across, provided.y()}, {across, consumed.y()}};
    }
    const qreal outOf = provided.x() + MinimumReach;
    const qreal into = consumed.x() - MinimumReach;
    const qreal between = (provided.y() + consumed.y()) / 2;
    return {{outOf, provided.y()}, {outOf, between}, {into, between}, {into, consumed.y()}};
}

QPainterPath rightAngled(const QPointF& provided, const QPointF& consumed)
{
    QPainterPath path(provided);
    for(const QPointF& corner : corners(provided, consumed))
    {
        path.lineTo(corner);
    }
    path.lineTo(consumed);
    return path;
}

// Whether a view of scene draws a rubber band; not when there is no scene.
bool drawsRubberBand(const QGraphicsScene* scene)
{
    if(scene == nullptr)
    {
        return false;
    }
    const QList<QGraphicsView*> views = scene->views();
    return std::ranges::any_of(views, [](const QGraphicsView* view) { return !view->rubberBandRect().isNull(); });
}

} // namespace

LinkItem::LinkItem(InterfaceItem& provided, InterfaceItem& consumed, LinkStyle style)
    : m_provided(provided), m_consumed(consumed), m_style(style)
{
    setZValue(-1); // Under the components: it only shows between them
    setFlag(ItemIsSelectable);
    setAcceptHoverEvents(true);
    m_provided.addLink(*this);
    m_consumed.addLink(*this);
    follow();
}

LinkItem::~LinkItem()
{
    m_provided.removeLink(*this);
    m_consumed.removeLink(*this);
}

QPainterPath LinkItem::route(LinkStyle style, const QPointF& provided, const QPointF& consumed)
{
    switch(style)
    {
    case LinkStyle::Straight:
        return straight(provided, consumed);
    case LinkStyle::RightAngled:
        return rightAngled(provided, consumed);
    case LinkStyle::Curved:
        break;
    }
    return curved(provided, consumed);
}

int LinkItem::type() const
{
    return Type;
}

InterfaceItem& LinkItem::provided() const
{
    return m_provided;
}

InterfaceItem& LinkItem::consumed() const
{
    return m_consumed;
}

LinkStyle LinkItem::style() const
{
    return m_style;
}

void LinkItem::setStyle(LinkStyle style)
{
    if(m_style != style)
    {
        m_style = style;
        follow();
    }
}

const QPainterPath& LinkItem::path() const
{
    return m_path;
}

void LinkItem::follow()
{
    prepareGeometryChange();
    m_path = route(m_style, m_provided.scenePos(), m_consumed.scenePos());
    QPainterPathStroker stroker;
    stroker.setWidth(PickWidth);
    m_shape = stroker.createStroke(m_path);
}

QRectF LinkItem::boundingRect() const
{
    return m_shape.boundingRect(); // Wider than what is drawn
}

QPainterPath LinkItem::shape() const
{
    return m_shape;
}

void LinkItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* widget)
{
    const QPalette palette = widget != nullptr ? widget->palette() : QApplication::palette();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(palette.color(isSelected() ? QPalette::Highlight : QPalette::ButtonText),
                         isSelected() || m_hovered ? ThickLineWidth : LineWidth));
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(m_path);
}

void LinkItem::hoverEnterEvent(QGraphicsSceneHoverEvent* /*event*/)
{
    // As they are named now: they can be renamed once it is drawn.
    const Nebula::Component& consumer = m_consumed.component();
    const Nebula::Component& provider = m_provided.component();
    setToolTip(tr("%1.%2 is bound to %3.%4\nClick it and press Delete, or right-click it, to remove it")
                   .arg(consumer.objectName(), m_consumed.name(), provider.objectName(), m_provided.name()));
    // The scene sends a move at once, see hoverMoveEvent().
}

void LinkItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    // The scene sends it the hover events under a component too, which does not take them: it is only hovered where it
    // is seen.
    setHovered(scene()->itemAt(event->scenePos(), QTransform()) == this);
}

void LinkItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* /*event*/)
{
    setHovered(false);
}

QVariant LinkItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    // A rubber band selects what is in it, but a link that only crosses it would be selected too, and removed with
    // what is.
    if(change == ItemSelectedChange && value.toBool() && drawsRubberBand(scene()))
    {
        return false;
    }
    return QGraphicsItem::itemChange(change, value);
}

void LinkItem::setHovered(bool hovered)
{
    if(m_hovered != hovered)
    {
        m_hovered = hovered;
        update();
    }
}
