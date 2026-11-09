# Lesson 6 · Log and history

*[The course](Course.md) › Lesson 6*

Two things almost every application needs come with Nebula, in `StandardPlugin`: a log, and a history of what can be
undone. Your components use them through interfaces, as they use each other's.

**You learn to** consume the interfaces of another plugin, write to the log, and make an action of the user undoable.

## What you need to know

- The interfaces are in `Nebula::Standard`: ask for it with `find_package(Nebula 3.0 REQUIRED COMPONENTS Core Standard)`
  and link it to the plugin, `LINK_LIBRARIES PRIVATE Nebula::Standard Qt6::Widgets`.
- `LogComponent` provides `log`, a `Nebula::ILog`: `info()`, `warning()`... with the name of the instance as source.
- `HistoryComponent` provides `history`, a `Nebula::IHistory`: `push()` a `Nebula::Command`, which it executes, then
  `undo()` and `redo()` it.
- NebulaStudio writes its messages to the terminal when started with `--console`.

[Using the standard plugins](Using-the-Standard-Plugins.md) tells the rest.

## Exercise 6.1 · Tell why

**Goal**: the clock refuses to start with an empty format, and only the name of the instance tells it. Have it write
why to the log, as a warning; have the alarm write a warning when it rings.

**Do**: both consume `log`, optionally. Place a `LogComponent`, bind its `log` to theirs, and start NebulaStudio from a
terminal with `--console`.

**Check**: empty the format of the clock and **Start**: the terminal shows `log: ClockComponent1: An empty format shows
nothing`. With a fixed time on the alarm: `log: AlarmComponent1: Ringing at 07:30`. Set the `level` of the log to
`critical`: neither is written any more.

<details>
<summary>Solution</summary>

```cpp
// ClockComponent.h and AlarmComponent.h
#include "Nebula/Standard/ILog.h"
// ...
    Nebula::ILog* m_log = nullptr; // Consumed
```

```cpp
// In both constructors
consumeInterface<Nebula::ILog>(QStringLiteral("log"), m_log);
```

```cpp
// ClockComponent::onStart()
if(m_format.isEmpty())
{
    if(m_log != nullptr)
    {
        m_log->warning(QStringLiteral("An empty format shows nothing"), objectName());
    }
    return false; // Refused: the application stops what it started
}
```

```cpp
// AlarmComponent::ring()
m_rung = true;
m_display.setText(QStringLiteral("Ringing!"));
if(m_log != nullptr)
{
    m_log->warning(QStringLiteral("Ringing at %1").arg(m_alarm.toString(QStringLiteral("hh:mm"))), objectName());
}
```

The log is optional: without one, the components work as before. Its level is the one of the whole process: set to
`critical`, it silences the warnings of every plugin, of Nebula and of Qt too.

</details>

## Exercise 6.2 · Undo a click

**Goal**: a `TallyComponent`, a count with a `+1` button, provided as `display`, and `Undo` and `Redo` buttons that go
back and forth through the clicks. The count is a property, `count`. Without a history, `+1` still counts, and `Undo`
and `Redo` are disabled.

**Do**: write it with a `Nebula::Command` that sets the count from one value to another, and back. Place it with a
`HistoryComponent`, bind their `history`, and show its display in a window.

**Check**: **Start**: click `+1` three times, `Undo` twice, `Redo` once: the count is 2. **Stop**, remove the link of
the history, **Start** again: `+1` still counts, and `Undo` and `Redo` are disabled.

<details>
<summary>Hint</summary>

A command is executed once as it is pushed, then undone and executed again from what the other left: keep the value
before and the value after. Hold the tally by a `QPointer`, so that the command knows when its instance is gone. The
callbacks of `consumeInterface()` enable and disable the buttons.

</details>

<details>
<summary>Solution</summary>

```cpp
// TallyComponent.h
#pragma once

#include <QLabel>
#include <QPushButton>
#include <QWidget>

#include "Nebula/Component/Component.h"
#include "Nebula/Standard/IHistory.h"

// A count the user raises with a button, in its "display"; each click is a step of the history it is given.
class TallyComponent : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "A count raised by a button, undone and redone with a history")
    Q_CLASSINFO("category", "Time")
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)

  public:
    TallyComponent();

    [[nodiscard]] int count() const;
    void setCount(int count);

  signals:
    void countChanged(int count);

  private:
    void layOut();
    void addOne();
    void enableHistory(bool enabled);

    QWidget m_panel;
    QLabel* m_label = new QLabel(QStringLiteral("0"), &m_panel); // The label and the buttons are owned by m_panel
    QPushButton* m_add = new QPushButton(QStringLiteral("+1"), &m_panel);
    QPushButton* m_undo = new QPushButton(QStringLiteral("Undo"), &m_panel);
    QPushButton* m_redo = new QPushButton(QStringLiteral("Redo"), &m_panel);
    int m_count = 0;
    Nebula::IHistory* m_history = nullptr; // Consumed
};
```

```cpp
// TallyComponent.cpp
#include "TallyComponent.h"

#include <QHBoxLayout>
#include <QPointer>

#include "Nebula/Standard/Command.h"
#include "TimePlugin.h"

NEBULA_REGISTER_COMPONENT(TimePlugin, TallyComponent);

namespace
{
// Sets the count of a tally from one value to another, and back.
class SetCount : public Nebula::Command
{
  public:
    SetCount(TallyComponent& tally, int from, int to)
        : Command(QStringLiteral("Count %1").arg(to)), m_tally(&tally), m_from(from), m_to(to)
    {
    }

    void execute() override { m_tally->setCount(m_to); }
    void undo() override { m_tally->setCount(m_from); }
    [[nodiscard]] bool isObsolete() const override { return m_tally.isNull(); } // Its instance was removed

  private:
    QPointer<TallyComponent> m_tally;
    int m_from;
    int m_to;
};
} // namespace

TallyComponent::TallyComponent()
{
    layOut();
    enableHistory(false);
    connect(m_add, &QPushButton::clicked, this, &TallyComponent::addOne);
    connect(m_undo, &QPushButton::clicked, this, [this] { m_history->undo(); });
    connect(m_redo, &QPushButton::clicked, this, [this] { m_history->redo(); });
    provideInterface<QWidget>(QStringLiteral("display"), &m_panel);
    consumeInterface<Nebula::IHistory>(
        QStringLiteral("history"), m_history,
        [this] { enableHistory(true); },   // m_history is set
        [this] { enableHistory(false); }); // m_history is about to be set back to nullptr
}

int TallyComponent::count() const
{
    return m_count;
}

void TallyComponent::setCount(int count)
{
    if(m_count == count)
    {
        return;
    }
    m_count = count;
    m_label->setText(QString::number(m_count));
    emit countChanged(m_count);
}

void TallyComponent::layOut()
{
    auto* layout = new QHBoxLayout(&m_panel); // Owned by m_panel
    layout->addWidget(m_label);
    layout->addWidget(m_add);
    layout->addWidget(m_undo);
    layout->addWidget(m_redo);
}

void TallyComponent::addOne()
{
    if(m_history == nullptr)
    {
        setCount(m_count + 1); // Without a history, nothing is undone
        return;
    }
    m_history->push(std::make_unique<SetCount>(*this, m_count, m_count + 1)); // Executes it, then keeps it
}

void TallyComponent::enableHistory(bool enabled)
{
    m_undo->setEnabled(enabled);
    m_redo->setEnabled(enabled);
}
```

The buttons only call `undo()` and `redo()` while the history is bound: they are disabled otherwise. A click made
before the history was bound is not in it, and is not undone.

</details>

## Questions

<details>
<summary>1. You click +1 three times, then Stop and Start again. What do Undo and the count do?</summary>

The count is back to its value of the edition, 0, and the history forgot what was done while the application ran:
`Undo` does nothing. Hold `count` in a [settings file](Settings-Files.md) to keep it.

</details>

<details>
<summary>2. Why does SetCount keep the value before rather than subtract one?</summary>

A command is undone from the state that executing it left, not from wherever the count is: keeping both values makes
undoing and redoing give back exactly what was there, whatever the command.

</details>

<details>
<summary>3. Your plugin links Nebula::Standard. Does it need StandardPlugin.nbdl to build?</summary>

No: it only needs the headers of the interfaces. The application needs `StandardPlugin.nbdl` in its `plugins/` to run
a `LogComponent` or a `HistoryComponent`, see [lesson 7](Course-7-Ship-an-Application.md).

</details>

**Next**: [Lesson 7 · Ship an application](Course-7-Ship-an-Application.md)
