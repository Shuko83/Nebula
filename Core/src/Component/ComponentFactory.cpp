// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Nebula/Component/ComponentFactory.h"

#include <QLoggingCategory>
#include <algorithm>
#include <ranges>
#include <utility>

#include "Component/MetaProperties.h"

namespace Nebula
{

namespace
{

Q_LOGGING_CATEGORY(lcComponents, "nebula.components")

// Q_CLASSINFO(name, ...) of metaObject or of one of its base classes; empty when none declares it.
QString classInfo(const QMetaObject& metaObject, const char* name)
{
    const int index = metaObject.indexOfClassInfo(name);
    return index < 0 ? QString() : QString::fromUtf8(metaObject.classInfo(index).value());
}

// The name of the class of metaObject, without its namespaces.
QString shortClassName(const QMetaObject& metaObject)
{
    const QString className = QString::fromLatin1(metaObject.className());
    const qsizetype separator = className.lastIndexOf(u"::");
    return separator < 0 ? className : className.mid(separator + 2);
}

ComponentInfo readInfo(const QMetaObject& metaObject, const QString& pluginId)
{
    ComponentInfo info;
    info.name = shortClassName(metaObject);
    info.pluginId = pluginId;
    info.id = info.pluginId + u'.' + info.name;
    info.description = classInfo(metaObject, "description");
    info.category = classInfo(metaObject, "category");
    // The properties of QObject and Component come first: they are not the component's.
    info.properties = declaredProperties(metaObject, Component::staticMetaObject);
    return info;
}

} // namespace

ComponentFactory::ComponentFactory() = default;

ComponentFactory::~ComponentFactory() = default;

ComponentFactory& ComponentFactory::instance()
{
    static ComponentFactory factory;
    return factory;
}

void ComponentFactory::add(const QString& pluginId, Plugin& plugin)
{
    if(std::ranges::any_of(m_types | std::views::values, [&](const Type& type) { return type.plugin == &plugin; }))
    {
        return;
    }
    for(const Plugin::ComponentType& componentType : plugin.m_componentTypes)
    {
        addType(readInfo(*componentType.metaObject, pluginId), plugin, componentType.builder);
    }
}

void ComponentFactory::addType(ComponentInfo info, Plugin& plugin, Plugin::Builder builder)
{
    if(m_types.contains(info.id))
    {
        qCWarning(lcComponents).noquote() << "Ignoring" << info.id << "- already registered";
        return;
    }
    const QString componentId = info.id;
    m_types.emplace(componentId, Type{.info = std::move(info), .plugin = &plugin, .builder = builder});
}

void ComponentFactory::remove(const QString& pluginId)
{
    std::erase_if(m_types, [&pluginId](const auto& type) { return type.second.info.pluginId == pluginId; });
}

QList<ComponentInfo> ComponentFactory::components() const
{
    QList<ComponentInfo> components;
    components.reserve(static_cast<qsizetype>(m_types.size()));
    for(const Type& type : m_types | std::views::values)
    {
        components.append(type.info);
    }
    return components;
}

std::optional<ComponentInfo> ComponentFactory::component(const QString& componentId) const
{
    const auto type = m_types.find(componentId);
    if(type == m_types.end())
    {
        return std::nullopt;
    }
    return type->second.info;
}

std::unique_ptr<Component> ComponentFactory::create(const QString& componentId) const
{
    const auto type = m_types.find(componentId);
    if(type == m_types.end())
    {
        return nullptr;
    }
    std::unique_ptr<Component> component = type->second.builder();
    component->m_componentId = componentId;
    return component;
}

} // namespace Nebula
