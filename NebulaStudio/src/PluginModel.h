// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <cstdint>

#include "Nebula/Plugin/PluginInfo.h"
#include "RowsModel.h"

namespace Nebula
{
class PluginManager;
}

using PluginRows = RowsModel<Nebula::PluginInfo, &Nebula::PluginInfo::id>;

// The plugins found by a PluginManager, one row per plugin, sorted by id.
class PluginModel : public PluginRows
{
    Q_OBJECT

  public:
    enum Column : std::uint8_t
    {
        Name,
        Version,
        State,
        ColumnCount,
    };

    // The plugin id, in every column.
    static constexpr int IdRole = Qt::UserRole;

    explicit PluginModel(const Nebula::PluginManager& plugins, QObject* parent = nullptr);

    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    // The first column of pluginId; invalid when it is not listed.
    [[nodiscard]] QModelIndex indexOf(const QString& pluginId) const;

    // Reads the plugins of the manager again: inserts the ones found since, and updates the others, e.g. once loaded.
    void refresh();

  protected:
    [[nodiscard]] QStringList headers() const override;

  private:
    // What the cell of info in column shows.
    [[nodiscard]] QVariant display(const Nebula::PluginInfo& info, int column) const;

    const Nebula::PluginManager& m_plugins;
};
