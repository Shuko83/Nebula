// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "EditionView.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

namespace
{

// The whole scene fits in a window at the smallest zoom level; the largest one is beyond what is read comfortably.
constexpr qreal MinimumZoom = 0.25;
constexpr qreal MaximumZoom = 4;
// What a key, or a notch of the wheel, multiplies or divides the zoom by.
constexpr qreal ZoomStep = 1.25;
// The angle a notch of the wheel turns, in eighths of a degree.
constexpr qreal WheelNotch = 120;

// Whether event comes with a modifier that makes a command of its key.
bool hasCommandModifier(const QKeyEvent& event)
{
    return (event.modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) != Qt::NoModifier;
}

} // namespace

EditionView::EditionView(QGraphicsScene* scene, QWidget* parent) : QGraphicsView(scene, parent) {}

qreal EditionView::zoom() const
{
    return transform().m11();
}

void EditionView::setZoom(qreal zoom)
{
    zoom = std::clamp(zoom, MinimumZoom, MaximumZoom);
    if(qFuzzyCompare(zoom, this->zoom()))
    {
        return;
    }
    setTransform(QTransform::fromScale(zoom, zoom));
    emit zoomChanged(zoom);
}

void EditionView::zoomIn()
{
    setZoom(zoom() * ZoomStep);
}

void EditionView::zoomOut()
{
    setZoom(zoom() / ZoomStep);
}

void EditionView::resetZoom()
{
    setZoom(1);
}

void EditionView::wheelEvent(QWheelEvent* event)
{
    if(!event->modifiers().testFlag(Qt::ControlModifier))
    {
        QGraphicsView::wheelEvent(event);
        return;
    }
    zoomAround(event->position().toPoint(), zoom() * std::pow(ZoomStep, event->angleDelta().y() / WheelNotch));
    event->accept();
}

void EditionView::keyPressEvent(QKeyEvent* event)
{
    // The scene first: a name edited there takes + and - as any other character.
    QGraphicsView::keyPressEvent(event);
    if(!event->isAccepted() && !hasCommandModifier(*event))
    {
        zoomByKey(*event);
    }
}

void EditionView::mousePressEvent(QMouseEvent* event)
{
    // The middle button alone pans the view, unknown to the scene: the selection, and a name being edited, stay.
    if(event->button() == Qt::MiddleButton && event->buttons() == Qt::MiddleButton)
    {
        startPan(event->position().toPoint());
    }
    if(!isPanning(*event)) // The other buttons are for the scene once the pan is over
    {
        QGraphicsView::mousePressEvent(event);
    }
}

void EditionView::mouseMoveEvent(QMouseEvent* event)
{
    if(!isPanning(*event))
    {
        QGraphicsView::mouseMoveEvent(event);
        return;
    }
    pan(event->position().toPoint());
}

void EditionView::mouseReleaseEvent(QMouseEvent* event)
{
    if(!m_panFrom)
    {
        QGraphicsView::mouseReleaseEvent(event);
        return;
    }
    // Its press was for the pan, or was not given to the scene.
    endPanIfReleased(*event);
    event->accept();
}

void EditionView::scrollBy(QPoint delta)
{
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() + delta.x());
    verticalScrollBar()->setValue(verticalScrollBar()->value() + delta.y());
}

void EditionView::zoomAround(QPoint cursor, qreal zoom)
{
    const QPointF under = mapToScene(cursor);
    setZoom(zoom);
    scrollBy(mapFromScene(under) - cursor); // By how far it went
}

void EditionView::zoomByKey(QKeyEvent& event)
{
    if(event.key() == Qt::Key_Plus)
    {
        zoomIn();
    }
    else if(event.key() == Qt::Key_Minus)
    {
        zoomOut();
    }
    event.setAccepted(event.key() == Qt::Key_Plus || event.key() == Qt::Key_Minus);
}

bool EditionView::isPanning(QMouseEvent& event)
{
    endPanIfReleased(event);
    if(m_panFrom)
    {
        event.accept();
    }
    return m_panFrom.has_value();
}

void EditionView::pan(QPoint at)
{
    scrollBy(*m_panFrom - at);
    m_panFrom = at;
}

void EditionView::startPan(QPoint at)
{
    // Started again when the release of the middle button was lost: the cursor from before is kept.
    if(!m_panFrom)
    {
        m_cursor = viewport()->cursor();
        viewport()->setCursor(Qt::ClosedHandCursor);
        // Each scroll replays the last mouse event to the scene, where the cursor was before the pan: what passes
        // there would be hovered, and the cursor would take its shape. Not while the view is not interactive.
        setInteractive(false);
    }
    m_panFrom = at;
}

void EditionView::endPanIfReleased(const QMouseEvent& event)
{
    if(m_panFrom && !event.buttons().testFlag(Qt::MiddleButton))
    {
        m_panFrom.reset();
        setInteractive(true);
        viewport()->setCursor(m_cursor);
    }
}
