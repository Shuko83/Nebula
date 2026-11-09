# Lesson 4 · Properties and modes

*[The course](Course.md) › Lesson 4*

From here on, you write code. You start from the `TimePlugin` of [your first plugin](Your-First-Plugin.md), its
`ClockComponent` and its `WindowComponent`, built and assembled in NebulaStudio: do that page first. This lesson gives
the clock what its users will want to set, and has it refuse to start when it cannot work.

**You learn to** add properties of several types, which NebulaStudio edits and a project saves, and refuse to start.

## What you need to know

- A property is a `Q_PROPERTY` with `READ`, `WRITE` and `NOTIFY`. Its setter changes the value only when it differs,
  then emits the signal.
- NebulaStudio edits, and a project saves, numbers, booleans, texts, dates and times, enumerations (`Q_ENUM`), flags
  (`Q_FLAG`), colors and URLs. See [writing components](Writing-Components.md#properties).
- `onStart()` returns `false` to refuse to start: the application stops what it started, and is edited again.

After each change: close NebulaStudio, build the plugin, start NebulaStudio again. A plugin stays loaded, and its file
in use, until NebulaStudio is closed.

## Exercise 4.1 · How often it refreshes

**Goal**: a property `interval`, the number of milliseconds between two refreshes of the clock, 1000 at first. A value
of 0 or less is refused.

**Do**: add the property to `ClockComponent`. Use the timer itself to keep the value.

**Check**: `interval` is listed in the properties of the clock, and edited there. Set the `format` to `hh:mm:ss.zzz` and
`interval` to 100, **Start**: the milliseconds change ten times a second. Set it to 0: it stays as it was. Save the
project: `"interval": 100` is in the file.

<details>
<summary>Hint</summary>

`QTimer::interval()` and `QTimer::setInterval()` keep the value; `setInterval()` applies at once, even while the timer
runs. Set 1000 in the constructor, before anything reads it.

</details>

<details>
<summary>Solution</summary>

```cpp
// ClockComponent.h
    Q_PROPERTY(int interval READ interval WRITE setInterval NOTIFY intervalChanged)
    // ...
    [[nodiscard]] int interval() const;
    void setInterval(int interval);

  signals:
    void intervalChanged(int interval);
```

```cpp
// ClockComponent.cpp
// In the constructor: m_timer.setInterval(1000);

int ClockComponent::interval() const
{
    return m_timer.interval();
}

void ClockComponent::setInterval(int interval)
{
    if(interval <= 0 || m_timer.interval() == interval)
    {
        return; // A timer of 0 ms would refresh the label as fast as it can
    }
    m_timer.setInterval(interval); // Applied at once, even while it runs
    emit intervalChanged(interval);
}
```

A setter that refuses a value keeps the old one, and emits nothing.

</details>

## Exercise 4.2 · Refuse to start

**Goal**: a clock whose `format` is empty shows nothing: it refuses to start.

**Do**: change `onStart()`.

**Check**: empty the `format` and **Start**: a message box says `The application cannot be started:` and
`ClockComponent1 could not be started`, and the application is edited again. Instances start in the order they were
placed: if the window started before the clock, it was stopped again.

<details>
<summary>Solution</summary>

```cpp
bool ClockComponent::onStart()
{
    if(m_format.isEmpty())
    {
        return false; // Refused: the application stops what it started
    }
    showTime();
    m_timer.start();
    return true;
}
```

The message names the instance, not the reason. [Lesson 6](Course-6-Log-and-History.md) has the clock tell why, in the
log.

</details>

## Exercise 4.3 · A style to choose from a list

**Goal**: a property `style`, `Plain`, `Bold` or `Large`, which changes the font of the clock.

**Do**: declare an enumeration in `ClockComponent`, with `Q_ENUM`, and a property of that type.

**Check**: NebulaStudio offers the three values in a list. **Large** makes the time large and bold. Saved, the project
writes `"style": "Large"`, the name of the value.

<details>
<summary>Hint</summary>

The enumeration is declared in the class, before the functions that use it, and `Q_ENUM(Style)` follows it. In the
signal, write its type in full, `ClockComponent::Style`, for Qt to find it.

</details>

<details>
<summary>Solution</summary>

```cpp
// ClockComponent.h
    Q_PROPERTY(Style style READ style WRITE setStyle NOTIFY styleChanged)

  public:
    enum class Style
    {
        Plain,
        Bold,
        Large,
    };
    Q_ENUM(Style)
    // ...
    [[nodiscard]] Style style() const;
    void setStyle(Style style);

  signals:
    void styleChanged(ClockComponent::Style style);

  private:
    void applyStyle();
    // ...
    Style m_style = Style::Plain;
```

```cpp
// ClockComponent.cpp
ClockComponent::Style ClockComponent::style() const
{
    return m_style;
}

void ClockComponent::setStyle(Style style)
{
    if(m_style == style)
    {
        return;
    }
    m_style = style;
    applyStyle();
    emit styleChanged(m_style);
}

void ClockComponent::applyStyle()
{
    QFont font = m_display.font();
    font.setBold(m_style != Style::Plain);
    font.setPointSize(m_style == Style::Large ? 28 : 11);
    m_display.setFont(font);
}
```

</details>

## Questions

<details>
<summary>1. You change the interval while the application runs, from your own code. What is it after Stop?</summary>

The value it had when the application started: stopping gives every property its value of the edition back. A settings
file that holds it would keep the new one, see [lesson 2](Course-2-Keep-What-the-User-Chooses.md).

</details>

<details>
<summary>2. Why does setInterval() not emit its signal when the value is the same?</summary>

The signal tells a change: NebulaStudio and the settings files follow it. A signal for nothing would write a settings
file for nothing, or loop between two components that follow each other.

</details>

<details>
<summary>3. A QPoint property: is it saved?</summary>

No: JSON does not hold it as it is, and NebulaStudio has no editor for it. It is listed in the viewer of the component,
not edited, not saved.

</details>

**Next**: [Lesson 5 · Interfaces of your own](Course-5-Interfaces-of-Your-Own.md)
