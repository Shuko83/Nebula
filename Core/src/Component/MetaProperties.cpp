// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "MetaProperties.h"

namespace Nebula
{

QList<QMetaProperty> declaredProperties(const QMetaObject& metaObject, const QMetaObject& base)
{
    QList<QMetaProperty> properties;
    for(int index = base.propertyCount(); index < metaObject.propertyCount(); ++index)
    {
        properties.append(metaObject.property(index));
    }
    return properties;
}

} // namespace Nebula
