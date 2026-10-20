// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ActionComponent.h"

#include <QKeySequence>

#include "WidgetsPlugin.h"

NEBULA_REGISTER_COMPONENT(WidgetsPlugin, ActionComponent);

ActionComponent::ActionComponent()
{
    m_action.setText(QStringLiteral("Action"));
    provideInterface<QAction>(QStringLiteral("action"), &m_action);
}

QString ActionComponent::text() const
{
    return m_action.text();
}

void ActionComponent::setText(const QString& text)
{
    if(m_action.text() == text)
    {
        return;
    }
    m_action.setText(text);
    emit textChanged(text);
}

QString ActionComponent::shortcut() const
{
    return m_action.shortcut().toString(QKeySequence::PortableText);
}

void ActionComponent::setShortcut(const QString& shortcut)
{
    const QKeySequence keys = QKeySequence::fromString(shortcut, QKeySequence::PortableText);
    if(m_action.shortcut() == keys)
    {
        return;
    }
    m_action.setShortcut(keys);
    emit shortcutChanged(this->shortcut());
}
