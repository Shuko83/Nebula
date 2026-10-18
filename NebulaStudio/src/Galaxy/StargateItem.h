// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>

#include "ComponentItem.h"

class DashedLinkItem;

namespace Nebula
{
class Stargate;
}

// A stargate in the edition zone, see Nebula::Stargate: a component, the icon of the Stargate before "Stargate", whose
// interfaces are those it exposes, the other way round: on its left edge those its galaxy provides, which it consumes
// from components of the galaxy, on its right edge those its galaxy consumes, which it provides to them. Then a row per
// property it exposes, the name it exposes it as and where it is, and dashed lines join it to the components of those
// properties. Its tooltip tells what its galaxy has through it. The names of its interfaces are edited in place by
// double-clicking them, as its name is; double-clicking it elsewhere asks for what it exposes to be chosen, see
// openRequested().
class StargateItem : public ComponentItem
{
    Q_OBJECT

  public:
    // stargate, owned by the application, outlives the item.
    explicit StargateItem(Nebula::Stargate& stargate);
    // With its links.
    ~StargateItem() override;

    [[nodiscard]] Nebula::Stargate& stargate() const;
    // Joins it to the components of its scene whose properties its stargate exposes now: those shown after it are only
    // joined to it then.
    void refreshLinks();

  protected:
    // Joined to the components of the scene it is added to.
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

  private:
    // Shows the properties its stargate exposes, and tells what its galaxy has through it.
    void refresh();
    // Draws its links again, where it and their components are now.
    void followLinks();

    Nebula::Stargate& m_stargate;
    QList<DashedLinkItem*> m_links; // In its scene rather than its children: under the components
};
