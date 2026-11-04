// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QString>

#include "Nebula/Standard/LogLevel.h"

namespace Nebula
{

// What a component writes its messages with: the interface "log" of a LogComponent of StandardPlugin. A message goes
// to Qt's journal, where the application writes its own (for a NebulaApplication, its console, --log-file and logger),
// under the category of the LogComponent. Whether it is written depends on the level of the whole application, which
// ILogControl drives.
class ILog
{
  public:
    virtual ~ILog() = default;

    // Writes message at level, after source and ": " when source is not empty: who writes, the name of its instance
    // for instance.
    virtual void write(LogLevel level, const QString& message, const QString& source) = 0;
    // Whether a message at level would be written: one that would not needs not be built.
    [[nodiscard]] virtual bool isWritten(LogLevel level) const = 0;

    void debug(const QString& message, const QString& source = {})
    {
        write(LogLevel::Debug, message, source);
    }
    void info(const QString& message, const QString& source = {})
    {
        write(LogLevel::Info, message, source);
    }
    void warning(const QString& message, const QString& source = {})
    {
        write(LogLevel::Warning, message, source);
    }
    void critical(const QString& message, const QString& source = {})
    {
        write(LogLevel::Critical, message, source);
    }
};

} // namespace Nebula
