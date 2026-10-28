// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Nebula/Component/PropertyObject.h"

#include <QObject>

#include "Component/MetaProperties.h"

namespace Nebula
{

QList<QMetaProperty> PropertyObject::properties() const
{
    if(object == nullptr)
    {
        return {};
    }
    QList<QMetaProperty> properties = declaredProperties(*object->metaObject(), QObject::staticMetaObject);
    properties.removeIf([](const QMetaProperty& property) { return !property.isDesignable() || !property.isStored(); });
    return properties;
}

} // namespace Nebula
