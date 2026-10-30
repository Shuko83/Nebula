// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "PanelWidgets.h"

#include <QAction>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTabWidget>
#include <QTreeView>
#include <QVBoxLayout>

#include "Nebula/Application/Universe.h"

QTreeView* createView(QAbstractItemModel& model)
{
    auto* view = new QTreeView;
    view->setModel(&model);
    view->setRootIsDecorated(false);
    view->setUniformRowHeights(true);
    view->setAllColumnsShowFocus(true);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->header()->setStretchLastSection(false);
    view->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    view->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    return view;
}

QTreeView* createDraggableView(QAbstractItemModel& model)
{
    QTreeView* view = createView(model);
    view->setDragEnabled(true);
    view->setDragDropMode(QAbstractItemView::DragOnly);
    return view;
}

QSplitter* createSplitter(QWidget* view, QWidget* viewer)
{
    auto* splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(view);
    splitter->addWidget(viewer);
    splitter->setStretchFactor(1, 1);
    return splitter;
}

QWidget* createColumn(QWidget* top, QWidget* below, Qt::Alignment alignment)
{
    auto* column = new QWidget;
    auto* layout = new QVBoxLayout(column);
    layout->setContentsMargins({});
    layout->addWidget(top, 0, alignment);
    layout->addWidget(below);
    return column;
}

QWidget* createRow()
{
    auto* row = new QWidget;
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins({});
    return row;
}

QWidget* createBar(QWidget* stretched, QWidget* other)
{
    QWidget* bar = createRow();
    auto* layout = static_cast<QHBoxLayout*>(bar->layout());
    layout->addWidget(stretched, 1);
    layout->addWidget(other);
    return bar;
}

QPushButton* createButton(const QString& text, const QString& toolTip)
{
    auto* button = new QPushButton(text);
    button->setToolTip(toolTip);
    return button;
}

void addPage(QTabWidget& tabs, QWidget& page)
{
    // Without an icon of its own, a widget has the one of its window, or of the application.
    if(page.testAttribute(Qt::WA_SetWindowIcon))
    {
        tabs.addTab(&page, page.windowIcon(), page.windowTitle());
        return;
    }
    tabs.addTab(&page, page.windowTitle());
}

void disableWhileRunning(const Nebula::Universe& application, QAction& action)
{
    action.setEnabled(!application.isRunning());
    QObject::connect(&application, &Nebula::Universe::runningChanged, &action,
                     [disabled = &action](bool running) { disabled->setEnabled(!running); });
}

void focusToReplace(QLineEdit& field)
{
    field.setFocus(Qt::ShortcutFocusReason);
    field.selectAll();
}
