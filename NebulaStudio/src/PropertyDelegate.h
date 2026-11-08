// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QMetaType>
#include <QStyledItemDelegate>

// The delegate of the values of a property table, see newPropertyTable(). The types the default editors of Qt take are
// edited by them; the others hasEditor() accepts are edited by those of ValueEditors.h, which write a value as soon as
// it is chosen: an enumeration in a list of its values, flags in a list of theirs to check, a URL in a field with a
// button that chooses a local file. A color is edited in QColorDialog, alpha channel included, opened by a
// double-click on it, or by F2 or Space: a single click edits the others. Nothing is edited while the table has no
// edit trigger.
// An enumeration shows the name of its value, flags those of theirs joined by |, a color a swatch of it, edged in the
// color of the edges of the palette, then its name, nothing when it is not valid, and a URL its text.
class PropertyDelegate : public QStyledItemDelegate
{
    Q_OBJECT

  public:
    // Its parent is the table it is the delegate of, which the dialog of a color is opened on.
    using QStyledItemDelegate::QStyledItemDelegate;

    // The name of a color; what QStyledItemDelegate shows otherwise, the name of a value for an enumeration.
    [[nodiscard]] QString displayText(const QVariant& value, const QLocale& locale) const override;
    // The editor of the value of index: one of ValueEditors.h, none for a color, which is edited in a dialog, the one
    // of QStyledItemDelegate otherwise.
    [[nodiscard]] QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                                        const QModelIndex& index) const override;
    // Over the value, as high as the list of flags, which it moves up to keep in the table.
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
                              const QModelIndex& index) const override;

  protected:
    // With the swatch of a color, again as the palette changes.
    void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override;
    // Opens the dialog of a color, see the class.
    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option,
                     const QModelIndex& index) override;
    // Keeps the editor of a URL while the dialog it opened has the focus: it writes what the dialog chooses.
    bool eventFilter(QObject* object, QEvent* event) override;

  private:
    // The editor of a value of type, one of ValueEditors.h; nullptr for another type.
    [[nodiscard]] QWidget* newEditor(QMetaType type, QWidget* parent) const;
    // editor, its value written as soon as it tells it is edited.
    template <typename Editor> Editor* writingAtOnce(Editor* editor) const;
    // Writes the value of the editor that tells it is edited, the sender.
    void commit();
    // Opens the dialog that chooses the color of index, which gives model the one chosen.
    void chooseColor(QAbstractItemModel& model, const QModelIndex& index);
};
