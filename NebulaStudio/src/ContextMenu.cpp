// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ContextMenu.h"

#include <QAction>
#include <utility>

ContextMenu::ContextMenu(QWidget* parent) : m_menu(parent) {}

QAction* ContextMenu::add(const QString& text, std::function<void()> chosen, const QIcon& icon, const QString& toolTip)
{
    QAction* action = m_menu.addAction(icon, text);
    action->setToolTip(toolTip);
    m_chosen.insert(action, std::move(chosen));
    return action;
}

void ContextMenu::addSeparator()
{
    m_menu.addSeparator();
}

void ContextMenu::exec(const QPoint& position)
{
    const QAction* chosen = m_menu.exec(position);
    if(const auto found = m_chosen.constFind(chosen); found != m_chosen.cend())
    {
        (*found)();
    }
}
