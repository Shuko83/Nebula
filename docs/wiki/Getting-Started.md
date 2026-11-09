# Getting started

In this page you get Nebula, then assemble a small application in NebulaStudio from the example components it comes
with, run it, save it, and run it again outside the editor. No code is written: that comes with
[your first plugin](Your-First-Plugin.md).

## Get Nebula

Nebula is built for 64-bit Windows with Visual Studio 2022 and Qt 6.12: that is what its CI checks, and what its
releases hold. There are three ways to get it.

### The zip of a release

Each [release](https://github.com/Shuko83/Nebula/releases) has a zip named after what its binaries are built for and
with, `Nebula_<version>_<platform>_<compiler>_Qt<major>.<minor>.zip`: `Nebula_3.0.0_win-x64_msvc2022_Qt6.12.zip` is
built for 64-bit Windows with Visual Studio 2022 and Qt 6.12. Unzip it anywhere. It holds everything: NebulaStudio and
the example applications, ready to run, and the libraries, headers and CMake files to build plugins and applications.

```text
Nebula_<version>_<platform>_<compiler>_Qt<major>.<minor>/
├── NebulaConfig.cmake               what find_package(Nebula) reads
├── NebulaStudio/Release/bin/        NebulaStudio.exe, the DLLs it needs, plugins/ and scene/
├── ExampleUniverseApp/Release/bin/  runs a project file given on its command line
├── ExampleApp/Release/bin/          lists the plugins next to it and runs a Greeter
├── Core/  Application/  Standard/   headers, libraries and CMake files, to build against
├── ExternalPlugin/Release/lib/      ExternalPlugin.nbdl, a plugin to open from NebulaStudio
├── ...                              the other plugins, each in its own folder
├── LICENSE
└── README.md
```

Each application has its own `plugins/` folder next to it, with the plugins of the repository: `ExamplePlugin`,
`WidgetsPlugin`, `StandardPlugin`, and `StudioPlugin`, the one NebulaStudio itself is made of.

### The installer of NebulaStudio

Next to the zip, a release has an installer of NebulaStudio for Windows, named as the zip is, with `_setup.exe` at the
end. It installs NebulaStudio for the user who runs it, in `%LOCALAPPDATA%\Programs\NebulaStudio`, and adds it to the
Start menu. It installs the editor and what it needs to start, and nothing else: none of the example plugins used below,
and no headers to build plugins. Take it to assemble applications from plugins you already have; take the zip to follow
this page, or to write plugins.

### Build it from source

You need CMake 3.19 or later, a C++20 compiler, and Qt 6 with its Widgets and Svg modules (and Test, for the tests).

```bash
git clone --recurse-submodules https://github.com/Shuko83/Nebula.git
```

```bash
cmake -S Nebula -B Nebula/build -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/msvc2022_64
```

```bash
cmake --build Nebula/build --config Release --parallel
```

`CMakeUtils` is a git submodule: `--recurse-submodules` fetches it, or `git submodule update --init` after a plain
clone. Qt is found through `CMAKE_PREFIX_PATH`, or through the `QTDIR` environment variable. `-DNEBULA_BUILD_TESTS=OFF`
leaves the tests out.

NebulaStudio is then in `Nebula/build/bin/Release` (`Nebula/build/bin` with a single-configuration generator such as
Ninja), next to the example applications, with the plugins in `plugins/` and the galaxy of NebulaStudio in `scene/`.
The build also stages a package like the zip of a release in `Nebula/build/package`: that is what a plugin of your own
is built against, see [your first plugin](Your-First-Plugin.md).

## Assemble a first application

The plugin `WidgetsPlugin`, in `plugins/` next to NebulaStudio, makes the main window of an application from its
components. You will assemble a window whose top bar has a button, and which counts the clicks on it.

1. **Start NebulaStudio.** A splash screen shows its version, then its main window: on the left the edition zone, where
   applications are assembled, and on the right the **Components** tab, among others. See
   [a tour of NebulaStudio](NebulaStudio-Tour.md).
2. **Load the plugin.** NebulaStudio lists the plugins next to it, but loads only those it needs. In the **Plugins**
   tab, select `WidgetsPlugin`, then click **Load**: its components join the **Components** tab.
3. **Find the components.** In the **Components** tab, choose **Category** in the list next to the search bar, then
   type `widgets`: the components of `WidgetsPlugin` remain.
4. **Place them.** Drag `MainWindowComponent`, `TopBarComponent`, `ActionComponent` and `CounterComponent` into the
   edition zone. Each becomes an instance, named after its component and a number: `MainWindowComponent1`,
   `TopBarComponent1`, `ActionComponent1` and `CounterComponent1`.
   `TopBarComponent1` and `CounterComponent1` show an exclamation mark in an amber disc: each of them misses an
   interface it cannot do without, and the list of problems, under the edition zone, tells which.
5. **Link them.** Each instance lists its interfaces, with a dot on its edge: what it consumes on the left, what it
   provides on the right. Drag from a dot on the right to a dot on the left to bind them:
   - `mainWindow` of `MainWindowComponent1` to `mainWindow` of `TopBarComponent1`: the bar goes in that window;
   - `action` of `ActionComponent1` to `actions` of `TopBarComponent1`, a list, drawn as two circles: the bar shows
     the action;
   - `action` of `ActionComponent1` to `action` of `CounterComponent1`: the counter counts its clicks;
   - `display` of `CounterComponent1` to `centralWidget` of `MainWindowComponent1`: the window shows the count.

   The amber marks go, and the list of problems says that every instance has what it needs.
6. **Set their properties.** Select `ActionComponent1`: its properties are listed on the right of the edition zone.
   Set its `text` to `Count`. Select `CounterComponent1`, and set its `text` to `Clicked %1 times`.
7. **Run it.** **Run** > **Start** (F5): a window opens, `Count` in its bar. Each click adds one to the count it shows.
   **Run** > **Stop** (Shift+F5) closes it, and every property gets the value it had before the start again: the count
   is back to 0.
8. **Save it.** **File** > **Save As...** proposes the `scene/` folder next to NebulaStudio: save it as `Counter.nbp`.

The file is JSON: the plugins the application needs, its instances with their properties, the bindings between their
interfaces, and where they are in the edition zone. The
[README](https://github.com/Shuko83/Nebula/blob/main/README.md#-project-file) describes each key.

## Run it on its own

A project file is an application. `ExampleUniverseApp`, one of the example applications, runs the one given on its
command line until its last window is closed:

```bash
ExampleUniverseApp <the scene folder of NebulaStudio>/Counter.nbp
```

It finds `WidgetsPlugin` in `plugins/` next to it: a project names its plugins by their id, and an application looks
for them among those it found first. Your own application runs a project the same way, with three lines of `main`: see
[running an application](Running-an-Application.md).

## Where to go next

- [Concepts](Concepts.md), for the words used above, and those to come.
- [Your first plugin](Your-First-Plugin.md), to make components of your own.
- [Assembling an application](Assembling-an-Application.md), for all that the edition zone does.
