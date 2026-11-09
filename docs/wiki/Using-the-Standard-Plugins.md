# Using the standard plugins

Nebula ships `StandardPlugin`, whose components serve any application: a **log** and a **history** of what can be
undone. It is in `plugins/` next to NebulaStudio and each application of the package. Your components use them through
interfaces: they consume them, and whoever assembles the application binds them to an instance of the standard
component, or leaves them unbound.

The [README](https://github.com/Shuko83/Nebula/blob/main/README.md#-standard-plugins) details each interface and
property.

## Link the interfaces

The interfaces are declared in `Nebula::Standard`, a library of headers only. Ask for it with Core, and link it to your
plugin:

```cmake
find_package(Nebula 3.0 REQUIRED COMPONENTS Core Standard)
```

```cmake
nebula_add_plugin(
    NAME TimePlugin
    DESCRIPTION "A clock, and a window to show it"
    LINK_LIBRARIES PRIVATE Nebula::Standard Qt6::Widgets
)
```

Your plugin does not link `StandardPlugin` itself: a plugin cannot be linked, and does not need to be. The application
loads both, and the binding of their instances hands your component the pointer.

## Write to the log

`LogComponent`, in the category Diagnostics, provides `log`, a `Nebula::ILog`. Consume it as any interface, optional
so that your component works without it, and write to it while the member is set. Here, the clock of
[your first plugin](Your-First-Plugin.md) tells how it shows the time when it starts:

```cpp
// ClockComponent.h
#include "Nebula/Standard/ILog.h"
// ...
    Nebula::ILog* m_log = nullptr; // Consumed
```

```cpp
// ClockComponent.cpp, in the constructor
consumeInterface<Nebula::ILog>(QStringLiteral("log"), m_log); // Optional: it works without a log

// In onStart()
if(m_log != nullptr)
{
    m_log->info(QStringLiteral("Showing the time as %1").arg(m_format), objectName());
}
```

`objectName()` is the name of the instance, `Clock1`: the message is written after it, `Clock1: Showing the time as
hh:mm`, under the category of the log, `log` by default. `debug()`, `info()`, `warning()` and `critical()` are the four
levels; `isWritten(level)` tells whether a message would be written, so that one that would not is not built.

In NebulaStudio, place a `LogComponent`, `LogComponent1`, and bind its `log` to the `log` of the clock. Its properties:

| Property | At first | What it does |
| --- | --- | --- |
| `logging` | `true` | Whether messages are written at all |
| `level` | `debug` | The least severe level written: `debug`, `info`, `warning` or `critical` |
| `category` | `log` | The category the messages written through `log` are in |

**Its level is the level of the whole process.** While it runs, it filters every message of the application, those of
every plugin, of Nebula and of Qt, not only those written through `log`. Hold `level` in a
[settings file](Settings-Files.md), and the user of the application keeps the level they choose. Another component
that consumes `logControl`, a `Nebula::ILogControl`, can start and stop the log, and set its level, while the
application runs.

The messages go where the application writes its own. For an application run by `NebulaApplication`, that is the
error output with `--console`, and a file with `--log-file`: see
[running an application](Running-an-Application.md#see-its-messages). Started from NebulaStudio, the application runs in
the process of NebulaStudio: start NebulaStudio with `--console` to see them, and know that the log filters the
messages of NebulaStudio too.

## Make actions undoable

`HistoryComponent`, in the category Edition, provides `history`, a `Nebula::IHistory`. A component that consumes it
pushes each action the user can undo as a `Nebula::Command` rather than doing it at once: `push()` executes it, then
keeps it. NebulaStudio itself keeps its edits that way.

A command knows how to do the action and how to undo it, and is executed and undone as many times as the user asks,
each time from the state the other left. Here, a component that holds notes the user adds:

```cpp
// NotesComponent.cpp
#include <QPointer>

#include "Nebula/Standard/Command.h"

namespace
{
// Adds a note to a NotesComponent, and takes it away again.
class AddNote : public Nebula::Command
{
  public:
    AddNote(NotesComponent& notes, QString note)
        : Command(QStringLiteral("Add note")), m_notes(&notes), m_note(std::move(note))
    {
    }

    void execute() override { m_notes->append(m_note); }
    void undo() override { m_notes->removeLast(); }
    [[nodiscard]] bool isObsolete() const override { return m_notes.isNull(); } // Its instance was removed

  private:
    QPointer<NotesComponent> m_notes;
    QString m_note;
};
} // namespace

NotesComponent::NotesComponent()
{
    consumeInterface<Nebula::IHistory>(QStringLiteral("history"), m_history); // Nebula::IHistory* m_history
}

void NotesComponent::addNote(const QString& note) // What its window calls as the user adds one
{
    if(m_history != nullptr)
    {
        m_history->push(std::make_unique<AddNote>(*this, note)); // Executes it, then keeps it to undo
    }
    else
    {
        append(note); // Without a history, nothing is undone
    }
}
```

- **Find what a command acts on again.** It is undone and executed long after it was made: hold what does not change
  in between, a name or a `QPointer`, rather than a raw pointer to something that may be gone.
- **Tell when it is obsolete.** `isObsolete()` returns `true` once what it acts on is gone: the history drops it rather
  than undo it.
- **One step of several actions** is a `Nebula::CompositeCommand`: `add()` its commands; it executes them in their
  order and undoes them the other way round.
- **Something has to call `undo()` and `redo()`**: a menu or shortcuts of one of your components, which consumes
  `history` as well, and enables its entries with `isUndoActive()` and `isRedoActive()`.
- `clear()` forgets everything, when what the commands act on is replaced: a document opened, say.

Its properties: `maxSize`, how many commands it keeps (100), and `undoActive` and `redoActive`, which turn `undo()` and
`redo()` off. Once the application stops, it forgets what was done while it ran.

## Where to go next

- [Settings files](Settings-Files.md), to keep the level of the log the user chooses.
- [Writing components](Writing-Components.md), for the rest of what a component does.
