// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QAbstractTableModel>
#include <QIcon>
#include <QList>
#include <QString>
#include <QStringList>
#include <cstdint>
#include <optional>

#include "Nebula/Application/GalaxyInfo.h"

class QFileSystemWatcher;
class QMimeData;

namespace Nebula
{
class Universe;
}

// The galaxies of a directory, its project files (*.nbp), one row per file, sorted by name whatever its case. A galaxy
// can be loaded when its file can be read and the application knows each of its plugins: the others are dimmed, and
// their state tells why, with a mark, and cannot be dragged into the edition zone as the others are. It follows the
// directory, and its files, as they are added, removed, renamed or written.
class GalaxyModel : public QAbstractTableModel
{
    Q_OBJECT

  public:
    enum Column : std::uint8_t
    {
        Name,
        State,
        ColumnCount,
    };

    // The path of its file, in every column.
    static constexpr int FilePathRole = Qt::UserRole;
    // What a galaxy that can be loaded is dragged as: the path of its file, in UTF-8.
    static constexpr QLatin1StringView MimeType{"application/x-nebula-galaxy"};

    // A galaxy of the directory, as its file tells it.
    struct Galaxy
    {
        QString filePath;
        std::optional<Nebula::GalaxyInfo> info; // nullopt when its file cannot be read
        QString error;                          // Why it cannot be
        QStringList missingPlugins;             // Those of its plugins the application does not know

        // Whether it can be opened: its file can be read, and the application knows each of its plugins.
        [[nodiscard]] bool isLoadable() const;
    };

    // Lists nothing until it is given a directory. A galaxy can be loaded with the plugins application knows.
    explicit GalaxyModel(const Nebula::Universe& application, QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                      int role = Qt::DisplayRole) const override;
    // A galaxy that can be loaded can be dragged, to be placed in the edition zone; the others cannot.
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;
    [[nodiscard]] QStringList mimeTypes() const override;
    [[nodiscard]] QMimeData* mimeData(const QModelIndexList& indexes) const override;

    // The directory listed, absolute; empty until it is given one.
    [[nodiscard]] QString directory() const;
    // Lists the galaxies of directory in place of the others, and follows it. It may not exist yet: its galaxies are
    // listed by the first refresh() once it does.
    void setDirectory(const QString& directory);

    // The galaxy of the file filePath; nullopt when it is not listed.
    [[nodiscard]] std::optional<Galaxy> galaxy(const QString& filePath) const;
    // The first column of the galaxy of filePath; invalid when it is not listed.
    [[nodiscard]] QModelIndex indexOf(const QString& filePath) const;

    // Reads the directory again: removes the galaxies whose file is gone, inserts the new ones, and reads the others
    // again, a file saved since or a plugin they missed known since.
    void refresh();

  private:
    // The galaxies of the directory as their files tell them now, in the order of the rows.
    [[nodiscard]] QList<Galaxy> read() const;
    // Follows the directory, once it exists, and the files of the galaxies listed: the directory alone does not tell
    // when one of them is written again in place.
    void watch();

    const Nebula::Universe& m_application;
    QString m_directory;
    QList<Galaxy> m_galaxies;                // In the order of the rows
    QFileSystemWatcher* m_watcher = nullptr; // Of m_directory and of the files of m_galaxies
    QIcon m_galaxyIcon;                      // Beside each name
    QIcon m_warningIcon;                     // Beside the state of a galaxy that cannot be loaded
};
