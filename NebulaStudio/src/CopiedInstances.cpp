// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "CopiedInstances.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonValue>
#include <QMimeData>
#include <algorithm>
#include <iterator>
#include <optional>

#include "PositionJson.h"

namespace
{

// What a project names a stargate's type, see Nebula::Stargate::Id: no stargate is placed.
const QString StargateType = QStringLiteral("Nebula.Stargate");

// json, an item as a project holds it, moved by offset.
QJsonObject movedItem(QJsonObject json, QPointF offset)
{
    writePosition(json, readPosition(json) + offset);
    return json;
}

// Each item of items, as a project holds them, moved by offset.
QJsonArray movedItems(const QJsonArray& items, QPointF offset)
{
    QJsonArray moved;
    for(const QJsonValue& value : items)
    {
        moved.append(movedItem(value.toObject(), offset));
    }
    return moved;
}

// binding, as a project holds it, the anchors of its link moved by offset.
QJsonObject movedBinding(QJsonObject binding, QPointF offset)
{
    if(binding.contains(u"anchors"))
    {
        binding.insert(u"anchors", movedItems(binding.value(u"anchors").toArray(), offset));
    }
    return binding;
}

// The anchors of the links of instances.
QList<QPointF> anchorsOf(const QJsonObject& instances)
{
    QList<QPointF> anchors;
    const QJsonArray bindings = instances.value(u"bindings").toArray();
    for(const QJsonValue& binding : bindings)
    {
        const QJsonArray points = binding.toObject().value(u"anchors").toArray();
        std::ranges::transform(points, std::back_inserter(anchors),
                               [](const QJsonValue& point) { return readPosition(point.toObject()); });
    }
    return anchors;
}

// The points of instances: a corner from where each of their components is, and the anchors of their links.
QList<QPointF> pointsOf(const QJsonObject& instances, QPointF corner)
{
    QList<QPointF> points = anchorsOf(instances);
    for(const QJsonObject& component : CopiedInstances::placed(instances))
    {
        points.append(readPosition(component) + corner);
    }
    return points;
}

// The binding json, as a project holds it, between components named again as names tells; nullopt when one of them is
// not among those names.
std::optional<QJsonObject> renamedBinding(QJsonObject json, const QHash<QString, QString>& names)
{
    for(const QStringView key : {u"consumer", u"provider"})
    {
        const auto name = names.constFind(json.value(key).toString());
        if(name == names.constEnd())
        {
            return std::nullopt;
        }
        json.insert(key, *name);
    }
    return json;
}

} // namespace

namespace CopiedInstances
{

void toClipboard(const QJsonObject& instances)
{
    QGuiApplication::clipboard()->setMimeData(mimeData(instances));
}

bool isInClipboard()
{
    const QMimeData* data = QGuiApplication::clipboard()->mimeData();
    return data != nullptr && data->hasFormat(MimeType);
}

QJsonObject fromClipboard()
{
    return read(QGuiApplication::clipboard()->mimeData());
}

QMimeData* mimeData(const QJsonObject& instances)
{
    auto* data = new QMimeData;
    data->setData(MimeType, QJsonDocument(instances).toJson(QJsonDocument::Compact));
    return data;
}

QJsonObject read(const QMimeData* data)
{
    return data != nullptr ? QJsonDocument::fromJson(data->data(MimeType)).object() : QJsonObject();
}

QList<QJsonObject> placed(const QJsonObject& instances)
{
    QList<QJsonObject> components;
    const QJsonArray all = instances.value(u"components").toArray();
    for(const QJsonValue& value : all)
    {
        if(value.toObject().value(u"type").toString() != StargateType)
        {
            components.append(value.toObject());
        }
    }
    return components;
}

QPointF topLeft(const QJsonObject& instances, QPointF corner)
{
    const QList<QPointF> points = pointsOf(instances, corner);
    if(points.isEmpty())
    {
        return {};
    }
    return {std::ranges::min(points, {}, &QPointF::x).x(), std::ranges::min(points, {}, &QPointF::y).y()};
}

QJsonObject moved(const QJsonObject& instances, QPointF offset)
{
    QJsonObject json = instances;
    json.insert(u"components", movedItems(instances.value(u"components").toArray(), offset));
    QJsonArray bindings;
    for(const QJsonValue& binding : instances.value(u"bindings").toArray())
    {
        bindings.append(movedBinding(binding.toObject(), offset));
    }
    json.insert(u"bindings", bindings);
    return json;
}

QJsonArray renamed(const QJsonArray& bindings, const QHash<QString, QString>& names)
{
    QJsonArray renamedBindings;
    for(const QJsonValue& value : bindings)
    {
        if(const std::optional<QJsonObject> binding = renamedBinding(value.toObject(), names))
        {
            renamedBindings.append(*binding);
        }
    }
    return renamedBindings;
}

} // namespace CopiedInstances
