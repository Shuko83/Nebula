// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QGraphicsItem>
#include <QGraphicsObject>
#include <QList>

// How the items of the edition zone are found among others, by their type.

// The graphics objects among items that are T, a QObject: a component, a stargate or a settings file. In their order.
template <typename T> [[nodiscard]] QList<T*> objectsAmong(const QList<QGraphicsItem*>& items)
{
    QList<T*> found;
    for(QGraphicsItem* item : items)
    {
        if(auto* typed = qobject_cast<T*>(item->toGraphicsObject()))
        {
            found.append(typed);
        }
    }
    return found;
}

// The graphics object items has, when it has a single item and it is T, a QObject; nullptr otherwise. The one selected
// alone, say.
template <typename T> [[nodiscard]] T* onlyObjectAmong(const QList<QGraphicsItem*>& items)
{
    return items.size() == 1 ? qobject_cast<T*>(items.constFirst()->toGraphicsObject()) : nullptr;
}

// The items among items that are T, which declares its Type: an interface, a link or a zone. In their order.
template <typename T> [[nodiscard]] QList<T*> itemsAmong(const QList<QGraphicsItem*>& items)
{
    QList<T*> found;
    for(QGraphicsItem* item : items)
    {
        if(auto* typed = qgraphicsitem_cast<T*>(item))
        {
            found.append(typed);
        }
    }
    return found;
}

// The first of items that is T, which declares its Type; nullptr when none is.
template <typename T> [[nodiscard]] T* firstAmong(const QList<QGraphicsItem*>& items)
{
    const QList<T*> found = itemsAmong<T>(items);
    return found.isEmpty() ? nullptr : found.constFirst();
}
