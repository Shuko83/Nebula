// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "EditDialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>

EditDialog::EditDialog(QObject& edited, QWidget* parent)
    : QDialog(parent), m_buttons(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel))
{
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(this, &QDialog::accepted, this, [this] { apply(); });
    connect(&edited, &QObject::destroyed, this, &QDialog::reject);
}

QLabel* EditDialog::newHint(const QString& text)
{
    auto* hint = new QLabel(text);
    hint->setWordWrap(true);
    return hint;
}

QDialogButtonBox* EditDialog::buttons() const
{
    return m_buttons;
}

void EditDialog::setAcceptable(bool acceptable)
{
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(acceptable);
}
