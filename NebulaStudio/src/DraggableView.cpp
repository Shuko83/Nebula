// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "DraggableView.h"

#include <QCoreApplication>

DraggableView::DraggableView(QWidget* parent) : QTreeView(parent)
{
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
    // What it tells, the widgets it is in are told: those it is in at the time.
    connect(this, &DraggableView::dragStarted, this, [this] { tellAncestors(dragStartedEvent()); });
    connect(this, &DraggableView::dragFinished, this, [this] { tellAncestors(dragFinishedEvent()); });
}

QEvent::Type DraggableView::dragStartedEvent()
{
    static const auto type = static_cast<QEvent::Type>(QEvent::registerEventType());
    return type;
}

QEvent::Type DraggableView::dragFinishedEvent()
{
    static const auto type = static_cast<QEvent::Type>(QEvent::registerEventType());
    return type;
}

void DraggableView::startDrag(Qt::DropActions supportedActions)
{
    emit dragStarted();
    QTreeView::startDrag(supportedActions); // Until the drag ends, on every platform
    emit dragFinished();
}

void DraggableView::tellAncestors(QEvent::Type type) const
{
    for(QWidget* ancestor = parentWidget(); ancestor != nullptr; ancestor = ancestor->parentWidget())
    {
        QEvent event(type);
        QCoreApplication::sendEvent(ancestor, &event);
    }
}
