// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QString>

class QJsonObject;
class QWidget;

// The project files NebulaStudio writes: those of the application, and those of the galaxies it edits.

// Where the project files are saved and looked for unless told otherwise: scene/ next to the application.
[[nodiscard]] QString defaultProjectDirectory();
// Writes json to the file filePath, whole or not at all; tells why in a box titled title, over parent, and returns
// false, when it cannot be written.
bool writeProjectFile(QWidget* parent, const QString& title, const QString& filePath, const QJsonObject& json);
