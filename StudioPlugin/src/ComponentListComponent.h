// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "StudioPartComponent.h"

// The components of the loaded plugins, see ComponentPanel, dragged into the edition zone to be instantiated there,
// searched for by their name, their description or their category: it provides the list as "widget", listed again
// as the Studio it consumes as "studio" tells that the plugins changed. It needs a QApplication.
class ComponentListComponent : public StudioPartComponent
{
    Q_OBJECT
    Q_CLASSINFO("description", "The components of the loaded plugins, which NebulaStudio instantiates")
    Q_CLASSINFO("category", "NebulaStudio")

  public:
    ComponentListComponent() = default;

  protected:
    [[nodiscard]] std::unique_ptr<QWidget> createPart(Studio& studio) override;
};
