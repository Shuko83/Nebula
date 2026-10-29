// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QPointF>
#include <map>
#include <memory>
#include <vector>

#include "LinkItem.h"

class QJsonArray;
class QJsonObject;

namespace Nebula
{
class Component;
} // namespace Nebula

// The anchors of the links of a scene, by what each link binds, see LinkItem::Ends: they outlive the links drawn,
// which are drawn again as the interfaces of their components are shown again. A project holds them in its bindings,
// in the "anchors" of each, the points its link goes through, in their order.
class LinkAnchors
{
  public:
    // Those of a link that binds ends, in their order; none when it has none.
    [[nodiscard]] QList<QPointF> of(const LinkItem::Ends& ends) const;
    // Keeps those of link, as they are now.
    void keep(const LinkItem& link);
    // Keeps those of links, as they are now, and forgets any other: what they were of is no longer bound.
    void keepOnly(const QList<LinkItem*>& links);
    void clear();

    // Reads those bindings tell, what a project holds of the links between components, its components.
    void read(const std::vector<std::unique_ptr<Nebula::Component>>& components, const QJsonArray& bindings);
    // bindings, those a project holds, each with the anchors of its link when it has some.
    [[nodiscard]] QJsonArray written(QJsonArray bindings) const;

  private:
    // Those of the link binding tells, as a project holds it, by the names its components and interfaces have now;
    // none when it has none.
    [[nodiscard]] QList<QPointF> named(const QJsonObject& binding) const;

    std::map<LinkItem::Ends, QList<QPointF>> m_anchors; // Those of each link that has some
};
