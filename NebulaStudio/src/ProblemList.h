// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QString>
#include <QWidget>

class PlaceholderStack;
class QTreeWidget;

namespace Nebula
{
class Component;
class Galaxy;
class Universe;
struct InterfaceInfo;
} // namespace Nebula

// What keeps the components of the application from working, one row per interface one of them cannot do without and
// that is bound to nothing, see Component::unboundMandatoryInterfaces(): the component, the interface, and how to bind
// it, naming the interfaces of its type the other components provide. Then one row per problem of a galaxy, see
// Galaxy::problems(): what of its file, or of the files of the galaxies it holds, could not be restored, a plugin
// unknown say. A sentence tells when there is none. It follows the components the application instantiates and
// removes, their names, and their interfaces and links, and what its galaxies miss, see
// Universe::problemsChanged(). Activating a row, by double-clicking it or pressing Enter, asks for its component: see
// componentActivated().
class ProblemList : public QWidget
{
    Q_OBJECT

  public:
    explicit ProblemList(const Nebula::Universe& application, QWidget* parent = nullptr);

  signals:
    // The row of the component named name was activated: it is to be shown.
    void componentActivated(const QString& name);

  private slots:
    // Refreshes it once the events being handled are: what changes the components, a removal above all, tells it
    // while they are still changing.
    void scheduleRefresh();

  private:
    // Lists what keeps the components from working now, and follows each component.
    void refresh();
    // Refreshes it when component changes its interfaces, its links or its name; once, however often it is asked.
    void follow(const Nebula::Component& component);
    // A row for each interface component cannot do without and that is bound to nothing, then, for a galaxy, for each
    // of its problems.
    void addProblems(const Nebula::Component& component);
    // The rows of the interfaces component cannot do without and that are bound to nothing.
    void addUnbound(const Nebula::Component& component);
    // The row of consumed, an interface component cannot do without, bound to nothing.
    void addProblem(const Nebula::Component& component, const Nebula::InterfaceInfo& consumed);
    // The rows of what galaxy misses, a problem each.
    void addMissing(const Nebula::Galaxy& galaxy);
    // A row: the component, the problem, and how to fix it; why tells the problem whole, in its tooltip.
    void addRow(const QString& component, const QString& problem, const QString& fix, const QString& why);

    const Nebula::Universe& m_application;
    bool m_refreshScheduled = false;

    QTreeWidget* m_tree = nullptr;       // Shown when a component does not work
    PlaceholderStack* m_stack = nullptr; // Of m_tree and of the sentence that tells every component works
};
