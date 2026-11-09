# Lesson 5 · Interfaces of your own

*[The course](Course.md) › Lesson 5*

So far, components met through Qt types: a `QWidget`, a `QAction`. An interface of your own, a class with the functions
one component offers another, lets them work together without knowing each other. This lesson writes an alarm that
asks a time source what time it is, a source that stands in for the clock, and a panel that shows any number of
widgets.

**You learn to** declare an interface, provide it, consume it, swap what provides it, and consume a list.

## What you need to know

- An interface is a class of virtual functions, in a header that both components include.
- `provideInterface<T>(name, pointer)` provides it; `T` is the type a consumer asks for.
- `consumeInterface<T>(name, member, requirement, onAvailable, onUnavailable)` consumes one: Nebula writes the member.
- `consumeInterfaceList<T>(name, list, onInserted, onRemoved)` consumes a list, bound to several in an order.

[Writing components](Writing-Components.md#interfaces) tells the rest.

## Exercise 5.1 · An alarm that asks the time

**Goal**: an interface `ITimeSource`, with a single function, `QTime time() const`. The clock provides one as `source`,
the time of the computer. A new component, `AlarmComponent`, consumes a `source`, which it cannot do without, and has
an `alarm` time, 07:30 at first. While it runs, it checks every second whether the source says it is the minute of the
alarm, and then shows `Ringing!` in the label it provides as `display`, once; otherwise it shows `Alarm at 07:30`.

**Do**: write `ITimeSource.h`, have the clock provide its `source`, and write `AlarmComponent`.

**Check**: the alarm is marked in amber until its `source` is bound. While you drag from its `source`, the `display` of
the clock is dimmed: not the same type. Bind it to the clock, set the alarm to the next minute, show its display in the
window, **Start**, and wait.

<details>
<summary>Hint</summary>

The clock does not derive from the interface: a small class of its own, `SystemTime`, implements it, and the clock
holds one as a member, as it holds its label. The alarm needs a `QTimer` and a `bool` to ring once; check at once in
`onStart()` too.

</details>

<details>
<summary>Solution</summary>

```cpp
// ITimeSource.h
#pragma once

#include <QTime>

// What tells the time: a clock, or anything that stands in for one.
class ITimeSource
{
  public:
    virtual ~ITimeSource() = default;

    [[nodiscard]] virtual QTime time() const = 0;
};
```

```cpp
// ClockComponent.h
#include "ITimeSource.h"

// The time of the computer.
class SystemTime : public ITimeSource
{
  public:
    [[nodiscard]] QTime time() const override { return QTime::currentTime(); }
};

// In ClockComponent, a member:
    SystemTime m_time;
```

```cpp
// ClockComponent.cpp, in the constructor
provideInterface<ITimeSource>(QStringLiteral("source"), &m_time);
```

```cpp
// AlarmComponent.h
#pragma once

#include <QLabel>
#include <QTime>
#include <QTimer>

#include "ITimeSource.h"
#include "Nebula/Component/Component.h"

// Rings once the time its "source" tells reaches its alarm, while it runs, and shows it in its "display".
class AlarmComponent : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Rings once the time it is given reaches its alarm")
    Q_CLASSINFO("category", "Time")
    Q_PROPERTY(QTime alarm READ alarm WRITE setAlarm NOTIFY alarmChanged)

  public:
    AlarmComponent();

    [[nodiscard]] QTime alarm() const;
    void setAlarm(QTime alarm);

  signals:
    void alarmChanged(QTime alarm);

  protected:
    bool onStart() override;
    void onStop() override;

  private:
    void check();
    void ring();
    void showWaiting();

    QLabel m_display;
    QTimer m_timer;
    QTime m_alarm{7, 30};
    bool m_rung = false;
    ITimeSource* m_source = nullptr; // Consumed
};
```

```cpp
// AlarmComponent.cpp
#include "AlarmComponent.h"

#include "TimePlugin.h"

NEBULA_REGISTER_COMPONENT(TimePlugin, AlarmComponent);

AlarmComponent::AlarmComponent()
{
    m_display.setAlignment(Qt::AlignCenter);
    showWaiting();
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &AlarmComponent::check);
    provideInterface<QWidget>(QStringLiteral("display"), &m_display);
    consumeInterface<ITimeSource>(QStringLiteral("source"), m_source, Nebula::InterfaceRequirement::Mandatory);
}

QTime AlarmComponent::alarm() const
{
    return m_alarm;
}

void AlarmComponent::setAlarm(QTime alarm)
{
    if(m_alarm == alarm)
    {
        return;
    }
    m_alarm = alarm;
    showWaiting();
    emit alarmChanged(m_alarm);
}

bool AlarmComponent::onStart()
{
    m_rung = false;
    showWaiting();
    m_timer.start();
    check(); // At once: the alarm may be due already
    return true;
}

void AlarmComponent::onStop()
{
    m_timer.stop();
}

void AlarmComponent::check()
{
    if(m_rung || m_source == nullptr)
    {
        return;
    }
    const QTime now = m_source->time();
    if(now.hour() == m_alarm.hour() && now.minute() == m_alarm.minute())
    {
        ring();
    }
}

void AlarmComponent::ring()
{
    m_rung = true;
    m_display.setText(QStringLiteral("Ringing!"));
}

void AlarmComponent::showWaiting()
{
    m_display.setText(QStringLiteral("Alarm at %1").arg(m_alarm.toString(QStringLiteral("hh:mm"))));
}
```

`m_source` is tested before it is used: `Mandatory` only tells who assembles, and the alarm starts without a source.

</details>

## Exercise 5.2 · A time that stands still

**Goal**: try the alarm without waiting for 07:30. A component `FixedTimeComponent` provides a `source` that always
says the same time, its property `time`, 07:30 at first.

**Do**: write it, then bind the `source` of the alarm to it instead of the clock.

**Check**: **Start**: the alarm shows `Ringing!` at once. Bind it to the clock again: it waits. The alarm did not
change: only what provides its interface did.

<details>
<summary>Solution</summary>

```cpp
// FixedTimeComponent.h
#pragma once

#include <QTime>

#include "ITimeSource.h"
#include "Nebula/Component/Component.h"

// Always the same time.
class FixedTime : public ITimeSource
{
  public:
    [[nodiscard]] QTime time() const override { return m_time; }
    void setTime(QTime time) { m_time = time; }

  private:
    QTime m_time{7, 30};
};

// Tells the same time whatever the time is, as "source": to try what depends on the time without waiting for it.
class FixedTimeComponent : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Always tells the same time")
    Q_CLASSINFO("category", "Time")
    Q_PROPERTY(QTime time READ time WRITE setTime NOTIFY timeChanged)

  public:
    FixedTimeComponent();

    [[nodiscard]] QTime time() const;
    void setTime(QTime time);

  signals:
    void timeChanged(QTime time);

  private:
    FixedTime m_source;
};
```

```cpp
// FixedTimeComponent.cpp
#include "FixedTimeComponent.h"

#include "TimePlugin.h"

NEBULA_REGISTER_COMPONENT(TimePlugin, FixedTimeComponent);

FixedTimeComponent::FixedTimeComponent()
{
    provideInterface<ITimeSource>(QStringLiteral("source"), &m_source);
}

QTime FixedTimeComponent::time() const
{
    return m_source.time();
}

void FixedTimeComponent::setTime(QTime time)
{
    if(m_source.time() == time)
    {
        return;
    }
    m_source.setTime(time);
    emit timeChanged(time);
}
```

A stand-in is how a component is tried in isolation: in a test, or in NebulaStudio.

</details>

## Exercise 5.3 · A panel for any number of widgets

**Goal**: a window shows one widget; you want the clock and the alarm. A `PanelComponent` consumes a list of
`QWidget`, `widgets`, shows them one above the other in the order of their bindings, and provides itself as `display`.

**Do**: write it, bind the displays of the clock and of the alarm to its `widgets`, and its `display` to the `content`
of the window.

**Check**: **Start**: the time above the alarm. Bind the alarm first and the clock second, in a new panel: the order
follows. Remove the link of the alarm: it leaves the panel.

<details>
<summary>Hint</summary>

A `QVBoxLayout` on a `QWidget` of the component. A widget that leaves the list is given back without a parent: it
belongs to the component that provides it.

</details>

<details>
<summary>Solution</summary>

```cpp
// PanelComponent.h
#pragma once

#include <QList>
#include <QVBoxLayout>
#include <QWidget>

#include "Nebula/Component/Component.h"

// Shows the widgets of its list "widgets" one above the other, in the order of their bindings, in its "display".
class PanelComponent : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Shows the widgets it is given, one above the other")
    Q_CLASSINFO("category", "Time")

  public:
    PanelComponent();

  private:
    void place(QWidget* widget);
    void giveBack(QWidget* widget);

    QWidget m_panel;
    QVBoxLayout* m_layout = new QVBoxLayout(&m_panel); // Owned by m_panel
    QList<QWidget*> m_widgets;                         // Consumed
};
```

```cpp
// PanelComponent.cpp
#include "PanelComponent.h"

#include "TimePlugin.h"

NEBULA_REGISTER_COMPONENT(TimePlugin, PanelComponent);

PanelComponent::PanelComponent()
{
    provideInterface<QWidget>(QStringLiteral("display"), &m_panel);
    consumeInterfaceList<QWidget>(
        QStringLiteral("widgets"), m_widgets,
        [this](QWidget* widget) { place(widget); },     // In m_widgets
        [this](QWidget* widget) { giveBack(widget); }); // About to leave m_widgets
}

void PanelComponent::place(QWidget* widget)
{
    // At its rank among those available: the layout holds the same widgets, in the same order.
    m_layout->insertWidget(static_cast<int>(m_widgets.indexOf(widget)), widget);
    widget->show();
}

void PanelComponent::giveBack(QWidget* widget)
{
    m_layout->removeWidget(widget);
    widget->setParent(nullptr); // It belongs to the component that provides it
}
```

`onInserted` is called once the widget is in `m_widgets`, `onRemoved` while it still is: its rank there is its rank in
the layout.

</details>

## Questions

<details>
<summary>1. Why does the clock hold a SystemTime rather than derive from ITimeSource?</summary>

A component creates the objects it is made of as members: its label, its timer, its time source. Each does one thing,
and the component stays the place where they are assembled.

</details>

<details>
<summary>2. Can the display of the clock be bound to the panel and to the window at once?</summary>

Nebula lets you, since an interface can be provided to several consumers, but a widget is shown in one place at a time:
the last one to take it wins. Bind a widget to one consumer only.

</details>

<details>
<summary>3. The alarm and the clock are in two different plugins. What do they share?</summary>

`ITimeSource.h`, the header of the interface: both include it, and its type is found by its name, `ITimeSource*`.
That is how [the standard plugins](Using-the-Standard-Plugins.md) share theirs.

</details>

**Next**: [Lesson 6 · Log and history](Course-6-Log-and-History.md)
