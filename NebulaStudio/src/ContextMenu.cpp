// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ContextMenu.h"

#include <QAction>
#include <utility>

ContextMenu::ContextMenu(QWidget* parent) : m_menu(parent) {}

QAction* ContextMenu::add(const QString& text, std::function<void()> chosen, const QIcon& icon, const QString& toolTip)
{
    return add(m_menu, text, std::move(chosen), icon, toolTip);
}

QAction* ContextMenu::add(QMenu& submenu, const QString& text, std::function<void()> chosen, const QIcon& icon,
                          const QString& toolTip)
{
    QAction* action = submenu.addAction(icon, text);
    action->setToolTip(toolTip);
    m_chosen.insert(action, std::move(chosen));
    return action;
}

QMenu& ContextMenu::addMenu(const QString& text, const QIcon& icon)
{
    return *m_menu.addMenu(icon, text);
}

void ContextMenu::addSeparator()
{
    m_menu.addSeparator();
}

void ContextMenu::exec(const QPoint& position)
{
    // The entry chosen in a submenu as well.
    const QAction* chosen = m_menu.exec(position);
    if(const auto found = m_chosen.constFind(chosen); found != m_chosen.cend())
    {
        (*found)();
    }
}
