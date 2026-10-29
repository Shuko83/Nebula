// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QJsonObject>
#include <QPointF>

// Where an item is in the edition zone, as the "x" and "y" of what a project file holds about it: see EditionScene.

inline void writePosition(QJsonObject& json, const QPointF& position)
{
    json.insert(u"x", position.x());
    json.insert(u"y", position.y());
}

// The origin when json does not tell.
[[nodiscard]] inline QPointF readPosition(const QJsonObject& json)
{
    return {json.value(u"x").toDouble(), json.value(u"y").toDouble()};
}
