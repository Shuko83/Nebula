// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QString>
#include <QWidget>

class PlaceholderStack;
class QCheckBox;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QTreeWidget;

// The choices of a dialog, checked in the first column of a tree, as the properties a settings file holds or what a
// stargate exposes: a title, and how many of the rows that can be checked are; a field that filters the rows, and a box
// that shows those checked only; then the tree, or in its place why it shows no row. The filter finds the rows whose
// text in the first column, or the one of a row above them, contains its own, whatever its case, see filterRows(); it
// is applied as its text or the box changes, not as a row is checked: the row would leave from under the pointer.
class ChoicePanel : public QWidget
{
    Q_OBJECT

  public:
    // What it shows.
    struct Texts
    {
        QString title;
        QString count;  // How many rows are checked, %1, among how many can be, %2
        QString filter; // In the field while it is empty
        QString filterToolTip;
        QString checkedOnly; // The box that shows the rows checked only
        QString checkedOnlyToolTip;
    };

    // Why it shows no row.
    struct WhyNone
    {
        QString empty;       // The tree has none
        QString noneChecked; // None is checked, while those checked only are shown
        QString noneFound;   // The filter finds none
    };

    // Shows tree, whose rows are listed already.
    ChoicePanel(QTreeWidget* tree, Texts texts, WhyNone whyNone, QWidget* parent = nullptr);

    // Tells how many rows are checked; once one is checked or unchecked.
    void refreshCount();

  private:
    void layOut();
    // The title, and how many rows are checked.
    [[nodiscard]] QHBoxLayout* newTitleRow() const;
    // The filter, and the box that shows the rows checked only.
    [[nodiscard]] QHBoxLayout* newFilterRow() const;
    // Where the tree is shown, or why it shows no row.
    [[nodiscard]] QWidget* newTreeArea();
    // Shows the rows the filter finds, those checked only when asked to, or why it shows none.
    void filter();
    [[nodiscard]] QString whyNoRow() const;

    Texts m_texts;
    WhyNone m_whyNone;
    QTreeWidget* m_tree = nullptr;
    QLabel* m_count = nullptr; // How many rows are checked
    QLineEdit* m_filter = nullptr;
    QCheckBox* m_checkedOnly = nullptr;
    PlaceholderStack* m_stack = nullptr; // m_tree, or why it shows no row
};
