// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Inspector.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

Inspector::Inspector(QWidget* parent)
    : QWidget(parent), m_stack(new QStackedWidget), m_placeholder(new QLabel), m_page(new QWidget),
      m_pageLayout(new QVBoxLayout(m_page)), m_form(new QFormLayout)
{
    m_placeholder->setAlignment(Qt::AlignCenter);
    m_placeholder->setWordWrap(true);
    m_pageLayout->addLayout(m_form);
    m_stack->addWidget(m_placeholder);
    m_stack->addWidget(m_page);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->addWidget(m_stack);
}

QLabel* Inspector::newValueLabel()
{
    auto* value = new QLabel;
    value->setTextInteractionFlags(Qt::TextSelectableByMouse);
    value->setWordWrap(true);
    return value;
}

QLabel* Inspector::addField(const QString& name)
{
    QLabel* value = newValueLabel();
    addRow(name, value);
    return value;
}

void Inspector::addRow(const QString& name, QWidget* field)
{
    m_form->addRow(name, field);
}

void Inspector::addBelow(QWidget* widget)
{
    m_pageLayout->addWidget(widget);
}

void Inspector::addButtons(std::initializer_list<QPushButton*> buttons)
{
    auto* row = new QHBoxLayout;
    for(QPushButton* button : buttons)
    {
        row->addWidget(button);
    }
    row->addStretch();
    m_pageLayout->addLayout(row);
}

void Inspector::addStretch()
{
    m_pageLayout->addStretch();
}

QPushButton* Inspector::newButton(const QString& text, const std::function<void()>& clicked)
{
    auto* button = new QPushButton(text);
    connect(button, &QPushButton::clicked, this, clicked);
    return button;
}

void Inspector::showPage()
{
    m_stack->setCurrentWidget(m_page);
}

void Inspector::showPlaceholder(const QString& text)
{
    m_placeholder->setText(text);
    m_stack->setCurrentWidget(m_placeholder);
}
