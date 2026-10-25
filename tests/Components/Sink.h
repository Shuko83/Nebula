// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Nebula/Component/Component.h"

class QTimer;

// Consumes the interface "object", a QObject it needs, "timer", a QTimer it can do without, and "objects", a list of
// QObjects: declared as it is built, as a project needs them to bind them.
class Sink : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Consumes an object, a timer and a list of objects")
    Q_CLASSINFO("category", "Test")

  public:
    Sink();

    // What it has of "object"; nullptr while it is unavailable.
    [[nodiscard]] QObject* object() const;
    // What it has of "objects", in the order they were bound.
    [[nodiscard]] QList<QObject*> objects() const;

  private:
    QObject* m_object = nullptr;
    QTimer* m_timer = nullptr;
    QList<QObject*> m_objects;
};
