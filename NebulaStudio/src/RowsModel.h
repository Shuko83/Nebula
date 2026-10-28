// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QAbstractTableModel>
#include <QList>
#include <QStringList>
#include <algorithm>
#include <functional>

// A table model whose rows are those of a list sorted by a key, Key of each row, as PluginModel, ComponentModel and
// GalaxyModel list what they read. Given the list read again, setRows() removes the rows gone, inserts the new ones
// where they belong and updates the others, as views expect it. Its columns are those headers() names.
template <typename Row, auto Key> class RowsModel : public QAbstractTableModel
{
  public:
    using QAbstractTableModel::QAbstractTableModel;

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override
    {
        return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
    }
    [[nodiscard]] int columnCount(const QModelIndex& parent = {}) const override
    {
        return parent.isValid() ? 0 : static_cast<int>(headers().size());
    }
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                      int role = Qt::DisplayRole) const override
    {
        const QStringList titles = headers();
        if(orientation != Qt::Horizontal || role != Qt::DisplayRole || section < 0 || section >= titles.size())
        {
            return {};
        }
        return titles.at(section);
    }

  protected:
    // The titles of the columns, in their order.
    [[nodiscard]] virtual QStringList headers() const = 0;

    [[nodiscard]] const QList<Row>& rows() const
    {
        return m_rows;
    }
    // The row of index, valid and of this model.
    [[nodiscard]] const Row& rowAt(const QModelIndex& index) const
    {
        return m_rows.at(index.row());
    }
    // Whether index is one of its cells.
    [[nodiscard]] bool isCell(const QModelIndex& index) const
    {
        return checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid);
    }
    // The row whose key is key; nullptr when there is none.
    [[nodiscard]] const Row* rowOf(const auto& key) const
    {
        const auto found = std::ranges::find(m_rows, key, Key);
        return found != m_rows.end() ? &*found : nullptr;
    }
    // The first column of the row whose key is key; invalid when there is none.
    [[nodiscard]] QModelIndex indexOfKey(const auto& key) const
    {
        const Row* row = rowOf(key);
        return row != nullptr ? index(static_cast<int>(row - m_rows.constData()), 0) : QModelIndex();
    }

    // Lists rows, sorted by their key as the rows listed are, in place of those listed.
    void setRows(const QList<Row>& rows)
    {
        removeGone(rows);
        insertNew(rows);
        if(!m_rows.isEmpty())
        {
            emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
        }
    }

  private:
    // Removes the rows whose key is none of those of rows, from the last one: the rows above it keep their number.
    void removeGone(const QList<Row>& rows)
    {
        for(qsizetype row = m_rows.size() - 1; row >= 0; --row)
        {
            if(std::ranges::find(rows, std::invoke(Key, m_rows.at(row)), Key) == rows.end())
            {
                beginRemoveRows({}, static_cast<int>(row), static_cast<int>(row));
                m_rows.removeAt(row);
                endRemoveRows();
            }
        }
    }
    // Inserts the rows of rows not listed yet where they belong, and updates the others.
    void insertNew(const QList<Row>& rows)
    {
        for(qsizetype row = 0; row < rows.size(); ++row)
        {
            if(row < m_rows.size() && std::invoke(Key, m_rows.at(row)) == std::invoke(Key, rows.at(row)))
            {
                m_rows[row] = rows.at(row);
                continue;
            }
            insert(row, rows.at(row));
        }
    }
    void insert(qsizetype row, const Row& inserted)
    {
        beginInsertRows({}, static_cast<int>(row), static_cast<int>(row));
        m_rows.insert(row, inserted);
        endInsertRows();
    }

    QList<Row> m_rows; // Sorted by key
};
