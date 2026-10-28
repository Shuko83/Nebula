// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QMetaProperty>

namespace Nebula
{

// The properties of metaObject that base, one of its base classes, does not have: those declared from base on.
[[nodiscard]] QList<QMetaProperty> declaredProperties(const QMetaObject& metaObject, const QMetaObject& base);

} // namespace Nebula
