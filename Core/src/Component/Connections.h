// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QMetaObject>

namespace Nebula
{

// Disconnects connections, and forgets them.
void disconnectAll(QList<QMetaObject::Connection>& connections);

} // namespace Nebula
