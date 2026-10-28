// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QCoreApplication>
#include <QMetaProperty>
#include <QTextStream>
#include <memory>
#include <optional>

#include "Nebula/Component/ComponentFactory.h"
#include "Nebula/Plugin/PluginManager.h"

namespace
{

QString toString(Nebula::ComponentState state)
{
    switch(state)
    {
    case Nebula::ComponentState::Editing:
        return QStringLiteral("Editing");
    case Nebula::ComponentState::Running:
        return QStringLiteral("Running");
    }
    return {};
}

// Loads the plugin pluginId, and tells out when it was built, or why it is not loaded.
void loadPlugin(Nebula::PluginManager& plugins, const QString& pluginId, QTextStream& out)
{
    if(!plugins.load(pluginId))
    {
        out << "  not loaded: " << plugins.errorString() << '\n';
        return;
    }
    if(const std::optional<Nebula::PluginInfo> loaded = plugins.plugin(pluginId))
    {
        out << "  built " << loaded->buildDate.toString(Qt::ISODate) << '\n';
    }
}

// Lists the plugins of plugins/ on out, and loads them: scan() reads their metadata without loading them; load()
// reads their build date and registers their components.
void loadPlugins(Nebula::PluginManager& plugins, QTextStream& out)
{
    for(const Nebula::PluginInfo& info : plugins.scan())
    {
        out << "Plugin " << info.id << ' ' << info.version.toString() << ": " << info.description << '\n';
        loadPlugin(plugins, info.id, out);
    }
}

// Lists properties on out: their name, their type, and whether they can be written.
void listProperties(const QList<QMetaProperty>& properties, QTextStream& out)
{
    for(const QMetaProperty& property : properties)
    {
        out << "  " << property.name() << ": " << property.typeName() << (property.isWritable() ? "" : ", read-only")
            << '\n';
    }
}

// Lists the components factory registers on out, with their properties.
void listComponents(const Nebula::ComponentFactory& factory, QTextStream& out)
{
    for(const Nebula::ComponentInfo& component : factory.components())
    {
        out << "\nComponent " << component.id << " [" << component.category << "]: " << component.description << '\n';
        listProperties(component.properties, out);
    }
}

// Tells on out what greeter does: its state, and its interfaces as they become available or not.
void follow(const Nebula::Component& greeter, QTextStream& out)
{
    QObject::connect(&greeter, &Nebula::Component::stateChanged,
                     [&out](Nebula::ComponentState state) { out << "  -> " << toString(state) << '\n'; });
    QObject::connect(&greeter, &Nebula::Component::interfaceAvailable,
                     [&out](const QString& name) { out << "  -> " << name << " available\n"; });
    QObject::connect(&greeter, &Nebula::Component::interfaceUnavailable,
                     [&out](const QString& name) { out << "  -> " << name << " unavailable\n"; });
}

// Tells on out the state of greeter, just created, and its interfaces: declared as soon as it is built, the Greeter
// only provides its own while it runs.
void describe(const Nebula::Component& greeter, QTextStream& out)
{
    out << "\nGreeter created: " << toString(greeter.state()) << '\n';
    for(const Nebula::InterfaceInfo& provided : greeter.providedInterfaces())
    {
        out << "  provides " << provided.name << ": " << provided.type.name()
            << (provided.available ? "" : ", unavailable") << '\n';
    }
}

// Runs greeter, which greets its recipient while it runs, and tells the greeting on out.
void run(Nebula::Component& greeter, QTextStream& out)
{
    greeter.setProperty("recipient", QStringLiteral("Nebula"));
    greeter.start();
    out << "  greeting: " << greeter.property("greeting").toString() << '\n';
    greeter.stop();
}

// Creates a Greeter, which only the factory does: `new Greeter` does not compile. Then tells on out what it is and
// does as it runs. Returns 1 when it cannot be created, 0 otherwise.
int runGreeter(QTextStream& out)
{
    const std::unique_ptr<Nebula::Component> greeter =
        Nebula::ComponentFactory::instance().create(QStringLiteral("FabienDG.Nebula.ExamplePlugin.Greeter"));
    if(greeter == nullptr)
    {
        return 1;
    }
    follow(*greeter, out);
    describe(*greeter, out);
    run(*greeter, out);
    return 0;
}

} // namespace

int main(int argc, char* argv[])
{
    const QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    Nebula::PluginManager plugins;
    loadPlugins(plugins, out);
    listComponents(Nebula::ComponentFactory::instance(), out);
    return runGreeter(out);
}
