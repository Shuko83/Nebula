// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "TabsComponent.h"

#include <QWidget>

#include "PanelWidgets.h"
#include "StudioPlugin.h"

NEBULA_REGISTER_COMPONENT(StudioPlugin, TabsComponent);

TabsComponent::TabsComponent()
{
    m_tabs.setDocumentMode(true);
    provideInterface<QWidget>(QStringLiteral("widget"), &m_tabs);
    consumeInterfaceList<QWidget>(
        QStringLiteral("pages"), m_pages, [this](QWidget* page) { showPage(page); },
        [this](QWidget* page) { hidePage(page); });
}

bool TabsComponent::isDocumentMode() const
{
    return m_tabs.documentMode();
}

void TabsComponent::setDocumentMode(bool documentMode)
{
    if(m_tabs.documentMode() == documentMode)
    {
        return;
    }
    m_tabs.setDocumentMode(documentMode);
    emit documentModeChanged(documentMode);
}

void TabsComponent::showPage(QWidget* page)
{
    if(m_tabs.indexOf(page) < 0)
    {
        // Those before it in the list are shown already.
        insertPage(m_tabs, *page, static_cast<int>(m_pages.indexOf(page)));
    }
}

void TabsComponent::hidePage(QWidget* page)
{
    if(m_pages.count(page) == 1)
    {
        m_tabs.removeTab(m_tabs.indexOf(page));
        page->setParent(nullptr);
    }
}
