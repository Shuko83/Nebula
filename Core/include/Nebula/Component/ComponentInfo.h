// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QMetaObject>
#include <QMetaProperty>
#include <QString>

#include "Core_export.h"

namespace Nebula
{

// What ComponentFactory knows about a registered component, read from its QMetaObject without instantiating it.
struct ComponentInfo
{
    QString id;                      // <plugin id>.<name>
    QString name;                    // Class name, without its namespace
    QString description;             // Q_CLASSINFO("description", ...), empty when not given
    QString category;                // Q_CLASSINFO("category", ...), empty when not given
    QString pluginId;                // Plugin that registered it
    QList<QMetaProperty> properties; // Its Q_PROPERTY, not those of QObject and Component
};

// The name of the component whose class is metaObject: its class name, without its namespace. What IPlugin::create()
// is given, and the end of its id.
[[nodiscard]] CORE_EXPORT QString componentName(const QMetaObject& metaObject);

} // namespace Nebula
