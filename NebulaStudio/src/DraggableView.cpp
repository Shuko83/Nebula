// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "DraggableView.h"

DraggableView::DraggableView(QWidget* parent) : QTreeView(parent)
{
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
}

void DraggableView::startDrag(Qt::DropActions supportedActions)
{
    emit dragStarted();
    QTreeView::startDrag(supportedActions); // Until the drag ends, on every platform
    emit dragFinished();
}
