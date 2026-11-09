# Deploying an application

What you give your users is a folder: the executable, the DLLs it needs, its plugins in `plugins/`, its galaxy in
`scene/`. This page makes that folder for `Clock.exe`, the application of
[running an application](Running-an-Application.md), with CMake.

## The folder

```text
Clock/
├── Clock.exe
├── Core.dll, Application.dll          Nebula
├── Qt6Core.dll, Qt6Gui.dll, Qt6Widgets.dll, ...
├── platforms/, styles/, imageformats/, ...   the plugins of Qt
├── msvcp140.dll, vcruntime140.dll, ...       the runtime of the compiler
├── plugins/
│   └── TimePlugin.nbdl                 every plugin the galaxy needs, those of Nebula included
└── scene/
    └── Clock.nbp                       the galaxy, and the galaxies it places, at the same relative paths
```

The build folder, `Acme/build/bin/Release`, already runs: it has everything but the runtime of the compiler, and
everything else that was built there. Make a clean folder rather than shipping it.

## Make it with CMake

Install rules of a component of their own, added to `Acme/Clock/CMakeLists.txt`, write the folder, and only it:

```cmake
# The folder to ship: cmake --install <build> --config Release --component Clock --prefix <folder>
install(PROGRAMS $<TARGET_FILE:Clock> $<TARGET_FILE:Nebula::Core> $<TARGET_FILE:Nebula::Application>
    DESTINATION . COMPONENT Clock EXCLUDE_FROM_ALL)
install(FILES $<TARGET_FILE:TimePlugin> DESTINATION plugins COMPONENT Clock EXCLUDE_FROM_ALL)
install(FILES Clock.nbp DESTINATION scene COMPONENT Clock EXCLUDE_FROM_ALL)
# The runtime of the compiler, so that it starts where Visual C++ is not installed.
set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP ON)
include(InstallRequiredSystemLibraries)
install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION . COMPONENT Clock EXCLUDE_FROM_ALL)
# Qt, once the rest is in place.
install(CODE "
    execute_process(COMMAND \"$<TARGET_FILE:Qt6::windeployqt>\" --no-compiler-runtime --no-translations
        \"\${CMAKE_INSTALL_PREFIX}/$<TARGET_FILE_NAME:Clock>\" COMMAND_ERROR_IS_FATAL ANY)
" COMPONENT Clock EXCLUDE_FROM_ALL)
```

```bash
cmake --install Acme/build --config Release --component Clock --prefix dist/Clock
```

`windeployqt` runs once the executable and the DLLs of Nebula are in place: it finds there the Qt they need, Widgets
included, and copies it with the plugins of Qt. The installer of NebulaStudio is made the same way, in
[`Installer/CMakeLists.txt`](https://github.com/Shuko83/Nebula/blob/main/Installer/CMakeLists.txt), which CPack then
packs into a setup with Inno Setup: a model for an installer of your own.

`cmu_add_executable()` also stages a package of your project in `<build>/package`, after each build: it is meant for
other projects to build against, not to ship. It has neither the DLLs of Nebula nor your galaxy.

## What to check

- **Every plugin of the galaxy**, and of the galaxies it places: yours, and those of Nebula it uses, such as
  `StandardPlugin.nbdl` for a log or a history, from `plugins/` next to NebulaStudio in the zip of a release.
- **The Qt only a plugin uses.** `windeployqt` looks at the executable and the DLLs next to it, not at the plugins: a Qt
  module that only a plugin uses, Qt Network or Qt Multimedia say, is not copied. Copy its DLL from the `bin` folder of
  your Qt, and the plugins of Qt it needs.
- **The galaxies it places**, in `scene/` at the paths the project gives them: a project writes them relative to its
  own file.
- **A single configuration.** Everything in `Release`, built with the Qt and the compiler of the Nebula you built
  against: Qt does not load a `Debug` plugin in a `Release` application.
- **Its name and organization.** They name the directory its settings files are written to by default, under
  `%LOCALAPPDATA%`: set them in `ApplicationOptions` before the first release, and keep them.
- **Where it writes.** Installed in `Program Files`, an application cannot write next to itself. Settings files with
  an empty directory are written to the configuration directory of the user, which is fine.

## Try it before you ship

Start the folder on a computer that has neither Qt nor Visual Studio, or at least from a terminal whose `PATH` holds
none of them:

```bash
dist/Clock/Clock.exe --console
```

What is missing tells itself: a DLL that Windows cannot find, a plugin of Qt that is not there
(`Could not find the Qt platform plugin "windows"`), or a galaxy whose plugin is not in `plugins/`. See
[troubleshooting](Troubleshooting.md).

## Licenses

Nebula is under the [Mozilla Public License 2.0](https://github.com/Shuko83/Nebula/blob/main/LICENSE): your plugins and
your application keep the license you choose. The files of Nebula you ship, `Core.dll` and `Application.dll` among
them, stay under the MPL: ship its license with them, and say where their source is found. Qt has its own terms, as
has the runtime of the compiler. The
[README](https://github.com/Shuko83/Nebula/blob/main/README.md#-license) tells the rest.
