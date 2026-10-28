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

// Appends to about format, with items, unless there is none.
void appendList(QStringList& about, const QString& format, const QStringList& items)
{
    if(!items.isEmpty())
    {
        about.append(format.arg(items.join(QStringLiteral(", "))));
    }
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
    refresh();
}

Nebula::Stargate& StargateItem::stargate() const
{
    return m_stargate;
}

void StargateItem::refreshLinks()
{
    QList<const Nebula::Component*> exposed; // The components it exposes properties of
    for(const Nebula::ExposedProperty& property : m_stargate.properties())
    {
        exposed.append(property.component);
    }
    m_links.refresh(exposed);
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
    setRows(propertyRows());
    setOpenable(aboutText(), tr("Double-click below its name to choose what it exposes"));
}

QList<ComponentItem::Row> StargateItem::propertyRows()
{
    QList<Row> rows;
    for(const Nebula::ExposedProperty& property : m_stargate.properties())
    {
        rows.append({property.name, placeOf(property)});
        // Where it is, should its component be renamed.
        connect(property.component, &QObject::objectNameChanged, this, &StargateItem::refresh, Qt::UniqueConnection);
    }
    return rows;
}

QString StargateItem::aboutText() const
{
    QStringList about{tr("Stargate: what the galaxy of its file has once placed")};
    appendList(about, tr("Provides %1"), interfacesText(Nebula::Exposure::Provided));
    appendList(about, tr("Consumes %1"), interfacesText(Nebula::Exposure::Consumed));
    appendList(about, tr("Exposes %1"), propertiesText());
    if(about.size() == 1)
    {
        about.append(tr("Exposes nothing yet"));
    }
    return about.join(u'\n');
}

QStringList StargateItem::interfacesText(Nebula::Exposure exposure) const
{
    QStringList interfaces;
    for(const Nebula::ExposedInterface& exposed : m_stargate.interfaces())
    {
        if(exposed.exposure == exposure)
        {
            interfaces.append(tr("%1 (%2)").arg(exposed.name, InterfaceItem::typeName(exposed.type)));
        }
    }
    return interfaces;
}

QStringList StargateItem::propertiesText() const
{
    QStringList properties;
    for(const Nebula::ExposedProperty& property : m_stargate.properties())
    {
        properties.append(tr("%1 (%2)").arg(property.name, placeOf(property)));
    }
    return properties;
}
