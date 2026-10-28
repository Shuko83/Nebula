// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Theme.h"

#include <QApplication>
#include <QFontDatabase>
#include <QPalette>
#include <QStyleHints>

namespace
{

// The family of Space Grotesk, as its file names it.
const QString TitleFamily = QStringLiteral("Space Grotesk");

// The night sky: the accent is light, and what is written over it dark, for the accent to stand out from the fields
// as much as its text does from it.
const ThemeColors Dark = {
    .space = QColor(0x0D0B1E),
    .field = QColor(0x15122B),
    .alternateField = QColor(0x1D1939),
    .surface = QColor(0x262046),
    .edge = QColor(0x7A6FB0),
    .text = QColor(0xEEEAFE),
    .mutedText = QColor(0xB3AAD9),
    .disabledText = QColor(0x6F6797),
    .accent = QColor(0xA78BFA),
    .onAccent = QColor(0x0D0B1E),
    .link = QColor(0x8AB4FF),
    .visitedLink = QColor(0xF49AD3),
    .warning = QColor(0xFBBF24),
    .onWarning = QColor(0x0D0B1E),
};

// Its dawn.
const ThemeColors Light = {
    .space = QColor(0xF4F2FC),
    .field = QColor(0xFFFFFF),
    .alternateField = QColor(0xF0ECFB),
    .surface = QColor(0xE8E3F8),
    .edge = QColor(0x8579B8),
    .text = QColor(0x1A1530),
    .mutedText = QColor(0x574F7A),
    .disabledText = QColor(0xA39CBF),
    .accent = QColor(0x6D28D9),
    .onAccent = QColor(0xFFFFFF),
    .link = QColor(0x1D4ED8),
    .visitedLink = QColor(0xA21C6B),
    .warning = QColor(0x9A4A00),
    .onWarning = QColor(0xFFFFFF),
};

// The icons of the terms of the glossary, in the order of GlossaryTerm.
const QStringList IconFiles = {QStringLiteral(":/icons/universe.svg"), QStringLiteral(":/icons/galaxy.svg"),
                               QStringLiteral(":/icons/astre.svg"), QStringLiteral(":/icons/comet.svg"),
                               QStringLiteral(":/icons/stargate.svg")};

// Roles of the palette, each with its color.
using RoleColors = QList<std::pair<QPalette::ColorRole, QColor>>;

// What is behind the texts of a scheme.
RoleColors backgroundRoles(const ThemeColors& colors)
{
    return {{QPalette::Window, colors.space},
            {QPalette::Base, colors.field},
            {QPalette::AlternateBase, colors.alternateField},
            {QPalette::Button, colors.surface},
            {QPalette::ToolTipBase, colors.surface}};
}

// The texts of a scheme.
RoleColors textRoles(const ThemeColors& colors)
{
    return {{QPalette::WindowText, colors.text},           {QPalette::Text, colors.text},
            {QPalette::PlaceholderText, colors.mutedText}, {QPalette::ButtonText, colors.text},
            {QPalette::BrightText, colors.text},           {QPalette::ToolTipText, colors.text}};
}

// The bevels of the style in the colors of a scheme.
RoleColors bevelRoles(const ThemeColors& colors)
{
    // How much the bevels of the style are lighter or darker than the surface, in percent.
    constexpr int BevelFactor = 130;
    return {{QPalette::Light, colors.surface.lighter(BevelFactor)},
            {QPalette::Midlight, colors.surface.lighter((100 + BevelFactor) / 2)},
            {QPalette::Mid, colors.edge},
            {QPalette::Dark, colors.edge.darker(BevelFactor)},
            {QPalette::Shadow, colors.space.darker(BevelFactor)}};
}

// What stands out in a scheme: the selection and the links.
RoleColors accentRoles(const ThemeColors& colors)
{
    return {{QPalette::Highlight, colors.accent},
            {QPalette::HighlightedText, colors.onAccent},
            {QPalette::Accent, colors.accent},
            {QPalette::Link, colors.link},
            {QPalette::LinkVisited, colors.visitedLink}};
}

// What changes in a scheme when it is disabled.
RoleColors disabledRoles(const ThemeColors& colors)
{
    return {{QPalette::WindowText, colors.disabledText}, {QPalette::Text, colors.disabledText},
            {QPalette::ButtonText, colors.disabledText}, {QPalette::PlaceholderText, colors.disabledText},
            {QPalette::Highlight, colors.edge},          {QPalette::HighlightedText, colors.text}};
}

// Gives roles their color in group of palette.
void setColors(QPalette& palette, QPalette::ColorGroup group, const RoleColors& roles)
{
    for(const auto& [role, color] : roles)
    {
        palette.setColor(group, role, color);
    }
}

// The roles of the palette in the colors of a scheme, the same whether its window is active or not.
QPalette palette(const ThemeColors& colors)
{
    QPalette palette;
    for(const RoleColors& roles : {backgroundRoles(colors), textRoles(colors), bevelRoles(colors), accentRoles(colors)})
    {
        setColors(palette, QPalette::All, roles);
    }
    setColors(palette, QPalette::Disabled, disabledRoles(colors));
    return palette;
}

void applyColorScheme(Qt::ColorScheme scheme)
{
    QApplication::setPalette(palette(themeColors(scheme)));
}

} // namespace

const ThemeColors& themeColors(Qt::ColorScheme scheme)
{
    return scheme == Qt::ColorScheme::Light ? Light : Dark;
}

const ThemeColors& currentThemeColors()
{
    return themeColors(QGuiApplication::styleHints()->colorScheme());
}

void applyTheme()
{
    // The one style of Qt that paints all it draws in the colors of the palette, on every platform.
    QApplication::setStyle(QStringLiteral("Fusion"));
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/SpaceGrotesk-Bold.ttf"));

    QStyleHints* hints = QGuiApplication::styleHints();
    applyColorScheme(hints->colorScheme());
    QObject::connect(hints, &QStyleHints::colorSchemeChanged, qApp, &applyColorScheme);
}

QFont titleFont(const QFont& font)
{
    QFont title = font;
    title.setFamilies({TitleFamily, font.family()}); // The second one when the first could not be loaded
    title.setBold(true);
    return title;
}

QIcon icon(GlossaryTerm term)
{
    return QIcon(IconFiles.value(static_cast<qsizetype>(term)));
}

QString logoPath()
{
    return QStringLiteral(":/nebula.svg");
}
