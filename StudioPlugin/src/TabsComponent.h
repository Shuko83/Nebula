// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QTabWidget>

#include "DragAside.h"
#include "Nebula/Component/Component.h"

class QWidget;

// Pages in tabs, one shown at a time: a QTabWidget it creates and provides as "widget", showing the widgets it
// consumes as the list "pages", each in the order of its binding and for as long as it has it, under its window
// title, beside its window icon when it has one of its own; the panels of NebulaStudio name themselves. In document
// mode, the tabs have no frame, on the background of the window. Hidden while dragging, they are set aside while rows
// dragged from the lists of their pages are out of them, until they are dropped, what they are beside taking their
// room: see DragAside. It needs a QApplication.
class TabsComponent : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Pages in tabs, one shown at a time, each under its title")
    Q_CLASSINFO("category", "Layout")
    Q_PROPERTY(bool documentMode READ isDocumentMode WRITE setDocumentMode NOTIFY documentModeChanged)
    Q_PROPERTY(bool hiddenWhileDragging READ isHiddenWhileDragging WRITE setHiddenWhileDragging NOTIFY
                   hiddenWhileDraggingChanged)

  public:
    TabsComponent();

    [[nodiscard]] bool isDocumentMode() const;
    void setDocumentMode(bool documentMode);
    // Whether the tabs are hidden while what is dragged from their pages is out of them, to be dropped beside them.
    [[nodiscard]] bool isHiddenWhileDragging() const;
    void setHiddenWhileDragging(bool hidden);

  signals:
    void documentModeChanged(bool documentMode);
    void hiddenWhileDraggingChanged(bool hidden);

  private:
    // Shows page in its place among those it has, once, when it has it twice.
    void showPage(QWidget* page);
    // Gives page back as it was provided, without a parent, which would delete it, unless it has it twice.
    void hidePage(QWidget* page);

    QTabWidget m_tabs;
    DragAside m_aside{m_tabs}; // Follows the drags of the lists of its pages
    QList<QWidget*> m_pages;   // Consumed
};
