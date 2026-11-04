// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "LogComponent.h"

#include <QLoggingCategory>
#include <QStringList>

#include "LogFilter.h"
#include "StandardPlugin.h"

using Nebula::LogLevel;

NEBULA_REGISTER_COMPONENT(StandardPlugin, LogComponent);

namespace
{

Q_LOGGING_CATEGORY(lcStandard, "nebula.standard")

// The names of the levels, the least severe first.
QString levelNames()
{
    QStringList names;
    for(const LogLevel level : Nebula::LogLevels)
    {
        names.append(Nebula::logLevelName(level));
    }
    return names.join(QStringLiteral(", "));
}

} // namespace

LogComponent::LogComponent()
{
    provideInterface<Nebula::ILog>(QStringLiteral("log"), &m_log);
    provideInterface<Nebula::ILogControl>(QStringLiteral("logControl"), &m_control);
}

LogComponent::~LogComponent()
{
    // The process does not keep the values of a component that is gone, even running.
    LogFilter::instance().leave(*this);
}

bool LogComponent::isLogging() const
{
    return m_logging;
}

void LogComponent::setLogging(bool logging)
{
    if(changeLogging(logging))
    {
        LogFilter::instance().share(*this);
    }
}

LogLevel LogComponent::level() const
{
    return m_level;
}

void LogComponent::setLevel(LogLevel level)
{
    if(changeLevel(level))
    {
        LogFilter::instance().share(*this);
    }
}

QString LogComponent::levelName() const
{
    return Nebula::logLevelName(m_level);
}

void LogComponent::setLevelName(const QString& name)
{
    const std::optional<LogLevel> level = Nebula::logLevelNamed(name);
    if(!level)
    {
        qCWarning(lcStandard).noquote() << QStringLiteral("%1: \"%2\" is not a log level (%3), %4 is kept")
                                               .arg(objectName(), name, levelNames(), levelName());
        return;
    }
    setLevel(*level);
}

QString LogComponent::category() const
{
    return m_log.category();
}

void LogComponent::setCategory(const QString& category)
{
    if(category == m_log.category() || !canBeCategory(category))
    {
        return;
    }
    m_log.setCategory(category);
    emit categoryChanged(category);
}

bool LogComponent::canBeCategory(const QString& category) const
{
    if(!category.isEmpty())
    {
        return true;
    }
    qCWarning(lcStandard).noquote()
        << QStringLiteral("%1: a log category needs a name, %2 is kept").arg(objectName(), m_log.category());
    return false;
}

bool LogComponent::onStart()
{
    LogFilter::instance().join(*this);
    return true;
}

void LogComponent::onStop()
{
    LogFilter::instance().leave(*this);
}

void LogComponent::follow(LogLevel level, bool logging)
{
    changeLevel(level);
    changeLogging(logging);
}

bool LogComponent::changeLogging(bool logging)
{
    if(m_logging == logging)
    {
        return false;
    }
    m_logging = logging;
    emit loggingChanged(m_logging);
    return true;
}

bool LogComponent::changeLevel(LogLevel level)
{
    if(m_level == level)
    {
        return false;
    }
    m_level = level;
    emit levelChanged(levelName());
    return true;
}
