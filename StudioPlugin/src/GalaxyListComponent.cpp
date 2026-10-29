// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "GalaxyListComponent.h"

#include "Galaxy/GalaxyPanel.h"
#include "Project.h"
#include "StudioPlugin.h"

NEBULA_REGISTER_COMPONENT(StudioPlugin, GalaxyListComponent);

GalaxyListComponent::GalaxyListComponent()
{
    consumeInterface<Project>(QStringLiteral("project"), m_project);
}

QString GalaxyListComponent::directory() const
{
    return m_directory;
}

void GalaxyListComponent::setDirectory(const QString& directory)
{
    if(m_directory == directory)
    {
        return;
    }
    m_directory = directory;
    if(auto* panel = qobject_cast<GalaxyPanel*>(part()))
    {
        panel->setDirectory(directory);
    }
    emit directoryChanged(directory);
}

std::unique_ptr<QWidget> GalaxyListComponent::createPart(Studio& studio)
{
    auto panel = std::make_unique<GalaxyPanel>(studio);
    panel->setDirectory(m_directory);
    connect(panel.get(), &GalaxyPanel::openRequested, this, &GalaxyListComponent::open);
    connect(panel.get(), &GalaxyPanel::directoryChosen, this, &GalaxyListComponent::choose);
    return panel;
}

void GalaxyListComponent::open(const QString& filePath)
{
    if(m_project != nullptr)
    {
        m_project->openGalaxy(filePath);
    }
}

void GalaxyListComponent::choose(const QString& directory)
{
    m_directory = directory;
    emit directoryChanged(directory);
}
