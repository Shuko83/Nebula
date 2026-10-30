// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QObject>
#include <QSettings>
#include <QString>

#include "LinkStyle.h"
#include "Nebula/Application/Universe.h"
#include "Preferences.h"

class QCommandLineParser;

// What the parts of NebulaStudio share: the application assembled in its edition zone, with the plugins it is made
// from, the preferences of its user, and what the parts tell one another. A part that loads or unloads a plugin, or
// opens one from its file, tells it with refreshPlugins(), for the lists of plugins and of components to read them
// again; one that writes a project file tells it with refreshGalaxies(), for the list of galaxies to read it again;
// tell() is for a sentence the status bar of the window shows, as are the settings files of the application that
// failed. MainWindow assembles these parts around a Studio, and so does the galaxy of NebulaStudio, see StudioPlugin/.
class Studio : public QObject
{
    Q_OBJECT

  public:
    // The preferences are those of the user for the application named name of organization, NebulaStudio, rather than
    // those of QCoreApplication, which would move the AppConfigLocation the applications run here write to.
    Studio(const QString& organization, const QString& name, QObject* parent = nullptr);

    [[nodiscard]] Nebula::Universe& application();
    [[nodiscard]] const Nebula::Universe& application() const;
    // Lists the plugins of the directories of commandLine, processed by a parser it was given to
    // Universe::addCommandLineOptions(), with those of plugins/, and sets the plugin root of the project files.
    void applyCommandLine(const QCommandLineParser& commandLine);

    // How the links of the edition zone are drawn: as the user chose them last, from one launch to the next.
    [[nodiscard]] LinkStyle linkStyle() const;
    // Draws the links in style from now on, and at the next launch; see linkStyleChanged().
    void setLinkStyle(LinkStyle style);

    // Has the lists of plugins and of components read them again: see pluginsChanged().
    void refreshPlugins();
    // Has the list of galaxies read its directory again: see galaxiesChanged().
    void refreshGalaxies();
    // Has message shown in the status bar: see told().
    void tell(const QString& message);

  signals:
    // A plugin was loaded, unloaded or opened from its file: the plugins and their components are to be listed again.
    void pluginsChanged();
    // A project file was written, or a plugin opened from its file: the galaxies are to be listed again.
    void galaxiesChanged();
    void linkStyleChanged(LinkStyle style);
    // message is a sentence to show in the status bar, for a while.
    void told(const QString& message);

  private:
    Nebula::Universe m_application; // What is assembled in the edition zone, with the plugins it is made from
    QSettings m_preferences;        // Of the user, kept from one launch to the next
    LinkStyle m_linkStyle = readLinkStyle(m_preferences);
};
