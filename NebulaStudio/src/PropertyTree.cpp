// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "PropertyTree.h"

#include <QHeaderView>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <algorithm>
#include <optional>

#include "Nebula/Component/Component.h"
#include "Nebula/Component/ComponentFactory.h"
#include "Nebula/Galaxy/Stargate.h"
#include "Nebula/Settings/SettingsFile.h"

namespace
{

// Lists under group the properties of the objects among objects named name.
void listGroup(QTreeWidgetItem* group, const QList<Nebula::PropertyObject>& objects, const QString& name,
               const ListProperties& list)
{
    for(const Nebula::PropertyObject& added : objects)
    {
        if(added.name == name)
        {
            list(group, added.object, added.properties());
        }
    }
}

// Adds group under parent when it has rows under it; deletes it otherwise.
void adoptIfFilled(QTreeWidgetItem& parent, QTreeWidgetItem* group)
{
    if(group->childCount() > 0)
    {
        parent.addChild(group);
    }
    else
    {
        delete group;
    }
}

// Opens the rows above row.
void openParents(const QTreeWidgetItem& row)
{
    for(QTreeWidgetItem* parent = row.parent(); parent != nullptr; parent = parent->parent())
    {
        parent->setExpanded(true);
    }
}

} // namespace

QTreeWidget* newChoiceTree(const QStringList& headers)
{
    auto* tree = new QTreeWidget;
    tree->setColumnCount(static_cast<int>(headers.size()));
    tree->setHeaderLabels(headers);
    tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    return tree;
}

QTreeWidgetItem* groupRow(const QString& text)
{
    auto* row = new QTreeWidgetItem({text});
    row->setFlags(Qt::ItemIsEnabled);
    return row;
}

QList<QMetaProperty> ownProperties(const Nebula::ComponentFactory& factory, const Nebula::Component& component)
{
    const std::optional<Nebula::ComponentInfo> info = factory.component(component.componentId());
    return info ? info->properties : QList<QMetaProperty>();
}

QList<QMetaProperty> heldProperties(const Nebula::Component& component, QObject* object,
                                    const QList<QMetaProperty>& properties)
{
    QList<QMetaProperty> held = properties;
    held.removeIf([&component, object](const QMetaProperty& property)
                  { return !Nebula::SettingsFile::canHold(component, object, QString::fromLatin1(property.name())); });
    return held;
}

void addObjectGroups(QTreeWidgetItem& componentRow, const Nebula::Component& component, const ListProperties& list)
{
    const QList<Nebula::PropertyObject> objects = component.propertyObjects();
    listGroup(&componentRow, objects, QString(), list); // With its own
    for(const QString& name : groupNames(objects))
    {
        QTreeWidgetItem* group = groupRow(name);
        listGroup(group, objects, name, list);
        adoptIfFilled(componentRow, group);
    }
}

void addIfFilled(QTreeWidget& tree, QTreeWidgetItem* row)
{
    if(row->childCount() == 0)
    {
        delete row;
        return;
    }
    tree.addTopLevelItem(row);
    row->setExpanded(true);
}

void openAbove(QTreeWidget& tree, const std::function<bool(const QTreeWidgetItem&)>& isChosen)
{
    for(QTreeWidgetItemIterator row(&tree); *row != nullptr; ++row)
    {
        if(isChosen(**row))
        {
            openParents(**row);
        }
    }
}

void makeCheckable(QTreeWidgetItem& row, int column, bool checked, Qt::ItemFlags flags)
{
    row.setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | flags);
    row.setCheckState(column, checked ? Qt::Checked : Qt::Unchecked);
}

QStringList groupNames(const QList<Nebula::PropertyObject>& objects)
{
    QStringList names;
    for(const Nebula::PropertyObject& added : objects)
    {
        if(!added.name.isEmpty() && !names.contains(added.name))
        {
            names.append(added.name);
        }
    }
    return names;
}

QMetaProperty metaPropertyOf(const QObject& object, const QString& name)
{
    const QMetaObject* metaObject = object.metaObject();
    return metaObject->property(metaObject->indexOfProperty(name.toLatin1().constData()));
}

QList<const Nebula::Stargate*> stargatesAmong(const std::vector<std::unique_ptr<Nebula::Component>>& components)
{
    QList<const Nebula::Stargate*> stargates;
    for(const std::unique_ptr<Nebula::Component>& component : components)
    {
        if(const auto* stargate = qobject_cast<const Nebula::Stargate*>(component.get()))
        {
            stargates.append(stargate);
        }
    }
    return stargates;
}

std::optional<QString> exposedNameOf(const Nebula::Stargate& stargate, const Nebula::ExposedProperty& target)
{
    const QList<Nebula::ExposedProperty>& exposed = stargate.properties();
    const auto isTarget = [&target](const Nebula::ExposedProperty& property)
    {
        return property.component == target.component && property.object == target.object &&
               property.property == target.property;
    };
    const auto found = std::ranges::find_if(exposed, isTarget);
    return found != exposed.end() ? std::optional(found->name) : std::nullopt;
}
