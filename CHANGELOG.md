# Changelog

What changes in Nebula, formerly AppForge, from one version to the next, the latest first. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/). Each version is told in more detail, in English and in
French, in its notes in [docs/release-notes](docs/release-notes/), the text of its GitHub release, with how to upgrade
to it.

## [Unreleased]

### Added

- Lists of interfaces (#67): `Component::consumeInterfaceList<T>()` consumes a `QList<T*>` to which several provided
  interfaces are bound, in their order, each one telling `onInserted` and `onRemoved`; `bindings(name)`,
  `unbindInterface(name, provider, providedName)` and `InterfaceInfo::list`. NebulaStudio draws a list as two stacked
  circles, with a link for each of its bindings, removed one by one.
- Interfaces and objects that depend on a property (#86): `Component::removeProvidedInterface()` and
  `removeConsumedInterface()` are protected, for a component to remove what it no longer has as a property changes,
  the widgets it provides, as many as its count, say; what is bound to an interface removed is unbound. `WidgetsPlugin`
  has a `LabelsComponent`, which provides as many labels as its `count`, each with its `text`.
- The style of the links in NebulaStudio (#84): **View** > **Links** draws them as **Curves**, as before, **Straight
  Lines** or **Right Angles**, in the application and in the galaxies opened. It is not saved with the project, but
  kept for the next launch of NebulaStudio.
- The links an interface can have, suggested in NebulaStudio (#82): **Bind to**, in the context menu of the dot of a
  consumed interface, lists the interfaces of its type the other instances of the scene provide, those it is bound to
  checked; choosing one binds them, or unbinds them when it is checked.
- Anchors on the links in NebulaStudio (#106): double-clicking a link, or **Add anchor point** in its menu, has it go
  through that point, which is dragged to move it and double-clicked to remove it. A project holds the anchors of a
  link in the `anchors` of its binding, which only NebulaStudio reads.
- Finding the instances of the edition zone in NebulaStudio (#63): the field above it, also reached with Ctrl+Shift+F,
  lists as it is typed the instances whose name, or the name of whose component, holds what is typed; choosing one, or
  pressing Enter, selects it and shows it in the middle of the edition zone, Enter again showing the next one.

### Changed

- `TopBarComponent` of `WidgetsPlugin` consumes the lists `actions` and `widgets` in place of `action1` to `action4`,
  `widget1` and `widget2`.
- `Component.h` changes: plugins built against 1.0.0 must be built again.
- `nebula_add_plugin()` generates the class of the plugin (#85), `<NAME>` in `<NAME>.h`, which its components include
  to register in it: a plugin no longer writes it, and `NEBULA_PLUGIN` no longer compiles, telling to remove the
  class written for 1.0.0.
- A plugin is the factory of its components (#94): `IPlugin` lists them, `components()`, and creates them,
  `create(name)`, the name being `Nebula::componentName()` of their class. The generated class of the plugin is its
  root object, deriving from `Nebula::Plugin<itself>`, and `ComponentFactory` has the plugin of a component create
  it (`add(pluginId, IPlugin&)`). Plugins built against 1.0.0 must be built again.
- A project gives a component its properties in the order the component declares them, rather than in the order of
  their names, and a galaxy the properties it exposes in the order it exposes them (#86): a property that tells what
  a component has is set before those that depend on it.

### Removed

- `IPlugin::plugin()` and `Plugin::instantiate()`: the root object is the plugin.
- `Component::builtByPlugin()`: a class deriving from `Component` is no longer abstract. A component is still created
  by its plugin only, which ends what it provides and consumes before its members are destroyed.

### Fixed

- A settings file no longer reads an entry whose object a value it gave just destroyed, as a count of widgets does
  (#86).
- NebulaStudio: the property editor is rebuilt once the value edited in it is written, rather than while it is, when
  that value adds or destroys objects of the component (#86).

## [1.0.0] - 2026-10-20

### Added

- Galaxies: a project file placed in an application as a single component, as many times as needed, each instance with
  its own components (`Universe::instantiateGalaxy()`). Its plugins are found by their id and loaded; a galaxy that
  would contain itself is refused.
- Stargates (`Nebula::Stargate`, `Universe::addStargate()`): what a placed galaxy provides, consumes and exposes of the
  interfaces and properties of its components.
- A placed galaxy is edited as the application is: `Universe::saveGalaxy()` and `reloadGalaxy()`, and in NebulaStudio
  **Open galaxy**, **Save Galaxy** (Ctrl+Alt+S), **Add stargate**, **Expose...** and **Edit stargate...**.
- The **Galaxies** tab of NebulaStudio: the galaxies of a directory, and whether their plugins are known
  (`Universe::readGalaxy()`, `missingPlugins()`).
- The description of a project: `Universe::description()`, and **File** > **Description...** in NebulaStudio.
- Mandatory consumed interfaces (`InterfaceRequirement::Mandatory`) and `Component::unboundMandatoryInterfaces()`;
  NebulaStudio marks the components that cannot work and lists what to bind under its property editor.
- `WidgetsPlugin`, an example whose components create their objects and assemble a main window.
- NebulaStudio: a theme, dark or light as the system is, at the AA contrast level of the WCAG; a logo and an icon for
  each term of the [glossary](docs/glossary.md); zoom of the edition zone, and moving it with the middle button; a
  single link selected and removed.
- Qt Test executables for the classes of Core and of NebulaStudio, run by CTest and by the CI.

### Changed

- AppForge is renamed Nebula: Forge becomes NebulaStudio, `ForgeApplication` becomes `Nebula::Universe`,
  `ExampleForgeApp` becomes `ExampleUniverseApp`; the package, its target and function become
  `find_package(Nebula)`, `Nebula::Core` and `nebula_add_plugin()`, the macros `NEBULA_*`, plugins `.nbdl` files and
  projects `.nbp` files.
- A component is either `Editing` or `Running`. Running an application no longer changes its project, and NebulaStudio
  locks its edition zone while it runs.
- A component creates its own objects in its constructor; the class of a plugin only gathers its components.
- Plugins built against 0.3.0 are refused: build them again.

### Removed

- `initialize()`, `onInitialize()` and the `Ready` state of a component.
- `build(<component>&)` in the class of a plugin: a plugin class that still declares one does not compile.
- `start(instance)` and **Start component**: nothing is added while an application runs.

## [0.3.0] - 2026-10-16

### Added

- Settings files, in a new `Settings` module: the values the user of an application gives some properties, kept from
  one run to the next. **Add settings file** in Forge.
- `--plugin-dir` and `--plugin-root` on the command line, through `ForgeApplication::addCommandLineOptions()` and
  `applyCommandLine()`.
- Forge: a splash screen, a logo, a search bar above the list of components, and **Start component**.

### Changed

- A plugin of a project is looked for by its id among the plugins found, its file being only a last resort.

### Fixed

- The context menu of a component no longer offers to add a zone under it.

## [0.2.0] - 2026-10-13

First published version.

### Added

- Core: plugins built with `appforge_add_plugin()`, `PluginManager`, components and `ComponentFactory`, interfaces
  bound by name and type, the properties of inner objects, and `ForgeApplication`, which opens and runs a project.
- Forge, the editor that assembles components, saves them as projects and runs them.
- The package for other projects: `find_package(AppForge 0.2 REQUIRED COMPONENTS Core)`.
- Examples: `ExamplePlugin`, `ExternalPlugin`, `ExampleApp` and `ExampleForgeApp`.

[1.0.0]: https://github.com/Shuko83/Nebula/releases/tag/1.0.0
[0.3.0]: https://github.com/Shuko83/Nebula/releases/tag/0.3.0
[0.2.0]: https://github.com/Shuko83/Nebula/releases/tag/0.2.0
