# Lesson 1 · Get ready

*[The course](Course.md) › Lesson 1*

You install Nebula, start NebulaStudio, learn where things are in its window, and load a plugin. Nothing is built
yet: at the end of this lesson, NebulaStudio is ready for the next ones, and you know its window.

**You need**: a computer with Windows 10 or 11, 64-bit. Nothing else.

## Exercise 1.1 · Install Nebula

**Goal**: Nebula 3.0.0 in the folder `C:\Nebula`.

**Steps**

1. Open the page of the releases of Nebula: <https://github.com/Shuko83/Nebula/releases>.

   *You should see:* a list of versions, the latest first.

2. Under **Nebula 3.0.0**, open **Assets** and click `Nebula_3.0.0_win-x64_msvc2026_Qt6.12.zip`.

   *You should see:* your browser downloads the file, to your `Downloads` folder unless you chose another one.

   Take the `.zip`, not the `_setup.exe` next to it: the setup installs NebulaStudio alone, without the example
   plugins that the course uses.

3. In the File Explorer, right-click the downloaded zip, choose **Extract All...**, type `C:\` as the destination,
   then click **Extract**.

   *You should see:* a new folder, `C:\Nebula_3.0.0_win-x64_msvc2026_Qt6.12`.

4. Rename that folder to `Nebula`: right-click it, **Rename**, type `Nebula`, press Enter.

   *You should see:* the folder `C:\Nebula`, with these folders in it, among others:

   ```text
   C:\Nebula\
   ├── NebulaStudio\Release\bin\         NebulaStudio.exe, the editor
   ├── ExampleUniverseApp\Release\bin\   an application that runs what you assemble
   ├── Core\  Application\  Standard\    what your own plugins are built against, from lesson 5
   └── NebulaConfig.cmake
   ```

**Check**: the file `C:\Nebula\NebulaStudio\Release\bin\NebulaStudio.exe` exists.

**If it does not work**

- *The folder is `C:\Nebula\Nebula_3.0.0_...\NebulaStudio`, one level too deep*: you extracted into `C:\Nebula`. Move
  what is inside the inner folder up one level, so that `C:\Nebula\NebulaStudio` exists.
- *Windows refuses to write in `C:\`*: extract to another folder you can write to, `C:\Users\<you>\Nebula` say, and
  read that folder wherever the course says `C:\Nebula`.

## Exercise 1.2 · Start NebulaStudio

**Goal**: NebulaStudio open, and a shortcut to open it again.

**Steps**

1. In the File Explorer, open `C:\Nebula\NebulaStudio\Release\bin`.
2. Double-click `NebulaStudio.exe`.

   *You should see:* a small window with the logo of Nebula and its version, `3.0.0`, for two seconds, then the main
   window, titled **NebulaStudio**.

3. To find it again: right-click `NebulaStudio.exe`, then **Show more options** > **Send to** > **Desktop (create
   shortcut)**.

   *You should see:* a NebulaStudio icon on your desktop.

**If it does not work**

- *A blue box says "Windows protected your PC"*: the file comes from the Internet and is not signed. Click **More
  info**, then **Run anyway**.
- *A box says that a galaxy cannot be found or loaded*: the folder `scene` or `plugins` next to `NebulaStudio.exe` is
  missing. Extract the zip again, whole.

## Exercise 1.3 · Find your way in the window

**Goal**: know the name of each part of the window. The course uses these names in every lesson.

```text
┌─ File  Edit  View  Run ──────────────────────────────────────────────────────────────┐  menus
├──────────────────────────────────────────────────┬───────────────────────────────────┤
│                                 [Find instances] │ Components │ Galaxies │ Plugins   │  (2) the tabs
│                                                  │ ┌───────────────────────────────┐ │
│                                                  │ │ a list                        │ │
│              (1) the edition zone                │ ├───────────────────────────────┤ │
│                                                  │ │ the viewer: what is selected  │ │
│     Drag a component from the list of            │ └───────────────────────────────┘ │
│     components and drop it here...               ├───────────────────────────────────┤
│                                                  │ (3) the properties                │
├──────────────────────────────────────────────────┤     Property │ Value              │
│ (4) the problems                                 ├───────────────────────────────────┤
│     Component │ Problem │ How to fix it          │ (5) the overview                  │
├──────────────────────────────────────────────────┴───────────────────────────────────┤
│ (6) the status bar: what just happened                                     Zoom 100% │
└──────────────────────────────────────────────────────────────────────────────────────┘
```

**Steps**

1. Look at the large area on the left, with a grid: **the edition zone** (1). This is where an application is
   assembled. For now it only shows a text that starts with `Drag a component from the list of components`.
2. Look at the top right: three **tabs** (2), **Components**, **Galaxies** and **Plugins**. Click each in turn, then
   come back to **Components**.

   *You should see:* each tab shows a list, and under the list a **viewer**, which says `Select a component`,
   `Select a galaxy` or `Select a plugin` until a row of the list is clicked.

3. Under the tabs: **the properties** (3), a table with two columns, **Property** and **Value**. It is empty until
   something is selected in the edition zone.
4. Under the edition zone: **the problems** (4). It says `Every component of the application has the interfaces it
   needs`: nothing is wrong, since nothing is there.
5. Bottom right: **the overview** (5), a small picture of the whole edition zone.
6. At the very bottom: **the status bar** (6). NebulaStudio writes there what it just did, or why it refused. On its
   right, `Zoom 100%`.
7. At the very top: the menus, **File**, **Edit**, **View** and **Run**. Open each to read its entries, without
   choosing any.

**Check**: you can point at the edition zone, the tabs, the viewer, the properties, the problems and the status bar.

## Exercise 1.4 · Load a plugin

A **plugin** is a file that brings **components**, the building blocks of applications. NebulaStudio lists the plugins
it finds next to it, but only loads those it needs for itself. To use the components of another one, you load it.

**Goal**: the components of `WidgetsPlugin` in the **Components** tab.

**Steps**

1. Click the **Components** tab and look at the column **Plugin** of its list.

   *You should see:* components of `StandardPlugin` and `StudioPlugin` only. Those are the two plugins NebulaStudio
   itself is made of.

2. Click the **Plugins** tab.

   *You should see:* four rows, with three columns, **Name**, **Version** and **State**:

   | Name | State |
   | --- | --- |
   | `ExamplePlugin` | Not loaded |
   | `StandardPlugin` | Loaded |
   | `StudioPlugin` | Loaded |
   | `WidgetsPlugin` | Not loaded |

3. Click the row `WidgetsPlugin`.

   *You should see:* the viewer, under the list, shows what this plugin is: its name, its description, its file, and
   two buttons, **Load** and **Unload**.

4. Click **Load**.

   *You should see:* the **State** of `WidgetsPlugin` is now `Loaded`.

5. Click the **Components** tab again.

   *You should see:* five more components, whose **Plugin** is `WidgetsPlugin`: `ActionComponent`,
   `CounterComponent`, `LabelsComponent`, `MainWindowComponent` and `TopBarComponent`.

6. Click `CounterComponent` in the list.

   *You should see:* the viewer tells what it is, `Counts the times an action is triggered while it runs`, and lists
   its properties, `count` and `text`.

**Check**: the list of components has five rows whose **Plugin** is `WidgetsPlugin`.

**Good to know**: NebulaStudio forgets which plugins you loaded when it is closed. Each time you start it, load
`WidgetsPlugin` again, unless you open a project that uses it: opening a project loads the plugins it needs.

## Exercise 1.5 · Place an instance, and take it away

An **instance** is one component placed in the application. Here you only try the gesture; the next lesson assembles
something real.

**Steps**

1. In the **Components** tab, put the mouse on `ActionComponent`. Press the left button, keep it pressed, move the
   mouse over the edition zone, and release the button.

   *You should see:* while you move, the right side of the window hides to leave room. Once you release, it comes
   back, and a box is in the edition zone where you released: `ActionComponent1` in bold, `ActionComponent` under
   it, then a row `action` with a small disc on the right edge of the box.

2. Click the box.

   *You should see:* its border changes color, it is selected, and the properties, on the right, list `text`, whose
   value is `Action`, and `shortcut`.

3. Press the Delete key.

   *You should see:* the box is gone.

4. Open the menu **Edit** and choose **Undo**, or press Ctrl+Z.

   *You should see:* the box is back, and the status bar says what was undone.

5. Close NebulaStudio with the cross of its window. It asks `The application has been modified. Do you want to save
   it?`: click **Discard**.

**Check**: NebulaStudio is closed, and nothing was saved.

## Your turn

Start NebulaStudio again, load `ExamplePlugin`, and find the description and the category of the component it brings.

<details>
<summary>Solution</summary>

1. Double-click the NebulaStudio shortcut.
2. **Plugins** tab, click `ExamplePlugin`, click **Load**.
3. **Components** tab: one more row, `Greeter`, whose **Category** is `Example`.
4. Click it: the viewer gives its description, `Greets its recipient when it starts`.

</details>

## What you learned

- Nebula is a folder: nothing is installed in Windows, and removing the folder removes it.
- NebulaStudio lists plugins in its **Plugins** tab; **Load** makes their components available.
- Dragging a component to the edition zone places an instance; Delete removes it; Ctrl+Z undoes.

**Next**: [Lesson 2 · Assemble without code](Course-2-Assemble-Without-Code.md)
