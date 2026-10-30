// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ComponentPanel.h"

#include <QComboBox>
#include <QLineEdit>
#include <QShortcut>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QTreeView>
#include <QVBoxLayout>
#include <array>
#include <cstddef>

#include "ComponentModel.h"
#include "ComponentViewer.h"
#include "Nebula/Component/ComponentFactory.h"
#include "PanelWidgets.h"
#include "Studio.h"

namespace
{

// What the components can be searched by: a column of ComponentModel, and the role of its data the text is looked
// for in. The description of a component is its tool tip, in every column.
struct SearchField
{
    const char* name; // Translated in the context of ComponentPanel
    int column;
    int role;
};

constexpr std::array SearchFields{
    SearchField{QT_TRANSLATE_NOOP("ComponentPanel", "Name"), ComponentModel::Name, Qt::DisplayRole},
    SearchField{QT_TRANSLATE_NOOP("ComponentPanel", "Description"), ComponentModel::Name, Qt::ToolTipRole},
    SearchField{QT_TRANSLATE_NOOP("ComponentPanel", "Category"), ComponentModel::Category, Qt::DisplayRole},
};

// Has filter look for the text it is given where the one of SearchFields at index tells.
void searchBy(QSortFilterProxyModel& filter, int index)
{
    const SearchField& field = SearchFields.at(static_cast<std::size_t>(index));
    filter.setFilterKeyColumn(field.column);
    filter.setFilterRole(field.role);
}

} // namespace

ComponentPanel::ComponentPanel(Studio& studio, QWidget* parent)
    : QWidget(parent), m_model(new ComponentModel(Nebula::ComponentFactory::instance(), this)),
      m_viewer(new ComponentViewer(Nebula::ComponentFactory::instance()))
{
    setWindowTitle(tr("Components"));
    m_model->refresh();
    connect(&studio, &Studio::pluginsChanged, m_model, &ComponentModel::refresh);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->addWidget(createSplitter(createList(), m_viewer));
}

QWidget* ComponentPanel::createList()
{
    auto* filter = new QSortFilterProxyModel(this);
    filter->setSourceModel(m_model);
    filter->setFilterCaseSensitivity(Qt::CaseInsensitive);
    QTreeView* view = createDraggableView(*filter);
    // Also when the search hides the selected component: the view selects one of those left, none when none is.
    showCurrentIn(*view, ComponentModel::IdRole, m_viewer, &ComponentViewer::setComponentId);
    return createColumn(createSearchBar(*filter), view);
}

QWidget* ComponentPanel::createSearchBar(QSortFilterProxyModel& filter)
{
    auto* search = new QLineEdit;
    search->setPlaceholderText(tr("Search components"));
    search->setClearButtonEnabled(true);
    connect(search, &QLineEdit::textChanged, &filter, &QSortFilterProxyModel::setFilterFixedString);
    addFindShortcut(*search);
    return createBar(search, createSearchFields(filter));
}

QComboBox* ComponentPanel::createSearchFields(QSortFilterProxyModel& filter)
{
    auto* fields = new QComboBox;
    for(const SearchField& field : SearchFields)
    {
        fields->addItem(tr(field.name));
    }
    fields->setToolTip(tr("Where the text is looked for"));
    searchBy(filter, fields->currentIndex());
    connect(fields, &QComboBox::currentIndexChanged, &filter,
            [searched = &filter](int index) { searchBy(*searched, index); });
    return fields;
}

void ComponentPanel::addFindShortcut(QLineEdit& search)
{
    auto* shortcut = new QShortcut(QKeySequence::Find, this);
    connect(shortcut, &QShortcut::activated, &search, [field = &search] { focusToReplace(*field); });
}
