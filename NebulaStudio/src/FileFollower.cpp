// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "FileFollower.h"

#include <QFileInfo>
#include <QStringList>

FileFollower::FileFollower(QObject* parent) : QObject(parent)
{
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &FileFollower::tellChange);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &FileFollower::tellChange);
}

void FileFollower::follow(const QString& filePath)
{
    m_filePath = filePath;
    watch();
}

void FileFollower::unwatch()
{
    const QStringList watched = m_watcher.files() + m_watcher.directories();
    if(!watched.isEmpty()) // Which it would warn about
    {
        m_watcher.removePaths(watched);
    }
}

void FileFollower::watch()
{
    unwatch();
    for(const QString& path : {m_filePath, QFileInfo(m_filePath).absolutePath()})
    {
        if(!m_filePath.isEmpty() && QFileInfo::exists(path))
        {
            m_watcher.addPath(path);
        }
    }
}

void FileFollower::tellChange()
{
    watch();
    emit changed();
}
