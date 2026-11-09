# Assembling an application

Everything here happens in the edition zone of NebulaStudio, on the left of its window. The
[README](https://github.com/Shuko83/Nebula/blob/main/README.md#-nebulastudio) tells each behavior in full detail;
this page tells how to get things done.

## Place instances

- **Place** a component: drag it from the **Components** tab and drop it in the edition zone. It becomes an instance,
  named after its component and a number, `Greeter1`, `Greeter2`..., shown where it was dropped.
- **Select and move** instances with the mouse; drag a rectangle over the background to select several.
- **Rename** one: double-click its names, or **Rename component** in its context menu, type, then Enter (Escape gives
  up). A name is unique in the application, and not empty: a project and the application know an instance by it.
- **Color** one: **Change component color...** in its context menu, its opacity included; **Reset component color**
  gives back the colors of the theme.
- **Remove** instances: the Delete key removes those selected, **Remove component** the one under the cursor. Their
  links go with them. Nothing asks first: **Undo** (Ctrl+Z) gives them back.

## Link interfaces

Each instance lists its interfaces under its names, each with a dot on its edge:

| Dot | Means |
| --- | --- |
| A circle, on the left | An interface it consumes |
| Two stacked circles, on the left | A list it consumes, bound to several in an order |
| An asterisk after the name | It cannot do without this one |
| A disc, on the right | An interface it provides, available |
| A dotted circle, on the right | An interface it provides, not available yet |

Hover a dot: its tooltip tells the type, whether it is mandatory, and its state.

- **Draw a link**: drag from the dot of a provided interface to the dot of a consumed one of the same type, or the other
  way round. The dots it cannot be bound to are dimmed while it is drawn. A single interface loses the link it had; a
  list keeps its links, the new one after them.
- **Or choose it from a menu**: right-click the dot of a consumed interface, then **Bind to**: the interfaces of its
  type that other instances provide, `Ticker1.display`, those it is bound to checked. Choose a checked one to remove
  that link.
- **Remove a link**: click it to select it, then Delete, or **Remove link** in its context menu.
- **Read a link**: hover it; its tooltip tells what it binds, `Window1.content is bound to Ticker1.display`.

## Fix what does not work

An instance that misses an interface it cannot do without shows an exclamation mark in an amber disc, and that
interface in amber. The list of problems, under the edition zone, has a row for each, with how to fix it: which
instance provides an interface of its type, or that one has to be placed first. Double-click a row to show its
instance. The application starts all the same: the marks only tell.

## Set properties

Select an instance: its properties are on the right of the edition zone, those of its component first, then those of
the objects it is made of, in groups. Click a value to edit it. An enumeration is chosen in the list of its values,
flags are checked in theirs, a color is chosen in a dialog (double-click, F2 or Space), a URL is typed or chosen with
the button in its field.

- A property in *italics* is held by a [settings file](Settings-Files.md): its value here is the default one.
- A property with the icon of the Stargate is exposed by a stargate: see
  [galaxies and stargates](Galaxies-and-Stargates.md).
- Selected, a galaxy placed shows the properties it exposes, for this instance only.

## Lay it out

- **Zones** group instances by what they are for: **Add zone** in the context menu of the edition zone, then type its
  title. Drag a zone by its title to move it with what is in it; drag an edge or a corner to resize it. Its context
  menu renames it, colors it or removes it, without what is in it.
- **The edge and the row of a dot**: Shift+right-click a dot. **Dot on the left edge** and **Dot on the right edge**
  move it, its links following; **Swap with** swaps its row with another interface of the instance.
- **Anchors** have a link go through points: double-click a link, or **Add anchor point** in its menu, to add one,
  drag it to move it, double-click it to remove it. **Remove anchor points** removes those of a link.
- **Branches** bind several consumers to one provided interface along one line: Shift+drag from a link, or one of its
  anchors, to the dot of a consumed interface. Their links share the line up to that point, a junction drawn as a
  filled disc.
- **The style of the links**, for every link: **View** > **Links** > **Curves**, **Straight Lines** or **Right
  Angles**. It is a choice of yours, not of the project.

## Move around

- **Zoom**: + and -, or Ctrl+mouse wheel around the cursor, from a quarter to four times the size; **Actual Size**
  (Ctrl+0) goes back to its size.
- **Pan**: drag with the middle button held, wherever it is pressed.
- **The overview**, on the right: click or drag in it to show that part of the scene.
- **Find an instance**: Ctrl+Shift+F, then type part of its name, or of the name of its component. Choose one, or press
  Enter: it is selected and shown, opening the galaxy it is in when it is in one. Enter again shows the next one found.

None of these change the project.

## Undo and redo

**Undo** (Ctrl+Z) undoes the last edit, one step at a time; **Redo** (Ctrl+Shift+Z, or Ctrl+Y on Windows) does it
again. Each edit is a step: an instance placed, moved, renamed, colored or removed, a link drawn or removed, an anchor,
a zone, a settings file or a stargate changed, a property set. A drag is one step, so is what an entry of a menu, the
Delete key or a dialog does. The status bar tells what was undone, `Undone: Move Greeter1`.

- The values a settings file holds, the theme, the style of the links, the zoom and the overview are not edits: they
  are not undone.
- **New**, **Open...** and unloading a plugin forget the steps, as closing NebulaStudio does.
- Nothing is undone while the application runs. While a name or a field is typed, Ctrl+Z undoes what is typed there.

## Copy, paste and duplicate

**Copy** (Ctrl+C) copies the instances selected, with their properties, their colors, where they are, and the links
between them with their anchors. **Paste** (Ctrl+V) places them in what the edition zone shows, a little further right
and down; **Paste** in the context menu of the edition zone places them where it was opened. **Cut** (Ctrl+X) copies,
then removes what is selected; **Duplicate** (Ctrl+D) pastes a copy at once.

- An instance pasted keeps its name, unless another one has it there: `Counter2` becomes `Counter3`, `Display`
  becomes `Display2`.
- Not copied: the stargates, the settings files, the zones, and a link to an instance that is not copied.
- What is copied is in the clipboard of the system: another NebulaStudio pastes it as well.

## Run it

**Run** > **Start** (F5) runs the application in place, as it runs on its own: a triangle in the corner of each
instance tells it runs. **Run** > **Stop** (Shift+F5) stops it.

- While it runs, nothing is edited: instances can be selected and their properties read, but the context menu and
  the Delete key only tell that the application has to be stopped first. **New** and **Open...** are disabled.
- When an instance refuses to start, a message box tells why, and the application is edited again.
- Stopped, every property gets back the value it had when it started: running the application does not change it.

## Save it

**File** > **New**, **Open...**, **Save** and **Save As...** work on `.nbp` files, in `scene/` next to NebulaStudio
until another directory is chosen, or next to the project once it has a file. **Description...** gives the project a
few words on what it is for, shown in the **Galaxies** tab. NebulaStudio asks whether to save a project that changed
before it is replaced or NebulaStudio is closed.

A project opened in part, a plugin it needs being missing say, tells what could not be restored, then forgets its file:
**Save** then asks where to save it, rather than write over what is missing.

## Plugins

The **Plugins** tab loads and unloads plugins. Unloading one whose components are placed names those instances and
asks first: they are removed with it. A plugin file stays in use until NebulaStudio is closed: a plugin built again is
read by the next NebulaStudio.
