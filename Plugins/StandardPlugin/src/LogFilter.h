// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QLoggingCategory>
#include <atomic>

#include "Nebula/Standard/LogLevel.h"

class LogComponent;

// The log of the whole process while log components run: a category filter of Qt, installed on top of the one before
// it, which it calls first. It then disables, in every category, the messages under the level of the running
// components, or all of them while they do not log. What the application's rules disable stays so. Fatal messages
// are not disabled: Qt does not let them be.
// The running components share their level and whether they log: the one that starts gives its own to the others,
// as does the one driven. The filter goes with the last one, or lets everything pass when another filter was
// installed on top of it, which calls it.
class LogFilter
{
  public:
    [[nodiscard]] static LogFilter& instance();

    // component runs: its values become those of the process, and of the other running components.
    void join(LogComponent& component);
    // component does not run any longer, or is destroyed; nothing when it does not run.
    void leave(LogComponent& component);
    // The values of component changed: given to the others and to the categories when it runs, nothing otherwise.
    void share(const LogComponent& component);

  private:
    LogFilter() = default;

    // The filter installed, called by Qt for each category.
    static void filter(QLoggingCategory* category);
    void disableUnder(QLoggingCategory& category) const;
    // The values of component become those of the process and of the other running components.
    void take(const LogComponent& component);
    void giveOthers(const LogComponent& component);
    void install();
    void uninstall();
    // Runs the installed filter again on every category.
    static void refilter();

    QList<LogComponent*> m_running;
    // Whether filter() is in the chain of filters: it stays under a filter installed on top of it.
    bool m_installed = false;
    // Read by filter(), which Qt also calls when a category is created, in any thread.
    // The filter before this one, while it is installed: Qt's, which applies the rules of the application, or another
    // one.
    std::atomic<QLoggingCategory::CategoryFilter> m_previous = nullptr;
    std::atomic<bool> m_active = false;
    std::atomic<Nebula::LogLevel> m_level = Nebula::LogLevel::Debug;
    std::atomic<bool> m_logging = true;
};
