// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QMainWindow>

#include "Nebula/Component/Component.h"

class Project;
class QMenu;
class QWidget;
class Studio;

// The main window of NebulaStudio, which the other parts fill: a QMainWindow it creates, shown while it runs, at its
// width and its height. The widget it consumes as "centralWidget", which it cannot do without, fills its middle; the
// menus it consumes as the list "menus" are in its menu bar, and the widgets of the list "statusWidgets" on the right
// of its status bar, each in the order of its binding, for as long as it has them. Its status bar tells where the
// plugins are, and what the Studio it consumes as "studio" tells, when it has one. Its title is the one of the project
// it consumes as "project", which tells its file, and it is closed once the project is saved, or its changes given up:
// it is not closed otherwise. It needs a QApplication.
class StudioWindowComponent : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "The main window of NebulaStudio, which the other parts fill")
    Q_CLASSINFO("category", "NebulaStudio")
    Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightChanged)

  public:
    StudioWindowComponent();

    // Its size each time it starts, in pixels.
    [[nodiscard]] int width() const;
    void setWidth(int width);
    [[nodiscard]] int height() const;
    void setHeight(int height);

  signals:
    void widthChanged(int width);
    void heightChanged(int height);

  protected:
    bool onStart() override;
    void onStop() override;
    // Keeps the window from closing while the project is neither saved nor given up.
    bool eventFilter(QObject* watched, QEvent* event) override;

  private:
    // Declares what it consumes: the widget in its middle, its menus and the widgets of its status bar.
    void consumeWidgets();
    // Declares what it consumes of the parts of NebulaStudio: the studio and the project.
    void consumeParts();
    // Shows the widget it consumes in the middle of the window.
    void showCentralWidget();
    // Takes the widget it consumes out of the window, without a parent, as it was provided.
    void giveBackCentralWidget();
    // Shows the menus of shown in the menu bar, in their order: those it has, or all of them but one about to go.
    void showMenus(const QList<QMenu*>& shown);
    // Shows the menus it has but menu, about to go.
    void showMenusBut(QMenu* menu);
    // Shows the widgets it has on the right of the status bar, in their order.
    void showStatusWidgets();
    // Gives widget back as it was provided, without a parent, which would delete it, unless it has it twice.
    void hideStatusWidget(QWidget* widget);
    // Tells in the status bar what the studio tells, from now on.
    void followStudio();
    // Titles the window after the project, from now on.
    void followProject();
    // No longer titles the window after the project, about to go.
    void forgetProject();
    // Whether the window can close: its project, when it has one, is saved or its changes given up, once asked.
    [[nodiscard]] bool canClose();

    QMainWindow m_window;
    QWidget* m_centralWidget = nullptr; // Consumed
    QList<QMenu*> m_menus;              // Consumed
    QList<QWidget*> m_statusWidgets;    // Consumed
    Studio* m_studio = nullptr;         // Consumed
    Project* m_project = nullptr;       // Consumed
    int m_width = 1440;
    int m_height = 800;
};
