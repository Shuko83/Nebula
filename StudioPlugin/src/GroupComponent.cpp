// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "GroupComponent.h"

#include <QVBoxLayout>
#include <QWidget>

#include "StudioPlugin.h"

NEBULA_REGISTER_COMPONENT(StudioPlugin, GroupComponent);

GroupComponent::GroupComponent() : m_layout(new QVBoxLayout(&m_group))
{
    m_group.setTitle(QStringLiteral("Group"));
    provideInterface<QWidget>(QStringLiteral("widget"), &m_group);
    consumeInterface<QWidget>(
        QStringLiteral("content"), m_content, Nebula::InterfaceRequirement::Mandatory, [this] { showContent(); },
        [this] { giveBackContent(); });
}

QString GroupComponent::title() const
{
    return m_group.title();
}

void GroupComponent::setTitle(const QString& title)
{
    if(m_group.title() == title)
    {
        return;
    }
    m_group.setTitle(title);
    emit titleChanged(title);
}

void GroupComponent::showContent()
{
    m_layout->addWidget(m_content);
    m_content->show(); // Hidden when it was taken from where it was shown
}

void GroupComponent::giveBackContent()
{
    m_layout->removeWidget(m_content);
    m_content->setParent(nullptr); // Given back as it was provided: the frame would delete it
}
