// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>

#include "StudioPartComponent.h"

class EditionZone;

// The plugins of the application of the Studio it consumes as "studio", see PluginPanel, which loads and unloads them:
// it provides the list as "widget", in the Plugins tab of NebulaStudio say. Unloading a plugin removes its components
// from the edition zone it consumes as "zone", when it has one, before the application removes them. It needs a
// QApplication.
class PluginListComponent : public StudioPartComponent
{
    Q_OBJECT
    Q_CLASSINFO("description", "The plugins NebulaStudio knows, loaded and unloaded there")
    Q_CLASSINFO("category", "NebulaStudio")

  public:
    PluginListComponent();

  protected:
    [[nodiscard]] std::unique_ptr<QWidget> createPart(Studio& studio) override;

  private:
    // Removes components, those of a plugin about to be unloaded, from the zone, when it has one.
    void removeFromZone(const QList<Nebula::Component*>& components);

    EditionZone* m_zone = nullptr; // Consumed
};
