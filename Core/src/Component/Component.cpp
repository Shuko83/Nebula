// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Nebula/Component/Component.h"

#include <QLoggingCategory>
#include <algorithm>
#include <utility>

#include "Component/Connections.h"

namespace Nebula
{

namespace
{

Q_LOGGING_CATEGORY(lcComponents, "nebula.components")

// The name of an interface a component provides or consumes.
constexpr auto nameOf = [](const auto& declared) -> const QString& { return declared.name; };

// The interface named name among interfaces, provided or consumed; nullptr when there is none.
template <typename Interfaces> auto* findNamed(Interfaces& interfaces, const QString& name)
{
    const auto found = std::ranges::find(interfaces, name, nameOf);
    return found != std::ranges::end(interfaces) ? &*found : nullptr;
}

// The index of the interface named name among interfaces; their count when there is none.
template <typename Interfaces> qsizetype indexOfNamed(const Interfaces& interfaces, const QString& name)
{
    return std::ranges::find(interfaces, name, nameOf) - interfaces.begin();
}

// Forgets the interface named name among interfaces.
template <typename Interfaces> void removeNamed(Interfaces& interfaces, const QString& name)
{
    interfaces.removeIf([&name](const auto& declared) { return declared.name == name; });
}

// The binding, among bindings, to providedName of provider; end() when there is none.
template <typename Bindings> auto findBound(Bindings& bindings, const Component* provider, const QString& providedName)
{
    return std::ranges::find_if(bindings, [provider, &providedName](const auto& bound)
                                { return bound.provider == provider && bound.providedName == providedName; });
}

// What the interfaces of declared, provided or consumed, are, in their order.
template <typename Interfaces> QList<InterfaceInfo> infos(const Interfaces& declared)
{
    QList<InterfaceInfo> interfaces;
    interfaces.reserve(declared.size());
    for(const auto& one : declared)
    {
        interfaces.append(one.info());
    }
    return interfaces;
}

// callback, told nothing of the pointer it is called for.
std::function<void(void*)> withoutPointer(std::function<void()> callback)
{
    if(!callback)
    {
        return {};
    }
    return [callback = std::move(callback)](void* /*pointer*/) { callback(); };
}

} // namespace

Component::Component() = default;

Component::~Component() = default;

QString Component::componentId() const
{
    return m_componentId;
}

ComponentState Component::state() const
{
    return m_state;
}

bool Component::start()
{
    if(m_state != ComponentState::Editing || !onStart())
    {
        return false;
    }
    setState(ComponentState::Running);
    return true;
}

void Component::stop()
{
    if(m_state != ComponentState::Running)
    {
        return;
    }
    onStop();
    setState(ComponentState::Editing);
}

bool Component::onStart()
{
    return true;
}

void Component::onStop() {}

QList<InterfaceInfo> Component::providedInterfaces() const
{
    return infos(m_providedInterfaces);
}

QList<InterfaceInfo> Component::consumedInterfaces() const
{
    return infos(m_consumedInterfaces);
}

bool Component::bindInterface(const QString& name, Component& provider, const QString& providedName)
{
    if(!canBind(name, provider, providedName))
    {
        return false;
    }
    if(!findNamed(m_consumedInterfaces, name)->list)
    {
        unbindInterface(name);
    }
    addBinding(name, provider, providedName);
    return true;
}

void Component::unbindInterface(const QString& name)
{
    const ConsumedInterface* consumed = findNamed(m_consumedInterfaces, name);
    // The last one first, each found again, and no more than there are: what an unbinding calls can change them.
    for(qsizetype count = consumed != nullptr ? consumed->bindings.size() : 0; count > 0; --count)
    {
        if(!unbindLast(name))
        {
            return;
        }
    }
}

void Component::unbindInterface(const QString& name, const Component& provider, const QString& providedName)
{
    withdrawInterface(name, provider, providedName);
    // Found again: what withdrawing calls can have the component declare another interface.
    ConsumedInterface* consumed = findNamed(m_consumedInterfaces, name);
    if(consumed != nullptr && consumed->unbind(&provider, providedName))
    {
        emit interfacesChanged();
    }
}

InterfaceBinding Component::binding(const QString& name) const
{
    const QList<InterfaceBinding> bound = bindings(name);
    return bound.isEmpty() ? InterfaceBinding{} : bound.constFirst();
}

QList<InterfaceBinding> Component::bindings(const QString& name) const
{
    const ConsumedInterface* consumed = findNamed(m_consumedInterfaces, name);
    return consumed != nullptr ? consumed->interfaceBindings() : QList<InterfaceBinding>();
}

QStringList Component::unboundMandatoryInterfaces() const
{
    QStringList unbound;
    for(const ConsumedInterface& consumed : m_consumedInterfaces)
    {
        if(consumed.requirement == InterfaceRequirement::Mandatory && consumed.bindings.isEmpty())
        {
            unbound.append(consumed.name);
        }
    }
    return unbound;
}

void Component::addPropertiesForQObject(QObject* object, const QString& name)
{
    if(!canAddProperties(object))
    {
        return;
    }
    m_propertyObjects.append(PropertyObject{.object = object, .name = name});
    connect(object, &QObject::destroyed, this, [this, object] { forgetPropertyObject(object); });
    emit propertyObjectsChanged();
}

QList<PropertyObject> Component::propertyObjects() const
{
    return m_propertyObjects;
}

InterfaceInfo Component::ProvidedInterface::info() const
{
    return {.name = name, .type = type, .available = pointer != nullptr};
}

InterfaceInfo Component::ConsumedInterface::info() const
{
    const bool available = std::ranges::any_of(bindings, [](const Bound& bound) { return bound.pointer != nullptr; });
    return {.name = name, .type = type, .available = available, .requirement = requirement, .list = list};
}

QList<InterfaceBinding> Component::ConsumedInterface::interfaceBindings() const
{
    QList<InterfaceBinding> bound;
    bound.reserve(bindings.size());
    for(const Bound& binding : bindings)
    {
        bound.append({.provider = binding.provider, .providedName = binding.providedName});
    }
    return bound;
}

QList<void*> Component::ConsumedInterface::pointers() const
{
    QList<void*> pointers;
    for(const Bound& bound : bindings)
    {
        if(bound.pointer != nullptr)
        {
            pointers.append(bound.pointer);
        }
    }
    return pointers;
}

QStringList Component::ConsumedInterface::providedNames(const Component* provider) const
{
    QStringList names;
    for(const Bound& bound : bindings)
    {
        if(bound.provider == provider)
        {
            names.append(bound.providedName);
        }
    }
    return names;
}

Component::Bound* Component::ConsumedInterface::bound(const Component* provider, const QString& providedName)
{
    const auto found = findBound(bindings, provider, providedName);
    return found != bindings.end() ? &*found : nullptr;
}

const Component::Bound* Component::ConsumedInterface::bound(const Component* provider,
                                                            const QString& providedName) const
{
    const auto found = findBound(bindings, provider, providedName);
    return found != bindings.end() ? &*found : nullptr;
}

void Component::ConsumedInterface::disconnect()
{
    for(Bound& binding : bindings)
    {
        disconnectAll(binding.connections);
    }
}

bool Component::ConsumedInterface::unbind(const Component* provider, const QString& providedName)
{
    const auto found = findBound(bindings, provider, providedName);
    if(found == bindings.end())
    {
        return false;
    }
    disconnectAll(found->connections);
    bindings.erase(found);
    return true;
}

void* Component::providedPointer(const QString& name, QMetaType type) const
{
    const ProvidedInterface* provided = findNamed(m_providedInterfaces, name);
    return provided != nullptr && provided->type == type ? provided->pointer : nullptr;
}

bool Component::provide(const QString& name, QMetaType type, void* pointer)
{
    if(!canProvide(name, type))
    {
        return false;
    }
    const bool declared = declareProvided(name, type);
    if(replacePointer(indexOfNamed(m_providedInterfaces, name), pointer) || declared)
    {
        emit interfacesChanged();
    }
    return true;
}

bool Component::consume(const QString& name, QMetaType type, InterfaceRequirement requirement,
                        std::function<void(void*)> setMember, std::function<void()> onAvailable,
                        std::function<void()> onUnavailable)
{
    return declareConsumed(
        ConsumedInterface{.name = name,
                          .type = type,
                          .requirement = requirement,
                          .setMember = [setMember = std::move(setMember)](const QList<void*>& pointers)
                          { setMember(pointers.isEmpty() ? nullptr : pointers.constFirst()); },
                          .onAvailable = withoutPointer(std::move(onAvailable)),
                          .onUnavailable = withoutPointer(std::move(onUnavailable))});
}

bool Component::consumeList(const QString& name, QMetaType type, InterfaceRequirement requirement,
                            std::function<void(const QList<void*>&)> setMember, std::function<void(void*)> onInserted,
                            std::function<void(void*)> onRemoved)
{
    return declareConsumed(ConsumedInterface{.name = name,
                                             .type = type,
                                             .requirement = requirement,
                                             .list = true,
                                             .setMember = std::move(setMember),
                                             .onAvailable = std::move(onInserted),
                                             .onUnavailable = std::move(onRemoved)});
}

bool Component::declareConsumed(ConsumedInterface consumed)
{
    if(findNamed(m_consumedInterfaces, consumed.name) != nullptr)
    {
        qCWarning(lcComponents).noquote() << "Interface" << consumed.name << "is already consumed";
        return false;
    }
    writeMember(consumed);
    m_consumedInterfaces.append(std::move(consumed));
    emit interfacesChanged();
    return true;
}

bool Component::renameProvidedInterface(const QString& name, const QString& newName)
{
    ProvidedInterface* provided = findNamed(m_providedInterfaces, name);
    if(provided == nullptr || findNamed(m_providedInterfaces, newName) != nullptr)
    {
        return false;
    }
    provided->name = newName;
    emit interfaceRenamed(name, newName); // What is bound to it follows
    emit interfacesChanged();
    return true;
}

bool Component::renameConsumedInterface(const QString& name, const QString& newName)
{
    ConsumedInterface* consumed = findNamed(m_consumedInterfaces, name);
    if(consumed == nullptr || findNamed(m_consumedInterfaces, newName) != nullptr)
    {
        return false;
    }
    consumed->disconnect(); // Its connections to its providers know it by its name
    consumed->name = newName;
    followProviders(newName);
    emit interfacesChanged();
    return true;
}

void Component::removeProvidedInterface(const QString& name)
{
    const qsizetype index = indexOfNamed(m_providedInterfaces, name);
    if(index == m_providedInterfaces.size())
    {
        return;
    }
    makeUnavailable(index); // First: what is bound to it lets go of its pointer while it is still valid
    removeNamed(m_providedInterfaces, name); // By its name: what was told can have it declare another interface
    emit interfaceRemoved(name);             // What was bound to it is unbound
    emit interfacesChanged();
}

void Component::removeConsumedInterface(const QString& name)
{
    if(findNamed(m_consumedInterfaces, name) == nullptr)
    {
        return;
    }
    unbindInterface(name);
    removeNamed(m_consumedInterfaces, name);
    emit interfacesChanged();
}

bool Component::canBind(const QString& name, const Component& provider, const QString& providedName) const
{
    const ConsumedInterface* consumed = findNamed(m_consumedInterfaces, name);
    const ProvidedInterface* provided = findNamed(provider.m_providedInterfaces, providedName);
    // Not to itself either: it would be told of its own destruction once its interfaces are gone.
    return &provider != this && consumed != nullptr && provided != nullptr && consumed->type == provided->type &&
           !(consumed->list && consumed->bound(&provider, providedName) != nullptr);
}

void Component::addBinding(const QString& name, Component& provider, const QString& providedName)
{
    // Found again: the callback unbindInterface() called can have the component declare another interface.
    findNamed(m_consumedInterfaces, name)->bindings.append(Bound{.provider = &provider, .providedName = providedName});
    followProvider(name, provider, providedName);
    emit interfacesChanged(); // Bound, before it is told again when it becomes available
    deliverInterface(name, provider, providedName);
}

std::pair<Component::ConsumedInterface*, Component::Bound*>
Component::findBinding(const QString& name, const Component* provider, const QString& providedName)
{
    ConsumedInterface* consumed = findNamed(m_consumedInterfaces, name);
    Bound* bound = consumed != nullptr ? consumed->bound(provider, providedName) : nullptr;
    return {bound != nullptr ? consumed : nullptr, bound};
}

bool Component::unbindLast(const QString& name)
{
    const ConsumedInterface* consumed = findNamed(m_consumedInterfaces, name);
    if(consumed == nullptr || consumed->bindings.isEmpty())
    {
        return false;
    }
    const Bound last = consumed->bindings.constLast();
    unbindInterface(name, *last.provider, last.providedName);
    return true;
}

void Component::unbindProvider(const QString& name, const Component* provider)
{
    const ConsumedInterface* consumed = findNamed(m_consumedInterfaces, name);
    // Copies of the names, as each unbinding changes them.
    const QStringList providedNames = consumed != nullptr ? consumed->providedNames(provider) : QStringList();
    for(const QString& providedName : providedNames)
    {
        unbindInterface(name, *provider, providedName);
    }
}

void Component::followProvider(const QString& name, Component& provider, const QString& providedName)
{
    // What the provider tells of is found when it tells it, as a stargate can rename what it provides. A list bound
    // twice to the same provider is told twice: the second time, there is nothing left to do.
    findBinding(name, &provider, providedName).second->connections =
        followPointers(name, &provider) + followBindings(name, &provider);
}

void Component::followProviders(const QString& name)
{
    const QList<Bound> bindings = findNamed(m_consumedInterfaces, name)->bindings; // A copy: following changes them
    for(const Bound& bound : bindings)
    {
        followProvider(name, *bound.provider, bound.providedName);
    }
}

QList<QMetaObject::Connection> Component::followPointers(const QString& name, Component* provider)
{
    return {connect(provider, &Component::interfaceAvailable, this,
                    [this, name, provider](const QString& available) { deliverInterface(name, *provider, available); }),
            connect(provider, &Component::interfaceUnavailable, this, [this, name, provider](const QString& unavailable)
                    { withdrawInterface(name, *provider, unavailable); })};
}

QList<QMetaObject::Connection> Component::followBindings(const QString& name, Component* provider)
{
    return {connect(provider, &Component::interfaceRenamed, this,
                    [this, name, provider](const QString& former, const QString& renamed)
                    { followRename(name, provider, former, renamed); }),
            connect(provider, &Component::interfaceRemoved, this,
                    [this, name, provider](const QString& removed) { unbindInterface(name, *provider, removed); }),
            connect(provider, &QObject::destroyed, this, [this, name, provider] { unbindProvider(name, provider); })};
}

void Component::followRename(const QString& name, const Component* provider, const QString& former,
                             const QString& renamed)
{
    if(Bound* bound = findBinding(name, provider, former).second)
    {
        bound->providedName = renamed;
    }
}

void Component::deliverInterface(const QString& name, const Component& provider, const QString& providedName)
{
    const auto [consumed, bound] = findBinding(name, &provider, providedName);
    if(bound == nullptr || bound->pointer != nullptr)
    {
        return;
    }
    bound->pointer = provider.providedPointer(providedName, consumed->type);
    if(bound->pointer != nullptr)
    {
        tellAvailable(*consumed, bound->pointer);
    }
}

void Component::withdrawInterface(const QString& name, const Component& provider, const QString& providedName)
{
    const auto [consumed, bound] = findBinding(name, &provider, providedName);
    if(bound == nullptr || bound->pointer == nullptr)
    {
        return;
    }
    // Unavailable from now on, should onUnavailable unbind it; the member still has what it has to let go of, until it
    // is written again once it is told.
    tellUnavailable(name, consumed->onUnavailable, std::exchange(bound->pointer, nullptr));
}

void Component::tellAvailable(const ConsumedInterface& consumed, void* pointer)
{
    writeMember(consumed);
    // A copy: it can have the component declare another interface.
    if(const std::function<void(void*)> onAvailable = consumed.onAvailable)
    {
        onAvailable(pointer);
    }
    emit interfacesChanged();
}

void Component::tellUnavailable(const QString& name, std::function<void(void*)> onUnavailable, void* pointer)
{
    if(onUnavailable)
    {
        onUnavailable(pointer);
    }
    // Found again: what it was told can have the component declare another interface.
    if(const ConsumedInterface* withdrawn = findNamed(m_consumedInterfaces, name))
    {
        writeMember(*withdrawn);
    }
    emit interfacesChanged();
}

void Component::writeMember(const ConsumedInterface& consumed)
{
    consumed.setMember(consumed.pointers());
}

bool Component::canProvide(const QString& name, QMetaType type) const
{
    const ProvidedInterface* provided = findNamed(m_providedInterfaces, name);
    if(provided == nullptr || provided->type == type)
    {
        return true;
    }
    qCWarning(lcComponents).noquote() << "Interface" << name << "cannot be provided as" << type.name()
                                      << "- already provided as" << provided->type.name();
    return false;
}

bool Component::declareProvided(const QString& name, QMetaType type)
{
    if(findNamed(m_providedInterfaces, name) != nullptr)
    {
        return false;
    }
    m_providedInterfaces.append(ProvidedInterface{.name = name, .type = type});
    return true;
}

bool Component::replacePointer(qsizetype index, void* pointer)
{
    if(m_providedInterfaces.at(index).pointer == pointer)
    {
        return false;
    }
    makeUnavailable(index);
    makeAvailable(index, pointer);
    return true;
}

void Component::makeUnavailable(qsizetype index)
{
    if(m_providedInterfaces.at(index).pointer == nullptr)
    {
        return;
    }
    const QString name = m_providedInterfaces.at(index).name; // A copy: what is told can change the list
    m_providedInterfaces[index].pointer = nullptr;
    emit interfaceUnavailable(name);
}

void Component::makeAvailable(qsizetype index, void* pointer)
{
    if(pointer == nullptr)
    {
        return;
    }
    const QString name = m_providedInterfaces.at(index).name; // A copy: what is told can change the list
    m_providedInterfaces[index].pointer = pointer;
    emit interfaceAvailable(name);
}

bool Component::canAddProperties(const QObject* object) const
{
    if(object != nullptr && object != this &&
       std::ranges::find(m_propertyObjects, object, &PropertyObject::object) == m_propertyObjects.end())
    {
        return true;
    }
    qCWarning(lcComponents).noquote() << "The properties of" << object << "cannot be added to" << objectName();
    return false;
}

void Component::forgetPropertyObject(const QObject* object)
{
    m_propertyObjects.removeIf([object](const PropertyObject& added) { return added.object == object; });
    emit propertyObjectsChanged();
}

void Component::releaseInterfaces()
{
    // By index, with copies of the names: what is called here can have the component declare another interface.
    for(qsizetype index = 0; index < m_consumedInterfaces.size(); ++index)
    {
        const QString name = m_consumedInterfaces.at(index).name;
        unbindInterface(name);
    }
    for(qsizetype index = 0; index < m_providedInterfaces.size(); ++index)
    {
        makeUnavailable(index);
    }
}

void Component::setState(ComponentState state)
{
    m_state = state;
    emit stateChanged(state);
}

} // namespace Nebula
