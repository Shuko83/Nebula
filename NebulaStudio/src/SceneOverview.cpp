// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "SceneOverview.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPen>

#include "EditionView.h"

namespace
{

// The size it asks for: the proportions of the scene of NebulaStudio, 4000 by 3000.
constexpr QSize Size(240, 180);
// How much of the color of the selection fills what the view shows, over the scene.
constexpr float ShownOpacity = 0.15F;

// Has overview only show its scene, whole: nothing is selected, dragged or typed there, and it has no scroll bars.
void showOnly(QGraphicsView& overview)
{
    overview.setInteractive(false);
    overview.setFocusPolicy(Qt::NoFocus);
    overview.setAcceptDrops(false);
    overview.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    overview.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    overview.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
}

// A pen of color width pixels wide, whatever the zoom.
QPen edgePen(const QColor& color, qreal width)
{
    QPen pen(color, width);
    pen.setCosmetic(true);
    return pen;
}

// Frames area with painter in color, filled with it, translucent.
void frame(QPainter& painter, const QRectF& area, QColor color)
{
    painter.setPen(edgePen(color, 2));
    color.setAlphaF(ShownOpacity);
    painter.setBrush(color);
    painter.drawRect(area);
}

} // namespace

SceneOverview::SceneOverview(EditionView& view, QWidget* parent) : QGraphicsView(parent), m_view(view)
{
    showOnly(*this);
    setToolTip(tr("The whole edition zone, what it shows framed: click or drag here to show another part of it"));
    followView();
}

QSize SceneOverview::sizeHint() const
{
    return Size;
}

void SceneOverview::followView()
{
    connect(&m_view, &EditionView::sceneChanged, this, &SceneOverview::showScene);
    connect(&m_view, &EditionView::shownAreaChanged, viewport(), qOverload<>(&QWidget::update));
    showScene(m_view.scene());
}

void SceneOverview::drawBackground(QPainter* painter, const QRectF& rect)
{
    // Not the grid of the scene, nor what it tells while it is empty: its area alone, within its edge.
    painter->fillRect(rect, palette().color(QPalette::Window));
    painter->setPen(edgePen(palette().color(QPalette::Mid), 1));
    painter->setBrush(palette().color(QPalette::Base));
    painter->drawRect(sceneRect());
}

void SceneOverview::drawForeground(QPainter* painter, const QRectF& /*rect*/)
{
    const QRectF shown = m_view.shownArea() & sceneRect();
    if(!shown.isEmpty())
    {
        frame(*painter, shown, palette().color(QPalette::Highlight));
    }
}

void SceneOverview::mousePressEvent(QMouseEvent* event)
{
    if(event->button() != Qt::LeftButton)
    {
        QGraphicsView::mousePressEvent(event);
        return;
    }
    showInView(event->position().toPoint());
    event->accept();
}

void SceneOverview::mouseMoveEvent(QMouseEvent* event)
{
    if(!event->buttons().testFlag(Qt::LeftButton))
    {
        QGraphicsView::mouseMoveEvent(event);
        return;
    }
    showInView(event->position().toPoint());
    event->accept();
}

void SceneOverview::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    fitScene();
}

void SceneOverview::showScene(QGraphicsScene* scene)
{
    if(this->scene() != nullptr)
    {
        this->scene()->disconnect(this);
    }
    setScene(scene);
    if(scene != nullptr)
    {
        connect(scene, &QGraphicsScene::sceneRectChanged, this, &SceneOverview::fitScene);
    }
    fitScene();
}

void SceneOverview::fitScene()
{
    if(scene() != nullptr)
    {
        fitInView(sceneRect(), Qt::KeepAspectRatio);
    }
}

void SceneOverview::showInView(QPoint at)
{
    m_view.centerOn(mapToScene(at));
}
