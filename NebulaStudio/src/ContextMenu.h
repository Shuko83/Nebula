// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QHash>
#include <QIcon>
#include <QMenu>
#include <QString>
#include <functional>

class QAction;
class QActionGroup;
class QPoint;

// A context menu whose entries each do what they are for once it is closed, rather than while it closes: editing a
// name, which takes the focus, needs it back where it was.
class ContextMenu
{
  public:
    explicit ContextMenu(QWidget* parent);

    // An entry telling text, which calls chosen once it is chosen, with icon before it and toolTip.
    QAction* add(const QString& text, std::function<void()> chosen, const QIcon& icon = {},
                 const QString& toolTip = {});
    // The same entry in submenu, one addMenu() returned.
    QAction* add(QMenu& submenu, const QString& text, std::function<void()> chosen, const QIcon& icon = {},
                 const QString& toolTip = {});
    // A submenu telling text, with icon before it, which it owns: its entries are added by add().
    QMenu& addMenu(const QString& text, const QIcon& icon = {});
    // A group of its entries, which it owns: those added to it are radio buttons, one of them checked at most.
    QActionGroup& addGroup();
    void addSeparator();
    // Shows it at position, the one of the screen, and does what the entry chosen is for, in a submenu or not;
    // nothing when none is. Returns what it did, as a step of a history names it: the text of the entry, after the
    // title of its submenu, without mnemonic nor ellipsis, Bind to Ticker1.display say; empty when none was chosen.
    QString exec(const QPoint& position);

  private:
    // An entry of the menu, or of one of its submenus.
    struct Entry
    {
        std::function<void()> chosen; // What it does
        QString step;                 // What it does, as a history names it
    };

    QMenu m_menu;
    QHash<const QAction*, Entry> m_entries;
};
