# Lesson 9 · Ship an application

*[The course](Course.md) › Lesson 9*

What you assembled runs in NebulaStudio. Your users do not have NebulaStudio: they want a program to double-click.
Here you make `Clock.exe`, which runs your project, then the folder you give away, with all it needs.

**You need**: [lesson 8](Course-8-Log-and-History.md) done.

## What you will make

```text
C:\work\Acme\
├── CMakeLists.txt          changed: one more folder to build
├── TimePlugin\             unchanged
└── Clock\                  new: the application
    ├── CMakeLists.txt
    ├── AlarmClock.nbp      the project it runs, made in NebulaStudio
    └── src\
        └── main.cpp        three lines of code
```

An application made with Nebula is always the same four things, side by side:

| What | Where, next to the executable |
| --- | --- |
| The executable | `Clock.exe` |
| The libraries of Nebula and of Qt | `Core.dll`, `Application.dll`, `Qt6Core.dll`... |
| The plugins of its components | `plugins\TimePlugin.nbdl`, `plugins\StandardPlugin.nbdl` |
| The project it runs | `scene\AlarmClock.nbp` |

## Exercise 9.1 · Assemble the alarm clock

**Goal**: a clean project, `AlarmClock.nbp`: the clock and the alarm in a window, with a log.

**Steps**

1. Start NebulaStudio with your plugin, and no project:

   ```powershell
   & "C:\Nebula\NebulaStudio\Release\bin\NebulaStudio.exe" --plugin-dir C:\work\Acme\build\bin\Release\plugins
   ```

2. **Plugins** tab: click `TimePlugin`, then **Load**.
3. From the **Components** tab, drop these six components, **in this order**, the log first:

   | # | Component | Its instance |
   | --- | --- | --- |
   | 1 | `LogComponent` | `LogComponent1` |
   | 2 | `ClockComponent` | `ClockComponent1` |
   | 3 | `FixedTimeComponent` | `FixedTimeComponent1` |
   | 4 | `AlarmComponent` | `AlarmComponent1` |
   | 5 | `PanelComponent` | `PanelComponent1` |
   | 6 | `WindowComponent` | `WindowComponent1` |

4. Make these six links, in this order:

   | # | Drag from the disc of | To |
   | --- | --- | --- |
   | 1 | `log`, in `LogComponent1` | `log`, in `ClockComponent1` |
   | 2 | `log`, in `LogComponent1` | `log`, in `AlarmComponent1` |
   | 3 | `source`, in `FixedTimeComponent1` | `source*`, in `AlarmComponent1` |
   | 4 | `display`, in `ClockComponent1` | `widgets`, in `PanelComponent1` |
   | 5 | `display`, in `AlarmComponent1` | `widgets`, in `PanelComponent1` |
   | 6 | `display`, in `PanelComponent1` | `content*`, in `WindowComponent1` |

5. Set two properties: `style` of `ClockComponent1` to `Large`, and `title` of `WindowComponent1` to `Alarm clock`.
6. **Run** > **Start**.

   *You should see:* a window titled `Alarm clock`, the time in large, and `Ringing!` under it. **Run** > **Stop**.

7. **File** > **Save As...**. Go to `C:\work\Acme`, make a new folder `Clock` there (right-click, **New** >
   **Folder**), open it, type `AlarmClock` as the file name, **Save**.
8. Close NebulaStudio.

**Check**: `C:\work\Acme\Clock\AlarmClock.nbp` exists.

The alarm is on the stand-in, so that you see it ring each time you try. Exercise 9.4 puts the real clock back.

## Exercise 9.2 · Write the application

**Steps**

1. Make the folder of its code:

   ```powershell
   New-Item -ItemType Directory -Force C:\work\Acme\Clock\src
   ```

2. Create the file `C:\work\Acme\Clock\src\main.cpp` with this content:

   ```cpp
   // Acme/Clock/src/main.cpp (complete)
   #include <QCommandLineParser>

   #include "Clock_info.h" // Written for you when the application is built: its name, its version
   #include "Nebula/Application/NebulaApplication.h"

   int main(int argc, char* argv[])
   {
       QCommandLineParser parser;
       Nebula::NebulaApplication app(argc, argv, parser, QStringLiteral("AlarmClock.nbp"),
                                     {.executable = {.name = QString::fromUtf8(ClockInfo::name),
                                                     .version = QString::fromUtf8(ClockInfo::version),
                                                     .organization = QString::fromUtf8(ClockInfo::organization)}});
       return app.exec();
   }
   ```

   | Line | What it does |
   | --- | --- |
   | `Nebula::NebulaApplication app(..., QStringLiteral("AlarmClock.nbp"), ...)` | Loads the project `AlarmClock.nbp`, looked for in the folder `scene` next to the executable, and its plugins, looked for in `plugins` |
   | `.executable = {...}` | The name, the version and the organization of your application: `Clock`, `1.0.0`, `AcmeCorp` |
   | `app.exec()` | Starts the instances, runs until the last window is closed, then stops them |

3. Create the file `C:\work\Acme\Clock\CMakeLists.txt` with this content:

   ```cmake
   # Acme/Clock/CMakeLists.txt (complete)
   cmu_add_executable(
       NAME Clock
       LINK_LIBRARIES PRIVATE Nebula::Application Qt6::Widgets
   )

   # Next to Clock.exe: the DLLs of Nebula, StandardPlugin in plugins/, and its project in scene/.
   add_custom_command(TARGET Clock POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:Clock> $<TARGET_FILE_DIR:Clock>
       COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:Clock>/plugins
       COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Nebula::StandardPlugin> $<TARGET_FILE_DIR:Clock>/plugins
       COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/AlarmClock.nbp
               $<TARGET_FILE_DIR:Clock>/scene/AlarmClock.nbp
       COMMAND_EXPAND_LISTS)
   add_dependencies(Clock TimePlugin) # The application loads the plugin without linking it

   # The folder to ship: cmake --install <build> --config Release --component Clock --prefix <folder>
   install(PROGRAMS $<TARGET_FILE:Clock> $<TARGET_FILE:Nebula::Core> $<TARGET_FILE:Nebula::Application>
       DESTINATION . COMPONENT Clock EXCLUDE_FROM_ALL)
   install(FILES $<TARGET_FILE:TimePlugin> $<TARGET_FILE:Nebula::StandardPlugin>
       DESTINATION plugins COMPONENT Clock EXCLUDE_FROM_ALL)
   install(FILES AlarmClock.nbp DESTINATION scene COMPONENT Clock EXCLUDE_FROM_ALL)
   set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP ON)
   include(InstallRequiredSystemLibraries)
   install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION . COMPONENT Clock EXCLUDE_FROM_ALL)
   install(CODE "
       execute_process(COMMAND \"$<TARGET_FILE:Qt6::windeployqt>\" --no-compiler-runtime --no-translations
           \"\${CMAKE_INSTALL_PREFIX}/$<TARGET_FILE_NAME:Clock>\" COMMAND_ERROR_IS_FATAL ANY)
   " COMPONENT Clock EXCLUDE_FROM_ALL)
   ```

   It has three parts. You do not need to understand each line to use it; here is what each part is for:

   | Part | What it does |
   | --- | --- |
   | `cmu_add_executable(...)` | Builds `Clock.exe` from the files of `src`, and copies Qt next to it |
   | `add_custom_command(...)` | After each build, puts next to `Clock.exe` the libraries of Nebula, `StandardPlugin`, and your project |
   | The `install(...)` lines | Describe the folder to ship, made in exercise 9.3 |

4. In `C:\work\Acme\CMakeLists.txt`, add `Application` and `StandardPlugin` to the line `find_package(Nebula ...)`, and
   one line at the end. The file becomes:

   ```cmake
   # Acme/CMakeLists.txt (complete)
   cmake_minimum_required(VERSION 3.21)
   project(Acme VERSION 1.0.0 LANGUAGES CXX)

   # The first part of the ids of your plugins: AcmeCorp.Acme.TimePlugin
   set(cmu_organization "AcmeCorp")

   find_package(Nebula 3.0 REQUIRED COMPONENTS Core Application Standard StandardPlugin)
   find_package(Qt6 REQUIRED COMPONENTS Widgets)

   # Executables in <build>/bin, plugins in <build>/bin/plugins
   set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

   add_subdirectory(TimePlugin)
   add_subdirectory(Clock)
   ```

5. Build:

   ```powershell
   cmake --build C:/work/Acme/build --config Release
   ```

   *You should see:* near the end, a line that ends with `bin\Release\Clock.exe`.

6. Run it, with `--console` to read its messages:

   ```powershell
   & "C:\work\Acme\build\bin\Release\Clock.exe" --console
   ```

   *You should see:* the window `Alarm clock`, without NebulaStudio, and in the terminal
   `log: AlarmComponent1: Ringing at 07:30`. Close the window: the application ends.

7. Ask its version:

   ```powershell
   & "C:\work\Acme\build\bin\Release\Clock.exe" --version
   ```

   *You should see:* a small box, or a line, that says `Clock 1.0.0`.

**If it does not work**

- *`Clock.exe` ends at once, with no window*: start it with `--console`. It says what is missing: `The galaxy
  AlarmClock.nbp is in none of ...` when the project is not in `scene`, or which plugin cannot be loaded.
- *The build says `AlarmClock.nbp` cannot be found*: the project was saved elsewhere than `C:\work\Acme\Clock`.
- *You changed `AlarmClock.nbp` in NebulaStudio and `Clock.exe` shows the old one*: build again, the project is
  copied next to the executable by each build.

## Exercise 9.3 · Make the folder to ship

The folder `build\bin\Release` runs on your computer, where Visual Studio is installed. The folder you give away
must also hold the libraries of the compiler, and nothing that is not needed.

**Steps**

1. In the terminal:

   ```powershell
   cmake --install C:/work/Acme/build --config Release --component Clock --prefix C:/work/Acme/dist/Clock
   ```

   *You should see:* lines that start with `-- Installing:`, then the names of the files of Qt.

2. Look at what was made:

   ```powershell
   Get-ChildItem C:\work\Acme\dist\Clock, C:\work\Acme\dist\Clock\plugins, C:\work\Acme\dist\Clock\scene -Name
   ```

   *You should see:* `Clock.exe`, `Core.dll`, `Application.dll`, the `Qt6...dll`, `msvcp140.dll` and
   `vcruntime140.dll`, the folders of Qt (`platforms`, `styles`...), then `StandardPlugin.nbdl` and `TimePlugin.nbdl`,
   then `AlarmClock.nbp`.

3. Try it as a user would, from a terminal that knows neither Qt nor Visual Studio:

   ```powershell
   $env:PATH = "C:\Windows\System32;C:\Windows"; & "C:\work\Acme\dist\Clock\Clock.exe"
   ```

   *You should see:* the alarm clock. Close it, then close this terminal: its `PATH` was changed for the test.

**Check**: copy `C:\work\Acme\dist\Clock` to a USB key or another computer, and double-click `Clock.exe` there.

## Exercise 9.4 · The real clock, and what the user keeps

Two last touches before you give it away: the alarm follows the real time, and the user keeps the time of their
alarm.

**Steps**

1. Open the project in NebulaStudio:

   ```powershell
   & "C:\Nebula\NebulaStudio\Release\bin\NebulaStudio.exe" --plugin-dir C:\work\Acme\build\bin\Release\plugins C:\work\Acme\Clock\AlarmClock.nbp
   ```

2. Drag from the disc of `source`, in `ClockComponent1`, to the circle of `source*`, in `AlarmComponent1`. Then click
   the box `FixedTimeComponent1` and press Delete.
3. Right-click an empty place, **Add settings file**. Under `AlarmComponent1`, check `alarm`. Click **OK**.
4. **File** > **Save**, then close NebulaStudio.
5. Build, then make the folder again:

   ```powershell
   cmake --build C:/work/Acme/build --config Release
   ```

   ```powershell
   cmake --install C:/work/Acme/build --config Release --component Clock --prefix C:/work/Acme/dist/Clock
   ```

6. Run `C:\work\Acme\dist\Clock\Clock.exe`.

   *You should see:* the time, and `Alarm at 07:30`. It rings at 07:30.

**Good to know**: the settings file of `Clock.exe` is written in a folder named after its organization and its name,
`AcmeCorp\Clock`, under `%LOCALAPPDATA%`. The value `alarm` in it is the one of the user: to change it without
building anything, open that file. Showing it in the window, for the user to change, is a component to write:
perhaps yours, in the next lesson.

## Your turn

Change the version of your application to `1.1.0`, and check that `Clock.exe --version` says so.

<details>
<summary>Solution</summary>

The version is the one of the project. In `C:\work\Acme\CMakeLists.txt`, change the second line to:

```cmake
project(Acme VERSION 1.1.0 LANGUAGES CXX)
```

Build, then:

```powershell
& "C:\work\Acme\build\bin\Release\Clock.exe" --version
```

It says `Clock 1.1.0`. `Clock_info.h`, which `main.cpp` includes, is written again by each build with the name and the
version of the project. The plugin is `1.1.0` too: the **Plugins** tab of NebulaStudio shows it.

</details>

## What you learned

- An application is an executable of three lines, with its libraries, its plugins in `plugins`, and its project in
  `scene`.
- `cmake --build` makes a folder that runs on your computer; `cmake --install` makes the folder to give away.
- `--console` shows why an application does not start.

**Next**: [Lesson 10 · Your own plugin](Course-10-Your-Own-Plugin.md)
