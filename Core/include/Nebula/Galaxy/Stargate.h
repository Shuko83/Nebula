// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QLatin1StringView>
#include <QList>
#include <QMetaType>
#include <QString>
#include <cstdint>
#include <memory>
#include <vector>

#include "Core_export.h"
#include "Nebula/Component/Component.h"

namespace Nebula
{

class Galaxy;
class Universe;

// Which way an interface a stargate exposes goes, seen from the galaxy the stargate is in.
enum class Exposure : std::uint8_t
{
    Provided, // The galaxy provides it: the stargate consumes it from a component of the galaxy
    Consumed, // The galaxy consumes it: the stargate provides it to components of the galaxy
};

// An interface a stargate exposes, as the galaxy it is in has it once placed.
struct ExposedInterface
{
    QString name;   // Its own among those its galaxy provides, or among those it consumes
    QMetaType type; // Of the pointer, e.g. QWidget*
    Exposure exposure = Exposure::Provided;

    [[nodiscard]] bool operator==(const ExposedInterface& other) const = default;
};

// A property a stargate exposes: one of a component of its galaxy, of one of the objects that component added with
// Component::addPropertiesForQObject(), or one a galaxy among them exposes in turn.
struct ExposedProperty
{
    QString name;                   // Its own among those its galaxy exposes
    Component* component = nullptr; // One of the galaxy of the stargate
    QObject* object = nullptr;      // component itself, or one of its objects; component when it is a galaxy
    QString property;               // Of object; one component exposes when it is a galaxy

    [[nodiscard]] bool operator==(const ExposedProperty& other) const = default;
};

// Where a galaxy meets what it is placed in: a component Core builds itself, see Universe::addStargate(), which exposes
// interfaces and properties of the components of its galaxy. Once the galaxy is placed, see
// Universe::instantiateGalaxy(), it has what each of its stargates exposes: it provides and consumes their interfaces,
// and their properties are its own, see Galaxy. A galaxy can have several stargates. Its own interfaces are those it
// exposes, the other way round: it consumes, from a component of its galaxy, an interface its galaxy provides, and
// provides, to components of its galaxy, one its galaxy consumes. They are bound as those of any component, the ones it
// consumes being mandatory: it exposes nothing until they are bound. The pointer passes as it is, and so does its
// availability: when the one that provides it withdraws it, those that consume it on the other side are told before it
// goes. In the application itself, which nothing places, they only pass what its components give them. What it exposes
// is not changed while it runs. In a galaxy placed, what it is changed with is what the galaxy has from then on: an
// interface exposed is provided or consumed by the galaxy, what is bound to one renamed is still bound, and what was
// bound to one removed no longer is; the other instances of the galaxy have it once its file is saved, see
// Universe::reloadGalaxy(). What the galaxy consumes is mandatory when one of its components needs it as the galaxy is
// read; one exposed since is not, until the galaxy is read again.
class CORE_EXPORT Stargate : public Component
{
    Q_OBJECT

  public:
    // Its componentId(): Core creates the stargates, they are not registered in ComponentFactory.
    static constexpr QLatin1StringView Id{"Nebula.Stargate"};

    // Unbound from what is bound to its interfaces, and from what they are bound to.
    ~Stargate() override;

    [[nodiscard]] QString componentId() const override;

    // In the order they were exposed.
    [[nodiscard]] QList<ExposedInterface> interfaces() const;
    // The one it exposes as name, the way exposure tells; nullptr when it exposes none.
    [[nodiscard]] const ExposedInterface* exposedInterface(Exposure exposure, const QString& name) const;
    // Exposes, as name, an interface of type: it consumes it, for its galaxy to provide it, or provides it, for its
    // galaxy to consume it, as exposure tells. Bound to nothing yet. Returns false, changing nothing, when it is not
    // changed, see Stargate, when type is not valid, and when name is empty or the name of an interface it, or another
    // stargate of its galaxy, exposes the same way.
    bool exposeInterface(Exposure exposure, const QString& name, QMetaType type);
    // Names name, an interface it exposes the way exposure tells, newName: what is bound to it still is. Returns false,
    // changing nothing, when it does not expose name, and as exposeInterface() does for newName.
    bool renameInterface(Exposure exposure, const QString& name, const QString& newName);
    // No longer exposes name, the way exposure tells: what was bound to it no longer is. Does nothing when it does not
    // expose name, or is not changed.
    void removeInterface(Exposure exposure, const QString& name);

    // In the order they were exposed.
    [[nodiscard]] const QList<ExposedProperty>& properties() const;
    // The one it exposes as name; nullptr when it exposes none.
    [[nodiscard]] const ExposedProperty* property(const QString& name) const;
    // Exposes property under its name. Returns false, changing nothing, when it is not changed, when the name is empty
    // or the name of a property it, or another stargate of its galaxy, exposes, when it is not a property a project
    // holds, see Universe::save(), of a component of its galaxy, nor one a galaxy among them exposes, and when a
    // stargate of its galaxy exposes it already. It leaves once its object, or its component, is destroyed.
    bool exposeProperty(const ExposedProperty& property);
    // Exposes name, a property it exposes, as newName. Returns false, changing nothing, when it does not expose name,
    // and as exposeProperty() does for newName.
    bool renameProperty(const QString& name, const QString& newName);
    // No longer exposes name. Does nothing when it does not expose it, or is not changed.
    void removeProperty(const QString& name);

    // Whether what it exposes can be changed: not while it runs.
    [[nodiscard]] bool isChangeable() const;

  signals:
    // What it exposes changed: an interface or a property was exposed, renamed or no longer is.
    void changed();

  private:
    friend class Universe; // Creates it
    friend class Galaxy;   // Has what it exposes

    // An interface it exposes, with what it passes.
    struct Passage
    {
        ExposedInterface info;
        int id = 0;              // What its callbacks know it by: its name can change
        void* pointer = nullptr; // What it was given: nullptr while it has nothing to pass
    };

    // Of the galaxy galaxy, one of application, or of the application itself when galaxy is nullptr.
    Stargate(Universe& application, Galaxy* galaxy);

    // The components of its galaxy, itself among them.
    [[nodiscard]] const std::vector<std::unique_ptr<Component>>& galaxyComponents() const;
    // The stargates of its galaxy, itself among them.
    [[nodiscard]] QList<const Stargate*> galaxyStargates() const;
    // Whether name is free for an interface exposed the way exposure tells: no stargate of its galaxy exposes it so.
    [[nodiscard]] bool isFreeInterfaceName(Exposure exposure, const QString& name) const;
    // Whether name is free for a property: no stargate of its galaxy exposes one under it.
    [[nodiscard]] bool isFreePropertyName(const QString& name) const;
    // Whether name can be renamed newName: it can be changed, and newName is not empty and, unless it is name, free
    // as isFree tells.
    [[nodiscard]] bool canRename(const QString& name, const QString& newName, bool isFree) const;
    // Whether component is one of its galaxy, other than a stargate.
    [[nodiscard]] bool isOfGalaxy(const Component* component) const;
    // Whether property can be exposed: under a free name, of a component of its galaxy that has it to expose, and
    // exposed by no stargate of its galaxy yet, whatever its name.
    [[nodiscard]] bool canExpose(const ExposedProperty& property) const;
    // Whether a stargate of its galaxy exposes property already, under any name: it would be set twice.
    [[nodiscard]] bool isExposed(const ExposedProperty& property) const;
    // Whether a component of its galaxy consumes name, which it provides, as Mandatory.
    [[nodiscard]] bool isNeeded(const QString& name) const;
    [[nodiscard]] Passage* passage(int id);
    [[nodiscard]] Passage* passage(Exposure exposure, const QString& name);
    // Keeps pointer, given for the passage id.
    void keep(int id, void* pointer);
    // Has its galaxy provide what the passage id, exposed the Provided way, is given, or nothing once it is no longer
    // available; once it is attached.
    void passToGalaxy(int id, bool available);
    // Provides to the components of its galaxy what the passage id, exposed the Consumed way, is given, or nothing
    // once it is no longer available.
    void passToComponents(int id, bool available);
    // Opens exposed, just exposed: it consumes or provides it, and its galaxy, once it is attached, the other way.
    void open(const Passage& exposed);
    // Consumes exposed, exposed the Provided way: what a component of its galaxy provides to it, its galaxy provides,
    // once it is attached.
    void openProvided(Passage exposed);
    // Provides exposed, exposed the Consumed way: what its galaxy is given, once it is attached, see attach().
    void openConsumed(Passage exposed);
    // Names renamed newName, on both sides, and tells it.
    void renamePassage(Passage& renamed, const QString& newName);
    // Renames name, exposed the Provided way: it consumes it, and its galaxy, once it is attached, provides it.
    void renameProvided(const QString& name, const QString& newName);
    // Renames name, exposed the Consumed way: it provides it, and its galaxy, once it is attached, consumes it.
    void renameConsumed(const QString& name, const QString& newName);
    // Withdraws name, exposed the way exposure tells, on both sides, while it is still a passage: what it passes is
    // withdrawn first.
    void close(Exposure exposure, const QString& name);
    void closeProvided(const QString& name);
    void closeConsumed(const QString& name);
    // Gives its galaxy what it exposes, once the galaxy is filled: from then on, what one side is given, the other
    // side has, and what it exposes, its galaxy has. What its galaxy consumes is mandatory there when a component of
    // the galaxy needs it.
    void attach();
    // Gives its galaxy exposed, see attach().
    void giveToGalaxy(const Passage& exposed);
    // Has its galaxy consume exposed, which it exposes the Consumed way, as requirement tells, and pass it what it is
    // given.
    void consumeForGalaxy(const Passage& exposed, InterfaceRequirement requirement);
    // Gives its galaxy nothing more: it is being destroyed.
    void detach();
    // Forgets the properties of object, or of its component, being destroyed.
    void forget(QObject* object);

    Universe& m_application;
    Galaxy* m_galaxy = nullptr; // The one it is in; nullptr when it is in the application
    bool m_attached = false;    // Whether its galaxy has what it exposes
    QList<Passage> m_passages;  // In the order they were exposed
    int m_nextId = 0;
    QList<ExposedProperty> m_properties; // In the order they were exposed
};

} // namespace Nebula
