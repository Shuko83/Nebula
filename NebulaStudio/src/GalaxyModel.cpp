// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "GalaxyModel.h"

#include <QApplication>
#include <QBrush>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QMimeData>
#include <QStyle>
#include <algorithm>

#include "Nebula/Application/Universe.h"
#include "Theme.h"

namespace
{

// The project files, as NebulaStudio saves them.
const QString GalaxyFilter = QStringLiteral("*.nbp");

// What state tells of galaxy: whether it can be loaded, and why not.
QString stateText(const GalaxyModel::Galaxy& galaxy)
{
    if(!galaxy.info)
    {
        return GalaxyModel::tr("Unreadable");
    }
    return galaxy.missingPlugins.isEmpty() ? GalaxyModel::tr("Loadable") : GalaxyModel::tr("Missing plugins");
}

} // namespace

bool GalaxyModel::Galaxy::isLoadable() const
{
    return info && missingPlugins.isEmpty();
}

GalaxyModel::GalaxyModel(const Nebula::Universe& application, QObject* parent)
    : QAbstractTableModel(parent), m_application(application), m_watcher(new QFileSystemWatcher(this)),
      m_galaxyIcon(icon(GlossaryTerm::Galaxy)),
      m_warningIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning))
{
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &GalaxyModel::refresh);
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &GalaxyModel::refresh);
}

int GalaxyModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_galaxies.size());
}

int GalaxyModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant GalaxyModel::data(const QModelIndex& index, int role) const
{
    if(!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
    {
        return {};
    }
    const Galaxy& galaxy = m_galaxies.at(index.row());
    if(role == FilePathRole)
    {
        return galaxy.filePath;
    }
    if(role == Qt::ForegroundRole)
    {
        // Dimmed when it cannot be loaded, in the color of the hints: it is still read.
        return galaxy.isLoadable() ? QVariant() : QVariant(QBrush(currentThemeColors().mutedText));
    }
    if(index.column() == Name)
    {
        switch(role)
        {
        case Qt::DisplayRole:
            // Named after its file, as Universe::readGalaxy() names it, even when it cannot be read.
            return QFileInfo(galaxy.filePath).completeBaseName();
        case Qt::DecorationRole:
            return m_galaxyIcon;
        case Qt::ToolTipRole:
            return galaxy.info && !galaxy.info->description.isEmpty() ? QVariant(galaxy.info->description) : QVariant();
        default:
            return {};
        }
    }
    if(index.column() == State)
    {
        switch(role)
        {
        case Qt::DisplayRole:
            return stateText(galaxy);
        case Qt::DecorationRole:
            return galaxy.isLoadable() ? QVariant() : QVariant(m_warningIcon); // Told without color too
        case Qt::ToolTipRole:
            if(!galaxy.info)
            {
                return galaxy.error;
            }
            return galaxy.missingPlugins.isEmpty()
                       ? QVariant()
                       : QVariant(tr("Missing: %1").arg(galaxy.missingPlugins.join(QStringLiteral(", "))));
        default:
            return {};
        }
    }
    return {};
}

QVariant GalaxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation != Qt::Horizontal || role != Qt::DisplayRole)
    {
        return {};
    }
    switch(section)
    {
    case Name:
        return tr("Name");
    case State:
        return tr("State");
    default:
        return {};
    }
}

Qt::ItemFlags GalaxyModel::flags(const QModelIndex& index) const
{
    // An invalid index is asked for the flags of the root, which checkIndex() would warn about.
    const Qt::ItemFlags flags = QAbstractTableModel::flags(index);
    if(!index.isValid() || !checkIndex(index, CheckIndexOption::ParentIsInvalid))
    {
        return flags;
    }
    return m_galaxies.at(index.row()).isLoadable() ? flags | Qt::ItemIsDragEnabled : flags;
}

QStringList GalaxyModel::mimeTypes() const
{
    return {MimeType};
}

QMimeData* GalaxyModel::mimeData(const QModelIndexList& indexes) const
{
    // The view selects a single row, one index per column: they all hold the same file.
    if(indexes.isEmpty())
    {
        return nullptr;
    }
    auto* data = new QMimeData;
    data->setData(MimeType, indexes.first().data(FilePathRole).toString().toUtf8());
    return data;
}

QString GalaxyModel::directory() const
{
    return m_directory;
}

void GalaxyModel::setDirectory(const QString& directory)
{
    const QStringList watched = m_watcher->directories();
    if(!watched.isEmpty())
    {
        m_watcher->removePaths(watched);
    }
    m_directory = QDir(directory).absolutePath();
    refresh();
}

std::optional<GalaxyModel::Galaxy> GalaxyModel::galaxy(const QString& filePath) const
{
    const auto found = std::ranges::find(m_galaxies, filePath, &Galaxy::filePath);
    return found == m_galaxies.end() ? std::nullopt : std::optional<Galaxy>(*found);
}

QModelIndex GalaxyModel::indexOf(const QString& filePath) const
{
    const auto found = std::ranges::find(m_galaxies, filePath, &Galaxy::filePath);
    return found == m_galaxies.end() ? QModelIndex() : index(static_cast<int>(found - m_galaxies.begin()), 0);
}

void GalaxyModel::refresh()
{
    const QList<Galaxy> galaxies = read();
    // Those whose file is gone first, from the last one, for the rows of the others to stay where they are meanwhile.
    for(qsizetype row = m_galaxies.size() - 1; row >= 0; --row)
    {
        if(std::ranges::find(galaxies, m_galaxies.at(row).filePath, &Galaxy::filePath) == galaxies.end())
        {
            beginRemoveRows({}, static_cast<int>(row), static_cast<int>(row));
            m_galaxies.removeAt(row);
            endRemoveRows();
        }
    }
    // Those left are in the order galaxies were read in, the same one: the new ones are inserted where they belong.
    for(qsizetype row = 0; row < galaxies.size(); ++row)
    {
        if(row < m_galaxies.size() && m_galaxies.at(row).filePath == galaxies.at(row).filePath)
        {
            m_galaxies[row] = galaxies.at(row);
            continue;
        }
        beginInsertRows({}, static_cast<int>(row), static_cast<int>(row));
        m_galaxies.insert(row, galaxies.at(row));
        endInsertRows();
    }
    if(!m_galaxies.isEmpty())
    {
        emit dataChanged(index(0, 0), index(rowCount() - 1, ColumnCount - 1));
    }
    watch();
}

QList<GalaxyModel::Galaxy> GalaxyModel::read() const
{
    QList<Galaxy> galaxies;
    if(m_directory.isEmpty())
    {
        return galaxies;
    }
    const QFileInfoList files =
        QDir(m_directory).entryInfoList({GalaxyFilter}, QDir::Files, QDir::Name | QDir::IgnoreCase);
    for(const QFileInfo& file : files)
    {
        Galaxy galaxy{.filePath = file.absoluteFilePath(), .info = std::nullopt, .error = {}, .missingPlugins = {}};
        galaxy.info = Nebula::Universe::readGalaxy(galaxy.filePath, galaxy.error);
        if(galaxy.info)
        {
            galaxy.missingPlugins = m_application.missingPlugins(*galaxy.info);
        }
        galaxies.append(galaxy);
    }
    return galaxies;
}

void GalaxyModel::watch()
{
    // Once it exists, and again once it is made again: a watcher forgets a path that is removed.
    if(QFileInfo(m_directory).isDir() && !m_watcher->directories().contains(m_directory))
    {
        m_watcher->addPath(m_directory);
    }
    // A file saved again is replaced, then forgotten, and the new ones are not followed yet.
    const QStringList watched = m_watcher->files();
    if(!watched.isEmpty())
    {
        m_watcher->removePaths(watched);
    }
    QStringList files;
    for(const Galaxy& galaxy : m_galaxies)
    {
        files.append(galaxy.filePath);
    }
    if(!files.isEmpty())
    {
        m_watcher->addPaths(files);
    }
}
