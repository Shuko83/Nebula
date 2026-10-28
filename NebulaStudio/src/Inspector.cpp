// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Inspector.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "PlaceholderStack.h"

Inspector::Inspector(QWidget* parent)
    : QWidget(parent), m_page(new QWidget), m_pageLayout(new QVBoxLayout(m_page)), m_form(new QFormLayout),
      m_stack(new PlaceholderStack(m_page, *this))
{
    m_pageLayout->addLayout(m_form);
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
    m_stack->showPage();
}

void Inspector::showPlaceholder(const QString& text)
{
    m_stack->showPlaceholder(text);
}
