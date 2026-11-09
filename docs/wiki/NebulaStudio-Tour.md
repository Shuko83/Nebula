# A tour of NebulaStudio

NebulaStudio is the editor of Nebula: it lists the components of the plugins it finds, assembles applications from them
in its edition zone, runs them in place, and saves them as project files. This page shows where things are; what they
do is in [assembling an application](Assembling-an-Application.md).

## Starting it

```bash
NebulaStudio [options] [project.nbp]
```

| Option | What it does |
| --- | --- |
| `--plugin-dir <directory>` | Lists the plugins of that directory too, after those of `plugins/` next to NebulaStudio; as many times as needed |
| `--plugin-root <directory>` | The directory the plugin files of a project are relative to: the one of NebulaStudio otherwise |
| `--console` | Writes the messages of NebulaStudio, and of the application it runs, to the terminal |
| `--log-file <file>` | Adds them to a file as well |
| `--log-level <level>` | The least severe messages written: `debug`, `info`, `warning`, `critical` or `fatal` |
| `--galaxy-dir <directory>` | Where NebulaStudio looks for its own galaxy, `NebulaStudio.nbp`, before `scene/` next to it |
| `--help`, `--version` | What they say |

A project file given after the options is opened once the window shows. A splash screen tells the version of Nebula, and
the Qt and the compiler NebulaStudio was built with, for two seconds or until it is clicked.

## Its window

```text
┌─────────────────────────────────────────────────┬──────────────────────────────────┐
│ Application › Hello1                 [Find...]  │  Components │ Galaxies │ Plugins │
│                                                 │  search bar, list, viewer        │
│                                                 ├──────────────────────────────────┤
│                 the edition zone                │  the properties of the           │
│                                                 │  instance selected               │
├─────────────────────────────────────────────────┼──────────────────────────────────┤
│ the problems of the application                 │  the overview                    │
└─────────────────────────────────────────────────┴──────────────────────────────────┘
```

- **The edition zone**, on the left, takes most of the room: the application being assembled. Above it, the path to
  the galaxy shown, when one is opened, and on the right the field that finds instances (Ctrl+Shift+F).
- **The problems**, under it: one row for each interface an instance cannot do without and that is bound to nothing,
  with how to fix it. Double-click a row to show its instance.
- **The tabs**, on the right:
  - **Components**: the components of the plugins loaded, with a viewer of the one selected, its description and its
    properties. The search bar (Ctrl+F) keeps those whose name holds what is typed; the list next to it searches their
    **Description** or their **Category** instead. Drag a component into the edition zone to place it.
  - **Galaxies**: the project files of a directory, `scene/` next to NebulaStudio until **Choose directory...** gives
    another one. A galaxy is **Loadable**, misses some plugins (**Missing plugins**), or cannot be read
    (**Unreadable**); the viewer tells why. Drag one into the edition zone to place it, or **Open** it.
  - **Plugins**: the plugins found in `plugins/` and in each `--plugin-dir`, with their version and their state.
    NebulaStudio loads only those it needs, its own and those of the projects and galaxies it opens: **Load** the one
    selected to have its components in the **Components** tab, **Unload** it to take them away. **Open plugin...**
    loads one from anywhere on the disk.
- **The properties** of the instance selected, under the tabs, edited there. Selected alone, a settings file shows the
  values its file holds instead.
- **The overview**, under them: the whole scene, what the edition zone shows of it framed. Click or drag in it to move
  there. **View** > **Overview** hides it or shows it again.

Drag the borders between them to resize them. While a component or a galaxy is dragged out of its tab, the right side
hides, to give the edition zone its room, and comes back once it is dropped.

The **status bar** tells what was just done, refused or found, and the zoom level of the edition zone.

## Its menus

| Menu | Entries |
| --- | --- |
| **File** | **New** (Ctrl+N), **Open...** (Ctrl+O), **Save** (Ctrl+S), **Save As...** (Ctrl+Shift+S), **Save Galaxy** (Ctrl+Alt+S), **Description...** |
| **Edit** | **Undo** (Ctrl+Z), **Redo** (Ctrl+Shift+Z, or Ctrl+Y), **Cut** (Ctrl+X), **Copy** (Ctrl+C), **Paste** (Ctrl+V), **Duplicate** (Ctrl+D) |
| **View** | **Find Instances** (Ctrl+Shift+F), **Zoom In** (Ctrl++), **Zoom Out** (Ctrl+-), **Actual Size** (Ctrl+0), **Links** (the style of the links), **Theme**, **Overview** |
| **Run** | **Start** (F5), **Stop** (Shift+F5) |

Each area has a context menu as well: right-click the edition zone, an instance, the dot of an interface, a link, a
zone, a settings file or a stargate. [Keyboard and mouse](Keyboard-and-Mouse.md) lists every shortcut.

## Its theme and its preferences

**View** > **Theme** chooses **Light**, **Dark**, or **System**, which follows the system as it changes. NebulaStudio
keeps that choice, the style of the links (**View** > **Links**) and whether the overview is shown, for the user who
made them, in a JSON file: `FabienDG/NebulaStudio.json` in their configuration directory, `%APPDATA%` under Windows.

```json
{"appearance": {"colorScheme": "dark"}, "edition": {"linkStyle": "rightAngles", "overview": false}}
```

These are the choices of the person who uses NebulaStudio: none of them is saved in a project.

## NebulaStudio is a galaxy

NebulaStudio is itself assembled with Nebula: its window is the galaxy `scene/NebulaStudio.nbp`, made of the components
of `StudioPlugin`, each a part of the editor. Open it from the **Galaxies** tab to see how its parts are bound, or
**Start** it to run a second NebulaStudio inside the first. Saved there, a change is the NebulaStudio that starts next
time; in a build of Nebula, the next build gives the file the content of `StudioPlugin/NebulaStudio.nbp` back.
