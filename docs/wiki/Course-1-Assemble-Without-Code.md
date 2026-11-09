# Lesson 1 · Assemble without code

*[The course](Course.md) › Lesson 1*

You assemble applications in NebulaStudio from the components of `WidgetsPlugin`, which comes with Nebula: a main
window, the bar at its top, actions, counters and labels. No code yet: what you learn here is what your own components
will have, properties and interfaces.

**You learn to** place instances, link their interfaces, set their properties, find what does not work, run an
application and save it.

## What you need to know

- In the **Plugins** tab, select `WidgetsPlugin` and click **Load**: its components join the **Components** tab.
- Drag a component into the edition zone to place an instance of it. Its interfaces are dots on its edges: what it
  consumes on the left, what it provides on the right.
- Drag from a dot on the right to a dot on the left to bind them. They must have the same type.
- **Run** > **Start** (F5) runs the application; **Run** > **Stop** (Shift+F5) stops it, and every property gets the
  value it had before again.

[Getting started](Getting-Started.md#assemble-a-first-application) walks through the first exercise step by step, and
[assembling an application](Assembling-an-Application.md) tells all the rest.

## The components of WidgetsPlugin

| Component | Provides | Consumes | Properties |
| --- | --- | --- | --- |
| `MainWindowComponent` | `mainWindow`, its window | `centralWidget`, the widget in its middle | `title`, `width`, `height` |
| `TopBarComponent` | | `mainWindow`, which it cannot do without; the lists `actions` and `widgets` | `title`, `movable` |
| `ActionComponent` | `action`, a `QAction` | | `text`, `shortcut` |
| `CounterComponent` | `display`, a label of its count | `action`, which it cannot do without | `count`, `text` (`%1` stands for the count) |
| `LabelsComponent` | `label1`, `label2`... as many as its `count` | | `count`, and the `text` of each label |

## Exercise 1.1 · Count the clicks

**Goal**: a window whose bar has a button, `Count`, and which shows how many times it was clicked.

**Do**: place a `MainWindowComponent`, a `TopBarComponent`, an `ActionComponent` and a `CounterComponent`, and bind:
the window to the bar, the action to the bar and to the counter, the display of the counter to the middle of the
window. Give the action the text `Count`.

**Check**: no instance shows an amber mark, and the list of problems says every instance has what it needs. **Start**:
each click on `Count` adds one to the count in the window. **Stop**: the window closes.

<details>
<summary>Hint</summary>

Four links: `mainWindow` → `mainWindow`, `action` → `actions` (the two stacked circles: a list), `action` → `action`,
`display` → `centralWidget`. The `text` of `ActionComponent1` is in its properties, on the right of the edition zone,
once it is selected.

</details>

<details>
<summary>Solution</summary>

| From (provides) | To (consumes) |
| --- | --- |
| `MainWindowComponent1.mainWindow` | `TopBarComponent1.mainWindow` |
| `ActionComponent1.action` | `TopBarComponent1.actions` |
| `ActionComponent1.action` | `CounterComponent1.action` |
| `CounterComponent1.display` | `MainWindowComponent1.centralWidget` |

Then `ActionComponent1.text` = `Count`. The same steps, one by one, are in
[getting started](Getting-Started.md#assemble-a-first-application).

</details>

## Exercise 1.2 · One button, two counters

**Goal**: the same button counts in two places, the middle of the window and the bar.

**Do**: add a second `CounterComponent`, whose `text` is `Also %1`, bound to the same action. Show its display in the
bar, after the button.

**Check**: **Start**: one click adds one to both counts.

<details>
<summary>Hint</summary>

An interface can be provided to several consumers: `ActionComponent1.action` is already bound to the bar and to the
first counter, bind it to the second one as well. The bar shows the widgets of its list `widgets` after its actions.

</details>

<details>
<summary>Solution</summary>

Two more links: `ActionComponent1.action` → `CounterComponent2.action`, and `CounterComponent2.display` →
`TopBarComponent1.widgets`. `CounterComponent2.text` = `Also %1`.

A widget is shown in one place at a time: the display of `CounterComponent1` stays in the middle of the window, the
one of `CounterComponent2` goes to the bar.

</details>

## Exercise 1.3 · Break it, fix it

**Goal**: see what NebulaStudio tells of an instance that does not work, and fix it without drawing.

**Do**: click the link between `ActionComponent1.action` and `CounterComponent2.action` to select it, then press
Delete. Read what NebulaStudio says. **Start**, click, **Stop**. Then bind it again from a menu, not by dragging.

**Check**: while the link is missing, `CounterComponent2` has an exclamation mark in an amber disc, and the list of
problems, under the edition zone, has a row for it. The application starts all the same, and the second count stays at
0. Bound again, the mark goes.

<details>
<summary>Hint</summary>

Right-click the dot of the interface the counter consumes: its menu binds it to what provides its type.

</details>

<details>
<summary>Solution</summary>

Right-click the `action` dot of `CounterComponent2`, then **Bind to** > `ActionComponent1.action`. Ctrl+Z, after the
Delete, would have given the link back as well: every edit is a step of the history.

A mandatory interface only tells: nothing prevents the application from starting without it. The amber mark and the
list of problems are there for whoever assembles it.

</details>

## Exercise 1.4 · Labels that follow a count

**Goal**: put three labels in the bar, then see what a component that makes interfaces from a property does.

**Do**: place a `LabelsComponent` and set its `count` to 3. Bind `label1`, `label2` and `label3` to the `widgets` of the
bar. Give each label a text: they are in the groups `label1`, `label2` and `label3` of its properties. Then set its
`count` to 1.

**Check**: with a `count` of 3, the instance has three provided interfaces, and **Start** shows the three texts in the
bar. With a `count` of 1, `label2` and `label3` are gone, with their links. Ctrl+Z gives them back.

<details>
<summary>Solution</summary>

The links: `LabelsComponent1.label1`, `label2` and `label3` → `TopBarComponent1.widgets`. The bar shows them in the
order of their bindings, after the actions and the display of `CounterComponent2`.

What a component provides can depend on one of its properties. That is why a project gives each component its
properties before its links: the `count` comes first, then the links to the labels it made.

</details>

## Exercise 1.5 · Run it on its own

**Goal**: run what you assembled without NebulaStudio.

**Do**: **File** > **Save As...**, `Counters.nbp`, in the `scene/` folder NebulaStudio proposes. Then, from a terminal,
run it with `ExampleUniverseApp`, from `ExampleUniverseApp/Release/bin/` of the zip.

**Check**: the window opens and counts; closing it ends the application.

<details>
<summary>Solution</summary>

```bash
ExampleUniverseApp <the scene folder of NebulaStudio>/Counters.nbp
```

The project names its plugins by their id: `ExampleUniverseApp` finds `WidgetsPlugin` in its own `plugins/`. Open
`Counters.nbp` in a text editor: the instances, their properties, the links (`bindings`), and where each instance is
in the edition zone.

</details>

## Questions

<details>
<summary>1. You clicked five times, then stopped. Why does CounterComponent1 show a count of 0 in its properties?</summary>

Stopping gives every property the value it had when the application started: running an application does not change
its project. [Lesson 2](Course-2-Keep-What-the-User-Chooses.md) keeps the count anyway.

</details>

<details>
<summary>2. Why can you not bind CounterComponent1.display to TopBarComponent1.actions?</summary>

`display` is a `QWidget`, `actions` a list of `QAction`: a link binds interfaces of the same type. While you drag, the
dots of another type are dimmed.

</details>

<details>
<summary>3. What tells you that an interface is a list?</summary>

Its dot is two stacked circles. It takes several links, kept in the order they were made.

</details>

**Next**: [Lesson 2 · Keep what the user chooses](Course-2-Keep-What-the-User-Chooses.md)
