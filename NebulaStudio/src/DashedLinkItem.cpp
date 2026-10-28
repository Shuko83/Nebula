// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "DashedLinkItem.h"

#include <QApplication>
#include <QGraphicsObject>
#include <QGraphicsScene>
#include <QPainter>
#include <QPalette>
#include <QWidget>
#include <utility>

#include "ComponentItem.h"

namespace
{

constexpr qreal LineWidth = 1;

} // namespace

DashedLinkItem::DashedLinkItem(QGraphicsItem& from, ComponentItem& component,
                               std::function<void(DashedLinkItem&)> removed)
    : m_from(from), m_component(&component), m_removed(std::move(removed))
{
    setZValue(-1); // Under the items it joins, as the links of interfaces are
    follow();
}

DashedLinkItem::~DashedLinkItem()
{
    m_removed(*this);
}

ComponentItem* DashedLinkItem::component() const
{
    return m_component;
}

void DashedLinkItem::follow()
{
    if(m_component == nullptr) // Gone before its component: it is about to go too
    {
        return;
    }
    prepareGeometryChange();
    m_line = QLineF(m_from.scenePos(), m_component->scenePos());
}

QRectF DashedLinkItem::boundingRect() const
{
    return QRectF(m_line.p1(), m_line.p2()).normalized().adjusted(-LineWidth, -LineWidth, LineWidth, LineWidth);
}

void DashedLinkItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* widget)
{
    const QPalette palette = widget != nullptr ? widget->palette() : QApplication::palette();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(palette.color(QPalette::Mid), LineWidth, Qt::DashLine));
    painter->drawLine(m_line);
}

DashedLinks::DashedLinks(QGraphicsObject& from) : m_from(from)
{
    connect(&m_from, &QGraphicsObject::xChanged, this, &DashedLinks::follow);
    connect(&m_from, &QGraphicsObject::yChanged, this, &DashedLinks::follow);
}

DashedLinks::~DashedLinks()
{
    clear();
}

void DashedLinks::refresh(const QList<const Nebula::Component*>& components)
{
    clear();
    const QList<ComponentItem*> items =
        m_from.scene() != nullptr ? componentItems(*m_from.scene()) : QList<ComponentItem*>();
    for(ComponentItem* item : items)
    {
        if(components.contains(&item->component()))
        {
            join(*item);
        }
    }
}

void DashedLinks::follow()
{
    for(DashedLinkItem* link : std::as_const(m_links))
    {
        link->follow();
    }
}

void DashedLinks::clear()
{
    while(!m_links.isEmpty())
    {
        delete m_links.constFirst(); // Which removes it
    }
}

void DashedLinks::join(ComponentItem& component)
{
    // As it moves, as its item does too.
    connect(&component, &QGraphicsObject::xChanged, this, &DashedLinks::follow, Qt::UniqueConnection);
    connect(&component, &QGraphicsObject::yChanged, this, &DashedLinks::follow, Qt::UniqueConnection);
    auto* link =
        new DashedLinkItem(m_from, component, [this](DashedLinkItem& removed) { m_links.removeOne(&removed); });
    m_links.append(link);
    m_from.scene()->addItem(link);
}
