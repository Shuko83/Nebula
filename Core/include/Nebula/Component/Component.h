// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QMetaObject>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <QStringList>
#include <functional>
#include <type_traits>
#include <utility>

#include "Core_export.h"
#include "Nebula/Component/IComponent.h"
#include "Nebula/Component/PropertyObject.h"

namespace Nebula
{

class ComponentFactory;
template <typename P> class Plugin;

// Base of every component: declare its properties with Q_PROPERTY, the interfaces it provides and consumes with
// provideInterface() and consumeInterface(), and override the on*() hooks.
// Only its plugin creates a component (see NEBULA_REGISTER_COMPONENT and IPlugin::create()): one created otherwise
// would release its interfaces once its own members are destroyed, see releaseInterfaces().
class CORE_EXPORT Component : public QObject, public IComponent
{
    Q_OBJECT
    Q_PROPERTY(QString componentId READ componentId CONSTANT)

  public:
    ~Component() override;

    [[nodiscard]] QString componentId() const override;
    [[nodiscard]] ComponentState state() const override;

    // Each call returns false, without calling its hook, when the current state does not allow it.
    bool start() override;
    void stop() override;

    [[nodiscard]] QList<InterfaceInfo> providedInterfaces() const override;
    [[nodiscard]] QList<InterfaceInfo> consumedInterfaces() const override;

    // Binds name, which it consumes, to providedName, which provider provides: it has its pointer whenever provider
    // makes it available, until unbindInterface() or the destruction of one of them. Replaces what name was bound to;
    // for a list, see consumeInterfaceList(), adds to what it is bound to, after them.
    // Returns false, changing nothing, when it does not consume name, provider does not provide providedName, they
    // are not of the same type, or name is a list already bound to providedName of provider.
    bool bindInterface(const QString& name, Component& provider, const QString& providedName);
    // name, which it consumes, becomes unavailable and is bound to nothing; for a list, to none of what it was bound
    // to.
    void unbindInterface(const QString& name);
    // name, which it consumes, is no longer bound to providedName of provider: for a list, it keeps what else it is
    // bound to. Does nothing when it is not.
    void unbindInterface(const QString& name, const Component& provider, const QString& providedName);
    // What name is bound to: nothing when it is unbound, or not an interface it consumes. For a list, the first of
    // bindings().
    [[nodiscard]] InterfaceBinding binding(const QString& name) const;
    // What name is bound to, in the order it was bound: one binding at most, unless it is a list. None when it is
    // unbound, or not an interface it consumes.
    [[nodiscard]] QList<InterfaceBinding> bindings(const QString& name) const;
    // The interfaces it consumes as InterfaceRequirement::Mandatory that are bound to nothing, in the order they were
    // declared: it does not work until each of them is bound, a list to one interface at least. None when it is
    // operational.
    [[nodiscard]] QStringList unboundMandatoryInterfaces() const;

    // The objects added with addPropertiesForQObject() that still live, in the order they were added.
    [[nodiscard]] QList<PropertyObject> propertyObjects() const;

  signals:
    void stateChanged(Nebula::ComponentState state);
    // The pointer of name, which it provides, can be retrieved with providedInterface().
    void interfaceAvailable(const QString& name);
    // The pointer retrieved from name, which it provides, must no longer be used: the component withdrew it, is about
    // to replace it, or is being destroyed.
    void interfaceUnavailable(const QString& name);
    // name, which it provides, is named newName from now on: what is bound to it still is. Only a stargate renames an
    // interface, see Stargate.
    void interfaceRenamed(const QString& name, const QString& newName);
    // name, which it provided, is no longer one of its interfaces: what was bound to it is unbound, once it was told it
    // unavailable. See removeProvidedInterface().
    void interfaceRemoved(const QString& name);
    // It declared, renamed or removed an interface, one it consumes was bound or unbound, or one it provides or
    // consumes became available or unavailable.
    void interfacesChanged();
    // An object was added to propertyObjects(), or one of them was destroyed.
    void propertyObjectsChanged();

  protected:
    Component();

    // Provides pointer as the interface name, of type T: providedInterface<T>(name) returns it. Without a pointer, the
    // interface is declared, or withdrawn: it is listed, but unavailable. A component declares its interfaces when it
    // is built, so that they are known before it runs, or when one of its properties is set, for those that depend on
    // it: a project sets its properties before it binds its interfaces. See removeProvidedInterface().
    // Replacing the pointer of an available interface emits interfaceUnavailable() first, then interfaceAvailable()
    // once it is replaced: what the previous pointer refers to must still exist during this call.
    // Returns false, with a warning, when name is already provided with another type.
    template <typename T> bool provideInterface(const QString& name, std::type_identity_t<T>* pointer = nullptr)
    {
        return provide(name, QMetaType::fromType<T*>(), const_cast<void*>(static_cast<const void*>(pointer)));
    }

    // Declares that it consumes the interface name, of type T, through member, one of its members: nullptr, until
    // name is bound to an interface another component provides (see bindInterface()) and that one is available.
    // onAvailable is called once member is set; onUnavailable before it is set back to nullptr, the last time what it
    // points to can be used: when the provider withdraws or replaces its pointer, when name is unbound, and when one
    // of the two components is destroyed.
    // Returns false, with a warning, when name is already consumed.
    template <typename T>
    bool consumeInterface(const QString& name, T*& member, std::function<void()> onAvailable = {},
                          std::function<void()> onUnavailable = {})
    {
        return consumeInterface<T>(name, member, InterfaceRequirement::Optional, std::move(onAvailable),
                                   std::move(onUnavailable));
    }

    // The same, telling whether the component needs name to work: consumedInterfaces() tells it, for who assembles
    // components, and nothing else changes.
    template <typename T>
    bool consumeInterface(const QString& name, T*& member, InterfaceRequirement requirement,
                          std::function<void()> onAvailable = {}, std::function<void()> onUnavailable = {})
    {
        return consume(
            name, QMetaType::fromType<T*>(), requirement, [&member](void* pointer)
            { member = static_cast<T*>(pointer); }, std::move(onAvailable), std::move(onUnavailable));
    }

    // Declares that it consumes the interface name as a list of T, through member, one of its members: several
    // interfaces other components provide can be bound to it (see bindInterface()), and member holds the pointers of
    // those that are available, in the order they were bound. onInserted is called with a pointer once it is in member;
    // onRemoved with one before it leaves member, the last time it can be used: when its provider withdraws or replaces
    // it, when what provides it is unbound, and when one of the two components is destroyed. The observers of a
    // subject, say, register and unregister there.
    // Returns false, with a warning, when name is already consumed.
    template <typename T>
    bool consumeInterfaceList(const QString& name, QList<T*>& member, std::function<void(T*)> onInserted = {},
                              std::function<void(T*)> onRemoved = {})
    {
        return consumeInterfaceList<T>(name, member, InterfaceRequirement::Optional, std::move(onInserted),
                                       std::move(onRemoved));
    }

    // The same, telling whether the component needs one of them at least to work, as consumeInterface() does.
    template <typename T>
    bool consumeInterfaceList(const QString& name, QList<T*>& member, InterfaceRequirement requirement,
                              std::function<void(T*)> onInserted = {}, std::function<void(T*)> onRemoved = {})
    {
        const auto cast = [](std::function<void(T*)> callback) -> std::function<void(void*)>
        {
            if(!callback)
            {
                return {};
            }
            return [callback = std::move(callback)](void* pointer) { callback(static_cast<T*>(pointer)); };
        };
        return consumeList(
            name, QMetaType::fromType<T*>(), requirement,
            [&member](const QList<void*>& pointers)
            {
                member.clear();
                for(void* pointer : pointers)
                {
                    member.append(static_cast<T*>(pointer));
                }
            },
            cast(std::move(onInserted)), cast(std::move(onRemoved)));
    }

    // Withdraws name, which it provides, and forgets it: what was bound to it is told it unavailable, then unbound, see
    // interfaceRemoved(); what the pointer refers to must still exist during this call. The interfaces of a component
    // can depend on one of its properties: the widgets it provides, as many as its count, say, which removes the last
    // ones as it decreases. Does nothing when it provides no name.
    void removeProvidedInterface(const QString& name);
    // Unbinds name, which it consumes, telling it unavailable, and forgets it. Does nothing when it consumes no name.
    void removeConsumedInterface(const QString& name);

    // Has the properties of object, see PropertyObject::properties(), edited and saved with its own: NebulaStudio edits
    // them under those of the component, and a project holds their values. Under name when it is not empty, as a group
    // of their own; the objects added with the same name share a group. For as long as object lives: it need not
    // outlive the component. A component adds its objects when it is built, so that a project finds them when it is
    // opened, or when one of its properties is set, for those that depend on it: a project sets the properties of the
    // component before those of its objects. Destroying object takes its properties away. Does nothing, with a warning,
    // when object is nullptr, the component itself, or already added.
    void addPropertiesForQObject(QObject* object, const QString& name = {});

    // Returning false from onStart() leaves the state unchanged.
    virtual bool onStart();
    virtual void onStop();

  private:
    friend class ComponentFactory;
    template <typename P> friend class Plugin; // Calls releaseInterfaces()
    // Provide and consume interfaces of a type they are told when they run only, the interfaces a galaxy exposes
    // through its stargates, rename and remove those of a stargate and of a galaxy, and release theirs before they go.
    friend class Galaxy;
    friend class Stargate;

    struct ProvidedInterface
    {
        QString name;
        QMetaType type;
        void* pointer = nullptr; // nullptr while unavailable

        [[nodiscard]] InterfaceInfo info() const;
    };

    // What an interface it consumes is bound to.
    struct Bound
    {
        Component* provider = nullptr;
        QString providedName;                       // What it is bound to in provider
        QList<QMetaObject::Connection> connections; // To provider
        void* pointer = nullptr;                    // What it was given of provider; nullptr while unavailable
    };

    struct ConsumedInterface
    {
        QString name;
        QMetaType type;
        InterfaceRequirement requirement = InterfaceRequirement::Optional;
        bool list = false;
        // Writes the member given to consumeInterface() or consumeInterfaceList(): the pointers given, in the order
        // of bindings.
        std::function<void(const QList<void*>&)> setMember;
        std::function<void(void*)> onAvailable;   // With the pointer given
        std::function<void(void*)> onUnavailable; // With the pointer taken back
        QList<Bound> bindings;                    // In the order they were bound; one at most, unless it is a list

        [[nodiscard]] InterfaceInfo info() const;
        [[nodiscard]] QList<InterfaceBinding> interfaceBindings() const;
        // The pointers it is given, in the order of its bindings.
        [[nodiscard]] QList<void*> pointers() const;
        // The names of the interfaces of provider it is bound to, in the order of its bindings.
        [[nodiscard]] QStringList providedNames(const Component* provider) const;
        // Its binding to providedName of provider; nullptr when it has none.
        Bound* bound(const Component* provider, const QString& providedName);
        [[nodiscard]] const Bound* bound(const Component* provider, const QString& providedName) const;
        // Disconnects its bindings from their providers.
        void disconnect();
        // Disconnects and forgets its binding to providedName of provider; false when it has none.
        bool unbind(const Component* provider, const QString& providedName);
    };

    [[nodiscard]] void* providedPointer(const QString& name, QMetaType type) const override;
    // provideInterface(), type being the one of pointer.
    bool provide(const QString& name, QMetaType type, void* pointer);
    // consumeInterface(), type being the one of the member setMember writes.
    bool consume(const QString& name, QMetaType type, InterfaceRequirement requirement,
                 std::function<void(void*)> setMember, std::function<void()> onAvailable,
                 std::function<void()> onUnavailable);
    // consumeInterfaceList(), type being the one of the pointers setMember writes.
    bool consumeList(const QString& name, QMetaType type, InterfaceRequirement requirement,
                     std::function<void(const QList<void*>&)> setMember, std::function<void(void*)> onInserted,
                     std::function<void(void*)> onRemoved);
    // Declares name, see consume() and consumeList().
    bool declareConsumed(ConsumedInterface consumed);
    // Names name, which it provides, newName: what is bound to it is still, see interfaceRenamed(). Returns false,
    // changing nothing, when it provides no name, or provides newName already.
    bool renameProvidedInterface(const QString& name, const QString& newName);
    // Names name, which it consumes, newName: it is still bound to what it was. Returns false, changing nothing, when
    // it consumes no name, or consumes newName already.
    bool renameConsumedInterface(const QString& name, const QString& newName);
    // Whether name, which it consumes, can be bound to providedName of provider: of the same type, another
    // component, and not bound to it yet when name is a list.
    [[nodiscard]] bool canBind(const QString& name, const Component& provider, const QString& providedName) const;
    // Binds name, which it consumes, after its other bindings, to providedName of provider, and gives it its pointer.
    void addBinding(const QString& name, Component& provider, const QString& providedName);
    // The binding of name, which it consumes, to providedName of provider, and name; nullptrs when there is none.
    std::pair<ConsumedInterface*, Bound*> findBinding(const QString& name, const Component* provider,
                                                      const QString& providedName);
    // Unbinds the last binding of name, which it consumes; false when it has none.
    bool unbindLast(const QString& name);
    // Unbinds name, which it consumes, from each interface of provider it is bound to.
    void unbindProvider(const QString& name, const Component* provider);
    // Follows, for name, which it consumes and is bound to providedName of provider, what provider does with it.
    void followProvider(const QString& name, Component& provider, const QString& providedName);
    // followProvider() for each binding of name, which it consumes.
    void followProviders(const QString& name);
    // Connects name, which it consumes, to what provider tells of the pointers of its interfaces.
    QList<QMetaObject::Connection> followPointers(const QString& name, Component* provider);
    // Connects name, which it consumes, to what provider tells of its interfaces and of itself: renamed, removed or
    // destroyed.
    QList<QMetaObject::Connection> followBindings(const QString& name, Component* provider);
    // former of provider, which name, consumed, is bound to, is now named renamed.
    void followRename(const QString& name, const Component* provider, const QString& former, const QString& renamed);
    // Gives name, which it consumes, the pointer of providedName of provider, when it is bound to it and has none.
    void deliverInterface(const QString& name, const Component& provider, const QString& providedName);
    // Takes back from name, which it consumes, the pointer of providedName of provider, when it has one.
    void withdrawInterface(const QString& name, const Component& provider, const QString& providedName);
    // Writes the member of consumed, given pointer, then tells it.
    void tellAvailable(const ConsumedInterface& consumed, void* pointer);
    // Tells name, which it consumes, that pointer is taken back, then writes its member without it. onUnavailable is
    // a copy: it can have the component declare another interface.
    void tellUnavailable(const QString& name, std::function<void(void*)> onUnavailable, void* pointer);
    // Writes the member of consumed with the pointers it is given.
    static void writeMember(const ConsumedInterface& consumed);
    // Whether name can be provided as type: it is not provided as another type yet, which is warned of.
    [[nodiscard]] bool canProvide(const QString& name, QMetaType type) const;
    // Declares name, provided as type, when it is not yet; returns whether it was not.
    bool declareProvided(const QString& name, QMetaType type);
    // Gives the interface it provides at index pointer, telling it becomes unavailable, then available again; returns
    // whether it changed. By index: a slot of the signals it emits can have the component declare another interface.
    bool replacePointer(qsizetype index, void* pointer);
    // Makes the interface it provides at index unavailable, when it is available, and tells it.
    void makeUnavailable(qsizetype index);
    // Makes the interface it provides at index available with pointer, unless it is nullptr, and tells it.
    void makeAvailable(qsizetype index, void* pointer);
    // Whether the properties of object can be added: another object, not added yet; warns when they cannot.
    [[nodiscard]] bool canAddProperties(const QObject* object) const;
    // Forgets object, whose properties were added, as it is destroyed.
    void forgetPropertyObject(const QObject* object);
    // Ends what it consumes and provides while it is still whole: Plugin calls it before destroying the component, as
    // the callbacks of what it consumes, and what it provides, may be gone once its destructors run.
    void releaseInterfaces();

    void setState(ComponentState state);

    QString m_componentId; // Set by ComponentFactory
    ComponentState m_state = ComponentState::Editing;
    QList<ProvidedInterface> m_providedInterfaces; // In the order they were declared
    QList<ConsumedInterface> m_consumedInterfaces; // In the order they were declared
    QList<PropertyObject> m_propertyObjects;       // In the order they were added
};

} // namespace Nebula
