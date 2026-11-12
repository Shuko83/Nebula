# Lesson 10 · Your own plugin

*[The course](Course.md) › Lesson 10*

You know how to assemble, how to write components that provide and consume, how to use the log and the history, and
how to ship. This lesson has no listing to copy: you make a plugin of your own idea. It gives you the order to work
in, a worked example of the first step, and a list to check your work against.

**You need**: lessons 1 to 9 done. Keep `C:\work\Acme` open: every file you need is a variation of one of its files.

## Step 1 · Choose an idea

Take one of these, or your own. Choose something small: three components are enough.

| Idea | Components, for a start |
| --- | --- |
| **Pomodoro timer** | A timer that provides the time left; a text that shows it; a counter of the sessions done, kept in a settings file |
| **Dice roller** | A die that provides its value; a panel of several dice (a list); a total; each roll undone with the history |
| **Stopwatch** | A stopwatch that provides its time; start, stop and lap buttons; the list of the laps |
| **Unit converter** | Units, each providing a conversion; a converter that consumes a list of them; an input and an output |
| **To-do list** | A list of tasks; a field to add one; a counter of those done; each action undone with the history |

## Step 2 · Design it on paper

Before any code, write one row for each component: what it provides, what it consumes, its properties. Here is the
design of a Pomodoro timer, as an example:

| Component | Provides | Consumes | Properties |
| --- | --- | --- | --- |
| `TimerComponent` | `remaining`, an `IRemainingTime` | `log`, a `Nebula::ILog`, optional | `minutes`, a number; `mode`, `Focus` or `Break` |
| `RemainingDisplayComponent` | `display`, a `QWidget` | `remaining`, an `IRemainingTime`, mandatory | `format`, a text |
| `SessionCounterComponent` | `display`, a `QWidget` | `remaining`, an `IRemainingTime`, mandatory | `count`, a number, kept in a settings file |

and of its interface:

```cpp
// What tells how much time is left.
class IRemainingTime
{
  public:
    virtual ~IRemainingTime() = default;

    [[nodiscard]] virtual int secondsLeft() const = 0;
};
```

Three questions to ask yourself for each row:

1. **Does it do one thing?** A component that counts and shows is two components.
2. **What is the least it needs from the others?** That is its interface: the time left, not the whole timer.
3. **What will the person who assembles want to set?** Those are its properties.

## Step 3 · Make the project

1. Make a new folder, `C:\work\Pomodoro` say, with the same shape as `C:\work\Acme`: a `CMakeLists.txt`, a folder for
   the plugin with its own `CMakeLists.txt`, and a `src` folder in it.
2. Copy the two `CMakeLists.txt` of [lesson 8](Course-8-Log-and-History.md#exercise-81--give-your-plugin-the-interfaces-of-nebula),
   and change three names: the name of the project, after `project(`; your organization, in `cmu_organization`; the
   name of the plugin, after `NAME` and in `add_subdirectory(`.
3. Configure it, as in [lesson 5](Course-5-Your-First-Component.md#exercise-54--build-the-plugin), with your folder in
   place of `C:/work/Acme`.

The name of the plugin is also the name of the header its components include: with `NAME PomodoroPlugin`, write
`#include "PomodoroPlugin.h"` and `NEBULA_REGISTER_COMPONENT(PomodoroPlugin, TimerComponent);`.

## Step 4 · Write one component at a time

Write the component that consumes nothing first. Build it, load it, place it, run it. Only then write the next one.
A plugin of three components written at once, that does not work, is hard to fix; one component is not.

| You want | Copy from |
| --- | --- |
| A component that provides a widget | `ClockComponent`, [lesson 5](Course-5-Your-First-Component.md) |
| A property: a text, a number, a choice | [Lesson 6](Course-6-Properties-and-Modes.md), the recipe |
| An interface of your own, provided and consumed | `ITimeSource`, `ClockComponent` and `AlarmComponent`, [lesson 7](Course-7-Interfaces-of-Your-Own.md) |
| A stand-in, to try a consumer alone | `FixedTimeComponent`, lesson 7 |
| A component that takes several | `PanelComponent`, lesson 7 |
| Messages | [Lesson 8](Course-8-Log-and-History.md), exercise 8.2 |
| An action the user undoes | `TallyComponent`, lesson 8 |

## Step 5 · Assemble, and ship

1. Assemble the application in NebulaStudio, the log first, and save it in the folder of your application.
2. Add a settings file for what the user chooses, as in [lesson 3](Course-3-Keep-What-the-User-Chooses.md).
3. If a group of instances comes back several times, make it a galaxy, as in [lesson 4](Course-4-Reuse-a-Part.md).
4. Make the executable and the folder to ship, as in [lesson 9](Course-9-Ship-an-Application.md): copy
   `Clock\CMakeLists.txt` and `main.cpp`, and replace `Clock`, `AlarmClock.nbp` and `TimePlugin` with your names.

## Check your work

Go through this list once it runs.

**The components**

- [ ] Each derives publicly from `Nebula::Component`, has `Q_OBJECT`, a description and a category.
- [ ] Each is registered with `NEBULA_REGISTER_COMPONENT`.
- [ ] What a component is made of is a member, created with it: a text, a timer, a class of its own.
- [ ] Each interface is declared in the constructor.

**The interfaces**

- [ ] A pointer that is consumed is tested before each use, or only used while its buttons are enabled.
- [ ] A widget taken from another component is given back without a parent, `setParent(nullptr)`.
- [ ] `Mandatory` only where the component does nothing without it.

**The properties**

- [ ] Each setter does nothing when the value is the same, else keeps it and emits its signal.
- [ ] A value that makes no sense is refused by the setter, or by `onStart()`.

**In NebulaStudio**

- [ ] No amber mark, and nothing in the problems.
- [ ] **Start** and **Stop** several times in a row: it behaves the same each time.
- [ ] Remove an instance while the others are there, then Ctrl+Z: nothing crashes.

**The application**

- [ ] It starts from the folder to ship, from a terminal whose `PATH` is only Windows.
- [ ] Started with `--console`, it writes no warning you did not expect.
- [ ] What the user chooses is still there at the next start.

## When you are stuck

- [Troubleshooting](Troubleshooting.md) goes from what you see to its cause.
- [Writing components](Writing-Components.md) tells all that a component can do, beyond the course.
- The components of [`WidgetsPlugin`](https://github.com/Shuko83/Nebula/tree/main/exemple/WidgetsPlugin/src) are
  short, and written the way the course teaches: read them.

## Share it

A plugin is one file, `.nbdl`. Give it with the version of Nebula it was built against, `3.0.0`, and the headers of
its interfaces if other plugins are to use them. Whoever receives it puts it in the `plugins` folder next to
NebulaStudio, or opens it with **Open plugin...** in the **Plugins** tab. Your plugin keeps the license you choose.

Tell the project what you made, and what was hard to learn, in its
[issues](https://github.com/Shuko83/Nebula/issues): it helps the next ones, and this course.
