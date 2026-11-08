// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Nebula/Application/WholeFile.h"

#include <QByteArray>
#include <QDir>
#include <QFileDevice>
#include <QFileInfo>
#include <QSaveFile>
#include <QThread>

namespace Nebula::WholeFile
{

namespace
{

// Writes data to the file filePath once, whole or not at all: what failed, with error set, NoError when it is written.
QFileDevice::FileError writeOnce(const QString& filePath, const QByteArray& data, QString& error)
{
    QSaveFile file(filePath);
    if(file.open(QIODevice::WriteOnly) && file.write(data) >= 0 && file.commit())
    {
        return QFileDevice::NoError;
    }
    error = file.errorString();
    return file.error();
}

// Writes data to the file filePath, again while its replacement is refused, see write(); false, with error set, when
// it cannot.
bool writeWhileRefused(const QString& filePath, const QByteArray& data, QString& error)
{
    QFileDevice::FileError failed = writeOnce(filePath, data, error);
    for(int attempt = 1; failed == QFileDevice::RenameError && attempt < Attempts; ++attempt)
    {
        QThread::sleep(Pause);
        failed = writeOnce(filePath, data, error);
    }
    return failed == QFileDevice::NoError;
}

} // namespace

bool write(const QString& filePath, const QByteArray& data, QString& error)
{
    if(!QDir().mkpath(QFileInfo(filePath).absolutePath()))
    {
        error = QStringLiteral("Its directory cannot be created");
        return false;
    }
    return writeWhileRefused(filePath, data, error);
}

} // namespace Nebula::WholeFile
