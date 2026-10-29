// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "StudioWindowComponent.h"

#include <QEvent>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QWidget>
#include <utility>

#include "Messages.h"
#include "Project.h"
#include "Studio.h"
#include "StudioPlugin.h"
#include "Theme.h"

NEBULA_REGISTER_COMPONENT(StudioPlugin, StudioWindowComponent);

namespace
{

// widgets, each once, in their order.
QList<QWidget*> distinct(const QList<QWidget*>& widgets)
{
    QList<QWidget*> distinct;
    for(QWidget* widget : widgets)
    {
        if(!distinct.contains(widget))
        {
            distinct.append(widget);
        }
    }
    return distinct;
}

} // namespace

StudioWindowComponent::StudioWindowComponent()
{
    m_window.setWindowTitle(tr("NebulaStudio")); // Until it has a project, which tells its file
    m_window.setWindowIcon(QIcon(logoPath()));
    m_window.statusBar()->addWidget(createPluginDirectoryLabel());
    m_window.installEventFilter(this);
    consumeWidgets();
    consumeParts();
}

void StudioWindowComponent::consumeWidgets()
{
    consumeInterface<QWidget>(
        QStringLiteral("centralWidget"), m_centralWidget, Nebula::InterfaceRequirement::Mandatory,
        [this] { showCentralWidget(); }, [this] { giveBackCentralWidget(); });
    consumeInterfaceList<QMenu>(
        QStringLiteral("menus"), m_menus, [this](QMenu* /*menu*/) { showMenus(m_menus); },
        [this](QMenu* menu) { showMenusBut(menu); });
    consumeInterfaceList<QWidget>(
        QStringLiteral("statusWidgets"), m_statusWidgets, [this](QWidget* /*widget*/) { showStatusWidgets(); },
        [this](QWidget* widget) { hideStatusWidget(widget); });
}

void StudioWindowComponent::consumeParts()
{
    consumeInterface<Studio>(
        QStringLiteral("studio"), m_studio, [this] { followStudio(); }, [this] { m_studio->disconnect(this); });
    consumeInterface<Project>(
        QStringLiteral("project"), m_project, [this] { followProject(); }, [this] { forgetProject(); });
}

int StudioWindowComponent::width() const
{
    return m_width;
}

void StudioWindowComponent::setWidth(int width)
{
    if(m_width == width)
    {
        return;
    }
    m_width = width;
    emit widthChanged(m_width);
}

int StudioWindowComponent::height() const
{
    return m_height;
}

void StudioWindowComponent::setHeight(int height)
{
    if(m_height == height)
    {
        return;
    }
    m_height = height;
    emit heightChanged(m_height);
}

bool StudioWindowComponent::onStart()
{
    m_window.resize(m_width, m_height);
    m_window.show();
    return true;
}

void StudioWindowComponent::onStop()
{
    m_window.hide();
}

bool StudioWindowComponent::eventFilter(QObject* watched, QEvent* event)
{
    if(watched == &m_window && event->type() == QEvent::Close && !canClose())
    {
        event->ignore();
        return true;
    }
    return Nebula::Component::eventFilter(watched, event);
}

bool StudioWindowComponent::canClose()
{
    return m_project == nullptr || m_project->saveChanges();
}

void StudioWindowComponent::showCentralWidget()
{
    m_window.setCentralWidget(m_centralWidget);
    m_centralWidget->show(); // Hidden when it was taken from a window
}

void StudioWindowComponent::giveBackCentralWidget()
{
    m_window.takeCentralWidget();
    m_centralWidget->setParent(nullptr); // Given back as it was provided: the window would delete it
}

void StudioWindowComponent::showMenus(const QList<QMenu*>& shown)
{
    QMenuBar* bar = m_window.menuBar();
    bar->clear(); // The actions of the menus are theirs: none is deleted
    for(QMenu* menu : shown)
    {
        bar->addMenu(menu);
    }
}

void StudioWindowComponent::showMenusBut(QMenu* menu)
{
    QList<QMenu*> shown = m_menus; // Which still has it
    shown.removeOne(menu);
    showMenus(shown);
}

void StudioWindowComponent::showStatusWidgets()
{
    QStatusBar* bar = m_window.statusBar();
    for(QWidget* widget : std::as_const(m_statusWidgets))
    {
        bar->removeWidget(widget); // Added again, in its place
    }
    for(QWidget* widget : distinct(m_statusWidgets))
    {
        bar->addPermanentWidget(widget);
        widget->show(); // Hidden when it was removed from a status bar
    }
}

void StudioWindowComponent::hideStatusWidget(QWidget* widget)
{
    if(m_statusWidgets.count(widget) == 1)
    {
        m_window.statusBar()->removeWidget(widget);
        widget->setParent(nullptr); // Given back as it was provided: the status bar would delete it
    }
}

void StudioWindowComponent::followStudio()
{
    connect(m_studio, &Studio::told, this, [this](const QString& message) { tell(*m_window.statusBar(), message); });
}

void StudioWindowComponent::followProject()
{
    m_window.setWindowTitle(m_project->title());
    connect(m_project, &Project::titleChanged, &m_window, &QWidget::setWindowTitle);
}

void StudioWindowComponent::forgetProject()
{
    m_project->disconnect(&m_window);
    m_window.setWindowTitle(tr("NebulaStudio"));
}
