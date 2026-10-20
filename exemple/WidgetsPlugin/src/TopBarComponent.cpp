// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "TopBarComponent.h"

#include <QAction>
#include <QHBoxLayout>
#include <QLayout>
#include <QMainWindow>
#include <QWidget>

#include "WidgetsPlugin.h"

NEBULA_REGISTER_COMPONENT(WidgetsPlugin, TopBarComponent);

TopBarComponent::TopBarComponent()
{
    m_bar.setWindowTitle(QStringLiteral("Top bar"));
    m_bar.setMovable(false);
    consumeInterface<QMainWindow>(
        QStringLiteral("mainWindow"), m_mainWindow, Nebula::InterfaceRequirement::Mandatory,
        [this]
        {
            m_mainWindow->addToolBar(Qt::TopToolBarArea, &m_bar);
            m_bar.show(); // Hidden when it was removed from a window
        },
        [this]
        {
            m_mainWindow->removeToolBar(&m_bar);
            m_bar.setParent(nullptr); // The window would delete it
        });
    for(std::size_t slot = 0; slot < ActionCount; ++slot)
    {
        consumeInterface<QAction>(
            QStringLiteral("action%1").arg(slot + 1), m_actions.at(slot), [this] { showActions(); },
            [this, slot] { showActions(slot); });
    }
    for(std::size_t slot = 0; slot < WidgetCount; ++slot)
    {
        // A widget of the bar is owned by the action the bar makes of it: the holder, not the widget consumed.
        auto* holder = new QWidget;
        auto* layout = new QHBoxLayout(holder);
        layout->setContentsMargins({});
        m_holders.at(slot) = holder;
        m_holderActions.at(slot) = m_bar.addWidget(holder);
        m_holderActions.at(slot)->setVisible(false);
        consumeInterface<QWidget>(
            QStringLiteral("widget%1").arg(slot + 1), m_widgets.at(slot), [this, slot] { showWidget(slot); },
            [this, slot] { hideWidget(slot); });
    }
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

void TopBarComponent::showActions(std::optional<std::size_t> leaving)
{
    for(QAction* action : m_actions)
    {
        if(action != nullptr)
        {
            m_bar.removeAction(action);
        }
    }
    for(std::size_t slot = 0; slot < ActionCount; ++slot)
    {
        if(m_actions.at(slot) != nullptr && slot != leaving)
        {
            // An action consumed twice is shown once, in the place of the last of its interfaces.
            m_bar.insertAction(m_holderActions.front(), m_actions.at(slot));
        }
    }
}

void TopBarComponent::showWidget(std::size_t slot)
{
    m_holders.at(slot)->layout()->addWidget(m_widgets.at(slot));
    m_widgets.at(slot)->show(); // Hidden when it was taken from where it was shown
    m_holderActions.at(slot)->setVisible(true);
}

void TopBarComponent::hideWidget(std::size_t slot)
{
    m_holderActions.at(slot)->setVisible(false);
    m_holders.at(slot)->layout()->removeWidget(m_widgets.at(slot));
    m_widgets.at(slot)->setParent(nullptr);
}
