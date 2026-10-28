// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "JsonFile.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonParseError>
#include <QSaveFile>

namespace Nebula::JsonFile
{

namespace
{

// The document data is; nullopt, with error set, when it is not JSON.
std::optional<QJsonDocument> parse(const QByteArray& data, QString& error)
{
    QJsonParseError parsed{};
    QJsonDocument document = QJsonDocument::fromJson(data, &parsed);
    if(parsed.error != QJsonParseError::NoError)
    {
        error = parsed.errorString();
        return std::nullopt;
    }
    return document;
}

} // namespace

std::optional<QJsonDocument> read(const QString& filePath, QString& error)
{
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
    {
        error = file.errorString();
        return std::nullopt;
    }
    return parse(file.readAll(), error);
}

bool write(const QString& filePath, const QJsonObject& object, QString& error)
{
    QSaveFile file(filePath);
    if(QDir().mkpath(QFileInfo(filePath).absolutePath()) && file.open(QIODevice::WriteOnly) &&
       file.write(QJsonDocument(object).toJson()) >= 0 && file.commit())
    {
        return true;
    }
    error = file.errorString();
    return false;
}

} // namespace Nebula::JsonFile
