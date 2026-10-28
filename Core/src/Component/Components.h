// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <memory>
#include <span>
#include <vector>

#include "Nebula/Component/Component.h"

namespace Nebula
{

// The components an application or a galaxy owns, in their order.
using ComponentSpan = std::span<const std::unique_ptr<Component>>;

// The components of type T among components, in their order.
template <typename T> [[nodiscard]] QList<T*> componentsOfType(ComponentSpan components)
{
    QList<T*> found;
    for(const std::unique_ptr<Component>& component : components)
    {
        if(auto* typed = qobject_cast<T*>(component.get()))
        {
            found.append(typed);
        }
    }
    return found;
}

// Starts components in their order. At the first that refuses, stops those started before it and returns it; nullptr
// when they all run.
Component* startInOrder(ComponentSpan components);
// Stops components from the last one; one that does not run is left as it is.
void stopInReverse(ComponentSpan components);

} // namespace Nebula
