// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QMetaEnum>
#include <QMetaType>
#include <QString>
#include <QUrl>
#include <QVariant>

class QColor;

// The values of the properties NebulaStudio edits whose types the default editors of Qt do not take, see
// PropertyDelegate: enumerations, flags, colors and URLs.

// The enumeration, or flags, type is: the Q_ENUM, or Q_FLAG, of the class it is declared in; not valid for another
// type, or one the class does not declare.
[[nodiscard]] QMetaEnum metaEnumOf(QMetaType type);
// value as one of type, an enumeration or flags; not valid when it cannot be one.
[[nodiscard]] QVariant enumValue(QMetaType type, int value);
// The name of color, as a project holds it: #rrggbb, or #aarrggbb when it is not opaque; empty when it is not valid.
[[nodiscard]] QString colorName(const QColor& color);
// The URL text is: the one of a local file when it is an absolute path, which QUrl would read the drive of as a scheme;
// text read as a URL otherwise.
[[nodiscard]] QUrl urlOf(const QString& text);
