// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "EditionZoneComponent.h"

#include "StudioPlugin.h"

NEBULA_REGISTER_COMPONENT(StudioPlugin, EditionZoneComponent);

EditionZoneComponent::EditionZoneComponent()
{
    // Once it has a studio, as its widget.
    provideInterface<EditionZone>(QStringLiteral("zone"));
    provideInterface<QMenu>(QStringLiteral("viewMenu"));
    provideInterface<QMenu>(QStringLiteral("runMenu"));
    provideInterface<QWidget>(QStringLiteral("zoomLabel"));
}

std::unique_ptr<QWidget> EditionZoneComponent::createPart(Studio& studio)
{
    auto zone = std::make_unique<EditionZone>(studio);
    provideMenus(*zone);
    provideInterface<EditionZone>(QStringLiteral("zone"), zone.get());
    return zone;
}

void EditionZoneComponent::provideMenus(EditionZone& zone)
{
    m_viewMenu = std::make_unique<QMenu>(tr("&View"));
    zone.addViewActions(*m_viewMenu);
    m_runMenu = std::make_unique<QMenu>(tr("&Run"));
    zone.addRunActions(*m_runMenu);
    m_zoomLabel.reset(zone.createZoomLabel());
    provideInterface<QMenu>(QStringLiteral("viewMenu"), m_viewMenu.get());
    provideInterface<QMenu>(QStringLiteral("runMenu"), m_runMenu.get());
    provideInterface<QWidget>(QStringLiteral("zoomLabel"), m_zoomLabel.get());
}

void EditionZoneComponent::withdrawPart()
{
    // First: what they were provided to gives them back while they still exist.
    provideInterface<EditionZone>(QStringLiteral("zone"), nullptr);
    provideInterface<QMenu>(QStringLiteral("viewMenu"), nullptr);
    provideInterface<QMenu>(QStringLiteral("runMenu"), nullptr);
    provideInterface<QWidget>(QStringLiteral("zoomLabel"), nullptr);
    m_viewMenu.reset();
    m_runMenu.reset();
    m_zoomLabel.reset();
}
