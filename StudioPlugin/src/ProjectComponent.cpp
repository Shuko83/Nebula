// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ProjectComponent.h"

#include "EditionZone.h"
#include "Studio.h"
#include "StudioPlugin.h"

NEBULA_REGISTER_COMPONENT(StudioPlugin, ProjectComponent);

ProjectComponent::ProjectComponent()
{
    // Once it has both.
    provideInterface<Project>(QStringLiteral("project"));
    provideInterface<QMenu>(QStringLiteral("fileMenu"));
    consumeInterface<Studio>(
        QStringLiteral("studio"), m_studio, Nebula::InterfaceRequirement::Mandatory, [this] { createProject(); },
        [this] { destroyProject(); });
    consumeInterface<EditionZone>(
        QStringLiteral("zone"), m_zone, Nebula::InterfaceRequirement::Mandatory, [this] { createProject(); },
        [this] { destroyProject(); });
}

void ProjectComponent::createProject()
{
    if(m_studio == nullptr || m_zone == nullptr)
    {
        return;
    }
    m_project = std::make_unique<Project>(*m_studio, *m_zone);
    m_fileMenu = std::make_unique<QMenu>(tr("&File"));
    m_project->addFileActions(*m_fileMenu);
    provideInterface<Project>(QStringLiteral("project"), m_project.get());
    provideInterface<QMenu>(QStringLiteral("fileMenu"), m_fileMenu.get());
}

void ProjectComponent::destroyProject()
{
    // First: what they were provided to gives them back while they still exist.
    provideInterface<Project>(QStringLiteral("project"), nullptr);
    provideInterface<QMenu>(QStringLiteral("fileMenu"), nullptr);
    m_fileMenu.reset();
    m_project.reset();
}
