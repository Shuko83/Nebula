# Troubleshooting

Most problems tell themselves in a message: start NebulaStudio, or your application, with `--console` to read them in
the terminal, or `--log-file <file>` to keep them.

## A plugin is not listed

NebulaStudio and the applications list the `.nbdl` files of `plugins/` next to them, and of each `--plugin-dir`, and
skip, with a warning, those they cannot load:

| Warning | Means | Fix |
| --- | --- | --- |
| `Built against Core 2.1.0 instead of 3.0.0` | The plugin was built against another major version of Nebula, or a newer one: a plugin built for 2.x, say | Build the plugin against the Nebula that runs it |
| `Built against another plugin interface` | Not a plugin of this Nebula | Build it with `nebula_add_plugin()` of this Nebula |
| `No id or name in its metadata` | Its metadata was not written by `nebula_add_plugin()` | Build it with `nebula_add_plugin()` |
| A message of Qt | The file is not a plugin Qt can load: a library of another compiler, or not a library | Build it with the compiler and the Qt of the Nebula that runs it |

A plugin whose id is already listed is skipped too: two copies of a plugin in two directories, the first one wins. A
plugin in another folder is only listed with `--plugin-dir`, or opened with **Open plugin...**.

A plugin built in `Debug` is not loaded by a `Release` application, nor the other way round: build it in the
configuration of the Nebula that runs it, `Release` for a release.

## A component is not in the Components tab

The tab shows the components of the plugins loaded. NebulaStudio lists every plugin it finds in the **Plugins** tab,
but loads only those it needs: select the plugin there and click **Load**. Opening a project, or placing a galaxy,
loads the plugins it needs.

## NebulaStudio crashes when an instance is placed

The plugin was built against other headers of Nebula than those of the NebulaStudio that runs it: a minor version of
Nebula can change the classes a component derives from. Build the plugin again against this Nebula.

## A plugin built again is not the one NebulaStudio runs

A plugin loaded stays in memory until NebulaStudio is closed, even unloaded: loading it again does not read its file
again, and its file cannot be replaced meanwhile, which fails the build. Close NebulaStudio, build, start it again.

## A galaxy cannot be placed

In the **Galaxies** tab, a galaxy is:

- **Missing plugins**: NebulaStudio knows none of its plugins with that id. Open them with **Open plugin...**, or start
  NebulaStudio with their directory in `--plugin-dir`. Only the id counts here, not the file the project names.
- **Unreadable**: its file is not a project, or not of this version. The viewer tells why.

A galaxy that would hold itself, directly or through another one, or the project it is dropped into, is refused, which
the status bar says.

## An instance is marked in amber

An exclamation mark in an amber disc: the instance misses an interface it cannot do without. The list of problems, under
the edition zone, names it and the instances that provide one of its type. Bind it: the mark goes. The application
starts all the same, but that instance probably does nothing.

## A link cannot be drawn

- The two interfaces have different types: their dots tell their types, and those that cannot be bound are dimmed
  while the link is drawn.
- Both are consumed, or both provided: a link goes from one an instance provides to one another consumes.
- The application runs: stop it first.
- A list is already bound to that interface: a list is bound once to each.

## A property is not in the properties panel, or is not saved

- It is read only: it has no `WRITE`.
- Its type has no editor: listed in the viewer of its component, but not edited. See
  [writing components](Writing-Components.md#properties) for the types that are.
- It is declared `DESIGNABLE false` or `STORED false`.
- Its value cannot be written to JSON and read back: a null date, a color that is not valid. It is saved once it has a
  value that can.
- Flags declared with `Q_ENUM` as well as `Q_FLAG`: a value made of several is not saved. Declare them with `Q_FLAG`
  alone.

## Nothing can be edited

The application runs: a triangle is in the corner of each instance, and the status bar says the application has to be
stopped first. **Run** > **Stop** (Shift+F5).

## Save asks where to save an opened project

The project was opened in part: what could not be restored, a plugin missing say, was told in a box, and NebulaStudio
forgot the file so that **Save** does not write over what is missing. Make the plugin known and open the project again,
or save it elsewhere.

## The values of the user are gone

- The instance was renamed: a settings file keeps the values under the name of the instance, and the instance starts
  again from those of the project. Renaming it back finds them.
- The file could not be read: it was set aside with `.bak` after its name, and written again with the values of the
  project. Repair it, or take the values back from it.
- It was run from NebulaStudio: a file without a directory is then written to the configuration directory of
  NebulaStudio, not to the one of the application delivered.

## An application ends at once

`exec()` of `NebulaApplication` returns `1` when the galaxy cannot be loaded, `2` when an instance refuses to start.
Start it with `--console` to read why:

- `The galaxy Clock.nbp is in none of C:\...\scene`: the galaxy is not in `scene/` next to the executable, nor in a
  `--galaxy-dir`.
- `The galaxy ... cannot be loaded:` followed by what could not be restored: a plugin not in `plugins/`, a component
  its plugin no longer has, a binding of interfaces that are gone. Open the project in NebulaStudio to see it.
- A DLL that Windows cannot find, before any message: Nebula, Qt or the runtime of the compiler is missing next to the
  executable. See [deploying an application](Deploying-an-Application.md).
- `Could not find the Qt platform plugin`: `platforms/` is missing next to the executable; `windeployqt` copies it.

## An application shows no messages

On Windows, an application without a console sends its messages nowhere you can see. `--console` writes them to the
terminal it is started from, `--log-file` to a file; `--log-level debug` writes them all. A running `LogComponent`
filters every message of the process at its own `level`.

## Something else

Search the [issues](https://github.com/Shuko83/Nebula/issues), or open one with what you did, what you expected, and the
messages of `--console`.
