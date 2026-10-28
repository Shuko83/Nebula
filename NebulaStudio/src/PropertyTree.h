// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QMetaProperty>
#include <QString>
#include <QStringList>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

class QObject;
class QTreeWidget;
class QTreeWidgetItem;

namespace Nebula
{
class Component;
class ComponentFactory;
class Stargate;
struct ExposedProperty;
struct PropertyObject;
} // namespace Nebula

// How NebulaStudio lists the properties of components, to edit them or to choose among them. The dialogs of settings
// files and of stargates list a row per component, its properties, then those of its objects, as the property editor
// shows them, and the groups of objects only open when one of their properties is chosen.

// What lists the properties of object, among properties, under parent.
using ListProperties =
    std::function<void(QTreeWidgetItem* parent, QObject* object, const QList<QMetaProperty>& properties)>;

// A tree of choices, whose columns are headers, as wide as what they show.
[[nodiscard]] QTreeWidget* newChoiceTree(const QStringList& headers);
// A row that only groups others: a component, or the objects of a name.
[[nodiscard]] QTreeWidgetItem* groupRow(const QString& text);
// The properties of the type of component, as its plugin registered it in factory; none when it is no longer
// registered.
[[nodiscard]] QList<QMetaProperty> ownProperties(const Nebula::ComponentFactory& factory,
                                                 const Nebula::Component& component);
// The properties among properties of object, component or one of its objects, that a project holds, as a settings
// file and a stargate can hold them.
[[nodiscard]] QList<QMetaProperty> heldProperties(const Nebula::Component& component, QObject* object,
                                                  const QList<QMetaProperty>& properties);
// Lists under componentRow the properties of the objects of component: those of the objects without a name with its
// own, those of the others under a row of their name, kept when it has some.
void addObjectGroups(QTreeWidgetItem& componentRow, const Nebula::Component& component, const ListProperties& list);
// Adds row to tree, open, when it has rows under it; deletes it otherwise.
void addIfFilled(QTreeWidget& tree, QTreeWidgetItem* row);
// Opens the rows above those isChosen accepts: the groups of objects, which can have many properties, a widget say,
// are only open when one is chosen.
void openAbove(QTreeWidget& tree, const std::function<bool(const QTreeWidgetItem&)>& isChosen);
// Makes row checkable in column, checked or not, with flags besides.
void makeCheckable(QTreeWidgetItem& row, int column, bool checked, Qt::ItemFlags flags = {});

// The names of the objects among objects that have one, each once, in the order they were added: the groups their
// properties are shown in, after those of the objects without a name.
[[nodiscard]] QStringList groupNames(const QList<Nebula::PropertyObject>& objects);
// The property name of object; not valid when it has none of that name.
[[nodiscard]] QMetaProperty metaPropertyOf(const QObject& object, const QString& name);
// The stargates among components.
[[nodiscard]] QList<const Nebula::Stargate*>
stargatesAmong(const std::vector<std::unique_ptr<Nebula::Component>>& components);
// The name stargate exposes target as, whatever the name of target; nullopt when it does not expose it.
[[nodiscard]] std::optional<QString> exposedNameOf(const Nebula::Stargate& stargate,
                                                   const Nebula::ExposedProperty& target);
