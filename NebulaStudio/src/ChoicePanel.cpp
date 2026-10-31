// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ChoicePanel.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>
#include <algorithm>
#include <utility>

#include "PlaceholderStack.h"
#include "PropertyTree.h"
#include "Theme.h"

namespace
{

// Where the rows are checked, and what the filter looks for.
constexpr int ChoiceColumn = 0;

bool isChecked(const QTreeWidgetItem& row)
{
    return row.checkState(ChoiceColumn) == Qt::Checked;
}

// Whether row is a choice: it can be checked, and groups no other row.
bool isChoice(const QTreeWidgetItem& row)
{
    return row.childCount() == 0 && row.flags().testFlag(Qt::ItemIsUserCheckable);
}

// The choices of tree.
QList<const QTreeWidgetItem*> choicesOf(QTreeWidget& tree)
{
    QList<const QTreeWidgetItem*> choices;
    for(QTreeWidgetItemIterator row(&tree); *row != nullptr; ++row)
    {
        if(isChoice(**row))
        {
            choices.append(*row);
        }
    }
    return choices;
}

// A field that tells text while it is empty, and what it does as its tool tip; a button empties it.
QLineEdit* newFilterEdit(const QString& text, const QString& toolTip)
{
    auto* edit = new QLineEdit;
    edit->setPlaceholderText(text);
    edit->setToolTip(toolTip);
    edit->setClearButtonEnabled(true);
    return edit;
}

QCheckBox* newCheckBox(const QString& text, const QString& toolTip)
{
    auto* box = new QCheckBox(text);
    box->setToolTip(toolTip);
    return box;
}

// The title of a part of a dialog.
QLabel* newTitle(const QString& text)
{
    auto* label = new QLabel(text);
    label->setFont(titleFont(label->font()));
    return label;
}

} // namespace

ChoicePanel::ChoicePanel(QTreeWidget* tree, Texts texts, WhyNone whyNone, QWidget* parent)
    : QWidget(parent), m_texts(std::move(texts)), m_whyNone(std::move(whyNone)), m_tree(tree), m_count(new QLabel),
      m_filter(newFilterEdit(m_texts.filter, m_texts.filterToolTip)),
      m_checkedOnly(newCheckBox(m_texts.checkedOnly, m_texts.checkedOnlyToolTip))
{
    m_count->setForegroundRole(QPalette::PlaceholderText); // The muted text of the theme
    layOut();
    connect(m_filter, &QLineEdit::textChanged, this, &ChoicePanel::filter);
    connect(m_checkedOnly, &QCheckBox::toggled, this, &ChoicePanel::filter);
    refreshCount();
    filter();
}

void ChoicePanel::refreshCount()
{
    const QList<const QTreeWidgetItem*> choices = choicesOf(*m_tree);
    const auto checked = std::ranges::count_if(choices, [](const QTreeWidgetItem* row) { return isChecked(*row); });
    m_count->setText(m_texts.count.arg(checked).arg(choices.size()));
}

void ChoicePanel::layOut()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins({}); // Those of the dialog
    layout->addLayout(newTitleRow());
    layout->addLayout(newFilterRow());
    layout->addWidget(newTreeArea(), 1);
}

QHBoxLayout* ChoicePanel::newTitleRow() const
{
    auto* row = new QHBoxLayout;
    row->addWidget(newTitle(m_texts.title));
    row->addStretch();
    row->addWidget(m_count);
    return row;
}

QHBoxLayout* ChoicePanel::newFilterRow() const
{
    auto* row = new QHBoxLayout;
    row->addWidget(m_filter, 1);
    row->addWidget(m_checkedOnly);
    return row;
}

QWidget* ChoicePanel::newTreeArea()
{
    auto* area = new QWidget;
    m_stack = new PlaceholderStack(m_tree, *area);
    m_stack->placeholder()->setForegroundRole(QPalette::PlaceholderText);
    return area;
}

void ChoicePanel::filter()
{
    const bool checkedOnly = m_checkedOnly->isChecked();
    const bool found = filterRows(*m_tree, ChoiceColumn, m_filter->text().trimmed(),
                                  [checkedOnly](const QTreeWidgetItem& row) { return !checkedOnly || isChecked(row); });
    m_stack->placeholder()->setText(whyNoRow());
    m_stack->showPageIf(found);
}

QString ChoicePanel::whyNoRow() const
{
    if(m_tree->topLevelItemCount() == 0)
    {
        return m_whyNone.empty;
    }
    // Only those checked are shown, then.
    return m_filter->text().trimmed().isEmpty() ? m_whyNone.noneChecked : m_whyNone.noneFound;
}
