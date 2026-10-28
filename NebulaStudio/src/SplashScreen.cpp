// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "SplashScreen.h"

#include <QApplication>
#include <QDateTime>
#include <QFontMetrics>
#include <QLocale>
#include <QPainter>
#include <QPixmap>
#include <QStringList>
#include <QSvgRenderer>
#include <chrono>
#include <string_view>

#include "NebulaStudio_info.h"
#include "Theme.h"

namespace
{

// How long it stays when it is not clicked.
constexpr auto ReadingTime = std::chrono::seconds(2);
// Its width, and the space around what it says, in device-independent pixels.
constexpr int Width = 480;
constexpr int Margin = 24;
// The size of the name of the framework, relative to the font of the application.
constexpr qreal TitleScale = 2.5;

QString fromInfo(std::string_view text)
{
    return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size()));
}

// When NebulaStudio was built, in the format of the locale; as the compiler wrote it when it cannot be read.
QString buildDateText()
{
    const QString text = fromInfo(NebulaStudioInfo::buildDate).simplified();
    const QDateTime buildDate = QLocale::c().toDateTime(text, QStringLiteral("MMM d yyyy HH:mm:ss"));
    return buildDate.isValid() ? QLocale().toString(buildDate, QLocale::ShortFormat) : text;
}

// The fonts the picture is written in, and the sizes they give it.
struct Layout
{
    QFont font;
    QFont nameFont; // Of the name of the framework
    int titleHeight = 0;
    int lineHeight = 0;

    // The height of the band, with the logo, the name and the tagline.
    [[nodiscard]] int bandHeight() const
    {
        return 2 * Margin + titleHeight + lineHeight;
    }
    // The logo is as high as the name and the tagline beside it.
    [[nodiscard]] int logoSize() const
    {
        return titleHeight + lineHeight;
    }
    // The size of the picture with lineCount lines under the band.
    [[nodiscard]] QSize size(qsizetype lineCount) const
    {
        return {Width, bandHeight() + 2 * Margin + static_cast<int>(lineCount) * lineHeight};
    }
};

Layout layoutOf(const QFont& font)
{
    QFont nameFont = titleFont(font);
    nameFont.setPointSizeF(font.pointSizeF() * TitleScale);
    return {.font = font,
            .nameFont = nameFont,
            .titleHeight = QFontMetrics(nameFont).height(),
            .lineHeight = QFontMetrics(font).lineSpacing()};
}

// What it says under the band, one per line; an empty one leaves a line's space.
QStringList lines()
{
    return {SplashScreen::tr("Version %1").arg(fromInfo(NebulaStudioInfo::version)),
            SplashScreen::tr("Built with Qt %1 and %2")
                .arg(fromInfo(NebulaStudioInfo::qtVersion), fromInfo(NebulaStudioInfo::compiler)),
            SplashScreen::tr("Built on %1").arg(buildDateText()),
            QString(),
            fromInfo(NebulaStudioInfo::copyright),
            SplashScreen::tr("Licensed under the Mozilla Public License 2.0")};
}

// A picture of size, at the resolution of the screens, filled with color.
QPixmap emptyPicture(QSize size, const QColor& color)
{
    const qreal ratio = qApp->devicePixelRatio();
    QPixmap pixmap(size * ratio);
    pixmap.setDevicePixelRatio(ratio);
    pixmap.fill(color);
    return pixmap;
}

// Draws text in the current font and pen of painter, on the line of height whose top is top, from left to the margin
// on the right; returns the top of the next one.
int drawLine(QPainter& painter, int left, int top, int height, const QString& text)
{
    const int width = Width - Margin - left;
    painter.drawText(QRect(left, top, width, height), Qt::AlignLeft | Qt::AlignVCenter,
                     painter.fontMetrics().elidedText(text, Qt::ElideRight, width));
    return top + height;
}

// The band, in the highlight color, and the edges of a picture of size, whatever is behind it.
void paintFrame(QPainter& painter, const QPalette& palette, const Layout& layout, QSize size)
{
    painter.fillRect(0, 0, Width, layout.bandHeight(), palette.color(QPalette::Highlight));
    painter.setPen(palette.color(QPalette::Mid));
    painter.drawRect(QRect(QPoint(), size).adjusted(0, 0, -1, -1));
}

// The logo, then the name of the framework and its tagline beside it, on the band.
void paintTitle(QPainter& painter, const QPalette& palette, const Layout& layout)
{
    QSvgRenderer(logoPath()).render(&painter, QRectF(Margin, Margin, layout.logoSize(), layout.logoSize()));
    const int left = Margin + layout.logoSize() + Margin / 2;
    painter.setPen(palette.color(QPalette::HighlightedText));
    painter.setFont(layout.nameFont);
    const int taglineTop = drawLine(painter, left, Margin, layout.titleHeight, fromInfo(NebulaStudioInfo::product));
    painter.setFont(layout.font);
    drawLine(painter, left, taglineTop, layout.lineHeight,
             SplashScreen::tr("Build modular applications by assembling components"));
}

// texts, one per line, under the band.
void paintLines(QPainter& painter, const QPalette& palette, const Layout& layout, const QStringList& texts)
{
    painter.setPen(palette.color(QPalette::WindowText));
    int top = layout.bandHeight() + Margin;
    for(const QString& text : texts)
    {
        top = drawLine(painter, Margin, top, layout.lineHeight, text);
    }
}

// The logo and the name of the framework on a band of the highlight color, then the rest of what it says, under it,
// at the resolution of the screens.
QPixmap picture()
{
    const QPalette palette = QApplication::palette();
    const Layout layout = layoutOf(QApplication::font());
    const QSize size = layout.size(lines().size());
    QPixmap pixmap = emptyPicture(size, palette.color(QPalette::Window));
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    paintFrame(painter, palette, layout, size);
    paintTitle(painter, palette, layout);
    paintLines(painter, palette, layout, lines());
    return pixmap;
}

} // namespace

SplashScreen::SplashScreen() : QSplashScreen(picture())
{
    m_readingTimer.setSingleShot(true);
    m_readingTimer.setInterval(ReadingTime);
    connect(&m_readingTimer, &QTimer::timeout, this, &SplashScreen::setRead);
}

void SplashScreen::showEvent(QShowEvent* event)
{
    QSplashScreen::showEvent(event);
    if(!m_read && !m_readingTimer.isActive())
    {
        m_readingTimer.start();
    }
}

void SplashScreen::mousePressEvent(QMouseEvent* /*event*/)
{
    setRead();
}

void SplashScreen::setRead()
{
    if(m_read)
    {
        return;
    }
    m_read = true;
    m_readingTimer.stop();
    emit read();
}
