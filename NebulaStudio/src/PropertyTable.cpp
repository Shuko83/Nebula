// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "PropertyTable.h"

#include <QCoreApplication>
#include <QHeaderView>
#include <QSignalBlocker>
#include <QTableWidget>
#include <algorithm>
#include <array>

#include "PropertyDelegate.h"
#include "PropertyValues.h"
#include "Theme.h"

namespace
{

// The types the default editors of Qt take, then those PropertyDelegate edits, but the enumerations and flags.
constexpr std::array EditableTypes{QMetaType::Bool,    QMetaType::Int,   QMetaType::UInt,  QMetaType::Double,
                                   QMetaType::QString, QMetaType::QDate, QMetaType::QTime, QMetaType::QDateTime,
                                   QMetaType::QColor,  QMetaType::QUrl};

// Whether row of table is a title, see showTitle().
bool isTitle(const QTableWidget& table, int row)
{
    return table.columnSpan(row, PropertyNameColumn) == PropertyColumnCount;
}

} // namespace

QTableWidget* newPropertyTable()
{
    auto* table = new QTableWidget(0, PropertyColumnCount);
    table->setHorizontalHeaderLabels({QCoreApplication::translate("PropertyTable", "Property"),
                                      QCoreApplication::translate("PropertyTable", "Value")});
    table->verticalHeader()->hide();
    table->horizontalHeader()->setSectionResizeMode(PropertyNameColumn, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setItemDelegateForColumn(PropertyValueColumn, new PropertyDelegate(table));
    return table;
}

bool hasEditor(QMetaType type)
{
    return std::ranges::find(EditableTypes, static_cast<QMetaType::Type>(type.id())) != EditableTypes.end() ||
           metaEnumOf(type).isValid();
}

void showTitle(QTableWidget& table, int row, const QString& title)
{
    auto* item = new QTableWidgetItem(title);
    item->setFlags(Qt::ItemIsEnabled);
    item->setFont(titleFont(item->font()));
    item->setBackground(table.palette().alternateBase()); // Again when the palette changes
    table.setItem(row, PropertyNameColumn, item);
    table.setSpan(row, PropertyNameColumn, 1, PropertyColumnCount);
}

void colorTitles(QTableWidget& table)
{
    const QSignalBlocker blocker(table); // itemChanged is for the values edited in the table
    for(int row = 0; row < table.rowCount(); ++row)
    {
        if(isTitle(table, row))
        {
            table.item(row, PropertyNameColumn)->setBackground(table.palette().alternateBase());
        }
    }
}
