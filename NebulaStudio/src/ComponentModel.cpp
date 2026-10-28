// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ComponentModel.h"

#include <QMimeData>

#include "Nebula/Component/ComponentFactory.h"
#include "Theme.h"

ComponentModel::ComponentModel(const Nebula::ComponentFactory& factory, QObject* parent)
    : ComponentRows(parent), m_factory(factory), m_icon(icon(GlossaryTerm::Astre))
{
}

QVariant ComponentModel::data(const QModelIndex& index, int role) const
{
    if(!isCell(index))
    {
        return {};
    }
    const Nebula::ComponentInfo& component = rowAt(index);
    if(role == IdRole || role == Qt::ToolTipRole) // In every column
    {
        return role == IdRole ? component.id : component.description;
    }
    return cellData(component, index.column(), role);
}

Qt::ItemFlags ComponentModel::flags(const QModelIndex& index) const
{
    const Qt::ItemFlags flags = ComponentRows::flags(index);
    return index.isValid() ? flags | Qt::ItemIsDragEnabled : flags;
}

QStringList ComponentModel::mimeTypes() const
{
    return {MimeType};
}

QMimeData* ComponentModel::mimeData(const QModelIndexList& indexes) const
{
    // The view selects a single row, one index per column: they all hold the same id.
    if(indexes.isEmpty())
    {
        return nullptr;
    }
    auto* data = new QMimeData;
    data->setData(MimeType, indexes.first().data(IdRole).toString().toUtf8());
    return data;
}

void ComponentModel::refresh()
{
    setRows(m_factory.components()); // Listed by id, as the rows are
}

QStringList ComponentModel::headers() const
{
    return {tr("Name"), tr("Category"), tr("Plugin")};
}

QVariant ComponentModel::cellData(const Nebula::ComponentInfo& component, int column, int role) const
{
    if(role == Qt::DecorationRole)
    {
        return column == Name ? QVariant(m_icon) : QVariant();
    }
    // In the order of the columns.
    return role == Qt::DisplayRole ? QVariantList{component.name, component.category, component.pluginId}.value(column)
                                   : QVariant();
}
