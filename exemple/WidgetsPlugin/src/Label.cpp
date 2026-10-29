// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Label.h"

Label::Label(const QString& text) : m_label(text) {}

QLabel& Label::widget()
{
    return m_label;
}

QString Label::text() const
{
    return m_label.text();
}

void Label::setText(const QString& text)
{
    if(m_label.text() == text)
    {
        return;
    }
    m_label.setText(text);
    emit textChanged(text);
}
