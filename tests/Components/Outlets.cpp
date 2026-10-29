// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Outlets.h"

#include <algorithm>

#include "Options.h"

namespace
{

// The name of the interface of the object number, from 1, and of the group of its properties.
QString outletName(qsizetype number)
{
    return QStringLiteral("object%1").arg(number);
}

} // namespace

Outlets::Outlets() = default;

Outlets::~Outlets() = default;

int Outlets::count() const
{
    return static_cast<int>(m_objects.size());
}

void Outlets::setCount(int count)
{
    count = std::max(count, 0);
    if(count == this->count())
    {
        return;
    }
    resize(count);
    setActive(m_active); // No more than count
    emit countChanged();
}

int Outlets::active() const
{
    return m_active;
}

void Outlets::setActive(int active)
{
    active = std::clamp(active, 0, count());
    if(active != m_active)
    {
        m_active = active;
        emit activeChanged();
    }
}

QList<Options*> Outlets::objects() const
{
    QList<Options*> objects;
    for(const std::unique_ptr<Options>& object : m_objects)
    {
        objects.append(object.get());
    }
    return objects;
}

void Outlets::resize(int count)
{
    while(this->count() < count)
    {
        addOutlet();
    }
    while(this->count() > count)
    {
        removeOutlet();
    }
}

void Outlets::addOutlet()
{
    const QString name = outletName(count() + 1);
    Options& object = *m_objects.emplace_back(std::make_unique<Options>());
    object.setObjectName(name);
    addPropertiesForQObject(&object, name);
    provideInterface<QObject>(name, &object);
}

void Outlets::removeOutlet()
{
    removeProvidedInterface(outletName(count())); // While what is bound to it can still let go of the object
    m_objects.pop_back();
}
