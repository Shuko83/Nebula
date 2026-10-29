// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "StudioComponent.h"

#include <QCommandLineParser>
#include <QCoreApplication>

#include "StudioPlugin.h"
#include "StudioPlugin_info.h"
#include "Theme.h"

NEBULA_REGISTER_COMPONENT(StudioPlugin, StudioComponent);

namespace
{

// Whose preferences a studio has: those of the user of NebulaStudio, the link style they chose there say.
constexpr auto PreferencesName = "NebulaStudio";

// Gives the application the theme of NebulaStudio, once: what follows the color scheme of the system is connected
// once.
void applyThemeOnce()
{
    [[maybe_unused]] static const bool applied = []
    {
        applyTheme();
        return true;
    }();
}

} // namespace

StudioComponent::StudioComponent()
    : m_studio(QString::fromUtf8(StudioPluginInfo::organization), QString::fromUtf8(PreferencesName))
{
    applyThemeOnce();
    applyCommandLine();
    provideInterface<Studio>(QStringLiteral("studio"), &m_studio);
}

void StudioComponent::applyCommandLine()
{
    QCommandLineParser parser;
    Nebula::Universe::addCommandLineOptions(parser);
    // Its other options are not known here: refused, and left out, while those of Universe are read.
    parser.parse(QCoreApplication::arguments());
    m_studio.applyCommandLine(parser);
}

void StudioComponent::onStop()
{
    m_studio.application().stop(); // Nothing when it does not run
}
