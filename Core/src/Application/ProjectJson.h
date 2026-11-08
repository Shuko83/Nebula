// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QMetaProperty>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <optional>

class QObject;

namespace Nebula
{
class Component;
} // namespace Nebula

// How a project, and a settings file, hold the properties of the components and of their objects: shared by
// Universe and SettingsFile.
namespace Nebula::ProjectJson
{

// value as JSON holds it; nullopt when it would not be read back as it is, its type being one JSON has nothing for.
// Through the conversions of QVariant: an enumeration by the name of its value, flags by the names of theirs joined by
// |, the empty string for none, a QColor by its name, #aarrggbb when it is not opaque, a QUrl by its text.
[[nodiscard]] std::optional<QJsonValue> toJson(const QVariant& value);
// json, read as a value of type: what QJsonValue::toVariant() makes of it, which writing a property or converting it
// to type turns into one; but the empty string is 0 for an enumeration, or flags, of type: what toJson() writes when
// their value has no name, no flag.
[[nodiscard]] QVariant fromJson(const QJsonValue& json, QMetaType type);
// Sets property of object to value, as JSON holds it, see fromJson(); false when it cannot be set.
bool writeJson(QObject& object, const QMetaProperty& property, const QJsonValue& value);

// The properties of object JSON holds, by name: those of properties that can be written, and whose value toJson()
// holds.
[[nodiscard]] QJsonObject writableProperties(const QObject& object, const QList<QMetaProperty>& properties);

// Sets the property name of object, one of properties, to value; false when it is none of them or cannot be set.
bool writeProperty(QObject& object, const QList<QMetaProperty>& properties, const QString& name,
                   const QJsonValue& value);
// Sets the properties of object, among properties, to values, by name; returns the names of those it could not set.
// In the order of properties, the one they are declared in, rather than the one of values: the property that tells what
// the object has, its count of widgets say, before those that depend on it.
QStringList writeProperties(QObject& object, const QList<QMetaProperty>& properties, const QJsonObject& values);

// The properties of the type of component, as its plugin registered it; empty when it is no longer registered.
[[nodiscard]] QList<QMetaProperty> componentProperties(const Component& component);

// An object a component added with Component::addPropertiesForQObject(), as JSON finds it again: the name it was
// added with, and how many of the objects of the component were added with that name before it.
struct ObjectKey
{
    QString name;
    qsizetype index = 0;
};

// The key of object among the objects of component; nullopt when it is none of them.
[[nodiscard]] std::optional<ObjectKey> objectKey(const Component& component, const QObject* object);
// The object of component key is the key of; nullptr when it has none.
[[nodiscard]] QObject* object(const Component& component, const ObjectKey& key);

// The properties a project holds for object, component itself or one of its objects, when they can be written and
// JSON holds their value: those ComponentFactory knows of a component, those PropertyObject::properties() lists of
// an object. None when object is neither.
[[nodiscard]] QList<QMetaProperty> savedProperties(const Component& component, const QObject* object);

} // namespace Nebula::ProjectJson
