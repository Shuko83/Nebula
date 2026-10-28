// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ProblemList.h"

#include <QHeaderView>
#include <QLabel>
#include <QMetaObject>
#include <QStringList>
#include <QStyle>
#include <QTreeWidget>
#include <cstdint>
#include <memory>

#include "InterfaceItem.h"
#include "Nebula/Application/Universe.h"
#include "Nebula/Component/Component.h"
#include "PlaceholderStack.h"

namespace
{

enum Column : std::uint8_t
{
    ComponentColumn,
    ProblemColumn,
    FixColumn,
    ColumnCount,
};

// The list of the problems: the component, the problem, and how to fix it.
QTreeWidget* newProblemTree()
{
    auto* tree = new QTreeWidget;
    tree->setColumnCount(ColumnCount);
    tree->setHeaderLabels({ProblemList::tr("Component"), ProblemList::tr("Problem"), ProblemList::tr("How to fix it")});
    tree->setRootIsDecorated(false);
    tree->setAllColumnsShowFocus(true);
    tree->header()->setStretchLastSection(true);
    tree->header()->setSectionResizeMode(ComponentColumn, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(ProblemColumn, QHeaderView::ResizeToContents);
    return tree;
}

// The interfaces of type component provides: <component>.<interface> each.
QStringList providedOfType(const Nebula::Component& component, QMetaType type)
{
    QStringList provided;
    const QList<Nebula::InterfaceInfo> interfaces = component.providedInterfaces();
    for(const Nebula::InterfaceInfo& info : interfaces)
    {
        if(info.type == type)
        {
            provided.append(QStringLiteral("%1.%2").arg(component.objectName(), info.name));
        }
    }
    return provided;
}

// The interfaces of type the components of application provide, other than consumer: <component>.<interface> each.
QStringList providersOf(const Nebula::Universe& application, QMetaType type, const Nebula::Component& consumer)
{
    QStringList providers;
    for(const std::unique_ptr<Nebula::Component>& component : application.components())
    {
        if(component.get() != &consumer)
        {
            providers.append(providedOfType(*component, type));
        }
    }
    return providers;
}

// How to bind name to one of providers, which there is one at least of.
QString dragText(const QString& name, const QStringList& providers)
{
    if(providers.size() == 1)
    {
        return ProblemList::tr("Drag from the dot of %1 to the dot of %2").arg(name, providers.constFirst());
    }
    return ProblemList::tr("Drag from the dot of %1 to the dot of one of %2")
        .arg(name, providers.join(QStringLiteral(", ")));
}

// How to bind name, of type, to one of providers, or to one yet to be added when there is none.
QString fixText(const QString& name, const QString& type, const QStringList& providers)
{
    if(providers.isEmpty())
    {
        return ProblemList::tr("Add a component that provides a %1, then drag from the dot of %2 to the dot of its %1")
            .arg(type, name);
    }
    return dragText(name, providers);
}

} // namespace

ProblemList::ProblemList(const Nebula::Universe& application, QWidget* parent)
    : QWidget(parent), m_application(application), m_tree(newProblemTree()),
      m_stack(new PlaceholderStack(m_tree, *this))
{
    connect(&m_application, &Nebula::Universe::componentsChanged, this, &ProblemList::scheduleRefresh);
    connect(m_tree, &QTreeWidget::itemActivated, this,
            [this](const QTreeWidgetItem* item) { emit componentActivated(item->text(ComponentColumn)); });
    m_stack->placeholder()->setText(tr("Every component of the application has the interfaces it needs"));
    m_stack->placeholder()->setForegroundRole(QPalette::PlaceholderText);
    refresh();
}

void ProblemList::scheduleRefresh()
{
    if(!m_refreshScheduled)
    {
        m_refreshScheduled = true;
        QMetaObject::invokeMethod(this, &ProblemList::refresh, Qt::QueuedConnection);
    }
}

void ProblemList::refresh()
{
    m_refreshScheduled = false;
    m_tree->clear();
    for(const std::unique_ptr<Nebula::Component>& component : m_application.components())
    {
        follow(*component);
        addProblems(*component);
    }
    m_stack->showPageIf(m_tree->topLevelItemCount() > 0);
}

void ProblemList::follow(const Nebula::Component& component)
{
    // Once each, those already followed included: the ones instantiated since are followed from now on.
    connect(&component, &Nebula::Component::interfacesChanged, this, &ProblemList::scheduleRefresh,
            Qt::UniqueConnection);
    connect(&component, &QObject::objectNameChanged, this, &ProblemList::scheduleRefresh, Qt::UniqueConnection);
}

void ProblemList::addProblems(const Nebula::Component& component)
{
    const QStringList unbound = component.unboundMandatoryInterfaces();
    const QList<Nebula::InterfaceInfo> consumed = component.consumedInterfaces();
    for(const Nebula::InterfaceInfo& info : consumed)
    {
        if(unbound.contains(info.name))
        {
            addProblem(component, info);
        }
    }
}

void ProblemList::addProblem(const Nebula::Component& component, const Nebula::InterfaceInfo& consumed)
{
    const QString type = InterfaceItem::typeName(consumed.type);
    const QStringList providers = providersOf(m_application, consumed.type, component);
    addRow(component.objectName(), tr("%1 (%2) is bound to nothing").arg(consumed.name, type),
           fixText(consumed.name, type, providers));
}

void ProblemList::addRow(const QString& component, const QString& problem, const QString& fix)
{
    auto* item = new QTreeWidgetItem(m_tree, {component, problem, fix});
    item->setIcon(ComponentColumn, style()->standardIcon(QStyle::SP_MessageBoxWarning));
    // The whole of it, should the columns be narrow.
    item->setToolTip(ProblemColumn, tr("%1: the component does not work without it").arg(problem));
    item->setToolTip(FixColumn, fix);
}
