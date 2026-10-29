// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QWidget>
#include <memory>

#include "Nebula/Component/Component.h"

class Studio;

// A component made of a part of NebulaStudio, a widget built around the Studio it consumes as "studio", which it
// cannot do without: the part is built once the component has the studio, provided as "widget" for as long as it has
// it, and destroyed as it loses it, once what it was provided to gave it back. A subclass builds its part, and provides
// what else it has of it. Not a component of its own: it is not registered in the plugin.
class StudioPartComponent : public Nebula::Component
{
    Q_OBJECT

  protected:
    StudioPartComponent();

    // The part, built around studio, which the component may follow; provided as "widget" once it is returned.
    [[nodiscard]] virtual std::unique_ptr<QWidget> createPart(Studio& studio) = 0;
    // Withdraws what else the component provides of its part, about to be destroyed; nothing by default.
    virtual void withdrawPart();
    // The part; nullptr while the component has no studio.
    [[nodiscard]] QWidget* part() const;

  private:
    // Builds the part around the studio, just consumed, and provides it.
    void buildPart();
    // Withdraws the part, then destroys it: the studio is about to go.
    void destroyPart();

    Studio* m_studio = nullptr; // Consumed
    std::unique_ptr<QWidget> m_part;
};
