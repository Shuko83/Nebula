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
// Save Galaxy and Description.... Until it loses one of them. Each time it starts, it opens its file, when it has one:
// NebulaStudio gives it the project file of its command line. It needs a QApplication.
class ProjectComponent : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "The project file of the application NebulaStudio assembles, and its File menu")
    Q_CLASSINFO("category", "NebulaStudio")
    Q_PROPERTY(QString file READ file WRITE setFile NOTIFY fileChanged)

  public:
    ProjectComponent();

    // The project file it opens each time it starts, in place of the application, once its changes are saved or given
    // up; none when it is empty.
    [[nodiscard]] QString file() const;
    void setFile(const QString& file);

  signals:
    void fileChanged(const QString& file);

  protected:
    bool onStart() override;

  private:
    // Builds the project once it has both the studio and the zone, and provides it with its File menu.
    void createProject();
    // Withdraws the project and its menu, then destroys them: the studio or the zone is about to go.
    void destroyProject();
    // Opens its file, while it runs and has a project, once the changes of the application are saved or given up.
    void openFile();

    Studio* m_studio = nullptr;    // Consumed
    EditionZone* m_zone = nullptr; // Consumed
    std::unique_ptr<Project> m_project;
    std::unique_ptr<QMenu> m_fileMenu;
    QString m_file;
};
