// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "OverviewComponent.h"

#include <QMenu>

#include "EditionZone.h"
#include "Studio.h"
#include "StudioPlugin.h"

NEBULA_REGISTER_COMPONENT(StudioPlugin, OverviewComponent);

OverviewComponent::OverviewComponent()
{
    m_separator.setSeparator(true);
    m_entry.setText(tr("O&verview"));
    m_entry.setToolTip(tr("Show the whole scene of the edition zone, small, beside it"));
    m_entry.setCheckable(true);
    m_entry.setChecked(true); // Until its studio keeps it otherwise
    connect(&m_entry, &QAction::toggled, this, &OverviewComponent::showAsChecked);
    connect(&m_entry, &QAction::triggered, this, &OverviewComponent::keepChoice);
    provideInterface<QWidget>(QStringLiteral("widget")); // Once it has the zone
    consumeParts();
}

void OverviewComponent::consumeParts()
{
    consumeInterface<Studio>(
        QStringLiteral("studio"), m_studio, [this] { followStudio(); }, [this] { m_studio->disconnect(&m_entry); });
    consumeInterface<EditionZone>(
        QStringLiteral("zone"), m_zone, Nebula::InterfaceRequirement::Mandatory, [this] { createOverview(); },
        [this] { destroyOverview(); });
    consumeInterface<QMenu>(QStringLiteral("viewMenu"), m_viewMenu, [this] { addEntry(); }, [this] { removeEntry(); });
}

bool OverviewComponent::onStart()
{
    // What it was provided to showed it as it took it, before its entry was checked as the studio keeps it, say.
    showAsChecked();
    return true;
}

void OverviewComponent::createOverview()
{
    m_overview.reset(m_zone->createOverview());
    provideInterface<QWidget>(QStringLiteral("widget"), m_overview.get());
    showAsChecked(); // Shown by what it was provided to, a splitter say, which may have it already
}

void OverviewComponent::destroyOverview()
{
    // First: what it was provided to gives it back while it still exists.
    provideInterface<QWidget>(QStringLiteral("widget"), nullptr);
    m_overview.reset();
}

void OverviewComponent::addEntry()
{
    m_viewMenu->addAction(&m_separator);
    m_viewMenu->addAction(&m_entry);
}

void OverviewComponent::removeEntry()
{
    m_viewMenu->removeAction(&m_separator);
    m_viewMenu->removeAction(&m_entry);
}

void OverviewComponent::followStudio()
{
    m_entry.setChecked(m_studio->isOverviewShown());
    connect(m_studio, &Studio::overviewShownChanged, &m_entry, &QAction::setChecked);
}

void OverviewComponent::keepChoice(bool shown)
{
    if(m_studio != nullptr)
    {
        m_studio->setOverviewShown(shown);
    }
}

void OverviewComponent::showAsChecked()
{
    // Not on its own, as a window, before it is where it is shown.
    if(m_overview != nullptr && m_overview->parentWidget() != nullptr)
    {
        m_overview->setVisible(m_entry.isChecked());
    }
}
