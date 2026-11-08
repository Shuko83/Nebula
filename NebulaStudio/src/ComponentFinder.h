// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QLineEdit>
#include <QList>
#include <QString>
#include <QStringList>

class ComponentItem;
class QGraphicsView;
class QStandardItemModel;

namespace Nebula
{
class Component;
class Galaxy;
class Universe;
} // namespace Nebula

// A field that finds the instances of an application by their name or by the name of their component, whatever its
// case: whether a component is instantiated, and where. It finds those of the application, and those of its galaxies,
// at any depth, each named after the galaxies it is in: Galaxy1.Ticker1. As it is typed, it offers those whose name,
// or the name of whose component, holds what is typed, with the name of their component: those of the application
// first, then those one galaxy down, and so on, each level in the order of their names. Choosing one, or pressing
// Enter, shows the galaxy it is in in the view, see galaxyRequested(), then selects it alone there and shows it in the
// middle of the view: the one whose name is what is typed, when one is, otherwise those whose own name it is, in
// galaxies, otherwise those that hold it, Enter going through them in turn: the one after the instance selected in the
// view, the first one after the last one, or, when none of them is selected there, the first one the view shows, or
// that is in a galaxy it shows. The field keeps the keys meanwhile. What it found, or that it found nothing, is told
// through told().
class ComponentFinder : public QLineEdit
{
    Q_OBJECT

  public:
    // An instance of the application, and where it is.
    struct Instance
    {
        Nebula::Component* component = nullptr;
        QList<Nebula::Galaxy*> galaxies; // Those it is in, from the application down; none when it is one of its own
        QString name;                    // The names of its galaxies, then its own, joined by dots
        QString componentName;           // As its item tells it, see componentNameOf()
    };

    // application and view outlive the field: it finds the instances of application, and shows them in the scene view
    // shows, the EditionScene of the application or of one of its galaxies, once it is the one they are in.
    ComponentFinder(const Nebula::Universe& application, QGraphicsView& view, QWidget* parent = nullptr);

    // Selects and shows the next instance found for text, as Enter does, see ComponentFinder, and returns what shows
    // it; nullptr, the selection unchanged, when none is found, or when the galaxy it is in is not shown. Nothing when
    // text is empty.
    ComponentItem* find(const QString& text);

  signals:
    // what is a sentence to show: what was found, which of those found it is, or that nothing was.
    void told(const QString& what);
    // The view is to show the galaxy at the end of path, the names of the galaxies from the application down to it,
    // where an instance found is: the application when path is empty. It is shown once this returns, or not at all.
    void galaxyRequested(const QStringList& path);

  private:
    // Offers what is typed among the instances of the application now, as they are named now: read again before each
    // completion.
    void refreshCompletions();
    // The galaxy of the scene the view shows; nullptr for the application, or when it shows none of an application.
    [[nodiscard]] Nebula::Galaxy* shownGalaxy() const;
    // The names of the galaxies from the application down to the one the view shows; none for the application.
    [[nodiscard]] QStringList shownPath() const;
    // Shows the one of found at rank, see showFound(const QStringList&, const QString&, const QString&).
    ComponentItem* showFound(const QList<Instance>& found, qsizetype rank);
    // Has the view show the galaxy at the end of path, when it does not, then shows the instance named name there, and
    // tells what; nullptr when it is not shown.
    ComponentItem* showFound(const QStringList& path, const QString& name, const QString& what);
    // The item of the instance named name in the galaxy at the end of path, when the view shows it; nullptr otherwise.
    [[nodiscard]] ComponentItem* shownItem(const QStringList& path, const QString& name) const;
    // Selects item alone, shows it in the middle of the view, tells what, and returns it; nothing when it is nullptr.
    ComponentItem* select(ComponentItem* item, const QString& what);

    const Nebula::Universe& m_application;
    QGraphicsView& m_view;
    QStandardItemModel* m_completions; // A row per instance: its name, then the name of its component
};
