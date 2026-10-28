// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "SettingsItem.h"

#include <QApplication>
#include <QDir>
#include <QFontMetricsF>
#include <QGraphicsScene>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QStringList>
#include <QStyleOptionGraphicsItem>
#include <algorithm>

#include "ComponentItem.h"
#include "DashedLinkItem.h"
#include "ItemShapes.h"
#include "Nebula/Component/Component.h"
#include "Nebula/Settings/SettingsFile.h"
#include "Theme.h"

namespace
{

// The sheet, centered on the position of the item, of the size of the names of a component.
constexpr QRectF Sheet(-80, -28, 160, 56);
constexpr qreal Padding = 8;
// The side of its folded corner, its top right one.
constexpr qreal Fold = 12;
// Where the name of its file is written, above "Settings file", clear of the fold.
constexpr QRectF Names = Sheet.adjusted(Padding, Padding, -Padding, -Padding);
constexpr QRectF NameRect(Names.left() + Fold, Names.top(), Names.width() - 2 * Fold, Names.height() / 2);
constexpr QRectF KindRect(Names.left(), NameRect.bottom(), Names.width(), Names.height() / 2);
// The sheet drawn, one pixel inside its bounds, as a component, and the ends of its fold.
constexpr QRectF Paper = Sheet.adjusted(1, 1, -1, -1);
constexpr QPointF FoldStart(Paper.right() - Fold, Paper.top());
constexpr QPointF FoldEnd(Paper.right(), Paper.top() + Fold);

// The outline of the sheet, its top right corner folded.
QPainterPath sheetOutline()
{
    QPainterPath outline(Paper.topLeft());
    outline.lineTo(FoldStart);
    outline.lineTo(FoldEnd);
    outline.lineTo(Paper.bottomRight());
    outline.lineTo(Paper.bottomLeft());
    outline.closeSubpath();
    return outline;
}

// The sheet of a settings file, highlighted when it is selected.
void paintSheet(QPainter& painter, const QPalette& palette, bool selected)
{
    const QPainterPath outline = sheetOutline();
    painter.fillPath(outline, palette.color(QPalette::Base));
    painter.setPen(outlinePen(palette, selected));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(outline);
    painter.drawPolyline(QPolygonF({FoldStart, QPointF(FoldStart.x(), FoldEnd.y()), FoldEnd}));
}

// The property of entry as its tooltip lists it: <component>.<property>, with the name of its object between them
// when it is of a named one.
QString entryName(const Nebula::SettingsEntry& entry)
{
    QString name = entry.component->objectName() + u'.';
    const QList<Nebula::PropertyObject> objects = entry.component->propertyObjects();
    const auto object = std::ranges::find(objects, entry.object, &Nebula::PropertyObject::object);
    if(object != objects.end() && !object->name.isEmpty())
    {
        name += object->name + u'.';
    }
    return name + entry.property;
}

} // namespace

SettingsItem::SettingsItem(Nebula::SettingsFile& file) : m_file(file)
{
    setFlags(ItemIsMovable | ItemIsSelectable);
    connect(&m_file, &Nebula::SettingsFile::changed, this,
            [this]
            {
                refreshLinks();
                refreshToolTip();
                update(); // Its name
            });
    refreshToolTip();
}

Nebula::SettingsFile& SettingsItem::file() const
{
    return m_file;
}

QRectF SettingsItem::boundingRect() const
{
    return Sheet;
}

void SettingsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    const QPalette palette = widget != nullptr ? widget->palette() : QApplication::palette();
    painter->setRenderHint(QPainter::Antialiasing);
    paintSheet(*painter, palette, option->state.testFlag(QStyle::State_Selected));
    paintNames(*painter, palette);
}

void SettingsItem::paintNames(QPainter& painter, const QPalette& palette) const
{
    const QFont font = painter.font();
    const QFont title = titleFont(font);
    painter.setFont(title);
    painter.setPen(palette.color(QPalette::Text));
    painter.drawText(NameRect, Qt::AlignCenter,
                     QFontMetricsF(title).elidedText(m_file.fileName(), Qt::ElideMiddle, NameRect.width()));
    painter.setFont(font);
    painter.setPen(palette.color(QPalette::PlaceholderText));
    painter.drawText(KindRect, Qt::AlignCenter, tr("Settings file"));
}

QVariant SettingsItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if(change == ItemSceneHasChanged)
    {
        refreshLinks();
    }
    return QGraphicsObject::itemChange(change, value);
}

void SettingsItem::refreshLinks()
{
    QList<const Nebula::Component*> held; // The components it holds properties of
    for(const Nebula::SettingsEntry& entry : m_file.entries())
    {
        held.append(entry.component);
    }
    m_links.refresh(held);
}

void SettingsItem::refreshToolTip()
{
    QStringList lines{QDir::toNativeSeparators(m_file.filePath())};
    for(const Nebula::SettingsEntry& entry : m_file.entries())
    {
        lines.append(entryName(entry));
    }
    if(m_file.entries().isEmpty())
    {
        lines.append(tr("No property yet: double-click to choose those it holds"));
    }
    setToolTip(lines.join(u'\n'));
}
