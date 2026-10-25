// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QMetaType>
#include <QString>
#include <cstdint>

namespace Nebula
{

// Whether a component needs an interface it consumes to work, see Component::consumeInterface(). Only told to who
// assembles components, as NebulaStudio does: nothing checks it, and a component starts without a mandatory interface
// as it does with it.
enum class InterfaceRequirement : std::uint8_t
{
    Optional,
    Mandatory,
};

// An interface of a component: a pointer it provides under a name, see Component::provideInterface(), or one it
// consumes, see Component::consumeInterface(), or the pointers it consumes as a list, see
// Component::consumeInterfaceList().
struct InterfaceInfo
{
    QString name;   // Unique among the interfaces its component provides, or among those it consumes
    QMetaType type; // Of the pointer, e.g. QWidget*
    // Whether the component has the pointer: it provides one, or the one it consumes is set; for a list, whether it
    // has one at least.
    bool available = false;
    // Whether the component needs the one it consumes; Optional for one it provides.
    InterfaceRequirement requirement = InterfaceRequirement::Optional;
    // Whether it consumes a list of them, to which several interfaces can be bound; false for one it provides.
    bool list = false;
};

class Component;

// What an interface a component consumes is bound to, or one of what a list it consumes is: see
// Component::bindInterface().
struct InterfaceBinding
{
    Component* provider = nullptr; // nullptr when it is bound to nothing
    QString providedName;          // What provider provides it as
};

} // namespace Nebula
