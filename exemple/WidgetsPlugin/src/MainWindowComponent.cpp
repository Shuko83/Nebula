// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "MainWindowComponent.h"

#include <QWidget>

#include "WidgetsPlugin.h"

NEBULA_REGISTER_COMPONENT(WidgetsPlugin, MainWindowComponent);

MainWindowComponent::MainWindowComponent()
{
    m_window.setWindowTitle(QStringLiteral("Nebula"));
    // Available at once: what fills it is in place before it shows.
    provideInterface<QMainWindow>(QStringLiteral("mainWindow"), &m_window);
    consumeInterface<QWidget>(
        QStringLiteral("centralWidget"), m_centralWidget, [this] { showCentralWidget(); },
        [this] { giveBackCentralWidget(); });
}

void MainWindowComponent::showCentralWidget()
{
    m_window.setCentralWidget(m_centralWidget);
    m_centralWidget->show(); // Hidden when it was taken from a window
}

void MainWindowComponent::giveBackCentralWidget()
{
    m_window.takeCentralWidget();
    m_centralWidget->setParent(nullptr); // Given back as it was provided: its parent would delete it
}

QString MainWindowComponent::title() const
{
    return m_window.windowTitle();
}

void MainWindowComponent::setTitle(const QString& title)
{
    if(m_window.windowTitle() == title)
    {
        return;
    }
    m_window.setWindowTitle(title);
    emit titleChanged(title);
}

int MainWindowComponent::width() const
{
    return m_width;
}

void MainWindowComponent::setWidth(int width)
{
    if(m_width == width)
    {
        return;
    }
    m_width = width;
    emit widthChanged(m_width);
}

int MainWindowComponent::height() const
{
    return m_height;
}

void MainWindowComponent::setHeight(int height)
{
    if(m_height == height)
    {
        return;
    }
    m_height = height;
    emit heightChanged(m_height);
}

bool MainWindowComponent::onStart()
{
    m_window.resize(m_width, m_height);
    m_window.show();
    return true;
}

void MainWindowComponent::onStop()
{
    m_window.hide();
}
