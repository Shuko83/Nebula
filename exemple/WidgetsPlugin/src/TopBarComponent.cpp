// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "TopBarComponent.h"

#include <QAction>
#include <QHBoxLayout>
#include <QLayout>
#include <QMainWindow>
#include <QWidget>
#include <QWidgetAction>
#include <utility>

#include "WidgetsPlugin.h"

NEBULA_REGISTER_COMPONENT(WidgetsPlugin, TopBarComponent);

TopBarComponent::TopBarComponent()
{
    m_bar.setWindowTitle(QStringLiteral("Top bar"));
    m_bar.setMovable(false);
    consumeInterfaces();
}

void TopBarComponent::consumeInterfaces()
{
    consumeInterface<QMainWindow>(
        QStringLiteral("mainWindow"), m_mainWindow, Nebula::InterfaceRequirement::Mandatory, [this] { addToWindow(); },
        [this] { removeFromWindow(); });
    consumeInterfaceList<QAction>(
        QStringLiteral("actions"), m_actions, [this](QAction* /*action*/) { showActions(m_actions); },
        [this](QAction* action) { showActionsBut(action); });
    consumeInterfaceList<QWidget>(
        QStringLiteral("widgets"), m_widgets, [this](QWidget* widget) { showWidget(widget); },
        [this](QWidget* widget) { hideWidget(widget); });
}

void TopBarComponent::addToWindow()
{
    m_mainWindow->addToolBar(Qt::TopToolBarArea, &m_bar);
    m_bar.show(); // Hidden when it was removed from a window
}

void TopBarComponent::removeFromWindow()
{
    m_mainWindow->removeToolBar(&m_bar);
    m_bar.setParent(nullptr); // The window would delete it
}

void TopBarComponent::showActionsBut(QAction* action)
{
    QList<QAction*> shown = m_actions; // Which still has it
    shown.removeOne(action);
    showActions(shown);
}

QString TopBarComponent::title() const
{
    return m_bar.windowTitle();
}

void TopBarComponent::setTitle(const QString& title)
{
    if(m_bar.windowTitle() == title)
    {
        return;
    }
    m_bar.setWindowTitle(title);
    emit titleChanged(title);
}

bool TopBarComponent::isMovable() const
{
    return m_bar.isMovable();
}

void TopBarComponent::setMovable(bool movable)
{
    if(m_bar.isMovable() == movable)
    {
        return;
    }
    m_bar.setMovable(movable);
    emit movableChanged(movable);
}

void TopBarComponent::showActions(const QList<QAction*>& shown)
{
    for(QAction* action : std::as_const(m_actions))
    {
        m_bar.removeAction(action);
    }
    insertActions(shown);
}

void TopBarComponent::insertActions(const QList<QAction*>& shown)
{
    QAction* widgets = holderAfter(-1);
    for(QAction* action : shown)
    {
        if(!m_bar.actions().contains(action))
        {
            m_bar.insertAction(widgets, action);
        }
    }
}

void TopBarComponent::showWidget(QWidget* widget)
{
    if(m_holders.contains(widget))
    {
        return;
    }
    auto* holder = new QWidget;
    auto* layout = new QHBoxLayout(holder);
    layout->setContentsMargins({});
    layout->addWidget(widget);
    widget->show(); // Hidden when it was taken from where it was shown
    m_holders.insert(widget, m_bar.insertWidget(holderAfter(m_widgets.indexOf(widget)), holder));
}

void TopBarComponent::hideWidget(QWidget* widget)
{
    if(m_widgets.count(widget) > 1)
    {
        return;
    }
    // Deleted with its action, once it no longer holds the widget.
    QAction* action = m_holders.take(widget);
    release(action, widget);
    m_bar.removeAction(action);
    delete action;
}

void TopBarComponent::release(QAction* holder, QWidget* widget)
{
    if(auto* widgetAction = qobject_cast<QWidgetAction*>(holder);
       widgetAction != nullptr && widgetAction->defaultWidget() != nullptr)
    {
        widgetAction->defaultWidget()->layout()->removeWidget(widget);
    }
    widget->setParent(nullptr);
}

QAction* TopBarComponent::holderAfter(qsizetype index) const
{
    for(qsizetype next = index + 1; next < m_widgets.size(); ++next)
    {
        if(QAction* holder = m_holders.value(m_widgets.at(next)))
        {
            return holder;
        }
    }
    return nullptr;
}
