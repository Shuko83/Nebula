// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "PluginListComponent.h"

#include "EditionZone.h"
#include "PluginPanel.h"
#include "StudioPlugin.h"

NEBULA_REGISTER_COMPONENT(StudioPlugin, PluginListComponent);

PluginListComponent::PluginListComponent()
{
    consumeInterface<EditionZone>(QStringLiteral("zone"), m_zone);
}

std::unique_ptr<QWidget> PluginListComponent::createPart(Studio& studio)
{
    auto panel = std::make_unique<PluginPanel>(studio);
    connect(panel.get(), &PluginPanel::removing, this, &PluginListComponent::removeFromZone);
    return panel;
}

void PluginListComponent::removeFromZone(const QList<Nebula::Component*>& components)
{
    // With what shows them, before the application would remove them itself.
    if(m_zone != nullptr)
    {
        m_zone->remove(components);
    }
}
