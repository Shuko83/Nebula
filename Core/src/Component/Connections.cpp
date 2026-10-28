// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Connections.h"

#include <QObject>

namespace Nebula
{

void disconnectAll(QList<QMetaObject::Connection>& connections)
{
    for(const QMetaObject::Connection& connection : std::as_const(connections))
    {
        QObject::disconnect(connection);
    }
    connections.clear();
}

} // namespace Nebula
