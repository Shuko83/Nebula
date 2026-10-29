# 🚀 Nebula

<img src="NebulaStudio/resources/nebula.svg" width="96" alt="The logo of Nebula">

**Nebula** is a C++ framework for building modular applications by assembling dynamic components.

It enables developers to construct software systems by combining reusable building blocks (*components*) provided by extensible plugins. Each component exposes configurable properties and communicates with others through explicit connections.

---

## ✨ Features

- 🔌 **Plugin-based architecture**  
  Load and extend functionality dynamically via plugins.

- 🧩 **Component-oriented design**  
  Build applications by assembling independent components.

- ⚙️ **Runtime configuration**  
  Components expose `Q_PROPERTY` for introspection and configuration.

- 🔗 **Flexible connections**  
  Components interact through explicit bindings.

- 💾 **Project serialization**  
  Save and load full application state using `.nbp` files.

- 🗂️ **User settings**  
  Keep the values the user of an application gives some properties, from one run to the next, in settings files.

---

## 🧠 Core Concepts

| Concept     | Description |
|------------|------------|
| Plugin     | Provides component implementations |
| Component  | A reusable functional unit |
| Instance   | A runtime instantiation of a component |
| Binding    | A connection between components |
| Project    | A full application definition |
| Settings file | The properties of instances whose values the user keeps from one run to the next |
| Galaxy     | A project placed in another one as a single instance, as many times as wanted, and edited through any of them |
| Stargate   | What a galaxy exposes once placed: interfaces and properties of its instances |

The common vocabulary of the framework, Nebula, Univers, Galaxy, Astre, Comet and Stargate, is defined in the [glossary](docs/glossary.md), with the name each of these notions has in the code today.

---

## 🔌 Writing a Plugin

A plugin is declared with `nebula_add_plugin`, available once `Core` is added, or found in an installed Nebula (see [Building a plugin outside Nebula](#building-a-plugin-outside-nebula)). CMake generates its metadata and its class, `<NAME>` in `<NAME>.h`, in which its components are registered (see below): the plugin only writes its components. That class is the plugin itself, the factory of its components: its root object, which implements `Nebula::IPlugin`, `components()` listing them and `create(name)` creating one.

```cmake
nebula_add_plugin(
    NAME Network
    DESCRIPTION "TCP and UDP components"   # optional
)
```

It accepts every keyword of `cmu_add_target` ([CMakeUtils](CMakeUtils/README.md)) and creates a `MODULE` library `Nebula::<NAME>` linked to `Nebula::Core`, named `<NAME>.nbdl` (Nebula DLL) and written to `plugins/` next to the executables (`CMAKE_RUNTIME_OUTPUT_DIRECTORY`, else `<build>/bin`). It is installed in `plugins/` next to each application too (`cmu_plugins_dir` of CMakeUtils); `set(cmu_plugins_dir "")` before the call keeps a plugin out of it. Its metadata is embedded in the binary and read without loading it:

| Key           | Value                                                                        |
| ------------- | ---------------------------------------------------------------------------- |
| `id`          | `<cmu_organization>.<PROJECT_NAME>.<NAME>`, e.g. `FabienDG.Nebula.Network` |
| `name`        | `NAME`                                                                       |
| `description` | `DESCRIPTION`, empty when not given                                          |
| `version`     | `VERSION`, else `PROJECT_VERSION`, else `0.0.0`                              |
| `coreVersion` | Version of `Core` the plugin is built against                                |

Its build date (`__DATE__` and `__TIME__`) is read when it is loaded. `NEBULA_PLUGIN_ID` holds the id while the plugin compiles.

`Nebula::PluginManager` finds and loads the plugins:

```cpp
Nebula::PluginManager plugins;
for(const Nebula::PluginInfo& info : plugins.scan()) // <application dir>/plugins, nothing is loaded
{
    qInfo() << info.id << info.version << info.description;
}
plugins.load(QStringLiteral("FabienDG.Nebula.Network"));
qInfo() << plugins.plugin(QStringLiteral("FabienDG.Nebula.Network"))->buildDate;
```

`scan()` skips, with a warning, the files that are not Nebula plugins, the plugins built against another major version of `Core` or a newer one, and the ids already found. `addFile(filePath)` adds a single plugin, anywhere on the disk, with the same checks; it tells why a file cannot be added through `errorString()`. Loading a plugin registers its components. `unload(pluginId)` forgets them and destroys its root object: none of its components may still be instantiated then, which `Universe::unloadPlugin()` sees to. A `PluginManager` unloads the plugins it loaded when it is destroyed: it has to outlive their components, as the one of a `Universe` does. An unloaded plugin can be loaded again, but its library stays in memory until the application exits: its file cannot be replaced meanwhile, and loading it again does not read the file again.

The [exemple](exemple/) folder holds `ExamplePlugin`, a plugin providing the `Greeter` component, `WidgetsPlugin`, next to it, whose components make the main window of an application (see [NebulaStudio](#-nebulastudio)), and `ExampleApp`, which loads the plugins next to it, lists their components and runs a `Greeter`, whose interface it follows: run it from `<build>/bin` (`<build>/bin/<config>` with Visual Studio).

### Building a plugin outside Nebula

An installed Nebula comes with `nebula_add_plugin`, its templates and the CMakeUtils it relies on: a project of its own only has to find the package.

```cmake
cmake_minimum_required(VERSION 3.19)
project(Acme VERSION 1.0.0 LANGUAGES CXX)

set(cmu_organization "AcmeCorp")   # first part of the plugin ids: AcmeCorp.Acme.Network

find_package(Nebula 2.0 REQUIRED COMPONENTS Core)

add_subdirectory(Network)          # nebula_add_plugin(NAME Network), its sources in Network/src
```

Point `CMAKE_PREFIX_PATH` at the package: `<build>/package`, staged after each build of Nebula, the prefix `cmake --install` wrote it to, or the unzipped package of a [release](https://github.com/Shuko83/Nebula/releases). `find_package` defines `Nebula::Core` and `nebula_add_plugin`, and includes CMakeUtils unless the project already did: call it after `project()`, in the root `CMakeLists.txt`, with the `cmu_*` variables set before it. Qt is found as in any project, through `CMAKE_PREFIX_PATH` or the `QTDIR` environment variable.

The plugin is written to `<build>/bin/plugins` (`<build>/bin/<config>/plugins` with Visual Studio): copy it to `plugins/` next to an application, or open it from NebulaStudio with **Open plugin...**. Its `coreVersion` is the version of the installed `Core`, whatever the version of its project. Build it in a configuration Nebula was installed in: Qt does not load a `Debug` plugin into a `Release` application, nor the other way round.

Installed with `-Dcmu_install_mode=PACKAGE`, Nebula is a single prefix whose package has no components: `find_package(Nebula 2.0 REQUIRED)`.

---

## 🧩 Writing a Component

A component belongs to a plugin. It derives from `Nebula::Component`, its name ends with `Component`, `MainWindowComponent` say, and it registers itself in the class of its plugin. It creates the objects it is made of itself, when it is constructed: a window, a timer, a class of its own holding its logic. It tells what can be set and followed with `Q_PROPERTY`, signals and `Q_CLASSINFO`, and meets the other components through the interfaces it provides and consumes, see below. The class of its plugin knows nothing of its objects: it only lists and creates the components of the plugin.

```cpp
// MainWindowComponent.h
class MainWindowComponent : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "The main window of the application, which other components fill") // optional
    Q_CLASSINFO("category", "Widgets")                                                               // optional
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    ...
  private:
    QMainWindow m_window; // Created with the component, destroyed with it
};

// MainWindowComponent.cpp
#include "WidgetsPlugin.h" // The class of the plugin, generated by nebula_add_plugin(NAME WidgetsPlugin)

NEBULA_REGISTER_COMPONENT(WidgetsPlugin, MainWindowComponent);
```

The plugin class is generated by `nebula_add_plugin`, named after the plugin (a `NAME` that is not a C identifier has its other characters replaced by `_`), in the build directory, which the plugin includes: it has nothing in it to write. Its components register in it as its library loads, before Qt creates it as the root object of the plugin. Registering a component in a class that does not derive from `Nebula::Plugin<itself>` does not compile, nor in one that has a `build()` for it, which Nebula no longer calls: what a component is made of is created in its constructor. `NEBULA_PLUGIN`, with which a plugin declared its class until 1.0.0, no longer compiles and tells to remove that class.

`Nebula::ComponentFactory::instance()` knows the components of the loaded plugins, read from their `QMetaObject` without instantiating them:

| Info          | Value                                                                    |
| ------------- | ------------------------------------------------------------------------ |
| `id`          | `<plugin id>.<name>`, e.g. `FabienDG.Nebula.ExamplePlugin.Greeter`     |
| `name`        | Class name, without its namespace                                        |
| `description` | `Q_CLASSINFO("description", ...)`, empty when not given                  |
| `category`    | `Q_CLASSINFO("category", ...)`, empty when not given                     |
| `properties`  | Its `Q_PROPERTY`, including those of its base classes up to `Component`  |

The factory has the plugin of a component create it, and is the way to instantiate one: the plugin instantiates a class its registration derives from the component, whose destructor ends what the component provides and consumes before the component's own members are destroyed. A component created otherwise, `new Greeter`, would still compile, but would tell those that consume its interfaces once these members are gone. A component therefore needs a default constructor and cannot be `final`; registering a class that is not a component, or lacks `Q_OBJECT`, does not compile either.

```cpp
std::unique_ptr<Nebula::Component> greeter =
    Nebula::ComponentFactory::instance().create(QStringLiteral("FabienDG.Nebula.ExamplePlugin.Greeter"));
greeter->setProperty("recipient", QStringLiteral("Nebula")); // Editing: being configured
greeter->start();                                               // Running
greeter->stop();                                                // Editing
```

A component has two modes: `Editing`, in which it is created, its properties set and its interfaces bound, and `Running`. Each step calls a hook the component can override (`onStart()`, `onStop()`); `onStart()` can refuse it by returning `false`. `stateChanged()` is emitted on every change.

A component also provides interfaces: pointers it makes available under a name, such as a `QWidget`, a class of its plugin, or both. It declares each one with `provideInterface<T>(name)` when it is built, so that it is known before the component runs, and calls it again with the pointer once it has one:

```cpp
Greeter::Greeter()
{
    provideInterface<IGreeter>(QStringLiteral("greeter")); // Declared: listed, but unavailable
}

void Greeter::setGreeter(IGreeter* greeter) // Called by its logic
{
    provideInterface<IGreeter>(QStringLiteral("greeter"), greeter); // Available; withdrawn with nullptr
}
```

`provideInterface()` is protected: only the component provides its interfaces. `T` is the type the pointer is retrieved with and is always written: `provideInterface<QWidget>(name, label)` provides a `QLabel` as a `QWidget`. A name keeps the type it was declared with.

The pointer can change while the component lives. Providing another one emits `interfaceUnavailable(name)`, replaces the pointer, then emits `interfaceAvailable(name)`; `interfaceUnavailable()` is also emitted when the interface is withdrawn and when the component is destroyed. Whoever retrieved the pointer stops using it then.

```cpp
for(const Nebula::InterfaceInfo& provided : component->providedInterfaces())
{
    qInfo() << provided.name << provided.type.name() << provided.available; // "greeter" IGreeter* false
}
IGreeter* greeter = component->providedInterface<IGreeter>(QStringLiteral("greeter")); // nullptr while unavailable
```

A component consumes interfaces the same way: `consumeInterface<T>(name, member, onAvailable, onUnavailable)` declares, when it is built, that one of its members is the pointer it gets under a name. The member is `nullptr` until the interface is bound to one another component provides, and that one is available:

```cpp
Window::Window()
{
    consumeInterface<QWidget>(
        QStringLiteral("content"), m_content,  // QWidget* m_content
        [this] { emit contentAvailable(); },   // m_content is set
        [this] { emit contentUnavailable(); }); // m_content is about to be set back to nullptr
}
```

Both callbacks are optional. `onUnavailable` is called while the member still points to what it has to let go of: when the provider withdraws or replaces its pointer, when the interface is unbound, and when one of the two components is destroyed.

A component that does not work without an interface it consumes says so with `Nebula::InterfaceRequirement::Mandatory`, after the member; interfaces are `Optional` otherwise. It is only told to who assembles components, by the `requirement` of `consumedInterfaces()`, and NebulaStudio marks the interface with an asterisk: nothing checks it, and the component starts without it all the same. `unboundMandatoryInterfaces()` names those of its mandatory interfaces that are bound to nothing: a component is operational when there is none, and NebulaStudio shows those that are not.

```cpp
consumeInterface<QWidget>(
    QStringLiteral("content"), m_content, Nebula::InterfaceRequirement::Mandatory,
    [this] { emit contentAvailable(); }, [this] { emit contentUnavailable(); });
```

A component can also consume a list of interfaces of a type, to which several interfaces other components provide are bound, the observers of a subject, say: `consumeInterfaceList<T>(name, member, onInserted, onRemoved)`, `member` being a `QList<T*>`, which holds the pointers of those that are available, in the order they were bound. `onInserted` is called with a pointer once it is in the list, `onRemoved` with one before it leaves the list, the last time it can be used, in the same cases as `onUnavailable`. `Nebula::InterfaceRequirement::Mandatory`, after the member, says that the component needs one of them at least:

```cpp
Subject::Subject()
{
    consumeInterfaceList<IObserver>(
        QStringLiteral("observers"), m_observers, // QList<IObserver*> m_observers
        [this](IObserver* observer) { observer->update(m_state); }, // In m_observers
        [](IObserver* observer) { observer->update({}); });         // About to leave m_observers
}
```

`bindInterface()` binds what a component consumes to what another one provides, whatever their names, as long as they have the same type; an interface can be provided to several consumers, and a consumer is bound to one provider at a time, but for a list, to which each binding adds one, after the others, and which is not bound twice to the same interface:

```cpp
window->bindInterface(QStringLiteral("content"), *ticker, QStringLiteral("display")); // false when it cannot be
window->unbindInterface(QStringLiteral("content"));
subject->unbindInterface(QStringLiteral("observers"), *observer, QStringLiteral("observer")); // That one only
```

`consumedInterfaces()` lists what a component consumes, as `providedInterfaces()` does, `list` telling a list apart, `binding(name)` tells what one of them is bound to, the provider and the name it provides it under, `bindings(name)` what a list is bound to, in its order, and `interfacesChanged()` is emitted whenever an interface is declared, bound or unbound, or becomes available or unavailable, on either side.

A component can also answer for the properties of the objects it is made of, a widget of its logic, say, with `addPropertiesForQObject()`, which only the component calls, as it does for its interfaces:

```cpp
void Ticker::setDisplay(QWidget* display) // Called by TickerLogic, which owns the display
{
    provideInterface<QWidget>(QStringLiteral("display"), display);
    addPropertiesForQObject(display, QStringLiteral("Display"));
}
```

NebulaStudio then edits the properties the object declares with `Q_PROPERTY`, base classes included, under those of the component, and a project holds their values with those of the component. Not `objectName`, nor the properties declared `DESIGNABLE false` or `STORED false`, as the visibility of a widget is. They are edited in a group titled with the name, shared by the objects added with the same one, or right after the properties of the component when no name is given. `propertyObjects()` lists the objects added, each for as long as it lives, and `propertyObjectsChanged()` is emitted when one is added or destroyed. A component adds its objects when it is built: a project sets the properties of the objects it finds when it is opened.

What a component provides, consumes and is made of can also depend on one of its properties: the labels it provides, as many as its `count`, say. As the property is set, the component declares the interfaces it gains with `provideInterface()` and adds the properties of their objects with `addPropertiesForQObject()`; it removes those it loses with `removeProvidedInterface(name)`, while the pointer still refers to something, then destroys their objects, which takes their properties away:

```cpp
void LabelsComponent::addLabel() // As its count increases
{
    const QString name = QStringLiteral("label%1").arg(count() + 1);
    Label& label = *m_labels.emplace_back(std::make_unique<Label>(name)); // A QObject holding a QLabel and its text
    addPropertiesForQObject(&label, name);
    provideInterface<QWidget>(name, &label.widget());
}

void LabelsComponent::removeLabel() // As its count decreases: the last one first
{
    removeProvidedInterface(QStringLiteral("label%1").arg(count())); // While the label still exists
    m_labels.pop_back();
}
```

`removeProvidedInterface()` tells what is bound to the interface that it is unavailable, as `interfaceUnavailable(name)` does, then unbinds it and emits `interfaceRemoved(name)`: a list keeps its other bindings. `removeConsumedInterface(name)` unbinds an interface the component consumes, telling `onUnavailable`, and forgets it. Both are protected, and do nothing for a name the component does not have. A project gives a component its properties in the order it declares them, then the properties of its objects, then its bindings: the property that tells what it has is declared before those that depend on it, and the objects and interfaces it gives are there to be set and bound. Such a property is best left out of the settings files: what it takes away while the application runs loses its links, which stopping it does not bring back.

---

## 🔨 NebulaStudio

`NebulaStudio` is the application that assembles applications from the components of the plugins, live.

It starts with a splash screen, in the colors of its theme: the version of Nebula, the Qt and the compiler NebulaStudio was built with and when, its copyright and its license. Its main window takes its place after two seconds, or as soon as it is clicked.

For now, it lists:

- the plugins of `plugins/` next to it, and those of the directories given on its command line, each with `--plugin-dir`: `NebulaStudio --plugin-dir <directory> --plugin-dir <another one>`, scanned after `plugins/`, in their order, a plugin found again being skipped. A viewer shows the selected one: its metadata, its state, a **Load** button and an **Unload** one; **Open plugin...** loads a plugin from anywhere on the disk. Unloading a plugin takes its components off the list of components. When instances of them are in the edition zone, NebulaStudio names them and asks first: they are removed with the plugin, with their links, or nothing is done. A plugin is not unloaded while the application runs: **Unload** is disabled until it is stopped. An unloaded plugin is loaded again with **Load**, or by opening a project that needs it; its file stays in use until NebulaStudio is closed, so a plugin built again is only read by the next NebulaStudio;
- next to the plugins, in the **Galaxies** tab, the galaxies of a directory, `scene/` next to NebulaStudio until **Choose directory...** gives another one: each project file there is a galaxy, named after its file, the icon of the Galaxy beside it. A galaxy can be loaded when its file can be read and NebulaStudio knows each of its plugins, found by their id among those it lists, loaded or not; the others are dimmed, with a warning mark beside their state, **Missing plugins** or **Unreadable**. The viewer of the selected one shows its name, its description, its file, its plugins and its state; when it cannot be loaded, an error in amber tells why: the plugins it misses, to open with **Open plugin...** or to give with `--plugin-dir`, or why its file cannot be read. **Open**, or double-clicking a galaxy that can be loaded, opens it in place of the application, once its changes are saved or given up, as **Open...** does; not while the application runs. The list follows the directory as its files are added, removed, renamed or saved, and a plugin opened with **Open plugin...** makes the galaxies that missed it loadable at once. Only the id of a plugin counts here: the `file` the project gives it is not looked at, unlike when the project is opened. A galaxy that can be loaded is dragged into the edition zone as a component is, see below; the others cannot be;
- in the edition zone, the galaxies placed there: a galaxy dropped from the **Galaxies** tab is placed in the application as one component, a Galaxy, as many times as wanted, each named after it and a number (`Hello1`, `Hello2`...) and with its own components, those of its file, bound as its file binds them and with the values it gives them; the plugins it needs are loaded then, by their id. Its item shows the icon of the Galaxy before the name of its galaxy, and its tooltip its file and its description. A galaxy that holds itself, directly or not, or the project it is dropped into, is refused, which the status bar tells, as it tells what its file misses when part of it cannot be restored. Its interfaces are those its stargates expose, see below, bound as those of any instance, and selecting it shows, under the edition zone, the properties they expose, under the names they expose them as, which are edited there for this instance only: the same properties, on the same objects, as those of its components. When an interface it consumes is needed by one of its components, the galaxy needs it too: bound to nothing, it is shown as an instance that does not work. Double-clicking it below its name, or **Open galaxy** in its context menu, shows it in place of the application: its components, their links, its settings files, its stargates and its zones, where its file shows them. A path above the edition zone, **Application** › `Home1` › `Hello1`, tells where it is and goes back to any level; a galaxy it holds opens the same way. It runs with the application, its components started in their order. Its settings files are among those of the application, named after it, as each instance has them: `settings.json` of `Hello1` is written as `Hello1.settings.json`, and is renamed with it. Unloading a plugin removes the galaxies that hold one of its components, after the question; the application is shown again when the galaxy shown goes;
- a galaxy opened is edited there as the application is: components and galaxies dropped, linked, renamed, colored and removed, zones, properties, settings files and stargates. What is changed there is changed in its file once it is saved, for every instance of it: **Save Galaxy** (Ctrl+Alt+S), in the **File** menu or at the end of the path, writes it to its file, then reads each instance of that file in the application again from it, the one edited included. Each keeps its name, the values of the properties it exposes and what is bound to its interfaces, under the names they still have; what one could not keep, a link to an interface its stargates no longer expose, say, is told in a message box. The values of the properties the galaxy exposes are written as the instance edited has them: those of the instances placed from then on, the others keeping theirs. Leaving a galaxy changed since it was shown, through the path, **New**, **Open...**, the list of problems or by closing NebulaStudio, asks whether to save it: **Save** saves it as **Save Galaxy** does, **Discard** reads it again from its file, and **Cancel** stays in it; a galaxy further down the path is asked about first. A stargate changed in a galaxy opened changes the galaxy at once, where it is placed: an interface exposed is one more of the galaxy, one renamed is still bound to what it was bound to, and one removed is no longer bound to anything; the other instances have it once it is saved. A galaxy dropped into a galaxy that it would hold, directly or not, is refused. A settings file added in a galaxy is named as its file names it, `settings.json`, `settings2.json`..., each instance writing its own, named after it: its dialog edits that name, and tells the name of the file of this instance. While the application runs, nothing is edited in a galaxy, **Save Galaxy** is disabled, and a galaxy with changes is not left until the application is stopped; closing NebulaStudio meanwhile only writes it, if asked to, without reading its instances again;
- the components of the loaded plugins, with a viewer of the selected one: what it is and its properties. The search bar above them, also reached with Ctrl+F, keeps those whose name holds what is typed, whatever its case: `lic` finds `maLicence`; next to it, **Description** or **Category** searches their descriptions or their categories instead. When the selected component is no longer listed, another one is selected, or none;
- between them, the edition zone of the application: a component dragged from its list and dropped there is instantiated, named after its component and a number (`Greeter1`, `Greeter2`...), and shown where it was dropped. Its instances can be moved and selected. The interfaces of an instance are listed under its names, each with a dot on an edge: those it consumes on the left, a circle, two stacked for a list, with an asterisk after the name of those the component cannot do without; those it provides on the right, a disc while the interface is available and a dotted circle otherwise. Hovering a dot tells at once, in a tooltip, whether the interface is provided or consumed, its type, whether it is mandatory and its state; it does too while a link is being drawn. Dragging from the dot of a provided interface to the dot of a consumed one of the same type, or the other way round, binds them and draws a link between them, in place of the link the consumed one had; a list keeps its links, the new one coming after them. While the link is being drawn, the dots it cannot be bound to are dimmed. Right-clicking the dot of a consumed interface offers the same links in its context menu, under **Bind to**: the interfaces of its type the other instances of the edition zone provide, named after their instance (`Ticker1.display`), those it is bound to checked, as radio buttons for an interface bound to one at most, as check boxes for a list. Choosing one binds them, as dragging does; choosing a checked one removes their link. When no other instance provides its type, the menu says so.
- an instance that does not work, as an interface it cannot do without is bound to nothing, says so: an exclamation mark in an amber disc, in the top left corner of its names, and the dot and the name of that interface in amber. Its tooltip, and the one of the dot, tell what is missing and how to bind it. Binding the interface takes the marks away; removing what it was bound to, or their link, brings them back.
- an instance is renamed and colored there too. Double-clicking its names, or **Rename component** in its context menu, edits its name in place, until Enter is pressed or anything else is clicked; Escape gives the edition up. An instance keeps a name no other one has, the one a project and `Universe::component()` know it by: a name already taken is refused, which the status bar tells, and so is no name at all. **Change component color...**, in the same menu, colors its names, which tells it from the others at a glance: the color is chosen with its opacity, and lets the instance show through as much as it is translucent. **Reset component color** gives it back the colors of the style.
- an instance is removed there as well: **Remove component**, in its context menu, removes the one under the cursor from the application, and the Delete key removes the selected ones, all of them when several are; while a name is edited, the key is for what is typed. Its links go with it: what consumed its interfaces is no longer bound to anything. An instance is removed at once, without a question. A link is removed the same way, without its instances: clicking it selects it, drawn in the color of the selection, and the Delete key removes it, as **Remove link** does in its context menu; the interface it went into is no longer bound to anything, or, for a list, to what the link came from only. Hovering a link draws it thicker, and its tooltip tells what it binds: `Window1.content is bound to Ticker1.display`. Dragging a rectangle over the edition zone selects the instances in it, but not the links that cross it: the Delete key then removes those instances with their own links, and no other link.
- a link is anchored there as well, to have it go through points of the edition zone, around an instance, say: double-clicking it, or **Add anchor point** in its context menu, adds an anchor where it was clicked, a small diamond on its line, which it goes through wherever its instances are, in the style of the links, from its provided interface to each of its anchors in turn, then to its consumed interface; an anchor added between two others is gone through between them. Dragging an anchor moves it, without moving what is selected. Double-clicking an anchor, or **Remove anchor point** in its context menu, removes it; **Remove anchor points**, in the menu of the link, removes them all. The anchors of a link are saved with the project, and kept when its instances are shown again, as their interfaces change; they go with the link when it is removed. While the application runs, they are neither added, moved nor removed.
- the edition zone is zoomed in and out, from a quarter of its size to four times it: the + and - keys zoom it around its middle, as do **Zoom In** (Ctrl++) and **Zoom Out** (Ctrl+-) in the **View** menu, and Ctrl with the mouse wheel around what is under the cursor; **Actual Size** (Ctrl+0) shows it at its own size again. While a name is edited, + and - are for what is typed. The status bar tells the zoom level. Dragging with the middle button held moves the edition zone, what was under the cursor following it: wherever it is pressed, on an instance as on the background, nothing is selected or moved, and a name being edited stays so. Zooming and moving only change what is seen, while the application is edited as while it runs, and in a galaxy opened as in the application: they are not saved with the project.
- the instances of the edition zone are found from the field above it, on the right, which Ctrl+Shift+F, or **Find Instances** in the **View** menu, gives the keys to: whether a component is instantiated there, and where. As it is typed, it lists the instances whose name, or the name of whose component, holds what is typed, whatever its case, each with the name of its component, in the order of their names: `greet` lists `Greeter1`, `Greeter2`, then `Hello`, an instance of `Greeter` renamed. Choosing one, or pressing Enter, selects it alone, its properties shown under the edition zone, and shows it in the middle of the edition zone. Pressing Enter again shows the next one found, the status bar telling which of them it is (`Greeter2 found, 2 of 3`), the first one again after the last; an instance named what is typed is found alone. When none is found, the status bar says so. What the edition zone shows is searched, the application or the galaxy opened, but not inside the galaxies placed there; it is searched while the application runs as well.
- the links are drawn as curves until another style is chosen in **View** > **Links**: **Curves** go out of the right edge of the provider and into the left edge of the consumer, bending between them; **Straight Lines** go from one dot to the other; **Right Angles** go out and in as curves do, in horizontal and vertical lines, across halfway between the two instances, or around them when the consumer is not far enough to the right of the provider. Every link has the style chosen, in the application as in a galaxy opened, and so does the link being drawn. As the zoom does, it only changes what is seen, while the application runs too: it is not saved with the project. NebulaStudio keeps it for the user who chose it, and draws the links in that style at its next launch, curves until another style is chosen; under Windows, it is in the registry, under `HKEY_CURRENT_USER\Software\FabienDG\NebulaStudio`.
- in the edition zone too, colored zones that group instances, by what they are for, say: **Add zone**, in its context menu, adds one where it was right-clicked, under a title typed at once; the menu of an instance does not offer it. Dragging a zone by its title moves it with what is in it then: the instances whose center is in it, and the zones that are entirely in it. Dragging an edge or a corner resizes it. Double-clicking its title edits it, and right-clicking anywhere in a zone renames it, changes its color or removes it, which leaves what is in it. Between its title and its edges, a zone lets the mouse through: instances are selected there as anywhere else.
- in the edition zone as well, the settings files of the application, which keep the values its user gives some properties, the profile they chose, say, from one run to the next; the value the project gives such a property becomes its default value. **Add settings file**, in the context menu of the edition zone, adds one where it was right-clicked, a sheet titled with the name of its file, and opens its dialog at once: the name of its file, `settings.json` unless another one is given, or `settings2.json`, `settings3.json`... when another file has that name, its directory, the configuration directory of the user unless another one is given, then the properties it holds, checked among those of the instances, and of their objects, that a project saves. A property is held by a single file: one another file holds is listed with the name of that file, and cannot be checked. Several instances can keep their properties in the same file, and an application can have several files, each of its own name: the dialog is not accepted with the name of another file, whatever its case and their directories, as the two would be written over each other. Dashed lines join a file to the instances whose properties it holds, and its tooltip tells where it is written and what it holds. Double-clicking it, or **Edit settings file...** in its context menu, opens its dialog again; **Write default values** writes the values of the project to the file, in place of those of the user, to start trials over or to make the file to deliver; **Remove settings file**, or the Delete key, removes it. Removing an instance removes its properties from the files; renaming it keeps them, but the values its user gave it stay under its former name in the file.
- in the edition zone as well, the stargates of the project, which tell what the galaxy its file is has once placed in another one: the interfaces and the properties of its instances they expose. **Add stargate**, in the context menu of the edition zone, adds one, `Stargate1`, `Stargate2`..., the icon of the Stargate before "Stargate", and opens its dialog at once: under each instance, its interfaces, then its properties, those a project saves, and those a galaxy among them exposes, which it exposes again, to check with a click on their box, each exposed under the name next to it, the one of the interface or of the property until another one is typed: double-clicking a row checks it and edits that name. Its interfaces are those it exposes, the other way round, on the edges of its item: on the left those the galaxy provides, which the stargate consumes from one of its instances, on the right those the galaxy consumes, which the stargate provides to those of its instances that consume it, the same pointer to each of those checked under the same name; they are bound and linked as those of any instance, and the left ones are mandatory: a stargate whose interface is bound to nothing does not work. Its properties are listed under them, with where they are (`Greeter1.recipient`), dashed lines joining it to their instances. A project can have several stargates; what one exposes, another cannot, and two of them do not expose an interface the same way under the same name, nor a property. **Expose...**, in the menu of the dot of an interface of an instance, adds a stargate next to it that exposes that interface, bound to it, and edits the name it exposes it as at once; double-clicking the name of an interface of a stargate renames it, its links kept. Double-clicking a stargate below its name, or **Edit stargate...** in its context menu, opens its dialog again, which also lists what it exposes that is bound to no instance any longer, to keep or not; a stargate is renamed, colored and removed as an instance is, with what it exposes. In the property editor, a property a stargate exposes has the icon of the Stargate, and its tooltip tells the stargate and the name it is exposed as.
- under the edition zone, the editable properties of the instance selected in it, with their values: those that can be written and whose type has an editor (texts, numbers, booleans, dates). They are edited there and follow the changes the component notifies; the others are only listed in the viewer of the component. The properties of the objects the component added with `addPropertiesForQObject()` are edited there as well, after its own, those of a named object in a group under its name: selecting a `Ticker` shows its `interval`, then the properties of its display under **Display**. They follow the objects the component adds or destroys, as a property is edited: the groups of the labels of a `LabelsComponent` come and go with its `count`, and so do the interfaces on its item, the links of those it removes with them. The name of a property a settings file holds is in italics: its value is the default one. While the application runs, they are only shown.
- under the properties, the problems of the application: a row for each interface an instance cannot do without and that is bound to nothing, with the instance, the interface and its type, and how to fix it: drag from its dot to the dot of an interface of its type another instance provides, named there (`Ticker1.display`), or, when none does, add a component that provides one first. Double-clicking a row, or pressing Enter on it, selects its instance in the edition zone and shows it. The list follows the instances, their names and their links as they change; a sentence tells when every instance has what it needs.
- in its **Run** menu, **Start** (F5) and **Stop** (Shift+F5): the application assembled in the edition zone runs there as it does on its own, see Running an Application below. A triangle in the corner of each instance tells that it runs. The application has two modes, as its instances do: it is edited, then it runs, and it is not edited while it runs. Until it is stopped, its instances can be selected, and their properties read under the edition zone, but nothing is dropped, moved, linked, renamed, recolored, added or removed in the edition zone, nor any property edited: its context menu and the Delete key only tell, in the status bar, that the application has to be stopped first. **New** and **Open...** are disabled meanwhile, as is **Start**; a refusal to start is told in a message box, and the application is edited again. While it runs, its properties change, from the values of the user its settings files give them, or from what its components do, and what changes those the files hold is written to them; **Stop** gives every property the value it had when it started, the one of the edition, and the project is saved with these values, even while it runs: running it does not change it. The status bar tells when a file cannot be read or written.

`WidgetsPlugin`, listed from the start, makes the main window of an application from its components, each of which creates its own widgets. A `MainWindowComponent` provides its window as `mainWindow`, shown while the application runs at its `width` and `height`, under its `title`, and shows in its middle the widget it consumes as `centralWidget`. A `TopBarComponent` cannot do without a `mainWindow`: it is the bar at the top of that window, showing the actions it consumes as the list `actions`, then the widgets of the list `widgets`, each in the order of its binding; it has a `title`, its name in the context menu of the window, and is `movable` or not. An `ActionComponent` provides an `action`, with its `text` and its `shortcut` (`Ctrl+N`), and a `CounterComponent`, which cannot do without an `action`, counts the times it is triggered while the application runs, and provides a `display` of its `count`, written as its `text` tells, `%1` standing for the count. Bind the `action` to the `actions` of the bar and to the counter, its `display` to the `centralWidget`, and **Start**: each click on the action in the bar adds one to the count in the window. A `LabelsComponent` provides as many labels as its `count`, one at first and ten at most, as `label1`, `label2`..., each a widget whose `text` is edited in the group of its name: setting its `count` adds the next ones, or removes the last ones with their links. Bound to the `widgets` of the bar, they show in it.

`ExternalPlugin`, opened with **Open plugin...** from `<build>/external-plugins` (`<build>/external-plugins/<config>` with Visual Studio), or listed from the start with `NebulaStudio --plugin-dir <that directory>`, has a pair to bind: the `display` a `Ticker` provides and the `content` a `Window` consumes, which it shows in a window. Its `ProfileSelector` provides a `display` too, the list of its `profiles` in which the user chooses its `profile`: once a settings file holds that `profile`, the application shows the one chosen the next time it runs.

It is built in `<build>/bin` (`<build>/bin/<config>` with Visual Studio), next to the plugins.

Its theme is the one of Nebula, space: the night sky and the violets, blues and pinks of a nebula. It is dark or light as the system is, and changes with it while NebulaStudio is open; it is dark when the system does not tell. The names in the edition zone, the titles of the groups of properties and the name on the splash screen are written in [Space Grotesk](https://github.com/floriankarsten/space-grotesk), the rest in the font of the system. Each text and its background, and the edges of what can be used, meet the contrasts of WCAG 2.1 level AA: 4.5 to 1 for a text, 3 to 1 for an edge; so does the title of a zone, written in black or white, whichever contrasts more with its color, and the name of an instance given a color. The zones are added in the colors of the theme, in turn. What has to be fixed, an instance that does not work, is shown in amber, which meets the contrasts of a text over the instances, and is told without color too, by a mark. Its colors are defined once for each scheme, in `NebulaStudio/src/Theme.cpp`, and every window and item takes its own from them.

The logo of Nebula, a nebula crossed by three astres bound to one another, is on its splash screen, it is the icon of its windows and of their buttons on the taskbar, and, on Windows, the one of `NebulaStudio.exe` in the file explorer and where it is pinned. It is drawn in `NebulaStudio/resources/nebula.svg`. Each term of the [glossary](docs/glossary.md) has its icon as well, drawn in `NebulaStudio/resources/icons/` in the colors of the theme, which read over its dark scheme as over its light one: the Univers on **New**, the Galaxy on the **Galaxies** tab, beside each galaxy of its list, on the galaxies placed in the edition zone, on **Open galaxy**, on the path to a galaxy shown, on **Description...** and on **Add zone**, the Astre beside each component of their list, the Comet on **Add settings file**, **Edit settings file...** and the dialog of a settings file, and the Stargate on **Add stargate**, **Expose...**, **Edit stargate...**, the stargates of the edition zone, their dialog, and the properties they expose in the property editor.

### NebulaStudio, a galaxy

NebulaStudio is a galaxy as well: `StudioPlugin/NebulaStudio.nbp` assembles its window from the components of `StudioPlugin`, in `StudioPlugin/`, each a part of NebulaStudio made of its own classes, bound to one another as the window of NebulaStudio binds them. The galaxy is copied to `scene/` next to the applications by each build, `<build>/bin/scene` (`<build>/bin/<config>/scene` with Visual Studio): NebulaStudio lists it in its **Galaxies** tab, opens it to show how its parts are bound, and runs it with **Start**, which shows a NebulaStudio of its own; `ExampleUniverseApp scene/NebulaStudio.nbp`, from `<build>/bin`, runs it as an application, until its window is closed. The copy is given the content of the one of `StudioPlugin/` again by each build: that one is the galaxy to edit.

| Component | What it is |
| --- | --- |
| `StudioComponent` | What the parts share, provided as `studio`: the application they assemble, with the plugins of `plugins/` next to the application that runs it and of the directories its command line gives with `--plugin-dir`, the preferences of the user of NebulaStudio, the style of the links say, and what the parts tell one another. The first one gives the application that runs it the theme of NebulaStudio, and stopping it stops the application it assembles |
| `EditionZoneComponent` | The edition zone, as `widget` and as `zone`, with its **View** menu, `viewMenu`, its **Run** menu, `runMenu`, and the label of its zoom level, `zoomLabel` |
| `ProjectComponent` | The project file of the application, as `project`, with its **File** menu, `fileMenu`; it needs the `zone` the application is shown in |
| `PluginListComponent` | The plugins, as `widget`: unloading one removes its instances from the `zone`, when it has one |
| `GalaxyListComponent` | The galaxies of its `directory`, as `widget`: `scene/` next to the application that runs it while it is empty, a relative one read from the directory of that application; one is opened by the `project`, when it has one. A directory chosen in the list while it runs is its `directory` until it is stopped, which a settings file can keep |
| `ComponentListComponent` | The components of the loaded plugins, as `widget` |
| `PropertyEditorComponent` | The editor of the properties of the instance selected in the `zone`, as `widget` |
| `ProblemListComponent` | What keeps the instances from working, as `widget`: activating a row shows its instance in the `zone`, when it has one |
| `StudioWindowComponent` | The main window, shown while it runs, at its `width` and `height`: the `centralWidget` in its middle, the `menus` in its menu bar and the `statusWidgets` on the right of its status bar, in the order of their bindings. It is titled after the `project`, which saves its changes or gives them up before it closes, and its status bar tells what the `studio` tells |
| `SplitterComponent` | Its `widgets` side by side, or one above the other when it is `vertical`, at their `sizes` (`320, 760, 360`), the one it has `stretched`, from 0, taking the room it gains |
| `TabsComponent` | Its `pages` in tabs, each under its window title, beside its window icon when it has one: the lists of NebulaStudio name themselves, **Plugins** and **Galaxies** |
| `GroupComponent` | Its `content` under a `title`, in a frame |

Each part is built once it has the `studio`, and destroyed as it loses it, those it was provided to giving it back first: removing the `StudioComponent` empties the window, and a new one bound to the parts fills it again. Bound otherwise, they make another NebulaStudio, its components on the left and its plugins on the right, say, without a line of code. Run by an application, the NebulaStudio of the galaxy lists `StudioPlugin` as loaded, by that application, and its components among the others, which share their list: dropped in its edition zone, they make one more NebulaStudio.

---

## 📁 Project File

NebulaStudio saves what is in its edition zone to a `.nbp` (Nebula Project) file, and opens it again, from its **File** menu: **New**, **Open...**, **Save** and **Save As...**. Unless another directory is chosen, the projects are saved to, and looked for in, `scene/` next to NebulaStudio, created the first time one is saved: `<build>/bin/scene` (`<build>/bin/<config>/scene` with Visual Studio). A project that has a file is saved and looked for next to it instead. A project file given on its command line is opened as soon as its main window shows: `NebulaStudio my.nbp`. Before the application is replaced, or NebulaStudio closed, it asks whether to save it when it changed since it was saved or opened.

A project file is a galaxy, named after its file: `Clock` for `Clock.nbp`. **Description...**, in the **File** menu, asks for its description, a few words on what it is for, saved with it and shown with it in the **Galaxies** tab; **New** starts without one.

A project file is a JSON object:

```json
{
    "version": 1,
    "description": "Shows the time in a window",
    "plugins": [
        { "id": "FabienDG.Nebula.ExternalPlugin", "file": "../../external-plugins/Debug/ExternalPlugin.nbdl" }
    ],
    "components": [
        {
            "name": "Ticker1",
            "type": "FabienDG.Nebula.ExternalPlugin.Ticker",
            "x": 200,
            "y": 120,
            "color": "#804e79a7",
            "properties": { "interval": 250 },
            "objects": [
                { "name": "Display", "properties": { "digitCount": 8, "windowTitle": "Clock" } }
            ]
        },
        { "name": "Window1", "type": "FabienDG.Nebula.ExternalPlugin.Window", "x": 500, "y": 160, "properties": {} },
        { "name": "Hello1", "type": "Nebula.Galaxy", "galaxy": "galaxies/Hello.nbp", "x": 200, "y": 520,
          "properties": { "name": "Nebula" } }
    ],
    "bindings": [
        { "consumer": "Window1", "consumed": "content", "provider": "Ticker1", "provided": "display",
          "anchors": [ { "x": 420, "y": 320 } ] }
    ],
    "settings": [
        {
            "file": "user.json",
            "directory": "",
            "x": 200,
            "y": 360,
            "entries": [
                { "component": "Ticker1", "property": "interval" },
                { "component": "Ticker1", "object": "Display", "property": "digitCount" }
            ]
        }
    ],
    "zones": [
        { "title": "Display", "color": "#59a14f", "x": 100, "y": 60, "width": 560, "height": 240 }
    ]
}
```

| Key          | Value                                                                                                   |
| ------------ | ------------------------------------------------------------------------------------------------------- |
| `version`    | `1`: a file of another version is not opened                                                            |
| `description` | What the galaxy the file is, is for, written when it has one                                           |
| `plugins`    | The plugins of its components, loaded when it is opened; `file` is where a plugin is looked for when it is not found by its `id`, as those of `plugins/` are, relative to the plugin root |
| `components` | The instances: their name, the id of their component, where they are in the edition zone, their color when they were given one, their properties and those of their objects; for a galaxy, of type `Nebula.Galaxy`, its `galaxy` file and the values of the properties it exposes instead; for a stargate, of type `Nebula.Stargate`, what it exposes |
| `bindings`   | What a component consumes (`consumer`, `consumed`) bound to what another provides (`provider`, `provided`); a list has one for each of its bindings, in its order; the `anchors` of its link, the points it goes through in their order, when it has some |
| `settings`   | Its settings files, written when it has some: the name of the `file`, its `directory`, where it is in the edition zone, and its `entries`, the properties it holds |
| `zones`      | The colored zones of the edition zone                                                                   |

`x`, `y`, `color`, `anchors` and `zones` are only for NebulaStudio: an application runs without them. The `color` of a component is written `#aarrggbb`, its opacity first: `#804e79a7` is half translucent; `#rrggbb` is read too, as an opaque color.

An entry of a settings file is a property of a `component`, or of one of its objects, given by the `object` name it was added with and, when it is not the first object of the component added with that name, its `index` among them, from 0. The `file` of each is its own: a second file of the same name, whatever its case, is reported when the project is opened, and keeps the name it was given then. The `directory` of the file is the configuration directory of the user when it is empty, `QStandardPaths::AppConfigLocation` of the application that runs (under `%LOCALAPPDATA%` on Windows), and is read from the directory of the application when it is relative. The file holds the values of the user under the names of the components, as a project holds them:

```json
{
    "Ticker1": {
        "properties": { "interval": 250 },
        "objects": [ { "name": "Display", "properties": { "digitCount": 8 } } ]
    }
}
```

What else the file holds is kept, so that applications can share one. Run from NebulaStudio, an application writes its files with an empty directory to the one of NebulaStudio, not to the one of the application delivered: what is tried in NebulaStudio does not change what its users have.

A galaxy placed in the project, of type `Nebula.Galaxy`, is held by its file only: the `galaxy` file, relative to the directory of the project, for both to be moved together, or whole when the project has no file yet or the galaxy is on another drive. Its components, their links, their values and its settings files are those of that file, read again each time the project is opened, and each time NebulaStudio saves the galaxy from one of its instances; a galaxy it holds is read the same way, relative to its own file. Saved from an instance, the file of a galaxy is written as a project is, its galaxies relative to it and its settings files under the names it gives them, without the names of the galaxies they are in. The `plugins` of the project are those of its components and of the components of its galaxies. A galaxy that cannot be placed any longer, its file gone or a plugin it needs unknown, is reported as missing, and what its file misses is reported after its name: `In Hello1: ...`.

What a galaxy has once placed is what the stargates of its file expose. A stargate, of type `Nebula.Stargate`, lists the interfaces the galaxy `provided`, each of which the stargate consumes from one of its components, the interfaces the galaxy `consumed`, which the stargate provides to its components, each with its `name` and the `type` of its pointer, and the properties it `exposed`, each under its `name`, given as an entry of a settings file is, by its `component`, its `object` and its `index` when it is of one, and its `property`. Its interfaces are bound in the `bindings` of its file, as those of any component, under their names. Each list is written when it is not empty:

```json
{
    "name": "Stargate1",
    "type": "Nebula.Stargate",
    "provided": [ { "name": "greeter", "type": "IGreeter*" } ],
    "consumed": [ { "name": "screen", "type": "QWidget*" } ],
    "exposed": [ { "name": "name", "component": "Greeter1", "property": "recipient" } ]
}
```

The project that places the galaxy binds its interfaces under those names, and holds, in the `properties` of the galaxy, the values of the properties it exposes, by the names they are exposed as, the ones of the edition while the application runs: each instance has its own, and the others are those of the file. A galaxy that exposes no property is written without them. What a stargate cannot expose any longer, a component gone, a type no component has, a name another stargate exposes the same way, is reported when the file is read, and so is a value a galaxy cannot be given.

A plugin of a project is looked for by its `id` first, among the plugins of `plugins/` and of the `--plugin-dir` directories, and loaded when it is not yet; its `file` is only looked at when none of them has that `id`. A project opens wherever its plugins are, then, given their directory.

The plugin root is the directory of NebulaStudio unless another one is given on its command line: `NebulaStudio --plugin-root <directory> my.nbp`; it is the directory of the application that runs a project unless `--plugin-root` or `Universe::setPluginRoot()` sets it. The files of the plugins are written relative to it, so that a project is opened again wherever NebulaStudio and its plugins are moved to together, or wherever the plugins alone are, given their root. A file on another drive than the root cannot be relative to it and is written whole; a file written whole is read as it is.

The `properties` of a component are those that can be written and whose value JSON holds as it is: numbers, booleans and texts, and whatever is read back from its text, as dates are. JSON holds them by name, but they are given back in the order the component declares them, before those of its objects and before the bindings, and those of a galaxy in the order it exposes them: a property that tells what a component has, its `count` of labels say, gives it what the rest of the project sets and binds.

Its `objects`, written when it has some, are those it added with `addPropertiesForQObject()`, in the order it added them: the `name` each was added with, empty when it has none, and its `properties`, chosen as those of the component are. When the project is opened, each is given to the first object of that name not given one yet; an object the component no longer has is reported as missing.

Opening a project tells what could not be restored, a plugin that is no longer where it was, say, and opens the rest. The file is then no longer the one **Save** writes to, which would write over what is missing.

---

## ▶️ Running an Application

A project is an application: `Nebula::Universe` instantiates its components as the project defines them and runs them. This is what NebulaStudio does when it starts one, and all the `main` of an application has to do:

```cpp
#include <QApplication>

#include "Nebula/Application/Universe.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv); // A QCoreApplication is enough when no component shows a widget

    Nebula::Universe application;
    if(!application.open(QStringLiteral("scene/my.nbp")) || !application.start())
    {
        qCritical().noquote() << application.errorString();
        return 1;
    }
    return QApplication::exec(); // The application is stopped when it is destroyed
}
```

It links to `Nebula::Core` only: the plugins of the project are loaded when it is opened, from `plugins/` next to the application or from the file the project gives, relative to the plugin root.

Its command line can take the options NebulaStudio takes, `--plugin-dir <directory>`, as many times as needed, and `--plugin-root <directory>`: `Universe::addCommandLineOptions()` adds them to a `QCommandLineParser`, and `applyCommandLine()`, before the project is opened, scans the directories and sets the root.

```cpp
QCommandLineParser parser;
parser.addHelpOption();
Nebula::Universe::addCommandLineOptions(parser);
parser.addPositionalArgument(QStringLiteral("project"), QStringLiteral("Project file to run."));
parser.process(app);

Nebula::Universe application;
application.applyCommandLine(parser); // Before open(): the plugins of the project are found in those directories too
```

| Function                | What it does                                                                                              |
| ----------------------- | --------------------------------------------------------------------------------------------------------- |
| `open(filePath)`        | Loads the plugins of the project, instantiates its components, sets their properties, binds their interfaces and reads its settings files. `false`, with `errorString()` set, when the file cannot be read or anything could not be restored |
| `start()`               | Keeps the values of the properties of its components and of their objects, those of the edition, then gives the properties its settings files hold the values of the user, and starts its components, in the order of the project. `false`, with `errorString()` set, when one refuses: those it started are stopped again, and it is edited again, as `stop()` leaves it. Does nothing while it runs |
| `stop()`                | Stops its components, the last one first, has its settings files write the values of the user, then gives every property the value of the edition again. It can be started again |
| `isRunning()`           | Whether it was started and not stopped since; `runningChanged()` is emitted when it changes. While it runs, it is not assembled: `instantiate()`, `rename()` and `remove()` refuse, and `save()` writes the values of the edition |
| `components()`, `component(name)` | Its instances, to reach one from the `main`: `application.component(QStringLiteral("Greeter1"))` |
| `setPluginRoot(directory)` | The directory the files of the plugins are relative to in a project; the directory of the application by default |
| `addCommandLineOptions(parser)`, `applyCommandLine(parser)` | Add `--plugin-dir` and `--plugin-root` to a `QCommandLineParser`, then do what they ask once it processed the command line: a `--plugin-dir` that is not a directory is skipped, with a warning |

An application can also be assembled without a project, while it does not run: `instantiate(componentId)` creates a component in it, `rename(component, name)` names it, unless the name is empty or the one of another instance, `Component::bindInterface()` binds it, `remove(component)` destroys it, which unbinds what was bound to its interfaces, `componentsChanged()` being emitted once a component is instantiated or removed, `unloadPlugin(pluginId)` removes the components of a plugin, those `componentsOf(pluginId)` lists, then unloads it, unless the application runs, and `save()` returns the project of what was assembled. `read()` and `load()` are the two halves of `open()`, for who needs the project itself, as NebulaStudio does for what it adds to it. `description()` and `setDescription()` are the description the project holds, replaced by `load()`, emptied by `clear()`.

A project file is a galaxy: `Universe::readGalaxy(filePath, error)` reads what it is without loading it, a `Nebula::GalaxyInfo` with its `name`, the one of its file without its extension, its `description` and the ids of its `plugins`, or `std::nullopt`, with `error` set, when `read()` cannot read it. `missingPlugins(galaxy)` lists those of its plugins the application knows none of by their id, among the plugins it found, loaded or not: when there are none, the galaxy can be loaded. NebulaStudio lists the galaxies of a directory that way.

`instantiateGalaxy(filePath, name)` places a galaxy in the application: a `Nebula::Galaxy`, a component that Core builds itself, of id `Nebula.Galaxy`, named `name` or after its galaxy and a number, which owns the components of its file, bound and set as the file has them, its settings files, and the galaxies it holds, at paths relative to its file. Its plugins are found by their id among those the application found, not by their file, and loaded when they are not yet. It returns `nullptr`, with `errorString()` set, while the application runs, when the file cannot be read, when one of its plugins is unknown or cannot be loaded, and when it would hold itself, or the file of the application, directly or not; what its file misses otherwise is in `Galaxy::problems()`. A `Galaxy` tells its `filePath()`, its `galaxyName()`, its `description()`, its `components()`, `component(name)` and `settingsFiles()`, and the `project()` its file holds. Started, it starts its components in their order, refusing as soon as one refuses, and stops them the other way round. Its settings files are started and stopped with those of the application, `settingsFile()` finds them, and they are named after it, its name and a dot before the name its file gives them, the names of the galaxies it is in first: each instance writes its own, which `rename()` renames. `components()` lists the galaxies, not what they hold; `componentsOf(pluginId)`, and so `unloadPlugin()`, counts the galaxies that hold a component of the plugin. `filePath()` and `setFilePath()` are the file of the project, which `open()` sets: its galaxies are written relative to its directory by `save()`, and read from it by `load()`.

What a galaxy has is what its stargates expose. `addStargate(name)` adds a `Nebula::Stargate` to the application, a component Core builds itself, of id `Nebula.Stargate`, named `name` or `Stargate1`, `Stargate2`..., unless the application runs. `exposeInterface(exposure, name, type)` exposes an interface of a type, `QMetaType::fromType<QWidget*>()` say: with `Nebula::Exposure::Provided`, the galaxy provides it and the stargate consumes it, mandatory, from one of its components; with `Nebula::Exposure::Consumed`, the galaxy consumes it and the stargate provides it to its components. They are then bound with `bindInterface()`, as those of any component. `exposeProperty({name, component, object, property})` exposes a property a project saves of one of its components, the component itself or one of its objects, or one a galaxy among them exposes, `object` being that galaxy and `property` the name it exposes it as. `renameInterface()`, `removeInterface()`, `renameProperty()` and `removeProperty()` change it, `interfaces()` and `properties()` list what it exposes, and `changed()` tells when it changes. A name is refused when another stargate of the galaxy exposes it the same way, and nothing is changed while the application runs: `isChangeable()` tells it. Changed in a galaxy placed, a stargate changes what the galaxy has at once: an interface exposed is provided or consumed by the galaxy, what is bound to one renamed stays bound, and what was bound to one removed no longer is; one the galaxy consumes is mandatory as the galaxy is read, when one of its components needs it, not when it was exposed since. In the application itself, which nothing places, a stargate only passes what its components give it. Placed, a `Galaxy` provides and consumes the interfaces its stargates expose, the pointer passing as it is, and so does its availability: when the one that provides it withdraws it, those that consume it on the other side are told before it goes; one it consumes is mandatory when one of its components needs it. `Galaxy::exposedProperties()` and `exposedProperty(name)` give the properties it exposes, each with the component and the object it is the property of, which are set on them: `save()` writes their values with the galaxy, and `load()` gives them back.

A galaxy placed is assembled through any of its instances, as the application is, while it does not run: `instantiate(componentId, name, galaxy)`, `instantiateGalaxy(filePath, name, galaxy)` and `addStargate(name, galaxy)` add to `galaxy`, one of the galaxies of the application or of those they hold, a component named among its own, `addSettingsFile(galaxy)` a settings file, and `rename()`, `remove()` and `removeSettingsFile()` act wherever the component or the file is; `galaxyOf(component)` tells the galaxy a component is in, `nullptr` in the application, and `components(galaxy)` the components of a galaxy, or of the application for `nullptr`. A galaxy that would hold the file of the galaxy it is placed in, directly or not, is refused. `saveGalaxy(galaxy)` returns the project its file is to hold, written as `save()` writes the application: its description, its components, their values and bindings, the files of its galaxies relative to its own, and its settings files under the names its file gives them; the values of what it exposes are those of the instance. Once that file is written, `galaxies(filePath)` lists the instances of the file, wherever they are, and `reloadGalaxy(galaxy)` reads one again from it, in place: its components and its settings files are destroyed, after `Galaxy::aboutToBeReloaded()`, then those of its file are placed, before `Galaxy::reloaded()`; it keeps its name, the values of the properties it still exposes, and what is bound to its interfaces, and what they are bound to, under the names they still have. It returns what could not be restored, a sentence each, a link to an interface no longer exposed, say, and changes nothing while the application runs, nor when the file cannot be read or one of its plugins is unknown.

Its settings files, `settingsFiles()`, keep the values the user gives some properties, from one run to the next; `Nebula::SettingsFile` is one of them. When the application starts, before any of its components is started, each file gives the properties it holds the values it has for them; a file that does not exist yet is first written with the values of the project, their default values, and a file that cannot be read is set aside, a `.bak` added to its name, then written again. While the application runs, what changes these properties is written to their files, changes made together being written once; when it stops, each file is written a last time, then the properties have their default values again, which `save()` writes, even while it runs. `settingsFileFailed()` tells when a file cannot be read or written: the application runs all the same. `addSettingsFile()` adds a file, named `settings.json`, or `settings2.json`, `settings3.json`... when another file has that name, `settingsFile(entry)` tells which one holds a property, `settingsFile(fileName)` which one has a name, whatever its case, and `removeSettingsFile()` removes one; a `SettingsFile` has its `setFileName()`, which refuses the name of another file of the application, whatever its case, its `name()`, the one its project gives it, which is its file name in the application and, in a galaxy, the name before which each instance puts its own, `Clock1.` say, changed with `setName()`, its `galaxy()`, `setDirectory()`, `add({component, object, property})`, the object being the component itself or one of its objects, `remove()`, and `writeDefaults()`, which writes the values of the project to its file while the application does not run.

The [exemple](exemple/) folder holds `ExampleUniverseApp`, which runs the project given on its command line until its last window is closed: assemble a `Ticker` and a `Window` in NebulaStudio, save them as `scene/clock.nbp`, then run `ExampleUniverseApp scene/clock.nbp` from `<build>/bin` (`<build>/bin/<config>` with Visual Studio). It takes `--plugin-dir` and `--plugin-root` too: `ExampleUniverseApp --plugin-dir ../external-plugins scene/clock.nbp` finds `ExternalPlugin` there even when the project no longer leads to its file (`../../external-plugins/<config>` with Visual Studio). With a `ProfileSelector` in place of the `Ticker`, and a settings file that holds its `profile`, the profile chosen in the window is the one it shows the next time. The main window of `WidgetsPlugin` runs the same way, its plugin being next to the application: closing it ends the application.

---

## 🤝 Contributing

The coding conventions the code follows, from formatting and naming to commits and pull requests, are described in [CONVENTIONS.md](CONVENTIONS.md) (in French). What changes from one version to the next is listed in [CHANGELOG.md](CHANGELOG.md), and told in more detail in the notes of each release, in [docs/release-notes](docs/release-notes/).

Each class of Core, and those of NebulaStudio that need no window, has its Qt Test executable in [tests](tests/), data-driven wherever it can be. They are built with the rest (`-DNEBULA_BUILD_TESTS=OFF` leaves them out) and run by CTest from the build directory, with no window shown:

```bash
ctest -C Debug --output-on-failure
```

The CI runs them on each pull request. How to run, debug and write them is described in [docs/testing.md](docs/testing.md) (in French).

---

## 📄 License

Nebula is licensed under the [Mozilla Public License 2.0](LICENSE). Copyright (C) 2026 Fabien Di Grande.

- Anyone may use, modify and distribute Nebula, in a product of any kind, commercial or not.
- A plugin or an application built on Nebula keeps the license its author chooses, open or closed.
- A file of Nebula stays under the MPL, modified or not: whoever distributes Nebula, as source or as binaries, says where the source of these files is found, with the changes made to them.
- The headers `nebula_add_plugin()` generates for a plugin come from files of Nebula and stay under the MPL too; the sources of the plugin do not.

This summary does not replace the [license](LICENSE). Each source file carries a two-line notice (copyright and `SPDX-License-Identifier: MPL-2.0`); for the files that cannot, such as `plugin.json.in`, the notice is this one:

> This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

The license covers the files of this repository: Qt and the `CMakeUtils` submodule are separate works, under their own terms. So is Space Grotesk, the font of the titles of NebulaStudio, which `NebulaStudio.exe` holds: it is under the [SIL Open Font License 1.1](NebulaStudio/resources/fonts/OFL.txt), copied next to NebulaStudio in the package.
