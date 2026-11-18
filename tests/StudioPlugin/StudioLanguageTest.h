// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QMainWindow>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <memory>

#include "Nebula/Application/Universe.h"

// The galaxy of NebulaStudio run with a language in the preferences of its user. The language is chosen once per
// process: each case is a test executable of its own.
class StudioLanguageRun
{
  public:
    // Has the preferences of the user, in a directory of their own, ask for language.
    bool prepare(const QByteArray& language)
    {
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
        QDir(m_settings.path()).mkpath(QStringLiteral("FabienDG"));
        QFile preferences(m_settings.filePath(QStringLiteral("FabienDG/NebulaStudio.json")));
        return preferences.open(QIODevice::WriteOnly) &&
               preferences.write(R"({"appearance": {"language": ")" + language + R"("}})") > 0;
    }

    // Opens the galaxy with the plugins of the build.
    bool open()
    {
        m_universe = std::make_unique<Nebula::Universe>();
        m_universe->plugins().scan(QStringLiteral(NEBULA_STUDIO_PLUGINS_DIR));
        return m_universe->open(QStringLiteral(NEBULA_STUDIO_GALAXY));
    }

    [[nodiscard]] Nebula::Universe& universe()
    {
        return *m_universe;
    }

    // The window of NebulaStudio, once it is shown.
    [[nodiscard]] static QMainWindow* window()
    {
        for(QWidget* widget : QApplication::topLevelWidgets())
        {
            auto* window = qobject_cast<QMainWindow*>(widget);
            if(window != nullptr && window->isVisible())
            {
                return window;
            }
        }
        return nullptr;
    }

  private:
    QTemporaryDir m_settings; // Outlives what writes there
    std::unique_ptr<Nebula::Universe> m_universe;
};
