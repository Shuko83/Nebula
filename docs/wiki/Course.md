# The Nebula course

A course in ten lessons that starts from nothing and ends with a plugin of your own. You only need to know what
Nebula is for: building applications out of components. Everything else is shown, one action at a time: where to
click, what to type, which file to create, and what you should see once it is done.

- **Lessons 1 to 4** need no code. You install Nebula, then assemble applications in NebulaStudio, its editor.
- **Lessons 5 to 9** write components in C++, a few lines at a time, and end with an application you can ship.
- **Lesson 10** is yours: a plugin of your own idea.

Do the lessons in order: each one starts from what the one before it left.

## How to read a lesson

Each lesson is made of exercises, and each exercise of numbered steps. A step is one action. After it, a line tells
what the screen shows when the action worked:

> 3. Click the **Plugins** tab.
>
>    *You should see:* a list of four plugins.

- **Bold** is a word written on the screen: a menu, a tab, a button.
- `This writing` is something to type, or a name to look for: a file, an instance, a property.
- When what you see differs from *You should see*, stop and read **If it does not work**, at the end of the exercise.
  Going on with a step that failed only moves the problem further.
- Each lesson ends with **Your turn**: a small task without steps, to check that you can do it alone. Its solution is
  folded under it: open it once you have tried.

## The folders of the course

The course always uses the same folders. Use them too, at least the first time: every command of the course can then be
copied as it is.

| Folder | What it holds | Made in |
| --- | --- | --- |
| `C:\Nebula` | Nebula 3.0.0, unzipped: NebulaStudio, the example plugins, and what plugins are built against | Lesson 1 |
| `C:\Qt\6.12.0\msvc2022_64` | Qt, which Nebula is built on | Lesson 5 |
| `C:\work\Acme` | Your own project, from the first line of code | Lesson 5 |

## The lessons

| Lesson | You learn to | You make |
| --- | --- | --- |
| [1. Get ready](Course-1-Get-Ready.md) | Install Nebula, start NebulaStudio, find your way in its window, load a plugin | A NebulaStudio ready to work |
| [2. Assemble without code](Course-2-Assemble-Without-Code.md) | Place instances, link them, set their properties, run and save | A window that counts clicks |
| [3. Keep what the user chooses](Course-3-Keep-What-the-User-Chooses.md) | Keep values from one run to the next in a settings file | A count that survives a restart |
| [4. Reuse a part](Course-4-Reuse-a-Part.md) | Make a galaxy, say what it offers, place it several times | Two counters from one file |
| [5. Your first component](Course-5-Your-First-Component.md) | Install the tools, make a project, write, build and load a plugin | A clock, and a window to show it |
| [6. Properties and modes](Course-6-Properties-and-Modes.md) | Add properties of several types, refuse to start | A clock you can set |
| [7. Interfaces of your own](Course-7-Interfaces-of-Your-Own.md) | Have two components work together through an interface you write | An alarm, and a panel |
| [8. Log and history](Course-8-Log-and-History.md) | Use the components that come with Nebula: a log, an undo history | Messages, and a count with Undo |
| [9. Ship an application](Course-9-Ship-an-Application.md) | Turn what you assembled into an executable, and make the folder to give away | `Clock.exe` |
| [10. Your own plugin](Course-10-Your-Own-Plugin.md) | Design and build a plugin from an idea of your own | Yours |

Count about an hour for each lesson, two for lesson 5, where the tools are installed.

## The words of the course

Six words come back in every lesson. Each is shown again where it is first used.

| Word | What it is |
| --- | --- |
| **Plugin** | A file, `.nbdl`, that brings components. NebulaStudio and applications load it |
| **Component** | A kind of building block: a window, a button, a counter. It is listed in NebulaStudio |
| **Instance** | One block of that kind, placed in an application, with a name of its own: `Counter1` |
| **Property** | A value of an instance that you set: the title of a window, the text of a button |
| **Interface** | What an instance offers to the others (it *provides* it), or needs from them (it *consumes* it) |
| **Link** | A line you draw from what one instance provides to what another consumes: they now work together |

[Concepts](Concepts.md) tells more about each of them, once you have seen them at work.
