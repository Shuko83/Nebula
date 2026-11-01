// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "DragAside.h"

#include <QCursor>
#include <QDragMoveEvent>
#include <QWidget>
#include <QWindow>
#include <utility>

#include "DraggableView.h"

DragAside::DragAside(QWidget& widget) : m_widget(widget) {}

bool DragAside::isActive() const
{
    return m_active;
}

void DragAside::setActive(bool active)
{
    m_active = active;
}

void DragAside::follow(const QWidget& page)
{
    for(const DraggableView* list : page.findChildren<DraggableView*>())
    {
        connect(list, &DraggableView::dragStarted, this, &DragAside::watchDrag, Qt::UniqueConnection);
        connect(list, &DraggableView::dragFinished, this, &DragAside::endDrag, Qt::UniqueConnection);
    }
}

void DragAside::forget(const QWidget& page)
{
    for(const DraggableView* list : page.findChildren<DraggableView*>())
    {
        disconnect(list, nullptr, this, nullptr);
    }
}

bool DragAside::eventFilter(QObject* watched, QEvent* event)
{
    if(watched == m_window.data() && event->type() == QEvent::DragMove)
    {
        setAsideIfOut(static_cast<const QDragMoveEvent*>(event)->position());
    }
    else if(watched == m_window.data() && event->type() == QEvent::DragLeave)
    {
        // The window is left, or the drag given up over it: where the cursor is tells which.
        setAsideIfOut(m_window->mapFromGlobal(QCursor::pos()));
    }
    return QObject::eventFilter(watched, event);
}

void DragAside::watchDrag()
{
    m_window = m_active ? m_widget.window()->windowHandle() : nullptr;
    if(m_window != nullptr)
    {
        m_window->installEventFilter(this);
    }
}

void DragAside::endDrag()
{
    if(m_window != nullptr)
    {
        m_window->removeEventFilter(this);
    }
    m_window = nullptr;
    // In its place, unless it was taken from there meanwhile: it would be shown as a window of its own.
    if(std::exchange(m_aside, false) && !m_widget.isWindow())
    {
        m_widget.show();
    }
}

void DragAside::setAsideIfOut(QPointF at)
{
    if(!m_aside && m_widget.isVisible() && !m_widget.rect().contains(m_widget.mapFrom(m_widget.window(), at.toPoint())))
    {
        m_aside = true;
        m_widget.hide();
    }
}
