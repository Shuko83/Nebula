// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ProjectFiles.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>

#include "Messages.h"
#include "Nebula/Application/WholeFile.h"

QString defaultProjectDirectory()
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/scene");
}

bool writeProjectFile(QWidget* parent, const QString& title, const QString& filePath, const QJsonObject& json)
{
    QString why;
    if(!Nebula::WholeFile::write(filePath, QJsonDocument(json).toJson(), why))
    {
        warn(parent, title,
             QCoreApplication::translate("ProjectFiles", "%1 cannot be saved:\n%2")
                 .arg(QDir::toNativeSeparators(filePath), why));
        return false;
    }
    return true;
}
