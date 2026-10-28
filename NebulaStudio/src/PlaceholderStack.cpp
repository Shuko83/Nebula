// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "PlaceholderStack.h"

#include <QLabel>
#include <QVBoxLayout>

PlaceholderStack::PlaceholderStack(QWidget* page, QWidget& owner)
    : QStackedWidget(&owner), m_page(page), m_placeholder(new QLabel)
{
    m_placeholder->setAlignment(Qt::AlignCenter);
    m_placeholder->setWordWrap(true);
    addWidget(m_placeholder);
    addWidget(m_page);
    auto* layout = new QVBoxLayout(&owner);
    layout->setContentsMargins({});
    layout->addWidget(this);
}

QLabel* PlaceholderStack::placeholder() const
{
    return m_placeholder;
}

void PlaceholderStack::showPage()
{
    setCurrentWidget(m_page);
}

void PlaceholderStack::showPlaceholder(const QString& text)
{
    m_placeholder->setText(text);
    setCurrentWidget(m_placeholder);
}

void PlaceholderStack::showPageIf(bool hasSomething)
{
    setCurrentWidget(hasSomething ? m_page : static_cast<QWidget*>(m_placeholder));
}
