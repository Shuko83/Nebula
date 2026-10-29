// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QAbstractItemView>
#include <QItemSelectionModel>
#include <QObject>
#include <QString>
#include <Qt>

class QAbstractItemModel;
class QAction;
class QLineEdit;
class QPushButton;
class QSplitter;
class QTabWidget;
class QTreeView;
class QWidget;

namespace Nebula
{
class Universe;
}

// The widgets the panels of NebulaStudio are made of: those MainWindow assembles, which the galaxy of NebulaStudio
// assembles too, see StudioPlugin/.

// A list of the rows of model, one selected at a time.
[[nodiscard]] QTreeView* createView(QAbstractItemModel& model);
// A list of the rows of model, which are dragged from it.
[[nodiscard]] QTreeView* createDraggableView(QAbstractItemModel& model);

// Has show, a member of viewer, called with the data of role of each row made current in view.
template <typename Viewer>
void showCurrentIn(const QAbstractItemView& view, int role, Viewer* viewer, void (Viewer::*show)(const QString&))
{
    QObject::connect(view.selectionModel(), &QItemSelectionModel::currentRowChanged, viewer,
                     [role, viewer, show](const QModelIndex& current)
                     { (viewer->*show)(current.data(role).toString()); });
}

// view above viewer.
[[nodiscard]] QSplitter* createSplitter(QWidget* view, QWidget* viewer);
// top above below, without margins; top aligned as alignment tells, across the width when it tells nothing.
[[nodiscard]] QWidget* createColumn(QWidget* top, QWidget* below, Qt::Alignment alignment = {});
// A widget whose children are side by side, without margins around them.
[[nodiscard]] QWidget* createRow();
// stretched, which takes the room left, next to other.
[[nodiscard]] QWidget* createBar(QWidget* stretched, QWidget* other);
// A button telling text, and what it does as its tool tip.
[[nodiscard]] QPushButton* createButton(const QString& text, const QString& toolTip);
// Inserts page in tabs at index, after the others when it is not one of theirs, under its window title, beside its
// window icon when it was given one: a panel names itself.
void insertPage(QTabWidget& tabs, QWidget& page, int index = -1);
// Disables action while application runs, from now on.
void disableWhileRunning(const Nebula::Universe& application, QAction& action);
// Gives field the keys with its text selected, even when it already has them: what is typed replaces the last search.
void focusToReplace(QLineEdit& field);
