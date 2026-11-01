// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QEvent>
#include <QTreeView>

// A list whose rows are dragged from it, not dropped on it, which tells when a drag starts and when it ends: what
// shows the list makes room meanwhile, see DragAside. The widgets it is in are told as well, by an event of the type
// of dragStartedEvent(), then of dragFinishedEvent(), wherever they are above it.
class DraggableView : public QTreeView
{
    Q_OBJECT

  public:
    explicit DraggableView(QWidget* parent = nullptr);

    // The types of the events the widgets it is in get as a drag starts, and as it ends.
    [[nodiscard]] static QEvent::Type dragStartedEvent();
    [[nodiscard]] static QEvent::Type dragFinishedEvent();

  signals:
    // Rows are dragged from the list, until dragFinished(), once they are dropped or the drag is given up.
    void dragStarted();
    void dragFinished();

  protected:
    void startDrag(Qt::DropActions supportedActions) override;

  private:
    // Sends each widget it is in an event of type.
    void tellAncestors(QEvent::Type type) const;
};
