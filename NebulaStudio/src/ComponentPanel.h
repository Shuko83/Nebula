// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QWidget>

class ComponentModel;
class ComponentViewer;
class QComboBox;
class QLineEdit;
class QSortFilterProxyModel;
class Studio;

// The components of the loaded plugins, dragged into the edition zone to be instantiated there, above the viewer of
// the one selected: what it is and its properties. Those listed are those whose name, description or category, as
// chosen next to the search bar above them, holds what is typed there, whatever its case; Ctrl+F gives the search bar
// the keys. It is listed again when a part tells its studio that the plugins changed. Named Components.
class ComponentPanel : public QWidget
{
    Q_OBJECT

  public:
    explicit ComponentPanel(Studio& studio, QWidget* parent = nullptr);

  private:
    // The components whose name, description or category, as chosen next to the search bar, holds what is typed there,
    // whatever its case: through a filter, which hands what is dragged over to the model, which makes the data.
    QWidget* createList();
    // What filter, of the components, looks for, and where.
    QWidget* createSearchBar(QSortFilterProxyModel& filter);
    // Where filter, of the components, looks for the text, among SearchFields.
    static QComboBox* createSearchFields(QSortFilterProxyModel& filter);
    // Ctrl+F selects what search, the search bar, holds, for what is typed to replace it.
    void addFindShortcut(QLineEdit& search);

    ComponentModel* m_model = nullptr;
    ComponentViewer* m_viewer = nullptr;
};
