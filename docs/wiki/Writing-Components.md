# Writing components

This page is the guide to what a component can do, once [your first plugin](Your-First-Plugin.md) builds. Each section
links to the part of the [README](https://github.com/Shuko83/Nebula/blob/main/README.md#-writing-a-component) that
details it.

## The shape of a component

A component is a class that:

- derives **publicly** from `Nebula::Component`, has `Q_OBJECT`, a default constructor, and is not `final`: its plugin
  creates it, through a class that derives from it;
- is named `<Name>Component`, `MainWindowComponent` say;
- creates the objects it is made of in its constructor, as **members**: a window, a timer, a class of its own holding its
  logic;
- declares there, in its constructor, the interfaces it provides and consumes, so that they are known before it is
  bound;
- registers itself in the class of its plugin, in its `.cpp`, with `NEBULA_REGISTER_COMPONENT(<Plugin>, <Component>);`;
- tells what it is with `Q_CLASSINFO("description", ...)` and `Q_CLASSINFO("category", ...)`, which NebulaStudio shows.

`NEBULA_REGISTER_COMPONENT` checks most of these rules as it compiles, and its message tells which one is broken.

A component is created by its plugin, through `Nebula::ComponentFactory`, never with `new`: only then is what it
provides and consumes let go of before its members are destroyed.

The components of [`WidgetsPlugin`](https://github.com/Shuko83/Nebula/tree/main/exemple/WidgetsPlugin/src) follow these
rules, and are short: read them as examples.

## Properties

Every `Q_PROPERTY` of a component that can be written is edited in NebulaStudio and saved with the project, when its
type allows it:

| Type | Edited in NebulaStudio as | Saved in a project as |
| --- | --- | --- |
| `bool`, `int`, `uint`, `double`, `QString` | a check box, a number, a text | a JSON boolean, number or string |
| `QDate`, `QTime`, `QDateTime` | a date, a time | its text; not saved while it is null |
| An enumeration declared with `Q_ENUM` | a list of its values | the name of its value: `"Fast"` |
| Flags declared with `Q_FLAG` | a list of check boxes | the names of those it has, joined by `\|`: `"Bold\|Italic"` |
| `QColor` | a swatch, chosen in a color dialog | `#rrggbb`, or `#aarrggbb` when it is not opaque; not saved while it is not valid |
| `QUrl` | its text, or a file chosen with a button | its text |

A property of another type is listed in the viewer of its component, not edited, and not saved unless JSON holds its
value as it is.

- **Notify its changes.** A setter that changes the value emits the `NOTIFY` signal: NebulaStudio shows the new value,
  and a [settings file](Settings-Files.md) that holds the property writes it.
- **Declare flags with `Q_FLAG` alone**, with `READ` and `WRITE` rather than `MEMBER`. Declared with `Q_ENUM` too, their
  values are named as the enumeration's, and a value made of several is not saved.
- **Leave out what is not a setting** with `DESIGNABLE false` or `STORED false`: neither edited nor saved.
- **Declare first what others depend on.** A project gives a component its properties in the order it declares them:
  a `count` that makes objects comes before the properties of those objects.

## The two modes

A component is `Editing` when it is created, while its properties are set and its interfaces bound, and `Running` once
started, until it is stopped. `state()` tells which, and `stateChanged()` when it changes.

- **Do the work while it runs.** Override `onStart()` to start it, a timer, a connection, a window shown, and
  `onStop()` to end it. Return `false` from `onStart()` to refuse to start: the application stops what it started and
  is edited again.
- **Expect its values back.** When the application stops, every property gets back the value it had when it started:
  what changed while it ran, a count say, is undone, unless a settings file keeps it.
- **Do not count on being bound while it runs.** Bindings are made while instances are edited: `bindInterface()` and
  `unbindInterface()` refuse while either side runs. The interfaces it has can still become unavailable then.

## Interfaces

### Providing one

```cpp
provideInterface<IGreeter>(QStringLiteral("greeter"));          // In the constructor: declared, unavailable
provideInterface<IGreeter>(QStringLiteral("greeter"), &m_logic); // Available
provideInterface<IGreeter>(QStringLiteral("greeter"), nullptr);  // Withdrawn
```

- The type in `<>` is the one the pointer is given as, and the one a consumer asks for: a `QLabel` provided as a
  `QWidget` is bound to whatever consumes a `QWidget`. A name keeps the type it was declared with.
- A class of your own is a type as well, an interface of your plugin, `IGreeter` say: its consumers include its header.
  Plugins share interfaces through such headers, as `Nebula::Standard` does for the
  [standard plugins](Using-the-Standard-Plugins.md).
- Providing another pointer tells the consumers that the former one goes, then gives them the new one.

### Consuming one

```cpp
consumeInterface<QWidget>(
    QStringLiteral("content"), m_content, Nebula::InterfaceRequirement::Mandatory,
    [this] { showContent(); },      // m_content is set
    [this] { giveBackContent(); }); // m_content is about to be set back to nullptr
```

- Nebula writes the member: never set it yourself.
- Both callbacks are optional. `onUnavailable` is called while the member still points to what it has to let go of:
  when the provider withdraws or replaces it, when the binding is undone, and when either instance is destroyed.
- `Mandatory` only tells who assembles: NebulaStudio marks the instance in amber until the interface is bound. Nothing
  else checks it, and the component starts without it: test the member before using it.

### Consuming a list

```cpp
consumeInterfaceList<IObserver>(
    QStringLiteral("observers"), m_observers,              // QList<IObserver*> m_observers
    [this](IObserver* observer) { observer->update(m_state); }, // In m_observers
    [](IObserver* observer) { observer->update({}); });         // About to leave m_observers
```

A list is bound to several provided interfaces, and holds those that are available, in the order of their bindings:
the actions of a tool bar, the observers of a subject. `TopBarComponent` of `WidgetsPlugin` consumes two.

### Giving back what you were given

An object a component takes from another, a widget it places in its window say, is the member of that other component.
Give it back without a parent in `onUnavailable`: `setParent(nullptr)`, `takeCentralWidget()`, `removeToolBar()`.
Otherwise the parent that holds it deletes it with itself, and the component that provides it deletes it again.

A `QToolBar` owns the widgets added to it: `TopBarComponent` puts each widget it consumes in a holder of its own,
which it owns.

## The properties of its objects

A component can have the properties of an object it is made of edited and saved with its own:

```cpp
addPropertiesForQObject(&m_display, QStringLiteral("Display"));
```

NebulaStudio shows them under the name given, in a group of their own. All that the object can write is then edited,
saved, and given back when the application stops: for a widget, its geometry, its colors, its `text` that the
component itself changes as it runs. Prefer a small `QObject` of your own that has only what is worth setting, as
`LabelsComponent` does with its `Label`, whose only property is the `text` of its label.

## Interfaces that follow a property

What a component provides can depend on one of its properties: as many labels as its `count`. As the property is set,
the component provides the interfaces it gains, and adds the properties of their objects; it removes those it loses
with `removeProvidedInterface(name)`, while their pointers are still valid, then destroys their objects. See
[`LabelsComponent`](https://github.com/Shuko83/Nebula/blob/main/exemple/WidgetsPlugin/src/LabelsComponent.cpp) and
the [README](https://github.com/Shuko83/Nebula/blob/main/README.md#-writing-a-component).

Keep such a property out of [settings files](Settings-Files.md): changed while the application runs, what it removes
loses its links, which stopping the application does not bring back.

## Writing files

A component that writes files of its own writes them with `Nebula::WholeFile::write()`
(`Nebula/Application/WholeFile.h`), as Nebula writes its settings files: whole or not at all, its directory created,
and a replacement refused on Windows, by an antivirus or a program that looks at the file, attempted again.

```cpp
QString error;
if(!Nebula::WholeFile::write(filePath, QJsonDocument(json).toJson(), error))
{
    qCWarning(lcClock) << filePath << "cannot be written:" << error;
}
```

## Trying a component without NebulaStudio

A test, or a small program, loads the plugin and creates the component as an application does:

```cpp
#include "Nebula/Component/ComponentFactory.h"
#include "Nebula/Plugin/PluginManager.h"

Nebula::PluginManager plugins; // Outlives the components of its plugins
plugins.addFile(QStringLiteral("plugins/TimePlugin.nbdl"));
plugins.load(QStringLiteral("AcmeCorp.Acme.TimePlugin"));

std::unique_ptr<Nebula::Component> clock =
    Nebula::ComponentFactory::instance().create(QStringLiteral("AcmeCorp.Acme.TimePlugin.ClockComponent"));
clock->setProperty("format", QStringLiteral("hh:mm"));
clock->start();
// ...
clock->stop();
```

`providedInterface<T>(name)` gives what it provides, and `bindInterface()` binds two of them. A component that shows
widgets needs a `QApplication`.

## Pitfalls

- **Change Nebula, rebuild the plugin.** A plugin built against other headers of Nebula crashes as soon as one of its
  components is created: build every plugin again after updating Nebula, those opened with **Open plugin...** too.
- **Same configuration as the application.** Qt does not load a `Debug` plugin in a `Release` application.
- **Read the message of a registration that does not compile.** A component that derives privately from
  `Nebula::Component`, is `final`, lacks `Q_OBJECT` or a default constructor, or leaves a pure virtual function, is
  refused by `NEBULA_REGISTER_COMPONENT`, which says why.
- **Null dates and invalid colors are not saved.** Give such properties a valid value by default.
- **Widgets need a `QApplication`.** A console application that loads a plugin of widgets cannot create its
  components.

See [troubleshooting](Troubleshooting.md) for what these look like when they happen.
