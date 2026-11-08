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

// Whether type is an enumeration or flags, Q_ENUM or Q_FLAG, which JSON holds by the names of their values.
bool isEnumeration(QMetaType type)
{
    return type.flags().testFlag(QMetaType::IsEnumeration);
}

// value as JSON holds it: what QJsonValue::fromVariant() makes of it, but the empty string for an enumeration whose
// value has no name, flags without any, which it would hold as null.
QJsonValue jsonOf(const QVariant& value)
{
    const QJsonValue json = QJsonValue::fromVariant(value);
    return json.isNull() && isEnumeration(value.metaType()) ? QJsonValue(QJsonValue::String) : json;
}

} // namespace

std::optional<QJsonValue> toJson(const QVariant& value)
{
    const QJsonValue json = jsonOf(value);
    QVariant read = fromJson(json, value.metaType());
    if(!read.convert(value.metaType()) || read != value)
    {
        return std::nullopt;
    }
    return json;
}

QVariant fromJson(const QJsonValue& json, QMetaType type)
{
    if(isEnumeration(type) && json.isString() && json.toString().isEmpty())
    {
        return QVariant(type); // 0, which has no name: no flag
    }
    return json.toVariant();
}

bool writeJson(QObject& object, const QMetaProperty& property, const QJsonValue& value)
{
    return property.write(&object, fromJson(value, property.metaType()));
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
    return property != properties.end() && writeJson(object, *property, value);
}

QStringList writeProperties(QObject& object, const QList<QMetaProperty>& properties, const QJsonObject& values)
{
    QStringList unset = values.keys();
    for(const QMetaProperty& property : properties)
    {
        const QString name = QString::fromLatin1(property.name());
        if(unset.contains(name) && writeJson(object, property, values.value(name)))
        {
            unset.removeOne(name);
        }
    }
    return unset;
}

QList<QMetaProperty> componentProperties(const Component& component)
{
    const std::optional<ComponentInfo> info = ComponentFactory::instance().component(component.componentId());
    return info ? info->properties : QList<QMetaProperty>();
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
