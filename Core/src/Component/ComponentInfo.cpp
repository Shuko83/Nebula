// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Nebula/Component/ComponentInfo.h"

namespace Nebula
{

QString componentName(const QMetaObject& metaObject)
{
    const QString className = QString::fromLatin1(metaObject.className());
    const qsizetype separator = className.lastIndexOf(u"::");
    return separator < 0 ? className : className.mid(separator + 2);
}

} // namespace Nebula
