// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "LogControl.h"

#include "LogComponent.h"

LogControl::LogControl(LogComponent& component) : m_component(component) {}

void LogControl::startLogging()
{
    m_component.setLogging(true);
}

void LogControl::stopLogging()
{
    m_component.setLogging(false);
}

bool LogControl::isLogging() const
{
    return m_component.isLogging();
}

void LogControl::setLevel(Nebula::LogLevel level)
{
    m_component.setLevel(level);
}

Nebula::LogLevel LogControl::level() const
{
    return m_component.level();
}
