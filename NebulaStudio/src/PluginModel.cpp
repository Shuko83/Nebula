// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "PluginModel.h"

#include "Nebula/Plugin/PluginManager.h"

PluginModel::PluginModel(const Nebula::PluginManager& plugins, QObject* parent) : PluginRows(parent), m_plugins(plugins)
{
}

QVariant PluginModel::data(const QModelIndex& index, int role) const
{
    if(!isCell(index))
    {
        return {};
    }
    const Nebula::PluginInfo& info = rowAt(index);
    if(role == IdRole || role == Qt::ToolTipRole)
    {
        return info.id;
    }
    return role == Qt::DisplayRole ? display(info, index.column()) : QVariant();
}

QModelIndex PluginModel::indexOf(const QString& pluginId) const
{
    return indexOfKey(pluginId);
}

void PluginModel::refresh()
{
    setRows(m_plugins.plugins()); // Listed by id, as the rows are
}

QStringList PluginModel::headers() const
{
    return {tr("Name"), tr("Version"), tr("State")};
}

QVariant PluginModel::display(const Nebula::PluginInfo& info, int column) const
{
    // In the order of the columns.
    const QVariantList cells{info.name, info.version.toString(),
                             m_plugins.isLoaded(info.id) ? tr("Loaded") : tr("Not loaded")};
    return cells.value(column);
}
