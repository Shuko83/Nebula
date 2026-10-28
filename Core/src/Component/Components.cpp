// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Components.h"

#include <algorithm>
#include <ranges>

namespace Nebula
{

bool isAmong(const Component* component, ComponentSpan components)
{
    return std::ranges::any_of(components, [component](const std::unique_ptr<Component>& known)
                               { return known.get() == component; });
}

Component* startInOrder(ComponentSpan components)
{
    for(auto component = components.begin(); component != components.end(); ++component)
    {
        if(!(*component)->start())
        {
            stopInReverse({components.begin(), component});
            return component->get();
        }
    }
    return nullptr;
}

void stopInReverse(ComponentSpan components)
{
    for(const std::unique_ptr<Component>& component : components | std::views::reverse)
    {
        component->stop(); // Does nothing to one that does not run
    }
}

} // namespace Nebula
