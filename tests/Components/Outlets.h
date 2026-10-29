// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QString>
#include <memory>
#include <vector>

#include "Nebula/Component/Component.h"

class Options;

// A component whose interfaces and objects depend on one of its properties, count: it provides as many objects as it
// tells, as object1, object2…, each an Options named after its interface, whose properties it adds under that name.
// Setting count adds the next ones, or removes the last ones with what is bound to them.
class Outlets : public Nebula::Component
{
    Q_OBJECT
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)
    // The number of one of them, 0 for none: never more than count, and set after it, as a project sets the properties
    // in the order they are declared, though JSON holds them by name, active before count.
    Q_PROPERTY(int active READ active WRITE setActive NOTIFY activeChanged)

  public:
    Outlets();
    ~Outlets() override; // Where Options is known

    [[nodiscard]] int count() const;
    // None below 0.
    void setCount(int count);
    [[nodiscard]] int active() const;
    // Between 0 and count.
    void setActive(int active);
    // What it provides, in their order.
    [[nodiscard]] QList<Options*> objects() const;

  signals:
    void countChanged();
    void activeChanged();

  private:
    // Adds or removes the last ones until it has count of them.
    void resize(int count);
    // Provides one more, after the others.
    void addOutlet();
    // Removes the last one, then destroys it, which takes its properties away.
    void removeOutlet();

    std::vector<std::unique_ptr<Options>> m_objects;
    int m_active = 0;
};
