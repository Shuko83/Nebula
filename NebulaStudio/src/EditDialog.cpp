// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "EditDialog.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "DialogHeader.h"

EditDialog::EditDialog(QObject& edited, const QIcon& icon, QWidget* parent)
    : QDialog(parent), m_header(new DialogHeader(icon)),
      m_buttons(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel))
{
    setWindowIcon(icon);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(this, &QDialog::accepted, this, [this] { apply(); });
    connect(&edited, &QObject::destroyed, this, &QDialog::reject);
}

DialogHeader& EditDialog::header() const
{
    return *m_header;
}

QDialogButtonBox* EditDialog::buttons() const
{
    return m_buttons;
}

void EditDialog::layOut(QWidget* content)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins({}); // The header across the dialog
    layout->setSpacing(0);
    layout->addWidget(m_header);
    layout->addWidget(content, 1);
}

void EditDialog::setAcceptable(bool acceptable)
{
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(acceptable);
}
