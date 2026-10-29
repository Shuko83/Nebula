// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "StudioPartComponent.h"

class EditionZone;

// What keeps the components of the application of the Studio it consumes as "studio" from working, see ProblemList:
// it provides the list as "widget". Activating a row shows its component, selected, in the middle of the edition zone
// it consumes as "zone", when it has one. It needs a QApplication.
class ProblemListComponent : public StudioPartComponent
{
    Q_OBJECT
    Q_CLASSINFO("description", "What keeps the components of the application NebulaStudio assembles from working")
    Q_CLASSINFO("category", "NebulaStudio")

  public:
    ProblemListComponent();

  protected:
    [[nodiscard]] std::unique_ptr<QWidget> createPart(Studio& studio) override;

  private:
    // Shows the component named name in the zone, when it has one.
    void showComponent(const QString& name);

    EditionZone* m_zone = nullptr; // Consumed
};
