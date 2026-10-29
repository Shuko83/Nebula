// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "LinkStyle.h"

class QSettings;

// What NebulaStudio keeps from one launch to the next for the user who runs it, in settings: those of the user for
// NebulaStudio, see MainWindow, or a file in the tests. Only how things are shown, which a project does not hold.

// How the links are drawn: curves until another style is written, or when the one written is not known.
[[nodiscard]] LinkStyle readLinkStyle(const QSettings& settings);
// Writes style by its name, which stays readable there whatever the order of LinkStyle.
void writeLinkStyle(QSettings& settings, LinkStyle style);
