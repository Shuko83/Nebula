// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QComboBox>
#include <QLineEdit>
#include <QListWidget>
#include <QMetaType>
#include <QPointer>
#include <QVariant>

class QFileDialog;

// The editors of the values of a property table whose types the default editors of Qt do not take, see
// PropertyDelegate. Each edits its value as its USER property, which QStyledItemDelegate gives it and reads back, and
// tells as the user chooses one, edited(), for the delegate to write it at once.

// Chooses the value of an enumeration in a list of their names, in the order the enumeration declares them.
class EnumEditor : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY(QVariant value READ value WRITE setValue USER true)

  public:
    // Edits a value of type, an enumeration, see metaEnumOf().
    explicit EnumEditor(QMetaType type, QWidget* parent = nullptr);

    [[nodiscard]] QVariant value() const;
    // Shows value, one of the type it edits; no value when it is none of its values.
    void setValue(const QVariant& value);

  signals:
    // The user chose a value.
    void edited();

  private:
    QMetaType m_type;
};

// Checks the values of flags in a list of their names, in the order the flags declare them, but for one that is 0: none
// checked is that one. A value made of others is checked when they all are.
class FlagsEditor : public QListWidget
{
    Q_OBJECT
    Q_PROPERTY(QVariant value READ value WRITE setValue USER true)

  public:
    // Edits a value of type, flags, see metaEnumOf().
    explicit FlagsEditor(QMetaType type, QWidget* parent = nullptr);

    [[nodiscard]] QVariant value() const;
    void setValue(const QVariant& value);
    // As high as its rows: it shows them all, over those of the table under its own.
    [[nodiscard]] QSize sizeHint() const override;

  signals:
    // The user checked or unchecked a value.
    void edited();

  private:
    // Adds the value of row to the flags, or removes it, as it is checked, then checks the rows again.
    void toggle(const QListWidgetItem* row);
    // Checks the rows of the values m_value has.
    void showChecks();

    QMetaType m_type;
    int m_value = 0;
};

// A URL in a field, with a button in it that chooses a local file in a dialog instead, which it has at once.
class UrlEditor : public QLineEdit
{
    Q_OBJECT
    Q_PROPERTY(QVariant value READ value WRITE setValue USER true)

  public:
    explicit UrlEditor(QWidget* parent = nullptr);

    // The URL its text is, see urlOf().
    [[nodiscard]] QVariant value() const;
    void setValue(const QVariant& value);
    // Whether its dialog is open: it stays meanwhile, though it no longer has the focus.
    [[nodiscard]] bool isBrowsing() const;

  signals:
    // The user chose a file in its dialog.
    void edited();

  private:
    // Opens the dialog that chooses a local file, on the one it has, if any.
    void browse();
    // Has url, chosen in its dialog, and tells it.
    void choose(const QUrl& url);

    QPointer<QFileDialog> m_dialog;
};
