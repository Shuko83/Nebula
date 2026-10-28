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

// What the tool tip of its state tells of galaxy: why it cannot be loaded.
QVariant stateToolTip(const GalaxyModel::Galaxy& galaxy)
{
    if(!galaxy.info)
    {
        return galaxy.error;
    }
    return galaxy.missingPlugins.isEmpty()
               ? QVariant()
               : QVariant(GalaxyModel::tr("Missing: %1").arg(galaxy.missingPlugins.join(QStringLiteral(", "))));
}

// The description of galaxy, in the tool tip of its name; none when it has none.
QVariant descriptionOf(const GalaxyModel::Galaxy& galaxy)
{
    return galaxy.info && !galaxy.info->description.isEmpty() ? QVariant(galaxy.info->description) : QVariant();
}

// The project files of directory, sorted by name whatever its case; none when directory is empty.
QFileInfoList galaxyFiles(const QString& directory)
{
    if(directory.isEmpty())
    {
        return {};
    }
    return QDir(directory).entryInfoList({GalaxyFilter}, QDir::Files, QDir::Name | QDir::IgnoreCase);
}

// Has watcher no longer follow paths; QFileSystemWatcher warns when it is given none.
void unwatch(QFileSystemWatcher& watcher, const QStringList& paths)
{
    if(!paths.isEmpty())
    {
        watcher.removePaths(paths);
    }
}

// Has watcher follow paths; QFileSystemWatcher warns when it is given none.
void watchAll(QFileSystemWatcher& watcher, const QStringList& paths)
{
    if(!paths.isEmpty())
    {
        watcher.addPaths(paths);
    }
}

} // namespace

bool ListedGalaxy::isLoadable() const
{
    return info && missingPlugins.isEmpty();
}

GalaxyModel::GalaxyModel(const Nebula::Universe& application, QObject* parent)
    : GalaxyRows(parent), m_application(application), m_watcher(new QFileSystemWatcher(this)),
      m_galaxyIcon(icon(GlossaryTerm::Galaxy)),
      m_warningIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning))
{
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &GalaxyModel::refresh);
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &GalaxyModel::refresh);
}

QVariant GalaxyModel::data(const QModelIndex& index, int role) const
{
    if(!isCell(index))
    {
        return {};
    }
    const Galaxy& galaxy = rowAt(index);
    if(role == FilePathRole || role == Qt::ForegroundRole)
    {
        return rowData(galaxy, role);
    }
    return index.column() == Name ? nameData(galaxy, role) : stateData(galaxy, role);
}

Qt::ItemFlags GalaxyModel::flags(const QModelIndex& index) const
{
    // An invalid index is asked for the flags of the root, which checkIndex() would warn about.
    const Qt::ItemFlags flags = GalaxyRows::flags(index);
    if(!index.isValid() || !checkIndex(index, CheckIndexOption::ParentIsInvalid))
    {
        return flags;
    }
    return rowAt(index).isLoadable() ? flags | Qt::ItemIsDragEnabled : flags;
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
    unwatch(*m_watcher, m_watcher->directories());
    m_directory = QDir(directory).absolutePath();
    refresh();
}

std::optional<GalaxyModel::Galaxy> GalaxyModel::galaxy(const QString& filePath) const
{
    const Galaxy* found = rowOf(filePath);
    return found != nullptr ? std::optional(*found) : std::nullopt;
}

QModelIndex GalaxyModel::indexOf(const QString& filePath) const
{
    return indexOfKey(filePath);
}

void GalaxyModel::refresh()
{
    setRows(read()); // In the order of the rows
    watch();
}

QStringList GalaxyModel::headers() const
{
    return {tr("Name"), tr("State")};
}

QVariant GalaxyModel::rowData(const Galaxy& galaxy, int role)
{
    if(role == FilePathRole)
    {
        return galaxy.filePath;
    }
    // Dimmed when it cannot be loaded, in the color of the hints: it is still read.
    return galaxy.isLoadable() ? QVariant() : QVariant(QBrush(currentThemeColors().mutedText));
}

QVariant GalaxyModel::nameData(const Galaxy& galaxy, int role) const
{
    if(role == Qt::DisplayRole)
    {
        // Named after its file, as Universe::readGalaxy() names it, even when it cannot be read.
        return QFileInfo(galaxy.filePath).completeBaseName();
    }
    if(role == Qt::DecorationRole)
    {
        return m_galaxyIcon;
    }
    return role == Qt::ToolTipRole ? descriptionOf(galaxy) : QVariant();
}

QVariant GalaxyModel::stateData(const Galaxy& galaxy, int role) const
{
    if(role == Qt::DisplayRole)
    {
        return stateText(galaxy);
    }
    if(role == Qt::DecorationRole)
    {
        return galaxy.isLoadable() ? QVariant() : QVariant(m_warningIcon); // Told without color too
    }
    return role == Qt::ToolTipRole ? stateToolTip(galaxy) : QVariant();
}

QList<GalaxyModel::Galaxy> GalaxyModel::read() const
{
    QList<Galaxy> galaxies;
    for(const QFileInfo& file : galaxyFiles(m_directory))
    {
        galaxies.append(readGalaxy(file.absoluteFilePath()));
    }
    return galaxies;
}

GalaxyModel::Galaxy GalaxyModel::readGalaxy(const QString& filePath) const
{
    Galaxy galaxy{.filePath = filePath, .info = std::nullopt, .error = {}, .missingPlugins = {}};
    galaxy.info = Nebula::Universe::readGalaxy(filePath, galaxy.error);
    if(galaxy.info)
    {
        galaxy.missingPlugins = m_application.missingPlugins(*galaxy.info);
    }
    return galaxy;
}

void GalaxyModel::watch()
{
    watchDirectory();
    watchFiles();
}

void GalaxyModel::watchDirectory()
{
    if(QFileInfo(m_directory).isDir() && !m_watcher->directories().contains(m_directory))
    {
        m_watcher->addPath(m_directory);
    }
}

void GalaxyModel::watchFiles()
{
    unwatch(*m_watcher, m_watcher->files());
    watchAll(*m_watcher, filePaths());
}

QStringList GalaxyModel::filePaths() const
{
    QStringList files;
    for(const Galaxy& galaxy : rows())
    {
        files.append(galaxy.filePath);
    }
    return files;
}
