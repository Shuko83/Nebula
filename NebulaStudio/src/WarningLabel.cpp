// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "WarningLabel.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QStyle>

#include "Theme.h"

namespace
{

// A label showing the warning icon of style, as a small icon.
QLabel* newWarningIcon(const QStyle& style)
{
    auto* icon = new QLabel;
    icon->setPixmap(
        style.standardIcon(QStyle::SP_MessageBoxWarning).pixmap(style.pixelMetric(QStyle::PM_SmallIconSize)));
    return icon;
}

} // namespace

WarningLabel::WarningLabel(QWidget* parent) : QWidget(parent), m_text(new QLabel)
{
    m_text->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_text->setWordWrap(true);
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins({});
    layout->addWidget(newWarningIcon(*style()), 0, Qt::AlignTop);
    layout->addWidget(m_text, 1);
    colorText();
    hide(); // Until it has something to tell
}

QString WarningLabel::text() const
{
    return m_text->text();
}

void WarningLabel::setText(const QString& text)
{
    m_text->setText(text);
    setVisible(!text.isEmpty());
}

void WarningLabel::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if(event->type() == QEvent::PaletteChange)
    {
        colorText();
    }
}

void WarningLabel::colorText()
{
    QPalette palette = m_text->palette();
    palette.setColor(QPalette::WindowText, currentThemeColors().warning);
    m_text->setPalette(palette);
}
