// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QString>
#include <QStringList>

namespace Nebula
{

// What a project file tells of the galaxy it holds, the components of a scene, read without loading it: see
// Universe::readGalaxy().
struct GalaxyInfo
{
    QString name;        // The name of its file without its extension: Clock for Clock.nbp
    QString description; // Empty when it has none
    QStringList plugins; // The ids of the plugins of its components, in the order of its file
    QString filePath;
};

} // namespace Nebula
