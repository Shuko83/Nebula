// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QString>

// Tells when a file changes: written in place, replaced, created or removed. A QFileSystemWatcher only tells that a
// file it watches is written in place: its directory is watched as well, which tells that a file was added, removed or
// replaced, as QSaveFile replaces it. Each is watched once it exists, again each time one of them changes.
class FileFollower : public QObject
{
    Q_OBJECT

  public:
    explicit FileFollower(QObject* parent = nullptr);

    // Follows the file filePath from now on, in place of the one followed until now; none when it is empty.
    void follow(const QString& filePath);

  signals:
    // The file followed changed, or another one of its directory.
    void changed();

  private:
    // No longer watches what it watched.
    void unwatch();
    // Watches the file and its directory, those of them that exist now, and nothing else.
    void watch();
    // Watches them again, as a file replaced is no longer watched, then tells it.
    void tellChange();

    QFileSystemWatcher m_watcher;
    QString m_filePath;
};
