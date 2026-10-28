// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ProjectJson.h"

#include <QObject>
#include <algorithm>

#include "Nebula/Component/Component.h"
#include "Nebula/Component/ComponentFactory.h"

namespace Nebula::ProjectJson
{

namespace
{

// The properties of the type of component, as its plugin registered it; empty when it is no longer registered.
QList<QMetaProperty> componentProperties(const Component& component)
{
    const std::optional<ComponentInfo> info = ComponentFactory::instance().component(component.componentId());
    return info ? info->properties : QList<QMetaProperty>();
}

// The properties of object, component or one of its property objects; empty when it is neither.
QList<QMetaProperty> objectProperties(const Component& component, const QObject* object)
{
    if(object == &component)
    {
        return componentProperties(component);
    }
    const QList<PropertyObject> objects = component.propertyObjects();
    const auto added = std::ranges::find(objects, object, &PropertyObject::object);
    return added != objects.end() ? added->properties() : QList<QMetaProperty>();
}

} // namespace

std::optional<QJsonValue> toJson(const QVariant& value)
{
    const QJsonValue json = QJsonValue::fromVariant(value);
    QVariant read = json.toVariant();
    if(!read.convert(value.metaType()) || read != value)
    {
        return std::nullopt;
    }
    return json;
}

QJsonObject writableProperties(const QObject& object, const QList<QMetaProperty>& properties)
{
    QJsonObject json;
    for(const QMetaProperty& property : properties)
    {
        const std::optional<QJsonValue> value = property.isWritable() ? toJson(property.read(&object)) : std::nullopt;
        if(value)
        {
            json.insert(QLatin1StringView(property.name()), *value);
        }
    }
    return json;
}

bool writeProperty(QObject& object, const QList<QMetaProperty>& properties, const QString& name,
                   const QJsonValue& value)
{
    const auto property = std::ranges::find(properties, name, [](const QMetaProperty& known)
                                            { return QString::fromLatin1(known.name()); });
    return property != properties.end() && property->write(&object, value.toVariant());
}

std::optional<ObjectKey> objectKey(const Component& component, const QObject* object)
{
    const QList<PropertyObject> objects = component.propertyObjects();
    const auto added = std::ranges::find(objects, object, &PropertyObject::object);
    if(added == objects.end())
    {
        return std::nullopt;
    }
    const auto sameName = [&added](const PropertyObject& other) { return other.name == added->name; };
    return ObjectKey{.name = added->name, .index = std::ranges::count_if(objects.begin(), added, sameName)};
}

QObject* object(const Component& component, const ObjectKey& key)
{
    qsizetype index = 0;
    const QList<PropertyObject> objects = component.propertyObjects();
    for(const PropertyObject& added : objects)
    {
        if(added.name == key.name && index++ == key.index)
        {
            return added.object;
        }
    }
    return nullptr;
}

QList<QMetaProperty> savedProperties(const Component& component, const QObject* object)
{
    QList<QMetaProperty> properties = objectProperties(component, object);
    // Only those a project holds: the others would not be read back.
    properties.removeIf([object](const QMetaProperty& property)
                        { return !property.isWritable() || !toJson(property.read(object)); });
    return properties;
}

} // namespace Nebula::ProjectJson
