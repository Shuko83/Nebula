// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ComponentViewer.h"

#include <QHeaderView>
#include <QLabel>
#include <QTreeWidget>
#include <optional>

#include "Nebula/Component/ComponentFactory.h"

namespace
{

// The list of the properties of a component: their name, their type and whether they can be written.
QTreeWidget* newPropertyList()
{
    auto* list = new QTreeWidget;
    list->setHeaderLabels({ComponentViewer::tr("Name"), ComponentViewer::tr("Type"), ComponentViewer::tr("Access")});
    list->setRootIsDecorated(false);
    list->setUniformRowHeights(true);
    list->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    return list;
}

// The row of property in the list of newPropertyList().
QTreeWidgetItem* propertyItem(const QMetaProperty& property)
{
    return new QTreeWidgetItem(
        {QString::fromLatin1(property.name()), QString::fromLatin1(property.typeName()),
         property.isWritable() ? ComponentViewer::tr("Read/write") : ComponentViewer::tr("Read-only")});
}

} // namespace

ComponentViewer::ComponentViewer(const Nebula::ComponentFactory& factory, QWidget* parent)
    : Inspector(parent), m_factory(factory), m_properties(newPropertyList())
{
    m_name = addField(tr("Name"));
    m_id = addField(tr("Id"));
    m_plugin = addField(tr("Plugin"));
    m_category = addField(tr("Category"));
    m_description = addField(tr("Description"));
    addRow(tr("Properties"), m_properties);
    showPlaceholder(tr("Select a component"));
}

void ComponentViewer::setComponentId(const QString& componentId)
{
    const std::optional<Nebula::ComponentInfo> component = m_factory.component(componentId);
    if(!component)
    {
        showPlaceholder(tr("Select a component"));
        return;
    }
    showInfo(*component);
    showPage();
}

void ComponentViewer::showInfo(const Nebula::ComponentInfo& component)
{
    m_name->setText(component.name);
    m_id->setText(component.id);
    m_plugin->setText(component.pluginId);
    m_category->setText(component.category);
    m_description->setText(component.description);
    showProperties(component.properties);
}

void ComponentViewer::showProperties(const QList<QMetaProperty>& properties)
{
    m_properties->clear();
    for(const QMetaProperty& property : properties)
    {
        m_properties->addTopLevelItem(propertyItem(property));
    }
}
