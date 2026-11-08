// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QMetaType>
#include <QString>
#include <cstdint>

class QTableWidget;

// The tables of values NebulaStudio edits, those of PropertyEditor and of SettingsValueEditor: a row per property, its
// name then its value, and rows that title the groups of those under them.

enum PropertyTableColumn : std::uint8_t
{
    PropertyNameColumn,
    PropertyValueColumn,
    PropertyColumnCount,
};

// A table of properties: their names, as wide as they are, then their values, in the room left, shown and edited by a
// PropertyDelegate.
[[nodiscard]] QTableWidget* newPropertyTable();
// Whether a cell edits a value of type: one the default editors of Qt take, an enumeration, flags, a color or a URL,
// see PropertyDelegate.
[[nodiscard]] bool hasEditor(QMetaType type);
// Shows title in row of table, over the whole of it, in the font of the titles, on the alternate color of the fields
// of the palette of table: again when it changes, see colorTitles().
void showTitle(QTableWidget& table, int row, const QString& title);
// Gives the titles of table, the rows showTitle() shows, the alternate color of the fields of its palette again: the
// palette has no role for them.
void colorTitles(QTableWidget& table);
