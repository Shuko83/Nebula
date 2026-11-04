// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QString>

#include "Log.h"
#include "LogControl.h"
#include "Nebula/Component/Component.h"
#include "Nebula/Standard/LogLevel.h"

// The log of the application. It provides "log", a Nebula::ILog, for components to write their messages in Qt's
// journal under its category, and "logControl", a Nebula::ILogControl, to drive it, from construction on.
// While it runs, its level and whether it logs hold for the whole process, every category of every plugin, of Core,
// of the application and of Qt (see LogFilter), and are shared with the other running instances. While it is edited,
// it filters nothing.
class LogComponent : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Writes the messages of components, and drives the log of the application while it runs")
    Q_CLASSINFO("category", "Diagnostics")
    Q_PROPERTY(bool logging READ isLogging WRITE setLogging NOTIFY loggingChanged)
    Q_PROPERTY(QString level READ levelName WRITE setLevelName NOTIFY levelChanged)
    Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY categoryChanged)

  public:
    LogComponent();
    ~LogComponent() override;

    // Whether messages are written, but those under level().
    [[nodiscard]] bool isLogging() const;
    void setLogging(bool logging);
    // The least severe level written.
    [[nodiscard]] Nebula::LogLevel level() const;
    void setLevel(Nebula::LogLevel level);
    // The name of level(), whatever its case; one of no level is refused with a warning.
    [[nodiscard]] QString levelName() const;
    void setLevelName(const QString& name);
    // The category of the messages written through "log"; an empty one is refused with a warning.
    [[nodiscard]] QString category() const;
    void setCategory(const QString& category);

  signals:
    void loggingChanged(bool logging);
    void levelChanged(const QString& level);
    void categoryChanged(const QString& category);

  protected:
    bool onStart() override;
    void onStop() override;

  private:
    friend class LogFilter; // Calls follow()

    // The values of the running instance that started or was driven last, which it shares.
    void follow(Nebula::LogLevel level, bool logging);
    bool changeLogging(bool logging);
    bool changeLevel(Nebula::LogLevel level);
    // Whether category can be the one of its messages; warns when it cannot.
    [[nodiscard]] bool canBeCategory(const QString& category) const;

    Log m_log;
    LogControl m_control{*this};
    Nebula::LogLevel m_level = Nebula::LogLevel::Debug;
    bool m_logging = true;
};
