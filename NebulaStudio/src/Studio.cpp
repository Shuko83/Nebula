// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Studio.h"

Studio::Studio(const QString& organization, const QString& name, QObject* parent)
    : QObject(parent), m_preferences(organization, name)
{
    // In the status bar, as what the edition zone tells, nor in the middle of a run, which goes on without the file.
    connect(&m_application, &Nebula::Universe::settingsFileFailed, this, &Studio::told);
}

Nebula::Universe& Studio::application()
{
    return m_application;
}

const Nebula::Universe& Studio::application() const
{
    return m_application;
}

void Studio::applyCommandLine(const QCommandLineParser& commandLine)
{
    m_application.applyCommandLine(commandLine);
    refreshPlugins();
    refreshGalaxies(); // Those that missed one of these plugins are loadable now
}

LinkStyle Studio::linkStyle() const
{
    return m_linkStyle;
}

void Studio::setLinkStyle(LinkStyle style)
{
    m_linkStyle = style;
    writeLinkStyle(m_preferences, style);
    emit linkStyleChanged(style);
}

void Studio::refreshPlugins()
{
    emit pluginsChanged();
}

void Studio::refreshGalaxies()
{
    emit galaxiesChanged();
}

void Studio::tell(const QString& message)
{
    emit told(message);
}
