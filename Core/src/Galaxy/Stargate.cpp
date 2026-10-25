// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Nebula/Galaxy/Stargate.h"

#include <QMetaProperty>
#include <algorithm>

#include "Application/ProjectJson.h"
#include "Nebula/Application/Universe.h"
#include "Nebula/Galaxy/Galaxy.h"

namespace Nebula
{

namespace
{

// Whether two properties exposed are the same property of the same object, whatever they are exposed as.
bool isSameProperty(const ExposedProperty& first, const ExposedProperty& second)
{
    return first.component == second.component && first.object == second.object && first.property == second.property;
}

} // namespace

Stargate::Stargate(Universe& application, Galaxy* galaxy) : m_application(application), m_galaxy(galaxy) {}

Stargate::~Stargate()
{
    detach();
    releaseInterfaces(); // While it is whole: what it consumes calls it back
}

QString Stargate::componentId() const
{
    return Id;
}

QList<ExposedInterface> Stargate::interfaces() const
{
    QList<ExposedInterface> interfaces;
    interfaces.reserve(m_passages.size());
    for(const Passage& passage : m_passages)
    {
        interfaces.append(passage.info);
    }
    return interfaces;
}

const ExposedInterface* Stargate::exposedInterface(Exposure exposure, const QString& name) const
{
    const auto found = std::ranges::find_if(m_passages, [exposure, &name](const Passage& passage)
                                            { return passage.info.exposure == exposure && passage.info.name == name; });
    return found != m_passages.end() ? &found->info : nullptr;
}

bool Stargate::exposeInterface(Exposure exposure, const QString& name, QMetaType type)
{
    if(!isChangeable() || !type.isValid() || name.isEmpty() || !isFreeInterfaceName(exposure, name))
    {
        return false;
    }
    const int id = m_nextId++;
    m_passages.append(
        Passage{.info = {.name = name, .type = type, .exposure = exposure}, .id = id, .pointer = nullptr});
    if(exposure == Exposure::Provided)
    {
        // What a component of its galaxy provides to it, its galaxy provides, once it is placed.
        const auto take = [this, id](void* pointer)
        {
            if(Passage* taken = passage(id))
            {
                taken->pointer = pointer;
            }
        };
        const auto pass = [this, id]
        {
            const Passage* passed = passage(id);
            if(m_attached && passed != nullptr)
            {
                m_galaxy->provide(passed->info.name, passed->info.type, passed->pointer);
            }
        };
        const auto withdraw = [this, id]
        {
            const Passage* withdrawn = passage(id);
            if(m_attached && withdrawn != nullptr)
            {
                m_galaxy->provide(withdrawn->info.name, withdrawn->info.type, nullptr);
            }
        };
        consume(name, type, InterfaceRequirement::Mandatory, take, pass, withdraw);
        if(m_attached)
        {
            m_galaxy->provide(name, type, nullptr); // Once it is given it
        }
    }
    else
    {
        provide(name, type, nullptr); // What its galaxy is given, once it is placed, see attach()
        if(m_attached)
        {
            consumeForGalaxy(m_passages.constLast(), InterfaceRequirement::Optional); // Bound to nothing yet
        }
    }
    emit changed();
    return true;
}

bool Stargate::renameInterface(Exposure exposure, const QString& name, const QString& newName)
{
    Passage* renamed = passage(exposure, name);
    if(renamed == nullptr || !isChangeable() || newName.isEmpty())
    {
        return false;
    }
    if(newName == name)
    {
        return true;
    }
    if(!isFreeInterfaceName(exposure, newName))
    {
        return false;
    }
    // Before the interface: what it tells reads both.
    renamed->info.name = newName;
    if(exposure == Exposure::Provided)
    {
        renameConsumedInterface(name, newName);
        if(m_attached)
        {
            m_galaxy->renameProvidedInterface(name, newName); // What is bound to it follows
        }
    }
    else
    {
        renameProvidedInterface(name, newName);
        if(m_attached)
        {
            m_galaxy->renameConsumedInterface(name, newName); // Still bound to what it was
        }
    }
    emit changed();
    return true;
}

void Stargate::removeInterface(Exposure exposure, const QString& name)
{
    if(!isChangeable() || passage(exposure, name) == nullptr)
    {
        return;
    }
    // While it is still a passage: what it passes is withdrawn first, on both sides.
    if(exposure == Exposure::Provided)
    {
        removeConsumedInterface(name);
        if(m_attached)
        {
            m_galaxy->removeProvidedInterface(name);
        }
    }
    else
    {
        if(m_attached)
        {
            m_galaxy->removeConsumedInterface(name);
        }
        removeProvidedInterface(name);
    }
    m_passages.removeIf([exposure, &name](const Passage& passage)
                        { return passage.info.exposure == exposure && passage.info.name == name; });
    emit changed();
}

const QList<ExposedProperty>& Stargate::properties() const
{
    return m_properties;
}

const ExposedProperty* Stargate::property(const QString& name) const
{
    const auto found = std::ranges::find(m_properties, name, &ExposedProperty::name);
    return found != m_properties.end() ? &*found : nullptr;
}

bool Stargate::exposeProperty(const ExposedProperty& property)
{
    if(!isChangeable() || property.name.isEmpty() || !isFreePropertyName(property.name) ||
       property.component == nullptr || property.object == nullptr)
    {
        return false;
    }
    const auto known = [&property](const std::unique_ptr<Component>& component)
    { return component.get() == property.component; };
    const std::vector<std::unique_ptr<Component>>& components = galaxyComponents();
    if(!std::ranges::any_of(components, known) || qobject_cast<const Stargate*>(property.component) != nullptr)
    {
        return false;
    }
    // One a galaxy exposes, or one a project holds.
    if(const auto* galaxy = qobject_cast<const Galaxy*>(property.component))
    {
        if(property.object != galaxy || !galaxy->exposedProperty(property.property))
        {
            return false;
        }
    }
    else
    {
        const QList<QMetaProperty> saved = ProjectJson::savedProperties(*property.component, property.object);
        if(!std::ranges::any_of(saved, [&property](const QMetaProperty& held)
                                { return QLatin1StringView(held.name()) == property.property; }))
        {
            return false;
        }
    }
    // Once in its galaxy: it would be set twice.
    for(const std::unique_ptr<Component>& component : components)
    {
        const auto* stargate = qobject_cast<const Stargate*>(component.get());
        if(stargate != nullptr &&
           std::ranges::any_of(stargate->properties(), [&property](const ExposedProperty& exposed)
                               { return isSameProperty(exposed, property); }))
        {
            return false;
        }
    }
    m_properties.append(property);
    connect(property.component, &QObject::destroyed, this, &Stargate::forget, Qt::UniqueConnection);
    connect(property.object, &QObject::destroyed, this, &Stargate::forget, Qt::UniqueConnection);
    emit changed();
    return true;
}

bool Stargate::renameProperty(const QString& name, const QString& newName)
{
    const auto renamed = std::ranges::find(m_properties, name, &ExposedProperty::name);
    if(renamed == m_properties.end() || !isChangeable() || newName.isEmpty())
    {
        return false;
    }
    if(newName == name)
    {
        return true;
    }
    if(!isFreePropertyName(newName))
    {
        return false;
    }
    renamed->name = newName;
    emit changed();
    return true;
}

void Stargate::removeProperty(const QString& name)
{
    if(isChangeable() &&
       m_properties.removeIf([&name](const ExposedProperty& exposed) { return exposed.name == name; }) > 0)
    {
        emit changed();
    }
}

bool Stargate::isChangeable() const
{
    return state() != ComponentState::Running;
}

void Stargate::builtByPlugin() {}

const std::vector<std::unique_ptr<Component>>& Stargate::galaxyComponents() const
{
    return m_galaxy != nullptr ? m_galaxy->components() : m_application.components();
}

bool Stargate::isFreeInterfaceName(Exposure exposure, const QString& name) const
{
    return std::ranges::none_of(galaxyComponents(),
                                [exposure, &name](const std::unique_ptr<Component>& component)
                                {
                                    const auto* stargate = qobject_cast<const Stargate*>(component.get());
                                    return stargate != nullptr && stargate->exposedInterface(exposure, name) != nullptr;
                                });
}

bool Stargate::isFreePropertyName(const QString& name) const
{
    return std::ranges::none_of(galaxyComponents(),
                                [&name](const std::unique_ptr<Component>& component)
                                {
                                    const auto* stargate = qobject_cast<const Stargate*>(component.get());
                                    return stargate != nullptr && stargate->property(name) != nullptr;
                                });
}

Stargate::Passage* Stargate::passage(int id)
{
    const auto found = std::ranges::find(m_passages, id, &Passage::id);
    return found != m_passages.end() ? &*found : nullptr;
}

Stargate::Passage* Stargate::passage(Exposure exposure, const QString& name)
{
    const auto found = std::ranges::find_if(m_passages, [exposure, &name](const Passage& passage)
                                            { return passage.info.exposure == exposure && passage.info.name == name; });
    return found != m_passages.end() ? &*found : nullptr;
}

void Stargate::attach()
{
    if(m_galaxy == nullptr || m_attached)
    {
        return;
    }
    m_attached = true;
    for(const Passage& exposed : std::as_const(m_passages))
    {
        if(exposed.info.exposure == Exposure::Provided)
        {
            // With what it has already: a component can provide its interface while it is edited.
            m_galaxy->provide(exposed.info.name, exposed.info.type, exposed.pointer);
            continue;
        }
        // Mandatory when one of the components it passes it to needs it.
        const auto needs = [this, &exposed](const std::unique_ptr<Component>& component)
        {
            const QList<InterfaceInfo> consumed = component->consumedInterfaces();
            return std::ranges::any_of(
                consumed,
                [this, &exposed, &component](const InterfaceInfo& consumedInterface)
                {
                    const QList<InterfaceBinding> bound = component->bindings(consumedInterface.name);
                    return consumedInterface.requirement == InterfaceRequirement::Mandatory &&
                           std::ranges::any_of(bound, [this, &exposed](const InterfaceBinding& to)
                                               { return to.provider == this && to.providedName == exposed.info.name; });
                });
        };
        consumeForGalaxy(exposed, std::ranges::any_of(m_galaxy->components(), needs) ? InterfaceRequirement::Mandatory
                                                                                     : InterfaceRequirement::Optional);
    }
}

void Stargate::consumeForGalaxy(const Passage& exposed, InterfaceRequirement requirement)
{
    // What its galaxy is given, it provides to components of its galaxy.
    const int id = exposed.id;
    const auto take = [this, id](void* pointer)
    {
        if(Passage* taken = passage(id))
        {
            taken->pointer = pointer;
        }
    };
    const auto pass = [this, id]
    {
        if(const Passage* passed = passage(id))
        {
            provide(passed->info.name, passed->info.type, passed->pointer);
        }
    };
    const auto withdraw = [this, id]
    {
        if(const Passage* withdrawn = passage(id))
        {
            provide(withdrawn->info.name, withdrawn->info.type, nullptr);
        }
    };
    m_galaxy->consume(exposed.info.name, exposed.info.type, requirement, take, pass, withdraw);
}

void Stargate::detach()
{
    m_attached = false;
}

void Stargate::forget(QObject* object)
{
    if(m_properties.removeIf([object](const ExposedProperty& exposed)
                             { return exposed.object == object || exposed.component == object; }) > 0)
    {
        emit changed();
    }
}

} // namespace Nebula
