// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ProblemListComponent.h"

#include "EditionZone.h"
#include "ProblemList.h"
#include "Studio.h"
#include "StudioPlugin.h"

NEBULA_REGISTER_COMPONENT(StudioPlugin, ProblemListComponent);

ProblemListComponent::ProblemListComponent()
{
    consumeInterface<EditionZone>(QStringLiteral("zone"), m_zone);
}

std::unique_ptr<QWidget> ProblemListComponent::createPart(Studio& studio)
{
    auto problems = std::make_unique<ProblemList>(studio.application());
    connect(problems.get(), &ProblemList::componentActivated, this, &ProblemListComponent::showComponent);
    return problems;
}

void ProblemListComponent::showComponent(const QString& name)
{
    if(m_zone != nullptr)
    {
        m_zone->showComponent(name);
    }
}
