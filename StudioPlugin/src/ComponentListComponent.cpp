// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ComponentListComponent.h"

#include "ComponentPanel.h"
#include "StudioPlugin.h"

NEBULA_REGISTER_COMPONENT(StudioPlugin, ComponentListComponent);

std::unique_ptr<QWidget> ComponentListComponent::createPart(Studio& studio)
{
    return std::make_unique<ComponentPanel>(studio);
}
