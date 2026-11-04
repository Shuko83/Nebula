// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QString>
#include <QStringView>
#include <QtGlobal>
#include <array>
#include <optional>

namespace Nebula
{

// The severity of a message. A level stands for itself and those after it: the least severe written, the others are.
// No fatal one: a component does not end the application by writing a message.
enum class LogLevel
{
    Debug,
    Info,
    Warning,
    Critical,
};

// Every level, the least severe first.
inline constexpr std::array LogLevels{LogLevel::Debug, LogLevel::Info, LogLevel::Warning, LogLevel::Critical};

// The name of level, the one --log-level of a NebulaApplication gives it: debug, info, warning or critical.
[[nodiscard]] inline QString logLevelName(LogLevel level)
{
    switch(level)
    {
    case LogLevel::Debug:
        return QStringLiteral("debug");
    case LogLevel::Info:
        return QStringLiteral("info");
    case LogLevel::Warning:
        return QStringLiteral("warning");
    case LogLevel::Critical:
        break;
    }
    return QStringLiteral("critical");
}

// The level named name, whatever its case; none for a name of no level.
[[nodiscard]] inline std::optional<LogLevel> logLevelNamed(QStringView name)
{
    for(const LogLevel level : LogLevels)
    {
        if(name.compare(logLevelName(level), Qt::CaseInsensitive) == 0)
        {
            return level;
        }
    }
    return std::nullopt;
}

// The type of Qt's messages of level, whose values are not in the order of severity.
[[nodiscard]] constexpr QtMsgType qtMsgType(LogLevel level)
{
    switch(level)
    {
    case LogLevel::Debug:
        return QtDebugMsg;
    case LogLevel::Info:
        return QtInfoMsg;
    case LogLevel::Warning:
        return QtWarningMsg;
    case LogLevel::Critical:
        break;
    }
    return QtCriticalMsg;
}

} // namespace Nebula
