// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QHash>
#include <QList>
#include <QString>
#include <QToolBar>

#include "Nebula/Component/Component.h"

class QAction;
class QMainWindow;
class QWidget;

// A bar at the top of the main window it consumes as "mainWindow", which it cannot do without: a QToolBar it creates,
// in that window for as long as it has it. The bar shows the actions it consumes as the list "actions", then the
// widgets it consumes as the list "widgets", each in the order of their bindings and for as long as it has it. It needs
// a QApplication.
class TopBarComponent : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "A bar of actions and widgets at the top of the main window")
    Q_CLASSINFO("category", "Widgets")
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(bool movable READ isMovable WRITE setMovable NOTIFY movableChanged)

  public:
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
    // Declares what it consumes: the window, the actions and the widgets the bar shows.
    void consumeInterfaces();
    // Shows the bar at the top of the window it consumes.
    void addToWindow();
    // Gives the bar back to itself, without a parent: the window would delete it.
    void removeFromWindow();
    // Shows the actions of shown, in their order, before the widgets: those it has, or all of them but one about to go.
    // An action it has twice is shown once, in the place of the first.
    void showActions(const QList<QAction*>& shown);
    // Shows the actions it has but action, about to go.
    void showActionsBut(QAction* action);
    // Inserts those of shown the bar does not show yet, in their order, before the widgets.
    void insertActions(const QList<QAction*>& shown);
    // Shows widget in its place, among those it has, after the actions; once, when it has it twice.
    void showWidget(QWidget* widget);
    // Gives widget back as it was provided, without a parent, which would delete it, unless it has it twice.
    void hideWidget(QWidget* widget);
    // Takes widget out of holder, the action of the holder of the bar that shows it, for the bar not to delete it with
    // holder, and gives it no parent.
    static void release(QAction* holder, QWidget* widget);
    // The holder in the bar of the first widget after index in m_widgets that it shows; nullptr when there is none.
    [[nodiscard]] QAction* holderAfter(qsizetype index) const;

    QToolBar m_bar;
    QMainWindow* m_mainWindow = nullptr; // Consumed
    QList<QAction*> m_actions;           // Consumed
    QList<QWidget*> m_widgets;           // Consumed
    // The action of the holder in the bar of each widget it shows: a widget of the bar is owned by the action the bar
    // makes of it, the holder, not the widget consumed.
    QHash<QWidget*, QAction*> m_holders;
};
