# Lesson 3 · Reuse a part

*[The course](Course.md) › Lesson 3*

A button and its counter make a part you may want several times, each with its own label and its own count. A
galaxy is that part, made once in a project file, and placed in another one as a single instance, as many times as
needed.

**You learn to** make a galaxy, expose with a stargate what the outside needs, place it several times, give each
instance its own values, and change all of them at once.

## What you need to know

- Any project file is a galaxy, named after its file. The **Galaxies** tab lists those of `scene/`.
- A stargate, **Add stargate** in the context menu of the edition zone, exposes interfaces and properties of the
  instances of its galaxy: once placed, the galaxy has those and nothing else.
- Double-click a galaxy placed, below its name, to edit what is inside; **Save Galaxy** (Ctrl+Alt+S) writes it to its
  file and reads every instance of it again.

[Galaxies and stargates](Galaxies-and-Stargates.md) tells the rest.

## Exercise 3.1 · Make the part

**Goal**: a galaxy `ClickCounter.nbp`, a button and the count of its clicks, that exposes the button, the count and the
text of the button.

**Do**: **File** > **New**. Place an `ActionComponent` and a `CounterComponent`, and bind the action to the counter.
Add a stargate that exposes the `action` of the button, the `display` of the counter, and the `text` of the button
under the name `label`. Give the project a description, then save it as `ClickCounter.nbp` in `scene/`.

**Check**: the stargate shows `action` and `display` on its edge, bound to their instances, and `label` under them.
`ClickCounter` is listed in the **Galaxies** tab, **Loadable**, with its description.

<details>
<summary>Hint</summary>

In the dialog of the stargate, check the interfaces and the property under their instances. Double-click a row to edit
the name it is exposed as.

</details>

<details>
<summary>Solution</summary>

**Add stargate**, then in its dialog: `ActionComponent1` > `action` (provided), `CounterComponent1` > `display`
(provided), `ActionComponent1` > `text`, renamed `label`. **File** > **Description...**: `A button and the count of its
clicks`. **File** > **Save As...**: `ClickCounter.nbp`.

The interfaces are provided by instances inside: once placed, the galaxy provides them, on the right of its item.

</details>

## Exercise 3.2 · Place it twice

**Goal**: a window whose bar has two buttons, each with its own count.

**Do**: **File** > **New**. Place a `MainWindowComponent` and a `TopBarComponent`, bound. Drag `ClickCounter` from the
**Galaxies** tab twice. Bind the two buttons and the two counts to the bar. Give the first `label` the value `First`,
the second `Second`.

**Check**: **Start**: two buttons, `First` and `Second`, each counting its own clicks. Save it as `TwoCounters.nbp`.

<details>
<summary>Solution</summary>

`ClickCounter1.action` and `ClickCounter2.action` → `TopBarComponent1.actions`; `ClickCounter1.display` and
`ClickCounter2.display` → `TopBarComponent1.widgets`. Select `ClickCounter1`: its only property is `label`, the one its
stargate exposes, set for this instance only.

</details>

## Exercise 3.3 · Each its own count

**Goal**: each counter keeps its count from one run to the next.

**Do**: open `ClickCounter1` (double-click it, below its name), add a settings file in the galaxy that holds the `count`
of its counter, then **Save Galaxy**. Back in the application, **Start**, click each button a different number of times,
**Stop**, **Start** again.

**Check**: each count goes on from its own value. Open each galaxy and select its settings file: its path names the
instance, `ClickCounter1.settings.json` and `ClickCounter2.settings.json`.

<details>
<summary>Solution</summary>

A settings file added in a galaxy belongs to each of its instances, named after it: each instance keeps the values of
its own users. **Save Galaxy** has `ClickCounter2`, read again from the file, get the settings file too.

</details>

## Exercise 3.4 · Change it once

**Goal**: change all the instances by changing the galaxy.

**Do**: open `ClickCounter1`, set the `text` of its counter to `%1 clicks`, then **Save Galaxy**.

**Check**: both counts show `%1 clicks`, while each keeps its `label`.

<details>
<summary>Solution</summary>

**Save Galaxy** writes the file, then reads every instance of it again: each keeps its name, the values of what it
exposes, and its links. The `text` of the counter is not exposed: it is the one of the file, for every instance.

</details>

## Questions

<details>
<summary>1. Why must the stargate expose action and display for the window to use them?</summary>

Placed, a galaxy has what its stargates expose, and nothing else: the instances inside are its own business.

</details>

<details>
<summary>2. Can you drop ClickCounter into ClickCounter?</summary>

No: a galaxy cannot hold itself, directly or through another one. NebulaStudio refuses it, and the status bar says so.

</details>

<details>
<summary>3. You change a property inside ClickCounter1 and leave it without saving. What happens?</summary>

NebulaStudio asks: **Save** writes the galaxy and reads its instances again, **Discard** reads it again from its file,
**Cancel** stays in it.

</details>

**Next**: [Lesson 4 · Properties and modes](Course-4-Properties-and-Modes.md), the first one with code.
