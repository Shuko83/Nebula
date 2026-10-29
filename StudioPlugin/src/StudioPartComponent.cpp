// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "StudioPartComponent.h"

#include "Studio.h"

StudioPartComponent::StudioPartComponent()
{
    provideInterface<QWidget>(QStringLiteral("widget")); // Once it has a studio
    consumeInterface<Studio>(
        QStringLiteral("studio"), m_studio, Nebula::InterfaceRequirement::Mandatory, [this] { buildPart(); },
        [this] { destroyPart(); });
}

void StudioPartComponent::withdrawPart() {}

QWidget* StudioPartComponent::part() const
{
    return m_part.get();
}

void StudioPartComponent::buildPart()
{
    m_part = createPart(*m_studio);
    provideInterface<QWidget>(QStringLiteral("widget"), m_part.get());
}

void StudioPartComponent::destroyPart()
{
    // First: what it was provided to gives it back while it still exists.
    provideInterface<QWidget>(QStringLiteral("widget"), nullptr);
    withdrawPart();
    m_part.reset();
}
