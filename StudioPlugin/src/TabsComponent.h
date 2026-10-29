// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QTabWidget>

#include "Nebula/Component/Component.h"

class QWidget;

// Pages in tabs, one shown at a time: a QTabWidget it creates and provides as "widget", showing the widgets it
// consumes as the list "pages", each in the order of its binding and for as long as it has it, under its window
// title, beside its window icon when it has one of its own; the panels of NebulaStudio name themselves. In document
// mode, the tabs have no frame, on the background of the window. It needs a QApplication.
class TabsComponent : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Pages in tabs, one shown at a time, each under its title")
    Q_CLASSINFO("category", "Layout")
    Q_PROPERTY(bool documentMode READ isDocumentMode WRITE setDocumentMode NOTIFY documentModeChanged)

  public:
    TabsComponent();

    [[nodiscard]] bool isDocumentMode() const;
    void setDocumentMode(bool documentMode);

  signals:
    void documentModeChanged(bool documentMode);

  private:
    // Shows page in its place among those it has, once, when it has it twice.
    void showPage(QWidget* page);
    // Gives page back as it was provided, without a parent, which would delete it, unless it has it twice.
    void hidePage(QWidget* page);

    QTabWidget m_tabs;
    QList<QWidget*> m_pages; // Consumed
};
