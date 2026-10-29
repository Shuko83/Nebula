// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "LinkAnchors.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <algorithm>

#include "Nebula/Component/Component.h"
#include "PositionJson.h"

namespace
{

// The one of components named name; nullptr when none is.
const Nebula::Component* componentNamed(const std::vector<std::unique_ptr<Nebula::Component>>& components,
                                        const QString& name)
{
    const auto found = std::ranges::find(components, name, [](const std::unique_ptr<Nebula::Component>& component)
                                         { return component->objectName(); });
    return found != components.end() ? found->get() : nullptr;
}

// What binding, as a project holds it, binds among components.
LinkItem::Ends endsOf(const std::vector<std::unique_ptr<Nebula::Component>>& components, const QJsonObject& binding)
{
    return {.consumer = componentNamed(components, binding.value(u"consumer").toString()),
            .consumed = binding.value(u"consumed").toString(),
            .provider = componentNamed(components, binding.value(u"provider").toString()),
            .provided = binding.value(u"provided").toString()};
}

// Whether binding, as a project holds it, is what ends binds, by the names they have now.
bool binds(const QJsonObject& binding, const LinkItem::Ends& ends)
{
    return ends.consumer != nullptr && ends.provider != nullptr &&
           binding.value(u"consumer").toString() == ends.consumer->objectName() &&
           binding.value(u"consumed").toString() == ends.consumed &&
           binding.value(u"provider").toString() == ends.provider->objectName() &&
           binding.value(u"provided").toString() == ends.provided;
}

QList<QPointF> readPoints(const QJsonArray& json)
{
    QList<QPointF> points;
    for(const auto& value : json)
    {
        points.append(readPosition(value.toObject()));
    }
    return points;
}

QJsonArray pointsJson(const QList<QPointF>& points)
{
    QJsonArray json;
    for(const QPointF& point : points)
    {
        QJsonObject position;
        writePosition(position, point);
        json.append(position);
    }
    return json;
}

// binding, as a project holds it, with anchors, the points its link goes through, when there are some.
QJsonObject withAnchors(QJsonObject binding, const QList<QPointF>& anchors)
{
    if(!anchors.isEmpty())
    {
        binding.insert(u"anchors", pointsJson(anchors));
    }
    return binding;
}

} // namespace

QList<QPointF> LinkAnchors::of(const LinkItem::Ends& ends) const
{
    const auto found = m_anchors.find(ends);
    return found != m_anchors.end() ? found->second : QList<QPointF>();
}

void LinkAnchors::keep(const LinkItem& link)
{
    const QList<QPointF> anchors = link.anchors();
    if(anchors.isEmpty())
    {
        m_anchors.erase(link.ends());
        return;
    }
    m_anchors.insert_or_assign(link.ends(), anchors);
}

void LinkAnchors::keepOnly(const QList<LinkItem*>& links)
{
    clear();
    for(const LinkItem* link : links)
    {
        keep(*link);
    }
}

void LinkAnchors::clear()
{
    m_anchors.clear();
}

void LinkAnchors::read(const std::vector<std::unique_ptr<Nebula::Component>>& components, const QJsonArray& bindings)
{
    for(const auto& value : bindings)
    {
        const QJsonObject binding = value.toObject();
        const QList<QPointF> anchors = readPoints(binding.value(u"anchors").toArray());
        if(!anchors.isEmpty())
        {
            m_anchors.insert_or_assign(endsOf(components, binding), anchors);
        }
    }
}

QJsonArray LinkAnchors::written(QJsonArray bindings) const
{
    for(QJsonValueRef value : bindings)
    {
        const QJsonObject binding = value.toObject();
        value = withAnchors(binding, named(binding));
    }
    return bindings;
}

QList<QPointF> LinkAnchors::named(const QJsonObject& binding) const
{
    const auto found =
        std::ranges::find_if(m_anchors, [&binding](const auto& anchored) { return binds(binding, anchored.first); });
    return found != m_anchors.end() ? found->second : QList<QPointF>();
}
