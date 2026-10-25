// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Nebula/Component/Component.h"

#include <QLoggingCategory>
#include <algorithm>
#include <utility>

namespace Nebula
{

namespace
{

Q_LOGGING_CATEGORY(lcComponents, "nebula.components")

// The binding, among bindings, to providedName of provider; end() when there is none.
template <typename Bindings> auto findBound(Bindings& bindings, const Component* provider, const QString& providedName)
{
    return std::ranges::find_if(bindings, [provider, &providedName](const auto& bound)
                                { return bound.provider == provider && bound.providedName == providedName; });
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
    QList<InterfaceInfo> interfaces;
    interfaces.reserve(m_providedInterfaces.size());
    for(const ProvidedInterface& provided : m_providedInterfaces)
    {
        interfaces.append(
            InterfaceInfo{.name = provided.name, .type = provided.type, .available = provided.pointer != nullptr});
    }
    return interfaces;
}

QList<InterfaceInfo> Component::consumedInterfaces() const
{
    QList<InterfaceInfo> interfaces;
    interfaces.reserve(m_consumedInterfaces.size());
    for(const ConsumedInterface& consumed : m_consumedInterfaces)
    {
        interfaces.append(InterfaceInfo{.name = consumed.name,
                                        .type = consumed.type,
                                        .available = std::ranges::any_of(consumed.bindings, [](const Bound& bound)
                                                                         { return bound.pointer != nullptr; }),
                                        .requirement = consumed.requirement,
                                        .list = consumed.list});
    }
    return interfaces;
}

bool Component::bindInterface(const QString& name, Component& provider, const QString& providedName)
{
    const auto consumed = std::ranges::find(std::as_const(m_consumedInterfaces), name, &ConsumedInterface::name);
    const auto provided =
        std::ranges::find(std::as_const(provider.m_providedInterfaces), providedName, &ProvidedInterface::name);
    // Not to itself either: it would be told of its own destruction once its interfaces are gone.
    if(&provider == this || consumed == m_consumedInterfaces.cend() ||
       provided == provider.m_providedInterfaces.cend() || consumed->type != provided->type ||
       (consumed->list && findBound(consumed->bindings, &provider, providedName) != consumed->bindings.cend()))
    {
        return false;
    }
    if(!consumed->list)
    {
        unbindInterface(name);
    }

    // Found again: the callback unbindInterface() called can have the component declare another interface.
    std::ranges::find(m_consumedInterfaces, name, &ConsumedInterface::name)
        ->bindings.append(Bound{.provider = &provider, .providedName = providedName});
    followProvider(name, provider, providedName);
    emit interfacesChanged(); // Bound, before it is told again when it becomes available
    deliverInterface(name, provider, providedName);
    return true;
}

void Component::followProvider(const QString& name, Component& provider, const QString& providedName)
{
    // What the provider tells of is found when it tells it, as a stargate can rename what it provides. A list bound
    // twice to the same provider is told twice: the second time, there is nothing left to do.
    Component* const from = &provider;
    QList<QMetaObject::Connection> connections = {
        connect(from, &Component::interfaceAvailable, this,
                [this, name, from](const QString& available) { deliverInterface(name, *from, available); }),
        connect(from, &Component::interfaceUnavailable, this,
                [this, name, from](const QString& unavailable) { withdrawInterface(name, *from, unavailable); }),
        connect(from, &Component::interfaceRenamed, this,
                [this, name, from](const QString& former, const QString& renamed)
                {
                    const auto consumed = std::ranges::find(m_consumedInterfaces, name, &ConsumedInterface::name);
                    if(consumed == m_consumedInterfaces.end())
                    {
                        return;
                    }
                    if(const auto bound = findBound(consumed->bindings, from, former);
                       bound != consumed->bindings.end())
                    {
                        bound->providedName = renamed;
                    }
                }),
        connect(from, &Component::interfaceRemoved, this,
                [this, name, from](const QString& removed) { unbindInterface(name, *from, removed); }),
        connect(from, &QObject::destroyed, this,
                [this, name, from]
                {
                    const auto consumed = std::ranges::find(m_consumedInterfaces, name, &ConsumedInterface::name);
                    if(consumed == m_consumedInterfaces.end())
                    {
                        return;
                    }
                    // Each of its bindings to the provider; from copies of the names, as each unbinding changes them.
                    QStringList provided;
                    for(const Bound& bound : std::as_const(consumed->bindings))
                    {
                        if(bound.provider == from)
                        {
                            provided.append(bound.providedName);
                        }
                    }
                    for(const QString& providedName : std::as_const(provided))
                    {
                        unbindInterface(name, *from, providedName);
                    }
                }),
    };
    ConsumedInterface& consumed = *std::ranges::find(m_consumedInterfaces, name, &ConsumedInterface::name);
    findBound(consumed.bindings, from, providedName)->connections = std::move(connections);
}

void Component::unbindInterface(const QString& name)
{
    const auto consumed = std::ranges::find(m_consumedInterfaces, name, &ConsumedInterface::name);
    if(consumed == m_consumedInterfaces.end())
    {
        return;
    }
    // The last one first, each found again, and no more than there are: what an unbinding calls can change them.
    for(qsizetype count = consumed->bindings.size(); count > 0; --count)
    {
        const auto unbound = std::ranges::find(m_consumedInterfaces, name, &ConsumedInterface::name);
        if(unbound == m_consumedInterfaces.end() || unbound->bindings.isEmpty())
        {
            return;
        }
        const Bound last = unbound->bindings.constLast();
        unbindInterface(name, *last.provider, last.providedName);
    }
}

void Component::unbindInterface(const QString& name, const Component& provider, const QString& providedName)
{
    withdrawInterface(name, provider, providedName);
    // Found again: what withdrawing calls can have the component declare another interface.
    const auto consumed = std::ranges::find(m_consumedInterfaces, name, &ConsumedInterface::name);
    if(consumed == m_consumedInterfaces.end())
    {
        return;
    }
    const auto bound = findBound(consumed->bindings, &provider, providedName);
    if(bound == consumed->bindings.end())
    {
        return;
    }
    for(const QMetaObject::Connection& connection : std::as_const(bound->connections))
    {
        disconnect(connection);
    }
    consumed->bindings.erase(bound);
    emit interfacesChanged();
}

InterfaceBinding Component::binding(const QString& name) const
{
    const QList<InterfaceBinding> bound = bindings(name);
    return bound.isEmpty() ? InterfaceBinding{} : bound.constFirst();
}

QList<InterfaceBinding> Component::bindings(const QString& name) const
{
    const auto consumed = std::ranges::find(m_consumedInterfaces, name, &ConsumedInterface::name);
    if(consumed == m_consumedInterfaces.end())
    {
        return {};
    }
    QList<InterfaceBinding> bound;
    bound.reserve(consumed->bindings.size());
    for(const Bound& binding : consumed->bindings)
    {
        bound.append({.provider = binding.provider, .providedName = binding.providedName});
    }
    return bound;
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
    if(object == nullptr || object == this ||
       std::ranges::find(m_propertyObjects, object, &PropertyObject::object) != m_propertyObjects.end())
    {
        qCWarning(lcComponents).noquote() << "The properties of" << object << "cannot be added to" << objectName();
        return;
    }
    m_propertyObjects.append(PropertyObject{.object = object, .name = name});
    connect(object, &QObject::destroyed, this,
            [this, object]
            {
                m_propertyObjects.removeIf([object](const PropertyObject& added) { return added.object == object; });
                emit propertyObjectsChanged();
            });
    emit propertyObjectsChanged();
}

QList<PropertyObject> Component::propertyObjects() const
{
    return m_propertyObjects;
}

void* Component::providedPointer(const QString& name, QMetaType type) const
{
    const auto provided = std::ranges::find(m_providedInterfaces, name, &ProvidedInterface::name);
    return provided != m_providedInterfaces.end() && provided->type == type ? provided->pointer : nullptr;
}

bool Component::provide(const QString& name, QMetaType type, void* pointer)
{
    // An index: a slot of the signals emitted here can have the component declare another interface.
    const qsizetype index =
        std::ranges::find(m_providedInterfaces, name, &ProvidedInterface::name) - m_providedInterfaces.begin();
    const bool declared = index == m_providedInterfaces.size();
    if(declared)
    {
        m_providedInterfaces.append(ProvidedInterface{.name = name, .type = type});
    }
    else if(m_providedInterfaces.at(index).type != type)
    {
        qCWarning(lcComponents).noquote() << "Interface" << name << "cannot be provided as" << type.name()
                                          << "- already provided as" << m_providedInterfaces.at(index).type.name();
        return false;
    }

    const bool replaced = m_providedInterfaces.at(index).pointer != pointer;
    if(replaced && m_providedInterfaces.at(index).pointer != nullptr)
    {
        m_providedInterfaces[index].pointer = nullptr;
        emit interfaceUnavailable(name);
    }
    if(replaced && pointer != nullptr)
    {
        m_providedInterfaces[index].pointer = pointer;
        emit interfaceAvailable(name);
    }
    if(declared || replaced)
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
    if(std::ranges::find(m_consumedInterfaces, consumed.name, &ConsumedInterface::name) != m_consumedInterfaces.end())
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
    const auto provided = std::ranges::find(m_providedInterfaces, name, &ProvidedInterface::name);
    if(provided == m_providedInterfaces.end() ||
       std::ranges::find(m_providedInterfaces, newName, &ProvidedInterface::name) != m_providedInterfaces.end())
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
    const auto consumed = std::ranges::find(m_consumedInterfaces, name, &ConsumedInterface::name);
    if(consumed == m_consumedInterfaces.end() ||
       std::ranges::find(m_consumedInterfaces, newName, &ConsumedInterface::name) != m_consumedInterfaces.end())
    {
        return false;
    }
    // Its connections to its providers know it by its name.
    for(Bound& bound : consumed->bindings)
    {
        for(const QMetaObject::Connection& connection : std::as_const(bound.connections))
        {
            disconnect(connection);
        }
        bound.connections.clear();
    }
    consumed->name = newName;
    const QList<Bound> bindings = consumed->bindings;
    for(const Bound& bound : bindings)
    {
        followProvider(newName, *bound.provider, bound.providedName);
    }
    emit interfacesChanged();
    return true;
}

void Component::removeProvidedInterface(const QString& name)
{
    const qsizetype index =
        std::ranges::find(m_providedInterfaces, name, &ProvidedInterface::name) - m_providedInterfaces.begin();
    if(index == m_providedInterfaces.size())
    {
        return;
    }
    // Told unavailable first: what is bound to it lets go of its pointer while it is still valid.
    if(m_providedInterfaces.at(index).pointer != nullptr)
    {
        m_providedInterfaces[index].pointer = nullptr;
        emit interfaceUnavailable(name);
    }
    // Found again: what was told can have the component declare another interface.
    m_providedInterfaces.removeIf([&name](const ProvidedInterface& declared) { return declared.name == name; });
    emit interfaceRemoved(name); // What was bound to it is unbound
    emit interfacesChanged();
}

void Component::removeConsumedInterface(const QString& name)
{
    if(std::ranges::find(m_consumedInterfaces, name, &ConsumedInterface::name) == m_consumedInterfaces.end())
    {
        return;
    }
    unbindInterface(name);
    m_consumedInterfaces.removeIf([&name](const ConsumedInterface& declared) { return declared.name == name; });
    emit interfacesChanged();
}

void Component::deliverInterface(const QString& name, const Component& provider, const QString& providedName)
{
    const auto consumed = std::ranges::find(m_consumedInterfaces, name, &ConsumedInterface::name);
    if(consumed == m_consumedInterfaces.end())
    {
        return;
    }
    const auto bound = findBound(consumed->bindings, &provider, providedName);
    if(bound == consumed->bindings.end() || bound->pointer != nullptr)
    {
        return;
    }
    void* pointer = provider.providedPointer(providedName, consumed->type);
    if(pointer == nullptr)
    {
        return;
    }
    bound->pointer = pointer;
    writeMember(*consumed);
    // A copy: it can have the component declare another interface.
    if(const std::function<void(void*)> onAvailable = consumed->onAvailable)
    {
        onAvailable(pointer);
    }
    emit interfacesChanged();
}

void Component::withdrawInterface(const QString& name, const Component& provider, const QString& providedName)
{
    const auto consumed = std::ranges::find(m_consumedInterfaces, name, &ConsumedInterface::name);
    if(consumed == m_consumedInterfaces.end())
    {
        return;
    }
    const auto bound = findBound(consumed->bindings, &provider, providedName);
    if(bound == consumed->bindings.end() || bound->pointer == nullptr)
    {
        return;
    }
    // Unavailable from now on, should onUnavailable unbind it; the member still has what it has to let go of, until it
    // is written again once it is told.
    void* const pointer = std::exchange(bound->pointer, nullptr);
    // A copy: it can have the component declare another interface.
    if(const std::function<void(void*)> onUnavailable = consumed->onUnavailable)
    {
        onUnavailable(pointer);
    }
    // Found again, for the same reason.
    if(const auto withdrawn = std::ranges::find(m_consumedInterfaces, name, &ConsumedInterface::name);
       withdrawn != m_consumedInterfaces.end())
    {
        writeMember(*withdrawn);
    }
    emit interfacesChanged();
}

void Component::writeMember(const ConsumedInterface& consumed)
{
    QList<void*> pointers;
    for(const Bound& bound : consumed.bindings)
    {
        if(bound.pointer != nullptr)
        {
            pointers.append(bound.pointer);
        }
    }
    consumed.setMember(pointers);
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
        if(m_providedInterfaces.at(index).pointer != nullptr)
        {
            const QString name = m_providedInterfaces.at(index).name;
            m_providedInterfaces[index].pointer = nullptr;
            emit interfaceUnavailable(name);
        }
    }
}

void Component::setState(ComponentState state)
{
    m_state = state;
    emit stateChanged(state);
}

} // namespace Nebula
