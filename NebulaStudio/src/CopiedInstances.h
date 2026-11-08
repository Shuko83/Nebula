// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QPointF>
#include <QString>

class QMimeData;

// The instances of an edition zone that are copied, see EditionScene::copy(): a piece of a project file, the "plugins",
// "components" and "bindings" Nebula::Universe::saveInstances() writes, with what the edition zone shows of them, as
// EditionScene::save() writes it: where they are, their color, the edges and the rows of the dots of their interfaces,
// and the anchors of their links. The clipboard holds them as MimeType, which another window of NebulaStudio reads.
namespace CopiedInstances
{

// What the clipboard holds them as.
inline const QString MimeType = QStringLiteral("application/x-nebula-instances");

// Puts instances in the clipboard, in place of what it holds.
void toClipboard(const QJsonObject& instances);
// Whether the clipboard holds instances.
[[nodiscard]] bool isInClipboard();
// The instances the clipboard holds; empty when it holds none.
[[nodiscard]] QJsonObject fromClipboard();
// instances, as mime data that holds them; who takes it owns it.
[[nodiscard]] QMimeData* mimeData(const QJsonObject& instances);
// The instances data holds; empty when it holds none.
[[nodiscard]] QJsonObject read(const QMimeData* data);

// The components of instances that are placed, in their order: all of them but the stargates.
[[nodiscard]] QList<QJsonObject> placed(const QJsonObject& instances);
// The top left corner of instances: the leftmost and topmost of the corners of their components, each at corner from
// where it is, and of the anchors of their links.
[[nodiscard]] QPointF topLeft(const QJsonObject& instances, QPointF corner);
// instances, moved by offset: their components and the anchors of their links.
[[nodiscard]] QJsonObject moved(const QJsonObject& instances, QPointF offset);
// bindings, between components of instances, those components named again as names tells, by the names they had.
[[nodiscard]] QJsonArray renamed(const QJsonArray& bindings, const QHash<QString, QString>& names);

} // namespace CopiedInstances
