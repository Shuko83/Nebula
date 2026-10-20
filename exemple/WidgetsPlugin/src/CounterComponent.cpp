// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "CounterComponent.h"

#include <QAction>

#include "WidgetsPlugin.h"

NEBULA_REGISTER_COMPONENT(WidgetsPlugin, CounterComponent);

CounterComponent::CounterComponent()
{
    m_display.setAlignment(Qt::AlignCenter);
    showCount();
    provideInterface<QWidget>(QStringLiteral("display"), &m_display);
    consumeInterface<QAction>(
        QStringLiteral("action"), m_action, Nebula::InterfaceRequirement::Mandatory,
        [this] { m_triggered = connect(m_action, &QAction::triggered, this, &CounterComponent::countTrigger); },
        [this] { disconnect(m_triggered); });
}

int CounterComponent::count() const
{
    return m_count;
}

void CounterComponent::setCount(int count)
{
    if(m_count == count)
    {
        return;
    }
    m_count = count;
    showCount();
    emit countChanged(m_count);
}

QString CounterComponent::text() const
{
    return m_text;
}

void CounterComponent::setText(const QString& text)
{
    if(m_text == text)
    {
        return;
    }
    m_text = text;
    showCount();
    emit textChanged(m_text);
}

void CounterComponent::countTrigger()
{
    if(state() == Nebula::ComponentState::Running)
    {
        setCount(m_count + 1);
    }
}

void CounterComponent::showCount()
{
    // Not arg(), which warns when the text has no %1.
    m_display.setText(QString(m_text).replace(QStringLiteral("%1"), QString::number(m_count)));
}
