// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ContextMenu.h"

#include <QAction>
#include <QActionGroup>
#include <utility>

namespace
{

// text, the one of an entry or of a menu, as a history names what it does: without its mnemonic nor its ellipsis.
QString plain(QString text)
{
    text.remove(u'&');
    if(text.endsWith(u"..."))
    {
        text.chop(3);
    }
    return text;
}

} // namespace

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
    const QString step = &submenu == &m_menu ? plain(text) : plain(submenu.title()) + u' ' + plain(text);
    m_entries.insert(action, {.chosen = std::move(chosen), .step = step});
    return action;
}

QMenu& ContextMenu::addMenu(const QString& text, const QIcon& icon)
{
    return *m_menu.addMenu(icon, text);
}

QActionGroup& ContextMenu::addGroup()
{
    return *new QActionGroup(&m_menu); // Exclusive
}

void ContextMenu::addSeparator()
{
    m_menu.addSeparator();
}

QString ContextMenu::exec(const QPoint& position)
{
    // The entry chosen in a submenu as well.
    const QAction* chosen = m_menu.exec(position);
    const auto found = m_entries.constFind(chosen);
    if(found == m_entries.cend())
    {
        return {};
    }
    const Entry entry = *found;
    entry.chosen();
    return entry.step;
}
