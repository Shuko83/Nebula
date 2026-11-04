// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "LogFilter.h"

#include "LogComponent.h"

using Nebula::LogLevel;

LogFilter& LogFilter::instance()
{
    static LogFilter filter;
    return filter;
}

void LogFilter::join(LogComponent& component)
{
    if(m_running.contains(&component))
    {
        return;
    }
    m_running.append(&component);
    take(component);
    install();
}

void LogFilter::leave(LogComponent& component)
{
    if(m_running.removeOne(&component) && m_running.isEmpty())
    {
        uninstall();
    }
}

void LogFilter::share(const LogComponent& component)
{
    if(!m_running.contains(&component))
    {
        return;
    }
    take(component);
    refilter();
}

void LogFilter::filter(QLoggingCategory* category)
{
    const LogFilter& self = instance();
    const QLoggingCategory::CategoryFilter previous = self.m_previous;
    // Not known yet: Qt runs it on every category as it installs it, before it returns the one it replaces.
    if(previous == nullptr)
    {
        return;
    }
    previous(category);
    if(self.m_active)
    {
        self.disableUnder(*category);
    }
}

void LogFilter::disableUnder(QLoggingCategory& category) const
{
    for(const LogLevel level : Nebula::LogLevels)
    {
        if(!m_logging || level < m_level)
        {
            category.setEnabled(Nebula::qtMsgType(level), false);
        }
    }
}

void LogFilter::take(const LogComponent& component)
{
    m_level = component.level();
    m_logging = component.isLogging();
    giveOthers(component);
}

void LogFilter::giveOthers(const LogComponent& component)
{
    // A copy: what the others tell as they follow may stop one of them.
    const QList<LogComponent*> running = m_running;
    for(LogComponent* other : running)
    {
        if(other != &component && m_running.contains(other))
        {
            other->follow(component.level(), component.isLogging());
        }
    }
}

void LogFilter::install()
{
    if(!m_installed)
    {
        m_previous = QLoggingCategory::installFilter(&LogFilter::filter);
        m_installed = true;
    }
    m_active = true;
    refilter();
}

void LogFilter::uninstall()
{
    m_active = false;
    const QLoggingCategory::CategoryFilter current = QLoggingCategory::installFilter(m_previous);
    if(current == &LogFilter::filter)
    {
        m_previous = nullptr;
        m_installed = false;
        return;
    }
    // Another filter is on top of this one, which it calls: it stays, this one under it letting everything pass.
    QLoggingCategory::installFilter(current);
}

void LogFilter::refilter()
{
    // Installing a filter runs it on every category: the installed one is installed again.
    const QLoggingCategory::CategoryFilter current = QLoggingCategory::installFilter(&LogFilter::filter);
    if(current != &LogFilter::filter)
    {
        QLoggingCategory::installFilter(current);
    }
}
