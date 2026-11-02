// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "LinkItem.h"

#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QList>
#include <QPainter>
#include <QPainterPathStroker>
#include <QPalette>
#include <QStringList>
#include <QWidget>
#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <tuple>
#include <utility>

#include "AnchorItem.h"
#include "InterfaceItem.h"
#include "Nebula/Component/Component.h"

namespace
{

// Under the components: it only shows between them. Over the other links once selected.
constexpr qreal ZValue = -1;
constexpr qreal SelectedZValue = -0.75;
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

// The stop of a line at anchor, between before and after, the points next to it there: gone through the way from the
// one before it to the one after it, rightwards when one is above the other. A junction is gone through the way from
// the one before it to it, whichever link goes through it: their line is one up to it.
LinkItem::Stop stopThrough(const QPointF& before, const AnchorItem& anchor, const QPointF& after)
{
    const QPointF towards = anchor.isJunction() ? anchor.pos() : after;
    return {.point = anchor.pos(), .leftwards = towards.x() < before.x()};
}

// The line of a link in style through stops, from each of them to the next one, as LinkItem::route() goes from a
// provided interface to a consumed one.
QList<QPainterPath> stretchesThrough(LinkStyle style, const QList<LinkItem::Stop>& stops)
{
    QList<QPainterPath> stretches;
    for(qsizetype index = 1; index < stops.size(); ++index)
    {
        stretches.append(LinkItem::route(style, stops.at(index - 1), stops.at(index)));
    }
    return stretches;
}

// stretch added to path, connected to its end when connected, as a new part of it otherwise.
void append(QPainterPath& path, const QPainterPath& stretch, bool connected)
{
    if(connected)
    {
        path.connectPath(stretch);
        return;
    }
    path.addPath(stretch);
}

// stretches one after the other, each connected to the end of the one before it.
QPainterPath connected(const QList<QPainterPath>& stretches)
{
    QPainterPath path(stretches.constFirst().elementAt(0));
    std::ranges::for_each(stretches, [&path](const QPainterPath& stretch) { path.connectPath(stretch); });
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

// The name of item as it is now, after the one of its component: Source1.object. They can be renamed once it is drawn.
QString qualifiedName(const InterfaceItem& item)
{
    return item.component().objectName() + u'.' + item.name();
}

// What count links that share a stretch of their line tell in their tooltip, %1 standing for the interfaces they are
// drawn to, %2 for the one they are drawn from.
QString aboutBindings(qsizetype count)
{
    const QString bound =
        count == 1 ? LinkItem::tr("%1 is bound to %2\nClick it and press Delete, or right-click it, to remove it\n"
                                  "Double-click it to anchor it there: it goes through that point, wherever its "
                                  "components are")
                   : LinkItem::tr("%1 are bound to %2 along this line\nClick it and press Delete, or right-click it, "
                                  "to remove them\nDouble-click it to anchor it there: they go through that point, "
                                  "wherever their components are");
    return bound + LinkItem::tr("\nShift+drag from it to bind another interface to %2 through that point");
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
    setZValue(ZValue);
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
    std::ranges::for_each(m_anchors, [this](AnchorItem* anchor) { leave(*anchor); });
    followSiblings(); // The line it shared is theirs alone now
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

void LinkItem::removeAnchor(AnchorItem& anchor)
{
    const QList<LinkItem*> through = anchor.links(); // A copy: it goes once the last one leaves it
    for(LinkItem* link : through)
    {
        QList<AnchorItem*> anchors = link->m_anchors;
        anchors.removeOne(&anchor);
        link->setAnchorItems(anchors);
    }
}

void LinkItem::removeJunction(AnchorItem& junction)
{
    const QList<LinkItem*> through = junction.links(); // A copy: it goes once the last one leaves it
    for(LinkItem* link : through)
    {
        link->setAnchorItems(link->m_anchors.sliced(link->m_anchors.indexOf(&junction) + 1));
    }
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
        points.append(anchor->pos()); // In the scene: it is not a child of the link
    }
    return points;
}

const QList<AnchorItem*>& LinkItem::anchorItems() const
{
    return m_anchors;
}

AnchorItem* LinkItem::after(const AnchorItem& anchor) const
{
    const qsizetype next = m_anchors.indexOf(&anchor) + 1;
    return next > 0 && next < m_anchors.size() ? m_anchors.at(next) : nullptr;
}

QList<AnchorItem*> LinkItem::anchorsUpTo(const AnchorItem& anchor) const
{
    return m_anchors.first(m_anchors.indexOf(&anchor) + 1);
}

void LinkItem::setAnchors(const QList<QPointF>& anchors)
{
    QList<AnchorItem*> items = sharedAnchors(anchors);
    for(qsizetype index = items.size(); index < anchors.size(); ++index)
    {
        items.append(new AnchorItem(anchors.at(index)));
    }
    setAnchorItems(items);
}

void LinkItem::setAnchorItems(const QList<AnchorItem*>& anchors)
{
    const QList<AnchorItem*> left = m_anchors;
    m_anchors = anchors;
    std::ranges::for_each(anchors, [this](AnchorItem* anchor) { join(*anchor); });
    std::ranges::for_each(left,
                          [this, &anchors](AnchorItem* anchor) { anchors.contains(anchor) ? void() : leave(*anchor); });
    anchorsChanged();
}

AnchorItem& LinkItem::addAnchor(const QPointF& position)
{
    const qsizetype stretch = nearestStretch(position);
    auto* anchor = new AnchorItem(position);
    const QList<LinkItem*> along = linksAlong(stretch); // Each goes through the same points before it
    for(LinkItem* link : along)
    {
        QList<AnchorItem*> anchors = link->m_anchors;
        anchors.insert(stretch, anchor);
        link->setAnchorItems(anchors);
    }
    return *anchor;
}

QList<LinkItem*> LinkItem::linksAlong(const QPointF& position)
{
    return linksAlong(nearestStretch(position));
}

LinkItem::Stop LinkItem::branchStop(const QPointF& position) const
{
    // The stop before the anchor at position, or before the stretch it is on.
    const auto anchor = std::ranges::find(m_anchors, position, &AnchorItem::pos);
    const qsizetype before = anchor != m_anchors.end() ? anchor - m_anchors.begin() : nearestStretch(position);
    return {.point = position, .leftwards = position.x() < stops().at(before).point.x()};
}

void LinkItem::anchorsChanged()
{
    followSiblings();
    if(m_anchored)
    {
        m_anchored(*this);
    }
}

void LinkItem::forgetAnchor(const AnchorItem& anchor)
{
    m_anchors.removeOne(&anchor);
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
    const QList<QPointF> points =
        QList<QPointF>{m_provided.scenePos()} + anchors() + QList<QPointF>{m_consumed.scenePos()};
    QList<Stop> stops{stopAt(m_provided)};
    for(qsizetype index = 0; index < m_anchors.size(); ++index)
    {
        stops.append(stopThrough(points.at(index), *m_anchors.at(index), points.at(index + 2)));
    }
    stops.append(stopAt(m_consumed));
    return stops;
}

qsizetype LinkItem::nearestStretch(const QPointF& position) const
{
    const QList<qreal> lengths = distances(m_style, stops(), position);
    return std::ranges::min_element(lengths) - lengths.begin();
}

QList<LinkItem*> LinkItem::linksAlong(qsizetype stretch)
{
    return stretch < m_anchors.size() ? m_anchors.at(stretch)->links() : QList<LinkItem*>{this};
}

QList<AnchorItem*> LinkItem::sharedAnchors(const QList<QPointF>& anchors) const
{
    QList<AnchorItem*> shared;
    AnchorItem* next = sharedAfter(shared, anchors);
    while(next != nullptr)
    {
        shared.append(next);
        next = sharedAfter(shared, anchors);
    }
    return shared;
}

AnchorItem* LinkItem::sharedAfter(const QList<AnchorItem*>& shared, const QList<QPointF>& anchors) const
{
    const qsizetype index = shared.size();
    const auto goesThere = [this, index, &shared, &anchors](const LinkItem* other)
    {
        const QList<AnchorItem*>& theirs = other->m_anchors;
        return other != this && theirs.size() > index && theirs.first(index) == shared &&
               theirs.at(index)->pos() == anchors.at(index);
    };
    const QList<LinkItem*>& others = m_provided.links();
    const auto found = index < anchors.size() ? std::ranges::find_if(others, goesThere) : others.end();
    return found != others.end() ? (*found)->m_anchors.at(index) : nullptr;
}

void LinkItem::join(AnchorItem& anchor)
{
    anchor.addLink(*this);
    anchor.setEditable(m_editable);
    if(scene() != nullptr && anchor.scene() != scene())
    {
        scene()->addItem(&anchor);
    }
}

void LinkItem::leave(AnchorItem& anchor)
{
    anchor.removeLink(*this);
    if(anchor.links().isEmpty())
    {
        delete &anchor;
    }
}

void LinkItem::followSiblings()
{
    for(LinkItem* link : m_provided.links())
    {
        link->follow();
    }
}

void LinkItem::follow()
{
    prepareGeometryChange();
    m_stretches = stretchesThrough(m_style, stops());
    m_path = connected(m_stretches);
    QPainterPathStroker stroker;
    stroker.setWidth(PickWidth);
    m_shape = stroker.createStroke(m_path);
    std::ranges::for_each(m_anchors, &AnchorItem::refresh); // Which junctions they are can change
}

QPainterPath LinkItem::ownPath() const
{
    QPainterPath path;
    for(qsizetype stretch = 0; stretch < m_stretches.size(); ++stretch)
    {
        if(drawsStretch(stretch))
        {
            append(path, m_stretches.at(stretch), stretch > 0 && drawsStretch(stretch - 1));
        }
    }
    return path;
}

bool LinkItem::drawsStretch(qsizetype stretch) const
{
    return stretch >= m_anchors.size() || m_anchors.at(stretch)->links().constFirst() == this;
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
    const bool thick = isSelected() || m_hovered;
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(palette.color(isSelected() ? QPalette::Highlight : QPalette::ButtonText),
                         thick ? ThickLineWidth : LineWidth));
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(thick ? m_path : ownPath()); // All of it, over the line it shares, which shows where it goes
}

void LinkItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    // The scene sends it the hover events under a component too, which does not take them: it is only hovered where it
    // is seen, and so are the links that share its line there.
    const bool seen = scene()->itemAt(event->scenePos(), QTransform()) == this;
    const QList<LinkItem*> along = linksAlong(event->scenePos());
    hoverAlong(seen ? along : QList<LinkItem*>());
    setToolTip(about(along));
}

void LinkItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* /*event*/)
{
    hoverAlong({});
}

void LinkItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mouseReleaseEvent(event); // Selects it alone, or toggles it with Control held
    // A click, as the base class tells it.
    if(event->button() == Qt::LeftButton && event->scenePos() == event->buttonDownScenePos(Qt::LeftButton))
    {
        selectAlong(event->scenePos());
    }
}

QVariant LinkItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    // A link that only crosses the rubber band would be selected too, and removed with what is.
    if(change == ItemSelectedChange && value.toBool() && drawsRubberBand(scene()))
    {
        return false;
    }
    followChange(change);
    return QGraphicsItem::itemChange(change, value);
}

void LinkItem::followChange(GraphicsItemChange change)
{
    if(change == ItemSelectedHasChanged)
    {
        setZValue(isSelected() ? SelectedZValue : ZValue);
        updateAnchors();
    }
    else if(change == ItemSceneHasChanged)
    {
        std::ranges::for_each(m_anchors, [this](AnchorItem* anchor) { join(*anchor); }); // Into its scene
    }
}

void LinkItem::setHovered(bool hovered)
{
    if(m_hovered != hovered)
    {
        m_hovered = hovered;
        update();
    }
}

void LinkItem::hoverAlong(const QList<LinkItem*>& hovered)
{
    for(LinkItem* link : m_provided.links())
    {
        link->setHovered(hovered.contains(link));
    }
}

QString LinkItem::about(const QList<LinkItem*>& links) const
{
    QStringList consumed;
    std::ranges::transform(links, std::back_inserter(consumed),
                           [](const LinkItem* link) { return qualifiedName(link->consumed()); });
    return aboutBindings(consumed.size()).arg(consumed.join(u", "), qualifiedName(m_provided));
}

void LinkItem::selectAlong(const QPointF& position)
{
    const QList<LinkItem*> along = linksAlong(position);
    for(LinkItem* link : along)
    {
        link->setSelected(isSelected());
    }
}

void LinkItem::updateAnchors()
{
    std::ranges::for_each(m_anchors, [](AnchorItem* anchor) { anchor->update(); });
}
