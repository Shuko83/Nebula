// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPen>
#include <QRectF>

// How the items of the edition zone are drawn alike: components, settings files and zones.

// The outline of an item whose bounds are bounds: one pixel inside them, its corners rounded as those of a component.
inline QPainterPath roundedOutline(const QRectF& bounds)
{
    constexpr qreal CornerRadius = 6;
    QPainterPath outline;
    outline.addRoundedRect(bounds.adjusted(1, 1, -1, -1), CornerRadius, CornerRadius);
    return outline;
}

// The pen of the outline of an item: in the highlight color when it is selected, thicker then.
inline QPen outlinePen(const QPalette& palette, bool selected)
{
    return selected ? QPen(palette.color(QPalette::Highlight), 2) : QPen(palette.color(QPalette::Mid), 1);
}

// Fills the part of outline in rect with color.
inline void fillWithin(QPainter& painter, const QPainterPath& outline, const QRectF& rect, const QColor& color)
{
    QPainterPath part;
    part.addRect(rect);
    painter.fillPath(outline.intersected(part), color);
}
