// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ProjectFiles.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include "Messages.h"

QString defaultProjectDirectory()
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/scene");
}

bool writeProjectFile(QWidget* parent, const QString& title, const QString& filePath, const QJsonObject& json)
{
    QSaveFile file(filePath);
    if(!file.open(QIODevice::WriteOnly) || file.write(QJsonDocument(json).toJson()) < 0 || !file.commit())
    {
        warn(parent, title,
             QCoreApplication::translate("ProjectFiles", "%1 cannot be saved:\n%2")
                 .arg(QDir::toNativeSeparators(filePath), file.errorString()));
        return false;
    }
    return true;
}
