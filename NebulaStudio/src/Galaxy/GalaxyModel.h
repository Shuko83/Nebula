// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QIcon>
#include <QList>
#include <QString>
#include <QStringList>
#include <cstdint>
#include <optional>

#include "Nebula/Galaxy/GalaxyInfo.h"
#include "RowsModel.h"

class QFileSystemWatcher;
class QMimeData;

namespace Nebula
{
class Universe;
}

// A galaxy of the directory of a GalaxyModel, as its file tells it.
struct ListedGalaxy
{
    QString filePath;
    std::optional<Nebula::GalaxyInfo> info; // nullopt when its file cannot be read
    QString error;                          // Why it cannot be
    QStringList missingPlugins;             // Those of its plugins the application does not know

    // Whether it can be opened: its file can be read, and the application knows each of its plugins.
    [[nodiscard]] bool isLoadable() const;
};

using GalaxyRows = RowsModel<ListedGalaxy, &ListedGalaxy::filePath>;

// The galaxies of a directory, its project files (*.nbp), one row per file, sorted by name whatever its case. A galaxy
// can be loaded when its file can be read and the application knows each of its plugins: the others are dimmed, and
// their state tells why, with a mark, and cannot be dragged into the edition zone as the others are. It follows the
// directory, and its files, as they are added, removed, renamed or written.
class GalaxyModel : public GalaxyRows
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

    using Galaxy = ListedGalaxy;

    // Lists nothing until it is given a directory. A galaxy can be loaded with the plugins application knows.
    explicit GalaxyModel(const Nebula::Universe& application, QObject* parent = nullptr);

    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
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

  protected:
    [[nodiscard]] QStringList headers() const override;

  private:
    // What the cells of galaxy have for role, one that is the same in every column.
    [[nodiscard]] static QVariant rowData(const Galaxy& galaxy, int role);
    // What the name of galaxy has for role.
    [[nodiscard]] QVariant nameData(const Galaxy& galaxy, int role) const;
    // What the state of galaxy has for role.
    [[nodiscard]] QVariant stateData(const Galaxy& galaxy, int role) const;
    // The galaxies of the directory as their files tell them now, in the order of the rows.
    [[nodiscard]] QList<Galaxy> read() const;
    // The galaxy of the file filePath as it tells it now.
    [[nodiscard]] Galaxy readGalaxy(const QString& filePath) const;
    // Follows the directory, once it exists, and the files of the galaxies listed: the directory alone does not tell
    // when one of them is written again in place.
    void watch();
    // Follows the directory, once it exists, and again once it is made again: a watcher forgets a path that is
    // removed.
    void watchDirectory();
    // Follows the files of the galaxies listed, in place of those it followed: a file saved again is replaced, then
    // forgotten, and the new ones are not followed yet.
    void watchFiles();
    // The files of the galaxies listed.
    [[nodiscard]] QStringList filePaths() const;

    const Nebula::Universe& m_application;
    QString m_directory;
    QFileSystemWatcher* m_watcher = nullptr; // Of m_directory and of the files of the galaxies listed
    QIcon m_galaxyIcon;                      // Beside each name
    QIcon m_warningIcon;                     // Beside the state of a galaxy that cannot be loaded
};
