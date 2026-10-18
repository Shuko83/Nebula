// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "StargateItem.h"

#include <QGraphicsScene>
#include <QStringList>
#include <algorithm>

#include "DashedLinkItem.h"
#include "InterfaceItem.h"
#include "Nebula/Galaxy/Stargate.h"
#include "Theme.h"

namespace
{

// Where property is, as its row tells it: <component>.<property>, with the name of its object between them when it is
// of a named one.
QString placeOf(const Nebula::ExposedProperty& property)
{
    QString place = property.component->objectName() + u'.';
    const QList<Nebula::PropertyObject> objects = property.component->propertyObjects();
    const auto object = std::ranges::find(objects, property.object, &Nebula::PropertyObject::object);
    if(object != objects.end() && !object->name.isEmpty())
    {
        place += object->name + u'.';
    }
    return place + property.property;
}

} // namespace

StargateItem::StargateItem(Nebula::Stargate& stargate) : ComponentItem(stargate, tr("Stargate")), m_stargate(stargate)
{
    setIcon(icon(GlossaryTerm::Stargate));
    setInterfacesRenamable(true);
    connect(&m_stargate, &Nebula::Stargate::changed, this,
            [this]
            {
                refresh();
                refreshLinks();
            });
    connect(this, &QGraphicsObject::xChanged, this, &StargateItem::followLinks);
    connect(this, &QGraphicsObject::yChanged, this, &StargateItem::followLinks);
    refresh();
}

StargateItem::~StargateItem()
{
    while(!m_links.isEmpty())
    {
        delete m_links.constFirst(); // Which removes it
    }
}

Nebula::Stargate& StargateItem::stargate() const
{
    return m_stargate;
}

void StargateItem::refreshLinks()
{
    while(!m_links.isEmpty())
    {
        delete m_links.constFirst();
    }
    if(scene() == nullptr)
    {
        return;
    }
    QList<const Nebula::Component*> exposed; // The components it exposes properties of
    for(const Nebula::ExposedProperty& property : m_stargate.properties())
    {
        exposed.append(property.component);
    }
    const QList<QGraphicsItem*> items = scene()->items();
    for(QGraphicsItem* item : items)
    {
        auto* component = qobject_cast<ComponentItem*>(item->toGraphicsObject());
        if(component == nullptr || !exposed.contains(&component->component()))
        {
            continue;
        }
        // As it moves, as it does too.
        connect(component, &QGraphicsObject::xChanged, this, &StargateItem::followLinks, Qt::UniqueConnection);
        connect(component, &QGraphicsObject::yChanged, this, &StargateItem::followLinks, Qt::UniqueConnection);
        auto* link =
            new DashedLinkItem(*this, *component, [this](DashedLinkItem& removed) { m_links.removeOne(&removed); });
        m_links.append(link);
        scene()->addItem(link);
    }
}

QVariant StargateItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if(change == ItemSceneHasChanged)
    {
        refreshLinks();
    }
    return ComponentItem::itemChange(change, value);
}

void StargateItem::refresh()
{
    QList<Row> rows;
    QStringList exposes;
    for(const Nebula::ExposedProperty& property : m_stargate.properties())
    {
        rows.append({property.name, placeOf(property)});
        exposes.append(tr("%1 (%2)").arg(property.name, placeOf(property)));
        // Where it is, should its component be renamed.
        connect(property.component, &QObject::objectNameChanged, this, &StargateItem::refresh, Qt::UniqueConnection);
    }
    setRows(rows);

    // What its galaxy has through it, the other way round from its own interfaces.
    QStringList provides;
    QStringList consumes;
    for(const Nebula::ExposedInterface& exposed : m_stargate.interfaces())
    {
        (exposed.exposure == Nebula::Exposure::Provided ? provides : consumes)
            .append(tr("%1 (%2)").arg(exposed.name, InterfaceItem::typeName(exposed.type)));
    }
    QStringList about{tr("Stargate: what the galaxy of its file has once placed")};
    if(!provides.isEmpty())
    {
        about.append(tr("Provides %1").arg(provides.join(QStringLiteral(", "))));
    }
    if(!consumes.isEmpty())
    {
        about.append(tr("Consumes %1").arg(consumes.join(QStringLiteral(", "))));
    }
    if(!exposes.isEmpty())
    {
        about.append(tr("Exposes %1").arg(exposes.join(QStringLiteral(", "))));
    }
    if(provides.isEmpty() && consumes.isEmpty() && exposes.isEmpty())
    {
        about.append(tr("Exposes nothing yet"));
    }
    setOpenable(about.join(u'\n'), tr("Double-click below its name to choose what it exposes"));
}

void StargateItem::followLinks()
{
    for(DashedLinkItem* link : std::as_const(m_links))
    {
        link->follow();
    }
}
