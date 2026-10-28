// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "InterfaceItem.h"

#include <QApplication>
#include <QFontMetricsF>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QToolTip>
#include <QWidget>
#include <numbers>

#include "Colors.h"
#include "ComponentItem.h"
#include "LinkItem.h"
#include "Nebula/Component/InterfaceInfo.h"
#include "Theme.h"

namespace
{

constexpr qreal DotRadius = 4.5;
constexpr qreal DotOutline = 1.5;
// How many dots make the outline of an unavailable interface.
constexpr int OutlineDots = 6;
// Around the dot, where hovering still shows its tooltip.
constexpr qreal HoverMargin = 3;
// Between the dot and the name.
constexpr qreal Spacing = 5;
// How far out of the component the circle behind the one of a list is.
constexpr qreal ListOffset = 3.5;
// How much of its color a dimmed interface keeps.
constexpr float DimmedOpacity = 0.3F;

// An outline of the dot made of OutlineDots dashes, as long as the gaps between them.
QPen dottedOutline(const QColor& color)
{
    const qreal step = 2 * std::numbers::pi * DotRadius / (2 * OutlineDots);
    QPen pen(color, DotOutline);
    pen.setCapStyle(Qt::FlatCap);
    pen.setDashPattern({step / DotOutline, step / DotOutline}); // In units of the width
    return pen;
}

} // namespace

InterfaceItem::InterfaceItem(Kind kind, const Nebula::InterfaceInfo& info, qreal nameWidth, ComponentItem& parent)
    : QGraphicsItem(&parent), m_componentItem(parent), m_kind(kind), m_name(info.name), m_type(info.type),
      m_mandatory(info.requirement == Nebula::InterfaceRequirement::Mandatory), m_list(info.list),
      m_nameWidth(nameWidth)
{
    setFlag(ItemSendsScenePositionChanges); // For its links to follow it
    setAcceptHoverEvents(true);
    setCursor(Qt::CrossCursor);
    setAvailable(info.available);
}

InterfaceItem::~InterfaceItem()
{
    // Each one removes itself from the list.
    while(!m_links.isEmpty())
    {
        delete m_links.constFirst();
    }
}

int InterfaceItem::type() const
{
    return Type;
}

InterfaceItem::Kind InterfaceItem::kind() const
{
    return m_kind;
}

const QString& InterfaceItem::name() const
{
    return m_name;
}

bool InterfaceItem::isList() const
{
    return m_list;
}

Nebula::Component& InterfaceItem::component() const
{
    return m_componentItem.component();
}

bool InterfaceItem::canLinkTo(const InterfaceItem& other) const
{
    return m_kind != other.m_kind && m_type == other.m_type && &component() != &other.component();
}

QString InterfaceItem::typeName(QMetaType type)
{
    // The type is the one of the pointer: what it points to is the interface.
    QString name = QString::fromLatin1(type.name());
    if(name.endsWith(u'*'))
    {
        name.chop(1);
    }
    return name;
}

void InterfaceItem::setAvailable(bool available)
{
    m_available = available;
    refreshToolTip();
}

void InterfaceItem::setMissing(bool missing)
{
    m_missing = missing;
    refreshToolTip();
}

void InterfaceItem::refreshToolTip()
{
    setToolTip(QStringLiteral("%1%2\n%3").arg(kindText(), requirementText(), stateText()));
    update();
}

QString InterfaceItem::kindText() const
{
    if(m_kind == Kind::Provided)
    {
        return tr("Provides %1").arg(typeName(m_type));
    }
    return m_list ? tr("Consumes a list of %1: several interfaces can be bound to it").arg(typeName(m_type))
                  : tr("Consumes %1").arg(typeName(m_type));
}

QString InterfaceItem::requirementText() const
{
    if(!m_mandatory)
    {
        return {};
    }
    return u'\n' + (m_list ? tr("Mandatory: the component does not work without one at least")
                           : tr("Mandatory: the component does not work without it"));
}

QString InterfaceItem::stateText() const
{
    const QString state = m_available ? tr("Available") : tr("Unavailable");
    if(!m_missing)
    {
        return state;
    }
    return state + u'\n' +
           tr("Bound to nothing: drag from this dot to a %1 interface another component provides")
               .arg(typeName(m_type));
}

void InterfaceItem::setDimmed(bool dimmed)
{
    if(m_dimmed == dimmed)
    {
        return;
    }
    m_dimmed = dimmed;
    update();
}

const QList<LinkItem*>& InterfaceItem::links() const
{
    return m_links;
}

void InterfaceItem::addLink(LinkItem& link)
{
    m_links.append(&link);
}

void InterfaceItem::removeLink(LinkItem& link)
{
    m_links.removeOne(&link);
}

QRectF InterfaceItem::nameRect() const
{
    const qreal offset = DotRadius + Spacing;
    return {m_kind == Kind::Provided ? -offset - m_nameWidth : offset, -RowHeight / 2, m_nameWidth, RowHeight};
}

QRectF InterfaceItem::boundingRect() const
{
    // From what shape() takes around its dot, or the circle behind it for a list, to its name, inside the component.
    const qreal dot = DotRadius + HoverMargin + (m_list ? ListOffset : 0);
    const qreal width = dot + DotRadius + Spacing + m_nameWidth;
    return {m_kind == Kind::Provided ? dot - width : -dot, -RowHeight / 2, width, RowHeight};
}

QPainterPath InterfaceItem::shape() const
{
    QPainterPath path;
    path.addEllipse(QPointF(0, 0), DotRadius + HoverMargin, DotRadius + HoverMargin);
    return path;
}

void InterfaceItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* widget)
{
    const QPalette palette = widget != nullptr ? widget->palette() : QApplication::palette();
    // The colors of the component, as ComponentItem paints them over the background of the edition zone. Opaque: the
    // colors of a style can be translucent, which would let the edge of the component show through the dot.
    const QColor background = over(palette.color(QPalette::Button), palette.color(QPalette::Base));
    const QColor color = over(inkColor(palette), background);
    paintDot(*painter, color, background);
    if(!m_componentItem.isNameEdited(*this)) // Its name is the text being edited otherwise
    {
        paintName(*painter, color);
    }
}

QColor InterfaceItem::inkColor(const QPalette& palette) const
{
    // In the warning color while the component misses it, which the asterisk after its name tells without color.
    // Dimmed, it fades into the component, but stays opaque over its edge.
    QColor ink = m_missing ? currentThemeColors().warning : palette.color(QPalette::ButtonText);
    ink.setAlphaF(m_dimmed ? ink.alphaF() * DimmedOpacity : ink.alphaF());
    return ink;
}

void InterfaceItem::paintDot(QPainter& painter, const QColor& color, const QColor& background) const
{
    const bool provided = m_kind == Kind::Provided;
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(provided && !m_available ? dottedOutline(color) : QPen(color, DotOutline));
    painter.setBrush(provided && m_available ? color : background);
    if(m_list) // Behind, out of the component, which a list is consumed from
    {
        painter.drawEllipse(QPointF(-ListOffset, 0), DotRadius, DotRadius);
    }
    painter.drawEllipse(QPointF(0, 0), DotRadius, DotRadius);
}

void InterfaceItem::paintName(QPainter& painter, const QColor& color) const
{
    const QRectF rect = nameRect();
    // A mandatory interface is marked as a required field is, by an asterisk after its name, which eliding keeps.
    const QString mark = m_mandatory ? QStringLiteral(" *") : QString();
    const QFontMetricsF metrics(painter.font());
    const QString name = metrics.elidedText(m_name, Qt::ElideRight, rect.width() - metrics.horizontalAdvance(mark));
    painter.setPen(color);
    const Qt::Alignment side = m_kind == Kind::Provided ? Qt::AlignRight : Qt::AlignLeft;
    painter.drawText(rect, side | Qt::AlignVCenter, name + mark);
}

void InterfaceItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    // At once, without the delay of a tooltip: its dot is small, and its type is what is looked for there.
    QToolTip::showText(event->screenPos(), toolTip(), event->widget());
}

void InterfaceItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* /*event*/)
{
    QToolTip::hideText();
}

QVariant InterfaceItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if(change == ItemScenePositionHasChanged)
    {
        for(LinkItem* link : std::as_const(m_links))
        {
            link->follow();
        }
    }
    return QGraphicsItem::itemChange(change, value);
}
