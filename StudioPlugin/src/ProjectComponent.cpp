// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ProjectComponent.h"

#include <QTimer>

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

QString ProjectComponent::file() const
{
    return m_file;
}

void ProjectComponent::setFile(const QString& file)
{
    if(m_file == file)
    {
        return;
    }
    m_file = file;
    emit fileChanged(m_file);
}

bool ProjectComponent::onStart()
{
    if(!m_file.isEmpty())
    {
        // Once the window shows, which starts after it: its boxes open over it.
        QTimer::singleShot(0, this, [this] { openFile(); });
    }
    return true;
}

void ProjectComponent::openFile()
{
    if(m_project != nullptr && state() == Nebula::ComponentState::Running && m_project->saveChanges())
    {
        m_project->openProjectFile(m_file);
    }
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
