// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QString>

#include "StudioPartComponent.h"

class Project;

// The galaxies of its directory, see GalaxyPanel, each telling whether the application of the Studio it consumes as
// "studio" knows its plugins: it provides the list as "widget", in the Galaxies tab of NebulaStudio say. A galaxy is
// opened in place of the application by the project it consumes as "project", when it has one. Its directory is
// scene/ next to the application that runs it while it is empty; a relative one is read from that application's
// directory. The one chosen in the list while it runs is its directory until it stops. It needs a QApplication.
class GalaxyListComponent : public StudioPartComponent
{
    Q_OBJECT
    Q_CLASSINFO("description", "The galaxies of a directory, which NebulaStudio places and opens")
    Q_CLASSINFO("category", "NebulaStudio")
    Q_PROPERTY(QString directory READ directory WRITE setDirectory NOTIFY directoryChanged)

  public:
    GalaxyListComponent();

    [[nodiscard]] QString directory() const;
    void setDirectory(const QString& directory);

  signals:
    void directoryChanged(const QString& directory);

  protected:
    [[nodiscard]] std::unique_ptr<QWidget> createPart(Studio& studio) override;

  private:
    // Opens the galaxy of the file filePath through the project, when it has one.
    void open(const QString& filePath);
    // directory was chosen in the list.
    void choose(const QString& directory);

    Project* m_project = nullptr; // Consumed
    QString m_directory;
};
