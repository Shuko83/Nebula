// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QSettings>

#include "LinkStyle.h"

// What NebulaStudio keeps from one launch to the next for the user who runs it, in settings: those of the user for
// NebulaStudio, see StudioComponent, or a file in the tests. Only how things are shown, which a project does not hold.

// The format of the preferences: a JSON file, an object whose members are the keys of the settings, each group an
// object of its own ("edition/linkStyle" is the member linkStyle of the member edition). A file that is not such an
// object is not read, and written again with the values set since.
[[nodiscard]] QSettings::Format preferencesFormat();

// How the links are drawn: curves until another style is written, or when the one written is not known.
[[nodiscard]] LinkStyle readLinkStyle(const QSettings& settings);
// Writes style by its name, which stays readable there whatever the order of LinkStyle.
void writeLinkStyle(QSettings& settings, LinkStyle style);

// The color scheme of the theme: Qt::ColorScheme::Unknown, the one of the system, until another one is written, or
// when the one written is not known.
[[nodiscard]] Qt::ColorScheme readColorScheme(const QSettings& settings);
// Writes scheme by its name: system for Qt::ColorScheme::Unknown, light or dark.
void writeColorScheme(QSettings& settings, Qt::ColorScheme scheme);

// Whether the overview of the edition zone is shown: until false is written, a boolean, whatever else is.
[[nodiscard]] bool readOverviewShown(const QSettings& settings);
void writeOverviewShown(QSettings& settings, bool shown);
