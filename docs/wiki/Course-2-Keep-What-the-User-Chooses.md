# Lesson 2 · Keep what the user chooses

*[The course](Course.md) › Lesson 2*

When an application stops, its properties get back the values of its project: the count of
[lesson 1](Course-1-Assemble-Without-Code.md) is back to 0 each time. A settings file keeps, from one run to the next,
the values of the properties it holds: what the user of the application chose, or what it did.

**You learn to** add a settings file, choose the properties it holds, see and change the values it keeps, and start
again from those of the project.

## What you need to know

- **Add settings file**, in the context menu of the edition zone, adds one and opens its dialog: its name, its
  directory, and the properties it holds, checked under their instances.
- When the application starts, the file gives these properties the values it has; while it runs, what changes them is
  written to it; when it stops, the properties get back the values of the project.
- Selected alone, a settings file shows on the right of the edition zone the values its file holds.

[Settings files](Settings-Files.md) tells the rest.

Start from `Counters.nbp`, saved at the end of lesson 1: **File** > **Open...**.

## Exercise 2.1 · A count that survives

**Goal**: the count of `CounterComponent1` goes on from where it was, the next time the application runs.

**Do**: add a settings file named `counters.json` that holds the `count` of `CounterComponent1`.

**Check**: **Start**, click three times, **Stop**: the properties of `CounterComponent1` show a `count` of 0 again.
Select the settings file: its value of `count` is 3. **Start** again: the window shows 3 at once, and goes on from
there.

<details>
<summary>Hint</summary>

Right-click an empty place of the edition zone. In the dialog, the properties are listed under each instance: expand
`CounterComponent1` and check `count`.

</details>

<details>
<summary>Solution</summary>

**Add settings file**, name `counters.json`, directory left empty, `CounterComponent1` > `count` checked, **OK**. A
dashed line joins the file to `CounterComponent1`, and `count` is in italics in its properties: a file holds it, and
its value there is the default one.

`CounterComponent` notifies its `count` as it changes (`NOTIFY`): that is what has the file write it while the
application runs.

</details>

## Exercise 2.2 · Play the user

**Goal**: change the value a user has, not the one of the project.

**Do**: select the settings file, and set the `count` it shows to 100. **Start**, **Stop**. Then look at the properties
of `CounterComponent1`, and at what **File** > **Save** would write.

**Check**: the window starts at 100. The `count` of the project stays 0: the file is what a user has, not the project.

<details>
<summary>Solution</summary>

A value edited in the inspector of the file is written to the file at once, as if the user had chosen it. The
project does not change: NebulaStudio does not even ask to save it.

</details>

## Exercise 2.3 · Start over

**Goal**: give the file the values of the project again, to try from the start, or to make the file you deliver.

**Do**: in the context menu of the settings file, **Write default values**.

**Check**: selected, the file shows a `count` of 0; **Start** starts at 0.

## Exercise 2.4 · Find the file

**Goal**: know where the file is, and what is in it.

**Do**: select the settings file and read the path above its values. Open it in a text editor, or with **View and edit
file...** in its context menu.

**Check**: it is a JSON object, the values under the name of the instance: `{"CounterComponent1": {"properties":
{"count": 0}}}`.

<details>
<summary>Solution</summary>

With its directory left empty, the file is in the configuration directory of the application that runs it: run from
NebulaStudio, the one of NebulaStudio, under `%LOCALAPPDATA%`. Your users have theirs, in the directory of your
application: what you try does not change what they have.

</details>

## Questions

<details>
<summary>1. You rename CounterComponent1 to Clicks. What happens to the count the user had?</summary>

It stays in the file under `CounterComponent1`, and `Clicks` starts from the value of the project: a file keeps the
values under the names of the instances.

</details>

<details>
<summary>2. Why not hold the count of LabelsComponent1 in a settings file?</summary>

That `count` makes interfaces: changed while the application runs, the labels it removes lose their links, which
stopping the application does not bring back.

</details>

<details>
<summary>3. Can a property be held by two settings files?</summary>

No: one that another file holds is listed with its name in the dialog, and cannot be checked.

</details>

**Next**: [Lesson 3 · Reuse a part](Course-3-Reuse-a-Part.md)
