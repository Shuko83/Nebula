// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Nebula/Standard/LogLevel.h"

namespace Nebula
{

// What drives the log of the whole application: the interface "logControl" of a LogComponent of StandardPlugin.
// While the component runs, its level and whether it logs hold for every message of the process, those of every
// plugin, of Core, of the application and of Qt, not only those written through its ILog. Driving it changes its
// properties, which tell it: nothing here does.
class ILogControl
{
  public:
    virtual ~ILogControl() = default;

    // The messages are written again, but those under level().
    virtual void startLogging() = 0;
    // No message is written any longer, but a fatal one, which Qt does not let be disabled.
    virtual void stopLogging() = 0;
    [[nodiscard]] virtual bool isLogging() const = 0;
    // The least severe level written: the messages under it are not.
    virtual void setLevel(LogLevel level) = 0;
    [[nodiscard]] virtual LogLevel level() const = 0;
};

} // namespace Nebula
