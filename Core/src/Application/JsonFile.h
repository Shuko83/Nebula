// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <optional>

// Reading and writing the JSON files of Core: projects and settings files.
namespace Nebula::JsonFile
{

// The document the file filePath holds; nullopt, with error set, when it cannot be read or is not JSON.
[[nodiscard]] std::optional<QJsonDocument> read(const QString& filePath, QString& error);
// Writes object to the file filePath, creating its directory when needed; false, with error set, when it cannot.
bool write(const QString& filePath, const QJsonObject& object, QString& error);

} // namespace Nebula::JsonFile
