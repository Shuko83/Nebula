<img src="https://raw.githubusercontent.com/Shuko83/Nebula/main/NebulaStudio/resources/nebula.svg" width="96" alt="The logo of Nebula">

**Nebula** is a C++ and Qt 6 framework for building applications out of components that plugins provide. You write
components in C++; you, or someone who never writes a line of it, assembles them in **NebulaStudio**, its editor, saves
the assembly as a project file, and runs it as an application.

This wiki walks you through it, one task at a time. The
[README](https://github.com/Shuko83/Nebula/blob/main/README.md) is the reference: each function, option and key of a
file is detailed there, and the wiki links to it rather than repeating it.

## Start here

- [Getting started](Getting-Started.md): get Nebula, then assemble and run a first application in NebulaStudio, without
  writing code.
- [Concepts](Concepts.md): plugins, components, interfaces, bindings, projects, galaxies, stargates and settings files,
  and how they fit together.
- [The course](Course.md): ten lessons that start from nothing, one explicit step at a time, from installing Nebula
  to a plugin of your own. New to Nebula? Start there.

## I write plugins

- [Your first plugin](Your-First-Plugin.md): a plugin of two components, built outside Nebula, assembled and run in
  NebulaStudio, step by step.
- [Writing components](Writing-Components.md): properties, the two modes, the interfaces a component provides and
  consumes, the objects it is made of, and what to avoid.
- [Using the standard plugins](Using-the-Standard-Plugins.md): write to the log of the application, and make what your
  component does undoable.

## I assemble applications in NebulaStudio

- [A tour of NebulaStudio](NebulaStudio-Tour.md): its window, its menus, its lists and where it keeps your choices.
- [Assembling an application](Assembling-an-Application.md): instances, links, zones, undo, copy and paste, running and
  saving.
- [Galaxies and stargates](Galaxies-and-Stargates.md): make a part of an application once, then place it as many times
  as needed.
- [Settings files](Settings-Files.md): keep what the user of an application chooses from one run to the next.
- [Keyboard and mouse](Keyboard-and-Mouse.md): every shortcut and gesture of the editor.

## I ship applications

- [Running an application](Running-an-Application.md): the `main` of an executable whose content is a project, its
  options and its messages.
- [Deploying an application](Deploying-an-Application.md): the folder to ship, and what has to be in it.

## When something goes wrong

- [Troubleshooting](Troubleshooting.md): a plugin that is not listed, an instance marked in amber, a property that is
  not saved, an application that ends at once...

## About this wiki

It describes Nebula as it is on `main`, as the README does: what is not released yet is listed under
**Unreleased** in the [changelog](https://github.com/Shuko83/Nebula/blob/main/CHANGELOG.md), and the
[releases](https://github.com/Shuko83/Nebula/releases) tell what each version brings. Its pages live in
[`docs/wiki/`](https://github.com/Shuko83/Nebula/tree/main/docs/wiki) of the repository, and are copied here each time
they change on `main`: to correct one, open a pull request there.

Nebula is licensed under the [Mozilla Public License 2.0](https://github.com/Shuko83/Nebula/blob/main/LICENSE): a
plugin or an application built on it keeps the license its author chooses.
