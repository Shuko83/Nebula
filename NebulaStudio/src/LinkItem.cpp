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
#include <limits>
#include <tuple>
#include <utility>

#include "AnchorItem.h"
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
// How many points along a stretch of a link tell how close a position is to it.
constexpr int Samples = 32;

// The way a line goes through stop, along the x axis: -1 leftwards, 1 rightwards.
qreal heading(const LinkItem::Stop& stop)
{
    return stop.leftwards ? -1 : 1;
}

// Out of from and into to the way the line goes through each, wherever they are: out of the right edge of the provider
// and into the left edge of the consumer, say.
QPainterPath curved(const LinkItem::Stop& from, const LinkItem::Stop& to)
{
    const qreal reach = std::max(MinimumReach, std::abs(to.point.x() - from.point.x()) / 2);
    QPainterPath path(from.point);
    path.cubicTo(from.point + QPointF(heading(from) * reach, 0), to.point - QPointF(heading(to) * reach, 0), to.point);
    return path;
}

QPainterPath straight(const QPointF& from, const QPointF& to)
{
    QPainterPath path(from);
    path.lineTo(to);
    return path;
}

// Across halfway between from and to.
QList<QPointF> halfway(const QPointF& from, const QPointF& to)
{
    const qreal across = (from.x() + to.x()) / 2;
    return {{across, from.y()}, {across, to.y()}};
}

// Out of from, back across halfway between their heights, and into to, each the way the line goes through it.
QList<QPointF> backAcross(const LinkItem::Stop& from, const LinkItem::Stop& to)
{
    const qreal outOf = from.point.x() + heading(from) * MinimumReach;
    const qreal into = to.point.x() - heading(to) * MinimumReach;
    const qreal between = (from.point.y() + to.point.y()) / 2;
    return {{outOf, from.point.y()}, {outOf, between}, {into, between}, {into, to.point.y()}};
}

// Out of from, past to, and back into it: the line goes through them in opposite ways, out of the right edges of two
// components, say.
QList<QPointF> around(const LinkItem::Stop& from, const LinkItem::Stop& to)
{
    const qreal way = heading(from);
    const qreal beyond = way * (std::max(way * from.point.x(), way * to.point.x()) + MinimumReach);
    return {{beyond, from.point.y()}, {beyond, to.point.y()}};
}

// Where a right-angled link turns: around them when it goes through from and to in opposite ways; otherwise halfway
// between them when to is far enough the way it goes, or out of from, back across, and into to.
QList<QPointF> corners(const LinkItem::Stop& from, const LinkItem::Stop& to)
{
    if(from.leftwards != to.leftwards)
    {
        return around(from, to);
    }
    if(heading(from) * (to.point.x() - from.point.x()) >= 2 * MinimumReach)
    {
        return halfway(from.point, to.point);
    }
    return backAcross(from, to);
}

QPainterPath rightAngled(const LinkItem::Stop& from, const LinkItem::Stop& to)
{
    QPainterPath path(from.point);
    for(const QPointF& corner : corners(from, to))
    {
        path.lineTo(corner);
    }
    path.lineTo(to.point);
    return path;
}

// The stops of a line from first to last through anchors, in their order: each anchor is gone through the way from the
// point before it to the one after it, rightwards when one is above the other.
QList<LinkItem::Stop> stopsThrough(const LinkItem::Stop& first, const QList<QPointF>& anchors,
                                   const LinkItem::Stop& last)
{
    const QList<QPointF> points = QList<QPointF>{first.point} + anchors + QList<QPointF>{last.point};
    QList<LinkItem::Stop> stops{first};
    for(qsizetype index = 1; index + 1 < points.size(); ++index)
    {
        stops.append({.point = points.at(index), .leftwards = points.at(index + 1).x() < points.at(index - 1).x()});
    }
    stops.append(last);
    return stops;
}

// The line of a link in style through stops, from the first one to the last one: from each of them to the next one,
// as LinkItem::route() goes from a provided interface to a consumed one.
QPainterPath through(LinkStyle style, const QList<LinkItem::Stop>& stops)
{
    QPainterPath path(stops.constFirst().point);
    for(qsizetype index = 1; index < stops.size(); ++index)
    {
        path.connectPath(LinkItem::route(style, stops.at(index - 1), stops.at(index)));
    }
    return path;
}

// The distance from position to path, as close as Samples points along it tell.
qreal distance(const QPainterPath& path, const QPointF& position)
{
    qreal nearest = std::numeric_limits<qreal>::max();
    for(int sample = 0; sample <= Samples; ++sample)
    {
        nearest =
            std::min(nearest, QLineF(position, path.pointAtPercent(static_cast<qreal>(sample) / Samples)).length());
    }
    return nearest;
}

// The distance from position to each stretch of the line of a link in style through stops, between each two of them
// next to each other, in their order.
QList<qreal> distances(LinkStyle style, const QList<LinkItem::Stop>& stops, const QPointF& position)
{
    QList<qreal> found;
    for(qsizetype index = 1; index < stops.size(); ++index)
    {
        found.append(distance(LinkItem::route(style, stops.at(index - 1), stops.at(index)), position));
    }
    return found;
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

bool LinkItem::Ends::operator<(const Ends& other) const
{
    return std::tie(consumer, consumed, provider, provided) <
           std::tie(other.consumer, other.consumed, other.provider, other.provided);
}

LinkItem::LinkItem(InterfaceItem& provided, InterfaceItem& consumed, LinkStyle style,
                   std::function<void(const LinkItem&)> anchored)
    : m_provided(provided), m_consumed(consumed), m_style(style), m_anchored(std::move(anchored))
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

QPainterPath LinkItem::route(LinkStyle style, const Stop& from, const Stop& to)
{
    switch(style)
    {
    case LinkStyle::Straight:
        return straight(from.point, to.point);
    case LinkStyle::RightAngled:
        return rightAngled(from, to);
    case LinkStyle::Curved:
        break;
    }
    return curved(from, to);
}

LinkItem::Stop LinkItem::stopAt(const InterfaceItem& dot)
{
    // Out of the left edge of a provider, or into the right edge of a consumer, the line goes leftwards.
    const bool onRight = dot.side() == InterfaceItem::Side::Right;
    return {.point = dot.scenePos(), .leftwards = dot.kind() == InterfaceItem::Kind::Provided ? !onRight : onRight};
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

LinkItem::Ends LinkItem::ends() const
{
    return {.consumer = &m_consumed.component(),
            .consumed = m_consumed.name(),
            .provider = &m_provided.component(),
            .provided = m_provided.name()};
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

QList<QPointF> LinkItem::anchors() const
{
    QList<QPointF> points;
    for(const AnchorItem* anchor : m_anchors)
    {
        points.append(anchor->pos()); // In the scene: it stays at its origin
    }
    return points;
}

void LinkItem::setAnchors(const QList<QPointF>& anchors)
{
    qDeleteAll(m_anchors);
    m_anchors.clear();
    for(const QPointF& position : anchors)
    {
        m_anchors.append(new AnchorItem(position, *this));
        m_anchors.constLast()->setEditable(m_editable);
    }
    anchorsChanged();
}

void LinkItem::addAnchor(const QPointF& position)
{
    QList<QPointF> points = anchors();
    points.insert(nearestStretch(position), position);
    setAnchors(points);
}

void LinkItem::removeAnchor(const AnchorItem& anchor)
{
    QList<QPointF> points = anchors();
    points.removeAt(m_anchors.indexOf(&anchor));
    setAnchors(points);
}

void LinkItem::anchorsChanged()
{
    follow();
    if(m_anchored)
    {
        m_anchored(*this);
    }
}

bool LinkItem::isEditable() const
{
    return m_editable;
}

void LinkItem::setEditable(bool editable)
{
    m_editable = editable;
    for(AnchorItem* anchor : std::as_const(m_anchors))
    {
        anchor->setEditable(editable);
    }
}

QList<LinkItem::Stop> LinkItem::stops() const
{
    return stopsThrough(stopAt(m_provided), anchors(), stopAt(m_consumed));
}

qsizetype LinkItem::nearestStretch(const QPointF& position) const
{
    const QList<qreal> lengths = distances(m_style, stops(), position);
    return std::ranges::min_element(lengths) - lengths.begin();
}

void LinkItem::follow()
{
    prepareGeometryChange();
    m_path = through(m_style, stops());
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
    setToolTip(tr("%1.%2 is bound to %3.%4\nClick it and press Delete, or right-click it, to remove it\n"
                  "Double-click it to anchor it there: it goes through that point, wherever its components are")
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
    // A link that only crosses the rubber band would be selected too, and removed with what is.
    if(change == ItemSelectedChange && value.toBool() && drawsRubberBand(scene()))
    {
        return false;
    }
    if(change == ItemSelectedHasChanged)
    {
        updateAnchors();
    }
    return QGraphicsItem::itemChange(change, value);
}

void LinkItem::updateAnchors()
{
    std::ranges::for_each(m_anchors, [](AnchorItem* anchor) { anchor->update(); });
}

void LinkItem::setHovered(bool hovered)
{
    if(m_hovered != hovered)
    {
        m_hovered = hovered;
        update();
    }
}
