// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QMetaObject>
#include <QString>
#include <QtPlugin>
#include <memory>
#include <string_view>

namespace Nebula
{

class Component;

// What a plugin is to Nebula: the factory of its components. Its root object implements it, the class nebula_add_plugin
// generates, deriving from Plugin, in which the components register with NEBULA_REGISTER_COMPONENT.
class IPlugin
{
  public:
    virtual ~IPlugin() = default;

    // __DATE__ " " __TIME__ of the plugin, e.g. "Oct  6 2026 21:14:03".
    [[nodiscard]] virtual std::string_view buildDate() const = 0;

    // The components it creates, one per NEBULA_REGISTER_COMPONENT, in the order they were registered.
    [[nodiscard]] virtual QList<const QMetaObject*> components() const = 0;
    // Creates its component named name, see componentName(), in the Editing state; nullptr when it has none of that
    // name.
    [[nodiscard]] virtual std::unique_ptr<Component> create(const QString& name) const = 0;
};

} // namespace Nebula

// Versioned: PluginManager skips a plugin built against another version of IPlugin.
#define NEBULA_PLUGIN_IID "FabienDG.Nebula.IPlugin/2.0"

Q_DECLARE_INTERFACE(Nebula::IPlugin, NEBULA_PLUGIN_IID)
