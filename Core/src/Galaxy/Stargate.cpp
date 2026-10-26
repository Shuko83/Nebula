// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Nebula/Galaxy/Stargate.h"

#include <QMetaProperty>
#include <algorithm>
#include <utility>

#include "Application/ProjectJson.h"
#include "Component/Components.h"
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

// Whether a passage is the one exposed the way exposure tells under name.
auto isPassage(Exposure exposure, const QString& name)
{
    return [exposure, &name](const auto& passage)
    { return passage.info.exposure == exposure && passage.info.name == name; };
}

// Whether a project holds property, of a component that is not a galaxy.
bool isHeld(const ExposedProperty& property)
{
    const QList<QMetaProperty> saved = ProjectJson::savedProperties(*property.component, property.object);
    return std::ranges::any_of(saved, [&property](const QMetaProperty& held)
                               { return QLatin1StringView(held.name()) == property.property; });
}

// Whether property is one its component has to expose: one it exposes when it is a galaxy, one a project holds
// otherwise.
bool isExposable(const ExposedProperty& property)
{
    if(const auto* galaxy = qobject_cast<const Galaxy*>(property.component))
    {
        return property.object == galaxy && galaxy->exposedProperty(property.property).has_value();
    }
    return isHeld(property);
}

// Whether component consumes, as Mandatory, providedName of provider.
bool needs(const Component& component, const Component* provider, const QString& providedName)
{
    const auto isBound = [provider, &providedName](const InterfaceBinding& to)
    { return to.provider == provider && to.providedName == providedName; };
    const QList<InterfaceInfo> consumed = component.consumedInterfaces();
    return std::ranges::any_of(consumed,
                               [&component, &isBound](const InterfaceInfo& info)
                               {
                                   return info.requirement == InterfaceRequirement::Mandatory &&
                                          std::ranges::any_of(component.bindings(info.name), isBound);
                               });
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
    const auto found = std::ranges::find_if(m_passages, isPassage(exposure, name));
    return found != m_passages.end() ? &found->info : nullptr;
}

bool Stargate::exposeInterface(Exposure exposure, const QString& name, QMetaType type)
{
    if(!isChangeable() || !type.isValid() || name.isEmpty() || !isFreeInterfaceName(exposure, name))
    {
        return false;
    }
    m_passages.append(
        Passage{.info = {.name = name, .type = type, .exposure = exposure}, .id = m_nextId++, .pointer = nullptr});
    open(m_passages.constLast());
    emit changed();
    return true;
}

bool Stargate::renameInterface(Exposure exposure, const QString& name, const QString& newName)
{
    Passage* renamed = passage(exposure, name);
    if(renamed == nullptr || !canRename(name, newName, isFreeInterfaceName(exposure, newName)))
    {
        return false;
    }
    if(newName != name)
    {
        renamePassage(*renamed, newName);
    }
    return true;
}

void Stargate::removeInterface(Exposure exposure, const QString& name)
{
    if(!isChangeable() || passage(exposure, name) == nullptr)
    {
        return;
    }
    close(exposure, name);
    m_passages.removeIf(isPassage(exposure, name));
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
    if(!canExpose(property))
    {
        return false;
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
    if(renamed == m_properties.end() || !canRename(name, newName, isFreePropertyName(newName)))
    {
        return false;
    }
    if(std::exchange(renamed->name, newName) != newName)
    {
        emit changed();
    }
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

const std::vector<std::unique_ptr<Component>>& Stargate::galaxyComponents() const
{
    return m_application.components(m_galaxy);
}

QList<const Stargate*> Stargate::galaxyStargates() const
{
    return componentsOfType<const Stargate>(galaxyComponents());
}

bool Stargate::isFreeInterfaceName(Exposure exposure, const QString& name) const
{
    return std::ranges::none_of(galaxyStargates(), [exposure, &name](const Stargate* stargate)
                                { return stargate->exposedInterface(exposure, name) != nullptr; });
}

bool Stargate::isFreePropertyName(const QString& name) const
{
    return std::ranges::none_of(galaxyStargates(),
                                [&name](const Stargate* stargate) { return stargate->property(name) != nullptr; });
}

bool Stargate::canRename(const QString& name, const QString& newName, bool isFree) const
{
    return isChangeable() && !newName.isEmpty() && (newName == name || isFree);
}

bool Stargate::isOfGalaxy(const Component* component) const
{
    return component != nullptr && qobject_cast<const Stargate*>(component) == nullptr &&
           isAmong(component, galaxyComponents());
}

bool Stargate::canExpose(const ExposedProperty& property) const
{
    return isChangeable() && !property.name.isEmpty() && isFreePropertyName(property.name) &&
           property.object != nullptr && isOfGalaxy(property.component) && isExposable(property) &&
           !isExposed(property);
}

bool Stargate::isExposed(const ExposedProperty& property) const
{
    const auto same = [&property](const ExposedProperty& exposed) { return isSameProperty(exposed, property); };
    return std::ranges::any_of(galaxyStargates(), [&same](const Stargate* stargate)
                               { return std::ranges::any_of(stargate->properties(), same); });
}

bool Stargate::isNeeded(const QString& name) const
{
    return std::ranges::any_of(m_galaxy->components(), [this, &name](const std::unique_ptr<Component>& component)
                               { return needs(*component, this, name); });
}

Stargate::Passage* Stargate::passage(int id)
{
    const auto found = std::ranges::find(m_passages, id, &Passage::id);
    return found != m_passages.end() ? &*found : nullptr;
}

Stargate::Passage* Stargate::passage(Exposure exposure, const QString& name)
{
    const auto found = std::ranges::find_if(m_passages, isPassage(exposure, name));
    return found != m_passages.end() ? &*found : nullptr;
}

void Stargate::keep(int id, void* pointer)
{
    if(Passage* kept = passage(id))
    {
        kept->pointer = pointer;
    }
}

void Stargate::passToGalaxy(int id, bool available)
{
    const Passage* passed = passage(id);
    if(m_attached && passed != nullptr)
    {
        m_galaxy->provide(passed->info.name, passed->info.type, available ? passed->pointer : nullptr);
    }
}

void Stargate::passToComponents(int id, bool available)
{
    if(const Passage* passed = passage(id))
    {
        provide(passed->info.name, passed->info.type, available ? passed->pointer : nullptr);
    }
}

void Stargate::open(const Passage& exposed)
{
    if(exposed.info.exposure == Exposure::Provided)
    {
        openProvided(exposed);
    }
    else
    {
        openConsumed(exposed);
    }
}

void Stargate::openProvided(Passage exposed)
{
    const int id = exposed.id;
    consume(
        exposed.info.name, exposed.info.type, InterfaceRequirement::Mandatory, [this, id](void* pointer)
        { keep(id, pointer); }, [this, id] { passToGalaxy(id, true); }, [this, id] { passToGalaxy(id, false); });
    if(m_attached)
    {
        m_galaxy->provide(exposed.info.name, exposed.info.type, nullptr); // Once it is given it
    }
}

void Stargate::openConsumed(Passage exposed)
{
    provide(exposed.info.name, exposed.info.type, nullptr);
    if(m_attached)
    {
        consumeForGalaxy(exposed, InterfaceRequirement::Optional); // Bound to nothing yet
    }
}

void Stargate::renamePassage(Passage& renamed, const QString& newName)
{
    const QString name = std::exchange(renamed.info.name, newName); // Before the interface: what it tells reads both
    if(renamed.info.exposure == Exposure::Provided)
    {
        renameProvided(name, newName);
    }
    else
    {
        renameConsumed(name, newName);
    }
    emit changed();
}

void Stargate::renameProvided(const QString& name, const QString& newName)
{
    renameConsumedInterface(name, newName);
    if(m_attached)
    {
        m_galaxy->renameProvidedInterface(name, newName); // What is bound to it follows
    }
}

void Stargate::renameConsumed(const QString& name, const QString& newName)
{
    renameProvidedInterface(name, newName);
    if(m_attached)
    {
        m_galaxy->renameConsumedInterface(name, newName); // Still bound to what it was
    }
}

void Stargate::close(Exposure exposure, const QString& name)
{
    if(exposure == Exposure::Provided)
    {
        closeProvided(name);
    }
    else
    {
        closeConsumed(name);
    }
}

void Stargate::closeProvided(const QString& name)
{
    removeConsumedInterface(name);
    if(m_attached)
    {
        m_galaxy->removeProvidedInterface(name);
    }
}

void Stargate::closeConsumed(const QString& name)
{
    if(m_attached)
    {
        m_galaxy->removeConsumedInterface(name);
    }
    removeProvidedInterface(name);
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
        giveToGalaxy(exposed);
    }
}

void Stargate::giveToGalaxy(const Passage& exposed)
{
    if(exposed.info.exposure == Exposure::Provided)
    {
        // With what it has already: a component can provide its interface while it is edited.
        m_galaxy->provide(exposed.info.name, exposed.info.type, exposed.pointer);
        return;
    }
    consumeForGalaxy(exposed,
                     isNeeded(exposed.info.name) ? InterfaceRequirement::Mandatory : InterfaceRequirement::Optional);
}

void Stargate::consumeForGalaxy(const Passage& exposed, InterfaceRequirement requirement)
{
    // What its galaxy is given, it provides to components of its galaxy.
    const int id = exposed.id;
    m_galaxy->consume(
        exposed.info.name, exposed.info.type, requirement, [this, id](void* pointer) { keep(id, pointer); },
        [this, id] { passToComponents(id, true); }, [this, id] { passToComponents(id, false); });
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
