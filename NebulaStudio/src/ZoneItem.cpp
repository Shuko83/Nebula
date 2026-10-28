// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ZoneItem.h"

#include <QCursor>
#include <QFontMetricsF>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>
#include <utility>

#include "Colors.h"
#include "ComponentItem.h"
#include "ItemShapes.h"
#include "NameEditor.h"
#include "Settings/SettingsItem.h"
#include "Theme.h"

namespace
{

constexpr qreal TitleHeight = 28;
constexpr qreal Padding = 8;
// How far inside its bounds a zone is resized by an edge, and how far from a corner by both edges of it.
constexpr qreal EdgeWidth = 6;
constexpr qreal CornerSize = 16;
// Enough for a title, and for its corners not to meet.
constexpr QSizeF MinimumSize(120, 60);
// How much of its color a zone keeps under its title, where the components are.
constexpr float FillOpacity = 0.15F;

// The cursor of the edges a zone is resized by, at least one.
Qt::CursorShape resizeCursor(Qt::Edges edges)
{
    const bool horizontal = edges.testAnyFlags(Qt::LeftEdge | Qt::RightEdge);
    const bool vertical = edges.testAnyFlags(Qt::TopEdge | Qt::BottomEdge);
    if(horizontal && vertical)
    {
        // A corner: the diagonal it is on.
        return edges.testFlag(Qt::LeftEdge) == edges.testFlag(Qt::TopEdge) ? Qt::SizeFDiagCursor : Qt::SizeBDiagCursor;
    }
    return horizontal ? Qt::SizeHorCursor : Qt::SizeVerCursor;
}

// lowEdge when value is near low, highEdge when it is near high, as near as a corner; none between them.
Qt::Edges edgeNear(qreal value, qreal low, qreal high, Qt::Edge lowEdge, Qt::Edge highEdge)
{
    if(value < low + CornerSize)
    {
        return lowEdge;
    }
    return value > high - CornerSize ? Qt::Edges(highEdge) : Qt::Edges();
}

// Whether item is moved with a zone it is in by its center: a component, or a settings file.
bool isComponentLike(const QGraphicsItem& item)
{
    const QGraphicsObject* object = item.toGraphicsObject();
    return qobject_cast<const ComponentItem*>(object) != nullptr ||
           qobject_cast<const SettingsItem*>(object) != nullptr;
}

// What a zone of color is filled with under its title, where the components are.
QColor fillColor(QColor color)
{
    color.setAlphaF(FillOpacity);
    return color;
}

} // namespace

ZoneItem::ZoneItem(const QSizeF& size, QString title, const QColor& color)
    : m_size(size.expandedTo(MinimumSize)), m_title(std::move(title)), m_color(color)
{
    setAcceptHoverEvents(true);
    stack();
}

int ZoneItem::type() const
{
    return Type;
}

const QString& ZoneItem::title() const
{
    return m_title;
}

const QColor& ZoneItem::color() const
{
    return m_color;
}

void ZoneItem::setColor(const QColor& color)
{
    m_color = color;
    update();
}

void ZoneItem::editTitle()
{
    if(m_titleEditor == nullptr)
    {
        m_titleEditor = new NameEditor(*this, [this](const QString& title) { m_title = title; });
    }
    m_titleEditor->edit(m_title, textColorOver(m_color), titleRect(), Qt::AlignLeft);
}

void ZoneItem::setEditable(bool editable)
{
    setAcceptedMouseButtons(editable ? Qt::AllButtons : Qt::NoButton);
    setAcceptHoverEvents(editable);
    unsetCursor(); // The one of an edge it is over, which would no longer be updated
    if(!editable)
    {
        // Let go of, should it be dragged: the button still held would move it on.
        m_moving = false;
        m_followers.clear();
        m_resizedEdges = {};
    }
}

QRectF ZoneItem::boundingRect() const
{
    return {QPointF(0, 0), m_size};
}

QPainterPath ZoneItem::shape() const
{
    // The second one is a hole in the first: a path is filled where an odd number of its outlines are around.
    QPainterPath path;
    path.addRect(boundingRect());
    path.addRect(boundingRect().adjusted(EdgeWidth, TitleHeight, -EdgeWidth, -EdgeWidth));
    return path;
}

void ZoneItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/)
{
    painter->setRenderHint(QPainter::Antialiasing);
    paintBody(*painter);
    if(m_titleEditor == nullptr || !m_titleEditor->isVisible()) // Its title is the text being edited otherwise
    {
        paintTitle(*painter);
    }
}

void ZoneItem::paintBody(QPainter& painter) const
{
    const QPainterPath outline = roundedOutline(boundingRect());
    painter.setPen(QPen(m_color, 1));
    painter.setBrush(fillColor(m_color));
    painter.drawPath(outline);
    fillWithin(painter, outline, QRectF(0, 0, m_size.width(), TitleHeight), m_color);
}

void ZoneItem::paintTitle(QPainter& painter) const
{
    const QFont font = titleFont(painter.font());
    painter.setFont(font);
    painter.setPen(textColorOver(m_color));
    const QRectF title = titleRect();
    painter.drawText(title, Qt::AlignLeft | Qt::AlignVCenter,
                     QFontMetricsF(font).elidedText(m_title, Qt::ElideRight, title.width()));
}

void ZoneItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    const Qt::Edges edges = edgesAt(event->pos());
    if(!edges)
    {
        unsetCursor();
    }
    else
    {
        setCursor(resizeCursor(edges));
    }
}

void ZoneItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if(event->button() != Qt::LeftButton)
    {
        event->ignore();
        return;
    }
    m_resizedEdges = edgesAt(event->pos());
    m_moving = !m_resizedEdges;
    m_followers = m_moving ? content() : QList<QGraphicsItem*>(); // What is in it now, not what it is then moved over
    m_grabOffset = event->scenePos() - grabbedCorner();
}

void ZoneItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if(m_moving)
    {
        moveWithContent(event->scenePos() - event->lastScenePos());
    }
    else if(m_resizedEdges != Qt::Edges())
    {
        resize(event->scenePos());
    }
}

void ZoneItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if(event->button() == Qt::LeftButton)
    {
        m_moving = false;
        m_followers.clear();
        m_resizedEdges = {};
    }
}

void ZoneItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    if(event->button() == Qt::LeftButton && !edgesAt(event->pos()))
    {
        editTitle();
    }
    else
    {
        QGraphicsItem::mouseDoubleClickEvent(event); // A press
    }
}

QRectF ZoneItem::titleRect() const
{
    return {Padding, 0, m_size.width() - 2 * Padding, TitleHeight};
}

Qt::Edges ZoneItem::edgesAt(const QPointF& position) const
{
    const QRectF bounds = boundingRect();
    if(bounds.adjusted(EdgeWidth, EdgeWidth, -EdgeWidth, -EdgeWidth).contains(position))
    {
        return {};
    }
    return edgeNear(position.x(), bounds.left(), bounds.right(), Qt::LeftEdge, Qt::RightEdge) |
           edgeNear(position.y(), bounds.top(), bounds.bottom(), Qt::TopEdge, Qt::BottomEdge);
}

QPointF ZoneItem::grabbedCorner() const
{
    const QRectF bounds = sceneBoundingRect();
    return {m_resizedEdges.testFlag(Qt::LeftEdge) ? bounds.left() : bounds.right(),
            m_resizedEdges.testFlag(Qt::TopEdge) ? bounds.top() : bounds.bottom()};
}

void ZoneItem::moveWithContent(const QPointF& offset)
{
    moveBy(offset.x(), offset.y());
    for(QGraphicsItem* follower : std::as_const(m_followers))
    {
        follower->moveBy(offset.x(), offset.y());
    }
}

bool ZoneItem::holds(const QRectF& bounds, const QGraphicsItem& item) const
{
    const QRectF itemBounds = item.sceneBoundingRect();
    const bool zone = &item != this && item.type() == Type;
    return (isComponentLike(item) && bounds.contains(itemBounds.center())) || (zone && bounds.contains(itemBounds));
}

QList<QGraphicsItem*> ZoneItem::content() const
{
    const QRectF bounds = sceneBoundingRect();
    QList<QGraphicsItem*> content = scene()->items(bounds, Qt::IntersectsItemBoundingRect);
    content.removeIf([this, &bounds](const QGraphicsItem* item) { return !holds(bounds, *item); });
    return content;
}

void ZoneItem::resize(const QPointF& scenePosition)
{
    const QRectF bounds = resizedBounds(scenePosition - m_grabOffset);
    prepareGeometryChange();
    setPos(bounds.topLeft()); // What is in it stays where it is: only dragging its title moves it
    m_size = bounds.size();
    stack();
}

void ZoneItem::stack()
{
    setZValue(-m_size.width() * m_size.height());
}

QRectF ZoneItem::resizedBounds(const QPointF& edges) const
{
    QRectF bounds = sceneBoundingRect();
    resizeHorizontally(bounds, edges.x());
    resizeVertically(bounds, edges.y());
    return bounds;
}

void ZoneItem::resizeHorizontally(QRectF& bounds, qreal x) const
{
    if(m_resizedEdges.testFlag(Qt::LeftEdge))
    {
        bounds.setLeft(std::min(x, bounds.right() - MinimumSize.width()));
    }
    else if(m_resizedEdges.testFlag(Qt::RightEdge))
    {
        bounds.setRight(std::max(x, bounds.left() + MinimumSize.width()));
    }
}

void ZoneItem::resizeVertically(QRectF& bounds, qreal y) const
{
    if(m_resizedEdges.testFlag(Qt::TopEdge))
    {
        bounds.setTop(std::min(y, bounds.bottom() - MinimumSize.height()));
    }
    else if(m_resizedEdges.testFlag(Qt::BottomEdge))
    {
        bounds.setBottom(std::max(y, bounds.top() + MinimumSize.height()));
    }
}
