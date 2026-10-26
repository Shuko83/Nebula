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

ComponentInfo readInfo(const QMetaObject& metaObject, const QString& pluginId)
{
    ComponentInfo info;
    info.name = componentName(metaObject);
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

void ComponentFactory::add(const QString& pluginId, IPlugin& plugin)
{
    if(std::ranges::any_of(m_types | std::views::values, [&](const Type& type) { return type.plugin == &plugin; }))
    {
        return;
    }
    for(const QMetaObject* metaObject : plugin.components())
    {
        addType(readInfo(*metaObject, pluginId), plugin);
    }
}

void ComponentFactory::addType(ComponentInfo info, IPlugin& plugin)
{
    if(m_types.contains(info.id))
    {
        qCWarning(lcComponents).noquote() << "Ignoring" << info.id << "- already registered";
        return;
    }
    const QString componentId = info.id;
    m_types.emplace(componentId, Type{.info = std::move(info), .plugin = &plugin});
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
    // Never nullptr: the name is one of those its plugin listed.
    std::unique_ptr<Component> component = type->second.plugin->create(type->second.info.name);
    component->m_componentId = componentId;
    return component;
}

} // namespace Nebula
