// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QLineEdit>
#include <QList>
#include <QString>

class ComponentItem;
class QGraphicsView;
class QStandardItemModel;

// A field that finds the instances the edition zone shows, by their name or by the name of their component, whatever
// its case: whether a component is instantiated there, and where. As it is typed, it offers those whose name, or the
// name of whose component, holds what is typed, with the name of their component, in the order of their names.
// Choosing one, or pressing Enter, selects it alone and shows it in the middle of the view: the one named what is
// typed, when one is; otherwise the one after the instance selected among those found, the first one when none is, or
// after the last one, Enter going through them in turn. The field keeps the keys meanwhile. What it found, or that it
// found nothing, is told through told().
class ComponentFinder : public QLineEdit
{
    Q_OBJECT

  public:
    // view outlives the field: it finds the instances of the scene view shows when it looks for them, the one of the
    // application or of a galaxy.
    explicit ComponentFinder(QGraphicsView& view, QWidget* parent = nullptr);

    // Selects and shows the next instance found for text, as Enter does, see ComponentFinder, and returns what shows
    // it; nullptr, the selection unchanged, when none is found. Nothing when text is empty.
    ComponentItem* find(const QString& text);

  signals:
    // what is a sentence to show: what was found, which of those found it is, or that nothing was.
    void told(const QString& what);

  private:
    // Offers what is typed among the instances shown now, as they are named now: read again before each completion.
    void refreshCompletions();
    // Selects the one of found at rank alone, shows it in the middle of the view, tells it, and returns it.
    ComponentItem* showFound(const QList<ComponentItem*>& found, qsizetype rank);

    QGraphicsView& m_view;
    QStandardItemModel* m_completions; // A row per instance shown: its name, then the name of its component
};
