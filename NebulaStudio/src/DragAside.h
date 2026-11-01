// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QObject>
#include <QPointF>
#include <QPointer>

class QWidget;
class QWindow;

// Sets a widget aside while rows dragged from one of the lists in it, each a DraggableView, are out of it: it is hidden
// as they leave it, what it is beside taking its room, until they are dropped or the drag is given up, when it is shown
// again. The lists it has, as they come and go, tell it when they are dragged from. The side of NebulaStudio where its
// lists are leaves its room to the edition zone, where what they hold is dropped. Nothing is set aside while it is not
// active, as at first.
class DragAside : public QObject
{
    Q_OBJECT

  public:
    explicit DragAside(QWidget& widget);

    [[nodiscard]] bool isActive() const;
    void setActive(bool active);

  protected:
    // Follows the drags of the lists in the widget, and sets it aside once what is dragged is out of it, as its window
    // tells.
    bool eventFilter(QObject* watched, QEvent* event) override;

  private:
    // Follows what the window of the widget tells of a drag, as rows are dragged from one of its lists.
    void watchDrag();
    // No longer watches the window, and shows the widget again when it set it aside.
    void endDrag();
    // Sets the widget aside once what is dragged is out of it, as the window, watched, tells with event.
    void followDrag(const QEvent& event);
    // Hides the widget when at, in its window, is out of it.
    void setAsideIfOut(QPointF at);

    QWidget& m_widget;
    bool m_active = false;
    QPointer<QWindow> m_window; // Of m_widget, watched while rows are dragged from it
    bool m_aside = false;       // m_widget, hidden while they are out of it
};
