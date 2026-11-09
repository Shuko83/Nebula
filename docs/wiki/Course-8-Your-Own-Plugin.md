# Lesson 8 · Your own plugin

*[The course](Course.md) › Lesson 8*

You have assembled applications, written components that provide and consume interfaces, used the log and the
history, and shipped an executable. Now make a plugin of your own idea, from its design to the folder you give your
users. There is no solution to this lesson: there is your plugin, and the checklist below to review it.

## The brief

Write a plugin, in a project of your own, with:

1. **At least three components**, each named `<Name>Component`, each doing one thing, with a description and a
   category.
2. **An interface of your own**, an abstract class in a header, provided by one component and consumed by another.
3. **A list**: a component that consumes several interfaces of a type, in the order of their bindings.
4. **Properties** that NebulaStudio edits and a project saves, one of them an enumeration, with a setter that refuses
   a value it cannot take.
5. **A mandatory interface**, and an `onStart()` that refuses to start when the component cannot work.
6. **The log**, for what the user of the application should be able to read.
7. **A settings file** in the application, for what its user chooses.
8. **A galaxy**, placed at least twice, with a stargate.
9. **An executable** that runs it, and **the folder to ship**, tried with a bare `PATH`.

The history is a bonus: an action of the user that `Undo` takes back.

## Ideas

Take one, or bring your own.

| Idea | Components, for a start |
| --- | --- |
| **Pomodoro timer** | A timer that provides a remaining time, a display of it, a button bar, a counter of the sessions done, kept in a settings file |
| **Dice roller** | Dice that provide their value, a panel of several dice (a list), a total, a history of the rolls to undo |
| **Stopwatch** | A stopwatch that provides its time, start, stop and lap buttons, a list of laps, a display that formats a time |
| **Unit converter** | Units that provide a conversion, a converter that consumes a list of them, an input and an output |
| **To-do list** | A list of tasks, an input to add one, a counter of those done, each action undoable |
| **Weather panel** | A source of readings (a fixed one to try with, as in [lesson 5](Course-5-Interfaces-of-Your-Own.md)), a display per reading, a panel of them |

## The steps

1. **Design on paper first.** List your components; for each, what it provides, what it consumes, and its properties.
   A table is enough:

   | Component | Provides | Consumes | Properties |
   | --- | --- | --- | --- |
   | `TimerComponent` | `remaining`: `IRemainingTime` | `log`: `Nebula::ILog` | `minutes`, `mode` (`Focus`, `Break`) |
   | `RemainingDisplayComponent` | `display`: `QWidget` | `remaining`: `IRemainingTime`, mandatory | `format` |
   | `SessionCounterComponent` | `display`: `QWidget` | `remaining`: `IRemainingTime` | `count` |

   An interface is the smallest thing one component needs of another: a time remaining, not the whole timer.
2. **Make the project**: a root `CMakeLists.txt` with your `cmu_organization`, a folder per plugin with its
   `nebula_add_plugin()`, and its sources in `src/`, as in [your first plugin](Your-First-Plugin.md).
3. **Write one component at a time**, and try each in NebulaStudio before the next. A stand-in that provides an
   interface with fixed values, as `FixedTimeComponent` does, lets you try a consumer alone.
4. **Assemble the application** in NebulaStudio, make the galaxy, add the settings file.
5. **Ship it**, as in [lesson 7](Course-7-Ship-an-Application.md), and try the folder on a computer that has neither Qt
   nor Visual Studio, if you have one.

## Review it

Go through this list once it works; each line links to what tells more.

- [ ] Each component derives publicly from `Nebula::Component`, has `Q_OBJECT`, and creates its objects as members, in
      its constructor. ([Writing components](Writing-Components.md#the-shape-of-a-component))
- [ ] Each declares its interfaces in its constructor. A pointer it consumes is tested before it is used.
- [ ] What a component takes from another, a widget it shows, it gives back without a parent in `onUnavailable`.
      ([Giving back what you were given](Writing-Components.md#giving-back-what-you-were-given))
- [ ] Each setter changes the value only when it differs, then emits its `NOTIFY` signal.
- [ ] No property that makes interfaces is held by a settings file.
      ([Settings files](Settings-Files.md#what-to-know))
- [ ] The application starts with no amber mark in NebulaStudio, and the list of problems is empty.
- [ ] Removing an instance while the others are edited crashes nothing: try it with each.
- [ ] The executable starts from the folder you ship, with a bare `PATH`, and `--console` shows no warning you did not
      expect. ([Troubleshooting](Troubleshooting.md))
- [ ] The plugin and the application are built in `Release`, with the Qt and the compiler of your Nebula.

## Share it

A plugin is a `.nbdl` file: give it, with the headers of its interfaces if other plugins are to use them, and the
version of Nebula it is built against. Your users put it in `plugins/` next to NebulaStudio, or open it with **Open
plugin...**. Your plugin keeps the license you choose; only the header that `nebula_add_plugin()` generates for it stays
under the MPL 2.0, Nebula's, as the [README](https://github.com/Shuko83/Nebula/blob/main/README.md#-license) tells.

Made something? Tell the project in its [issues](https://github.com/Shuko83/Nebula/issues): what was hard to learn
helps the next ones, and this course too.
