// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "DialogHeader.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>

#include "Theme.h"

namespace
{

// The side of the icon.
constexpr int IconSize = 40;
// How much larger the title is than the text of the dialog.
constexpr qreal TitleScale = 1.5;

// A label of plain text: what it shows can be typed.
QLabel* newPlainLabel()
{
    auto* label = new QLabel;
    label->setTextFormat(Qt::PlainText);
    return label;
}

// font, scale times larger, whether its size is in points or in pixels.
QFont scaled(QFont font, qreal scale)
{
    if(font.pointSizeF() > 0)
    {
        font.setPointSizeF(font.pointSizeF() * scale);
        return font;
    }
    font.setPixelSize(qRound(font.pixelSize() * scale));
    return font;
}

// The title, in the font of the titles, larger than font.
QLabel* newTitle(const QFont& font)
{
    QLabel* label = newPlainLabel();
    label->setFont(scaled(titleFont(font), TitleScale));
    return label;
}

// What it is for, muted, which wraps.
QLabel* newText()
{
    QLabel* label = newPlainLabel();
    label->setWordWrap(true);
    label->setForegroundRole(QPalette::PlaceholderText); // The muted text of the theme
    return label;
}

QLabel* newIcon(const QIcon& icon)
{
    auto* label = new QLabel;
    label->setPixmap(icon.pixmap(QSize(IconSize, IconSize)));
    return label;
}

} // namespace

DialogHeader::DialogHeader(const QIcon& icon, QWidget* parent)
    : QWidget(parent), m_title(newTitle(font())), m_text(newText())
{
    auto* texts = new QVBoxLayout;
    texts->addWidget(m_title);
    texts->addWidget(m_text);
    auto* layout = new QHBoxLayout(this);
    layout->addWidget(newIcon(icon), 0, Qt::AlignTop);
    layout->addLayout(texts, 1);
}

QString DialogHeader::title() const
{
    return m_title->text();
}

void DialogHeader::setTitle(const QString& title)
{
    m_title->setText(title);
}

QString DialogHeader::text() const
{
    return m_text->text();
}

void DialogHeader::setText(const QString& text)
{
    m_text->setText(text);
}

void DialogHeader::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.fillRect(rect(), palette().alternateBase());
    painter.setPen(palette().color(QPalette::Mid));
    painter.drawLine(rect().bottomLeft(), rect().bottomRight());
}
