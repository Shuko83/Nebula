// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QMenu>
#include <memory>

#include "Nebula/Component/Component.h"
#include "Project.h"

class EditionZone;
class Studio;

// The project file of the application NebulaStudio assembles, see Project, as the edition zone it consumes as "zone"
// shows the application of the Studio it consumes as "studio": it cannot do without either. Once it has both, it
// provides the project as "project", for a StudioWindowComponent to ask whether to save it before it closes, or a
// GalaxyListComponent to open a galaxy in its place, and its File menu as "fileMenu": New, Open..., Save, Save As...,
// Save Galaxy and Description.... Until it loses one of them. It needs a QApplication.
class ProjectComponent : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "The project file of the application NebulaStudio assembles, and its File menu")
    Q_CLASSINFO("category", "NebulaStudio")

  public:
    ProjectComponent();

  private:
    // Builds the project once it has both the studio and the zone, and provides it with its File menu.
    void createProject();
    // Withdraws the project and its menu, then destroys them: the studio or the zone is about to go.
    void destroyProject();

    Studio* m_studio = nullptr;    // Consumed
    EditionZone* m_zone = nullptr; // Consumed
    std::unique_ptr<Project> m_project;
    std::unique_ptr<QMenu> m_fileMenu;
};
