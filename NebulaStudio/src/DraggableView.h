// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QTreeView>

// A list whose rows are dragged from it, not dropped on it, which tells when a drag starts and when it ends: what
// shows the list makes room meanwhile, see DragAside.
class DraggableView : public QTreeView
{
    Q_OBJECT

  public:
    explicit DraggableView(QWidget* parent = nullptr);

  signals:
    // Rows are dragged from the list, until dragFinished(), once they are dropped or the drag is given up.
    void dragStarted();
    void dragFinished();

  protected:
    void startDrag(Qt::DropActions supportedActions) override;
};
