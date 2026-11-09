# The Nebula course

A course in eight lessons, with exercises, that starts from nothing and ends with a plugin of your own. The first
three lessons need no code: you assemble applications in NebulaStudio from components that come with Nebula. The next
four write components in C++, one feature at a time, and turn them into an application you can ship. The last one is
yours: a plugin of your own idea, from its design to the folder you give your users.

## Before you start

- **For the whole course**: the zip of the 3.0.0 [release](https://github.com/Shuko83/Nebula/releases) unzipped, or a
  build of Nebula, see [getting started](Getting-Started.md). The installer of NebulaStudio is not enough: it has
  neither the example plugins of lessons 1 to 3 nor the files to build plugins.
- **From lesson 4**: Visual Studio and the Qt of your Nebula, CMake 3.21 or later, and C++ you can read: classes,
  virtual functions, lambdas. Qt helps: signals, `Q_PROPERTY`, widgets.

## How the exercises work

Each exercise has a goal, what to do, and how to check that you did it. Below them, folded, a hint and a solution:
open them once you have tried. The solutions of lessons 4 to 7 were built and run against Nebula 3.0.0.

Each lesson ends with a few questions, their answers folded as well: if one surprises you, the lesson links to the page
that tells more.

## The lessons

| Lesson | You learn to | You make |
| --- | --- | --- |
| [1. Assemble without code](Course-1-Assemble-Without-Code.md) | Place instances, link their interfaces, set their properties, run and save | A window that counts clicks, twice |
| [2. Keep what the user chooses](Course-2-Keep-What-the-User-Chooses.md) | Hold properties in a settings file, see and change what the user has | A count that survives a restart |
| [3. Reuse a part](Course-3-Reuse-a-Part.md) | Make a galaxy, expose what it offers, place it several times | Two counters from one file |
| [4. Properties and modes](Course-4-Properties-and-Modes.md) | Add properties of several types, refuse to start | A clock you can set |
| [5. Interfaces of your own](Course-5-Interfaces-of-Your-Own.md) | Declare an interface, provide it, consume it, consume a list | An alarm, and a panel that shows the clock and the alarm |
| [6. Log and history](Course-6-Log-and-History.md) | Write to the log, make an action undoable | A tally with Undo and Redo |
| [7. Ship an application](Course-7-Ship-an-Application.md) | Turn a galaxy into an executable, make the folder to ship | `Clock.exe` |
| [8. Your own plugin](Course-8-Your-Own-Plugin.md) | Design and build a plugin from an idea of your own | Yours |

Lessons 1 to 3 build on one another, as do lessons 4 to 7, which continue the `TimePlugin` of
[your first plugin](Your-First-Plugin.md): do that page first. Count about an hour for each lesson, more for the last.

## Where things are

The pages of this wiki tell more about each subject: [concepts](Concepts.md) for the words,
[assembling an application](Assembling-an-Application.md) for the editor, [writing components](Writing-Components.md)
for the code, and [troubleshooting](Troubleshooting.md) when something does not do what you expect.
