// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QSplitter>
#include <QString>

#include "DragAside.h"
#include "Nebula/Component/Component.h"

class QWidget;

// Widgets side by side, or one above the other, which the user resizes by dragging between them: a QSplitter it
// creates and provides as "widget", showing the widgets it consumes as the list "widgets", each in the order of its
// binding and for as long as it has it. Its sizes are those it starts with, in pixels, and the widget it stretches,
// from 0, the one that takes the room it gains as it grows. Hidden while dragging, it is set aside while rows dragged
// from the lists of its widgets are out of it, until they are dropped, what it is beside taking its room: see
// DragAside. It needs a QApplication.
class SplitterComponent : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Widgets side by side, or one above the other, resized by dragging between them")
    Q_CLASSINFO("category", "Layout")
    Q_PROPERTY(bool vertical READ isVertical WRITE setVertical NOTIFY verticalChanged)
    Q_PROPERTY(QString sizes READ sizes WRITE setSizes NOTIFY sizesChanged)
    Q_PROPERTY(int stretched READ stretched WRITE setStretched NOTIFY stretchedChanged)
    Q_PROPERTY(bool hiddenWhileDragging READ isHiddenWhileDragging WRITE setHiddenWhileDragging NOTIFY
                   hiddenWhileDraggingChanged)

  public:
    SplitterComponent();

    // Whether its widgets are one above the other, rather than side by side.
    [[nodiscard]] bool isVertical() const;
    void setVertical(bool vertical);
    // The size of each widget, in their order, separated by commas: "320, 760, 360"; as the splitter divides them
    // when empty, or for those it does not tell.
    [[nodiscard]] QString sizes() const;
    void setSizes(const QString& sizes);
    // The widget that takes the room it gains, from 0; none, every widget taking its share, when it is not one of
    // them.
    [[nodiscard]] int stretched() const;
    void setStretched(int stretched);
    // Whether it is hidden while what is dragged from its widgets is out of it, to be dropped beside it.
    [[nodiscard]] bool isHiddenWhileDragging() const;
    void setHiddenWhileDragging(bool hidden);

  signals:
    void verticalChanged(bool vertical);
    void sizesChanged(const QString& sizes);
    void stretchedChanged(int stretched);
    void hiddenWhileDraggingChanged(bool hidden);

  private:
    // Shows widget in its place among those it has, once, when it has it twice.
    void showWidget(QWidget* widget);
    // Gives widget back as it was provided, without a parent, which would delete it, unless it has it twice.
    void hideWidget(QWidget* widget);
    // Gives its widgets their stretch and their sizes.
    void divide();

    QSplitter m_splitter;
    DragAside m_aside{m_splitter}; // Follows the drags of the lists of its widgets
    QList<QWidget*> m_widgets;     // Consumed
    QString m_sizes;
    int m_stretched = -1;
};
