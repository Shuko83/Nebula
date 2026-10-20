// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QString>
#include <QToolBar>
#include <array>
#include <cstddef>
#include <optional>

#include "Nebula/Component/Component.h"

class QAction;
class QMainWindow;
class QWidget;

// A bar at the top of the main window it consumes as "mainWindow", which it cannot do without: a QToolBar it creates,
// in that window for as long as it has it. The bar shows the actions it consumes as "action1" to "action4", then the
// widgets it consumes as "widget1" and "widget2", each in the place of its interface and for as long as it has it, the
// other places closing up. It needs a QApplication.
class TopBarComponent : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "A bar of actions and widgets at the top of the main window")
    Q_CLASSINFO("category", "Widgets")
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(bool movable READ isMovable WRITE setMovable NOTIFY movableChanged)

  public:
    // How many actions, and how many widgets, it consumes.
    static constexpr std::size_t ActionCount = 4;
    static constexpr std::size_t WidgetCount = 2;

    TopBarComponent();

    // Its name in the context menu of the window, which shows or hides it.
    [[nodiscard]] QString title() const;
    void setTitle(const QString& title);
    // Whether the user can move it to another edge of the window.
    [[nodiscard]] bool isMovable() const;
    void setMovable(bool movable);

  signals:
    void titleChanged(const QString& title);
    void movableChanged(bool movable);

  private:
    // Shows the actions it has, in the order of their interfaces, before the widgets: all of them but the one of the
    // interface leaving, which is about to go.
    void showActions(std::optional<std::size_t> leaving = std::nullopt);
    void showWidget(std::size_t slot);
    // Gives the widget of slot back as it was provided: without a parent, which would delete it.
    void hideWidget(std::size_t slot);

    QToolBar m_bar;
    QMainWindow* m_mainWindow = nullptr;                 // Consumed
    std::array<QAction*, ActionCount> m_actions{};       // Consumed
    std::array<QWidget*, WidgetCount> m_widgets{};       // Consumed
    std::array<QWidget*, WidgetCount> m_holders{};       // In the bar, each holding the widget of its slot
    std::array<QAction*, WidgetCount> m_holderActions{}; // Of the holders in the bar, hidden while they hold nothing
};
