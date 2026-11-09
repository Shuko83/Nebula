# Changelog

What changes in Nebula, formerly AppForge, from one version to the next, the latest first. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/). Each version is told in more detail, in English and in
French, in its notes in [docs/release-notes](docs/release-notes/), the text of its GitHub release, with how to upgrade
to it.

## [Unreleased]

### Added

- The [wiki](https://github.com/Shuko83/Nebula/wiki) (#160), which walks through Nebula step by step: getting started,
  a first plugin built outside Nebula, writing components, the standard plugins, NebulaStudio, galaxies and settings
  files, running and deploying an application, and troubleshooting, and a course of eight lessons with exercises and
  their solutions, from assembling without code to a plugin of one's own. Its pages are written in `docs/wiki/` and
  copied to the wiki of the repository by a workflow each time they change on `main`.

## [3.0.0] - 2026-11-09

### Added

- `StandardPlugin` (#130), the first standard plugin, shipped with Nebula for any application, built with NebulaStudio
  and the example applications and installed next to each application of the package. Its components provide the
  interfaces of `Nebula::Standard`, a library of headers only that a plugin links to consume them:
  `find_package(Nebula 3.0 REQUIRED COMPONENTS Core Standard)`.
- `LogComponent` (#130), the log of the application. It provides `log`, a `Nebula::ILog`, which writes the messages of
  components to Qt's journal under its `category`, after their source, and `logControl`, a `Nebula::ILogControl`,
  which starts and stops the log and sets its level (`Nebula::LogLevel`). While it runs, its `logging` and its `level`
  hold for every message of the process, those of every plugin, of Core and of Qt included, on top of the rules of the
  application: what drives it drives the log for everyone. Its properties follow, so that a settings file keeps the
  level the user chose.
- The values of a settings file in NebulaStudio (#133): selected alone in the edition zone, a settings file shows,
  beside it, in place of the properties of an instance, the values its file holds for the properties it holds, under
  the names of their instances; a property the file holds no value for shows its default value, in italics. A value
  edited there is written to the file at once, and what another program writes to the file, or the application while
  it runs, is shown at once; while it runs, the values are only shown. **View and edit file...**, in the context menu
  of a settings file, shows its text in a dialog, which writes what is typed to the file once it is a JSON object, and
  follows the file as well.
- `SettingsFile::readValues()`, what its file holds for each of its entries, and `SettingsFile::writeValue()`, which
  writes a value to its file for one of them (#133).
- Enumerations, flags, colors and URLs in the property editor of NebulaStudio (#135): an enumeration (`Q_ENUM`) is
  chosen in the list of its values, flags (`Q_FLAG`) checked in the list of theirs, a color chosen in a color dialog,
  opacity included, and shown with a swatch of it, and a URL typed, or a local file chosen in a dialog. A project and
  a settings file hold an enumeration by the name of its value, flags by the names of theirs joined by `|`, an empty
  text for none, a color by its name, `#aarrggbb` when it is not opaque, and a URL by its text: the dialogs of a
  settings file and of a stargate offer them too.
- `HistoryComponent` (#132), in `StandardPlugin`: the history of what an application does that can be undone. It
  provides `history`, a `Nebula::IHistory`, which executes the `Nebula::Command` pushed to it, then undoes it and does
  it again one step at a time; a `Nebula::CompositeCommand` makes one step of several. It keeps `maxSize` commands, and
  `undoActive` and `redoActive` turn undoing and redoing off.
- Undo and redo in NebulaStudio (#132): its **Edit** menu undoes the last edit of the application, or of a galaxy
  opened, with Ctrl+Z, and does it again with Ctrl+Shift+Z. Each edit is one step, a drag from the press of the button
  to its release included, and undoing it gives back exactly what was there, the links of an instance removed included.
  NebulaStudio keeps them in the `HistoryComponent` of its galaxy.
- `Universe::restoreGalaxy()`, which gives a galaxy placed what was saved of it, without reading its file (#132).
- `NEBULA_VERSION_SUFFIX` (#139), a suffix of the version, a dash then letters or digits: built with
  `-DNEBULA_VERSION_SUFFIX=-rc0`, Nebula 3.0.0 is a `3.0.0-rc0`, in the details of each binary, the version
  NebulaStudio tells and the name of the package. CMakeUtils gives it to every target, as `cmu_version_suffix`.
- Cut, copy, paste and duplicate in NebulaStudio (#136): its **Edit** menu cuts (Ctrl+X), copies (Ctrl+C), pastes
  (Ctrl+V) and duplicates (Ctrl+D) the instances selected in the edition zone, components and galaxies placed, with
  their properties, the values a galaxy exposes, their colors, the edges and the rows of their dots, and the links
  between them, with their anchors and their junctions. Pasted in the application, in a galaxy opened or in another
  window of NebulaStudio, each keeps its name, unless another instance has it (`Counter2` becomes `Counter3`), a little
  further than where it was copied, and the instances pasted are selected alone. The context menu of an instance cuts,
  copies and duplicates it, and the one of the edition zone pastes there. Each is one step of the history.
- `Universe::saveInstances()`, which writes some instances and the bindings between them as a piece of a project, and
  `Universe::insertInstances()`, which places such a piece in the application or in one of its galaxies, under free
  names, or nothing when one of them cannot be placed (#136).
- `Nebula::WholeFile::write()` (#152), which writes a file whole or not at all, its directory created when needed,
  attempting again a replacement Windows refuses, as Nebula writes its settings files and NebulaStudio its projects.
- An installer of NebulaStudio for Windows (#140), published with each release next to its zip and named as it is,
  `NebulaStudio_3.0.0_win-x64_msvc2022_Qt6.12_setup.exe`. It installs NebulaStudio with what it needs to start, the
  DLLs of Nebula, of Qt and of the compiler, the plugins of its galaxy and its galaxy, for the user who runs it, without
  asking to be an administrator, or for all the users; it adds NebulaStudio to the Start menu and to **Settings** >
  **Apps**, where it is uninstalled, and installing another version replaces the one installed. `cpack -C Release`
  builds it from a build, with Inno Setup 6.
- Finding the instances of the galaxies in NebulaStudio (#144): the field above the edition zone finds the instances
  of the galaxies placed in the application too, at any depth, each named after the galaxies it is in
  (`Clock1.Greeter1`), after those of the application; choosing one, or pressing Enter, opens the galaxy it is in and
  shows it in the middle of the edition zone.

### Changed

- The major version of Nebula is 3 (#159), the public API of Core having changed with #137, see below: Core 3.0.0
  refuses the plugins built against 2.x, to be built again, and `find_package(Nebula 3.0 …)` finds the package, which
  `find_package(Nebula 2.1 …)` no longer does.
- The package is built with Qt 6.12, formerly Qt 6.8 (#141), as the continuous integration builds and tests Nebula:
  a plugin built against it is built with Qt 6.12.
- The zip of a release, and the folder in it, are named after what its binaries are built for and with (#139),
  `Nebula_<version>_<platform>_<compiler>_Qt<major>.<minor>`: `Nebula_3.0.0_win-x64_msvc2022_Qt6.12.zip`, formerly
  `Nebula-2.1.0-win64-msvc2022-qt6.8.zip`.
- On Windows, the details of each binary tell what it is built for and with (#139): its description is its name
  followed by its platform, its compiler and its Qt, `NebulaStudio (win-x64, MSVC 19.44.35211.0, Qt 6.12.0)`, which
  Windows shows as the name of an executable in the Task Manager as well; its numeric version has the four numbers of
  the version. The binaries keep their names.
- While an application runs, Core refuses to change its bindings and its settings files (#137), as it refused to
  instantiate, rename or remove its components: `Component::bindInterface()` returns `false` while the consumer, the
  provider or what the interface is bound to runs, and the two `Component::unbindInterface()` return `false`, changing
  nothing, formerly `void`; `SettingsFile::setFileName()`, `setName()` and `add()` return `false`, and
  `SettingsFile::remove()` and `setDirectory()` return `false`, changing nothing, formerly `void`, with
  `SettingsFile::errorString()` telling why; `Universe::addSettingsFile()` returns `nullptr`, formerly a reference to
  the file, as `addSettingsFile(Galaxy&)` does now, and `Universe::removeSettingsFile()` returns `false`, formerly
  `void`. `save()` thus writes the bindings of the edition while the application runs, as it writes the values of its
  properties. What Core does itself still unbinds meanwhile: an interface that a property removes, as a settings file
  changes it, and a component destroyed. Code that keeps the reference `addSettingsFile()` returned is to keep the
  pointer instead; `Component.h` changes, hence the major version.

### Fixed

- On Windows, a settings file, a project, a galaxy or the text of a settings file written while something looks at it
  is no longer refused now and then, "Access is denied" (#152): NebulaStudio follows the files it shows, which made
  the second of two writings close together fail at times, when a value was edited in the inspector of a settings
  file or written by the application running. Its replacement is attempted again, ten times at most, 10 ms apart.
- Flags with no value set, when their enumeration names none for 0, are written `""` in a project or a settings file
  and read again (#135), rather than written `null` and given back their default value once opened.

## [2.1.0] - 2026-11-03

### Added

- The galaxy of NebulaStudio (#103): `StudioPlugin/NebulaStudio.nbp` assembles the window of NebulaStudio from the
  components of `StudioPlugin`, each a part of NebulaStudio (its edition zone, its project, its lists, its property
  editor, its problems and its window) or a layout (`SplitterComponent`, `TabsComponent`, `GroupComponent`). Copied to
  `scene/` by each build, it is listed in the **Galaxies** tab, opened and started from NebulaStudio, or run on its own:
  `ExampleUniverseApp scene/NebulaStudio.nbp`. The package has it next to NebulaStudio and `ExampleUniverseApp`.
- `Nebula::Application` (#73), a library of its own: `NebulaApplication`, the `QApplication` of an executable whose
  content is a galaxy, loads it as it is built and runs it with `exec()`, which returns the code of the application.
  `ApplicationOptions`, given by the code, and its command line, when it is given a `QCommandLineParser`, tell it the
  directories of the plugins and of the galaxy, what the executable tells of itself, and where the messages go: their
  level, the console, a log file and a logger (`--plugin-dir`, `--plugin-root`, `--galaxy-dir`, `--log-level`,
  `--log-file`, `--console`). `find_package(Nebula 2.1 REQUIRED COMPONENTS Application)`.
- `Universe::scanPluginDirectory()`, which lists the plugins of a directory as `--plugin-dir` does (#73).
- The `file` of `ProjectComponent` (#73): the project file it opens each time it starts.
- `hiddenWhileDragging` of `SplitterComponent` and `TabsComponent` (#104): the widget is hidden while what is dragged
  from the lists in it is out of it, until it is dropped, what it is beside taking its room.
- The edge and the row of the dot of an interface in NebulaStudio (#117): Shift+right-clicking the dot opens a menu of
  its own. **Dot on the left edge** and **Dot on the right edge** put it on that edge of its instance, its name next to
  it; its links, the one being drawn included, go out of that edge, and **Expose...** adds the stargate on that side.
  **Swap with** swaps its row with the one of another interface of the instance, whatever their kinds. A project holds
  them in the `sides` and the `rows` of the component, when they are not those of their kinds.
- The theme of NebulaStudio is chosen in **View** > **Theme** (#118): **System**, as it was until then, follows the
  color scheme of the system; **Light** and **Dark** keep NebulaStudio light or dark whatever the system is, the title
  bars of its windows too where the system lets them. It changes at once, and is kept for its user, from one launch to
  the next.
- The overview of the edition zone in NebulaStudio (#64), under the properties: the whole scene shown, the application
  or the galaxy opened, small, what the edition zone shows of it framed; clicking it, or dragging in it, shows that
  part in the edition zone. **View** > **Overview** shows it or hides it, which is kept for the user, from one launch to
  the next. `OverviewComponent` provides it to the galaxy of NebulaStudio.
- Junctions of the links in NebulaStudio (#126): Shift+dragging from a link, or from one of its anchors, to an
  interface that can be bound to the one it comes from binds them through that point, a junction, a filled disc. Up to
  it, the links go through the same points, along one line drawn once, which clicking, or the Delete key, selects or
  removes as a whole; moving the junction moves them all. Double-clicking it, or **Remove junction point**, has each go
  from its provided interface again. A project holds a junction in the `anchors` of each link through it, which go
  through it again once it is opened.

### Changed

- NebulaStudio is made of parts that stand on their own, which its galaxy assembles (#103): `Studio`, what they
  share, `EditionZone`, `Project`, `PluginPanel`, `GalaxyPanel` and `ComponentPanel`.
- NebulaStudio runs its galaxy (#73): `NebulaStudio.exe` is a `NebulaApplication` of `scene/NebulaStudio.nbp`, whose
  `Project` opens the project file of its command line, its splash screen shown over its window; `MainWindow` is
  removed. It takes the options of `NebulaApplication`, `--help` and `--version` among them, and tells in a box why its
  galaxy cannot be loaded. It lists `StudioPlugin` as loaded, by the application that runs it.
- The dialog of a settings file in NebulaStudio (#105): a header tells the name of the file and what it is for;
  checking an instance, or a group of objects, checks all its properties, and tells whether all, some or none of them
  are held; the properties are filtered by a text, or restricted to those held, whose number is told; a name refused
  is told in amber, and the default values of the properties are named so.
- The dialog of a stargate in NebulaStudio (#116), as the one of a settings file: a header tells the name of the
  stargate and what it is for; checking an instance, or a group of objects, checks all it has to expose, each under its
  name, and tells whether all, some or none of it is exposed; the interfaces and the properties are filtered by a text,
  or restricted to those exposed, whose number is told; what another stargate exposes has the icon of the Stargate.
- The window of NebulaStudio gives most of its room to the edition zone (#104), on its left, the problems under it,
  without the frames of the Application and Components groups. On its right, the components, the galaxies and the
  plugins are in tabs, the components first, above the properties of the instance selected. That right side is hidden
  while a component or a galaxy dragged from it is out of it, until it is dropped; what the edition zone shows stays
  where it is on the screen as it goes and comes back, and the instance just dropped stays in view. The names in the
  lists of NebulaStudio are whole however narrow they are.
- A link goes through each of its anchors the way from the point before it to the one after it (#117): leftwards when
  the one after it is on the left, rather than always rightwards, which looped around the anchors of a link going back
  to the left.
- The preferences of NebulaStudio, the style of its links and its theme, are kept in a JSON file,
  `FabienDG/NebulaStudio.json` in the configuration directory of the user, `%APPDATA%` under Windows, rather than in the
  registry (#118): a style of the links chosen before is to be chosen again.

### Removed

- The check that refused to register a component in a plugin class declaring `build(<component>&)` (#88): the plugin
  class is generated since 2.0.0 and has none. A plugin written before 1.0.0 meets `NEBULA_PLUGIN` first, whose message
  now tells to move what its `build()` functions created to the constructor of each component before removing its
  class.

### Fixed

- NebulaStudio: the check boxes and the fields are edged in the edge color of the theme (#105). Fusion drew their
  edges from the color of the windows, which hid them in the dark scheme.
- Registering a component that derives privately from `Nebula::Component` is refused with a message telling it must
  derive publicly (#88), rather than by an error in `Plugin.h`, on a conversion of `Plugin::Instance`, that did not
  tell why. Registering in a class that does not derive publicly from `Nebula::Plugin<itself>`, accepted until then, is
  refused too. The message on a `final` component names `Plugin::Instance`, which derives from it.

## [2.0.0] - 2026-10-30

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
- `nebula_add_plugin()` generates the class of the plugin (#85), `<NAME>` in `<NAME>.h`, which its components include
  to register in it: a plugin no longer writes it, and `NEBULA_PLUGIN` no longer compiles, telling to remove the
  class written for 1.0.0.
- A plugin is the factory of its components (#94): `IPlugin` lists them, `components()`, and creates them,
  `create(name)`, the name being `Nebula::componentName()` of their class. The generated class of the plugin is its
  root object, deriving from `Nebula::Plugin<itself>`, and `ComponentFactory` has the plugin of a component create
  it (`add(pluginId, IPlugin&)`).
- A project gives a component its properties in the order the component declares them, rather than in the order of
  their names, and a galaxy the properties it exposes in the order it exposes them (#86): a property that tells what
  a component has is set before those that depend on it.
- Plugins built against 1.0.0 are refused: build them again. `IPlugin`, `Plugin.h` and `Component.h` change, and so
  does the major version of Core.

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

[3.0.0]: https://github.com/Shuko83/Nebula/releases/tag/3.0.0
[2.1.0]: https://github.com/Shuko83/Nebula/releases/tag/2.1.0
[2.0.0]: https://github.com/Shuko83/Nebula/releases/tag/2.0.0
[1.0.0]: https://github.com/Shuko83/Nebula/releases/tag/1.0.0
[0.3.0]: https://github.com/Shuko83/Nebula/releases/tag/0.3.0
[0.2.0]: https://github.com/Shuko83/Nebula/releases/tag/0.2.0
