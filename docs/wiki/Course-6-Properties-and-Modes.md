# Lesson 6 · Properties and modes

*[The course](Course.md) › Lesson 6*

Your clock has one property, `format`. Here you give it two more, a number and a choice in a list, and you have it
refuse to start when it cannot work.

**You need**: [lesson 5](Course-5-Your-First-Component.md) done: `C:\work\Acme` builds, and `Clock.nbp` is saved.

Each exercise changes the two files of the clock, `C:\work\Acme\TimePlugin\src\ClockComponent.h` and
`ClockComponent.cpp`. The lines to add are given with the place to put them; at the end of the lesson, both files
are given whole, to compare with yours.

## The recipe of a property

A property always takes the same five additions. You will do them twice in this lesson.

| # | Where | What |
| --- | --- | --- |
| 1 | `.h`, with the other `Q_PROPERTY` lines | The `Q_PROPERTY` line: its type, its name, its three functions |
| 2 | `.h`, under `public:` | The declarations of the function that reads it and of the one that writes it |
| 3 | `.h`, under `signals:` | The declaration of the signal that tells it changed |
| 4 | `.h`, under `private:` | The member that keeps its value, when nothing keeps it already |
| 5 | `.cpp` | The code of the two functions |

## Exercise 6.1 · A number: how often the clock refreshes

**Goal**: a property `interval`, the number of milliseconds between two refreshes, 1000 at first. A value of 0 or less
is refused.

**Steps**

1. In `ClockComponent.h`, under the line `Q_PROPERTY(QString format ...)`, add:

   ```cpp
       Q_PROPERTY(int interval READ interval WRITE setInterval NOTIFY intervalChanged)
   ```

2. In `ClockComponent.h`, under the line `void setFormat(const QString& format);`, add:

   ```cpp
       [[nodiscard]] int interval() const;
       void setInterval(int interval);
   ```

3. In `ClockComponent.h`, under the line `void formatChanged(const QString& format);`, add:

   ```cpp
       void intervalChanged(int interval);
   ```

   No member to add: the timer, `m_timer`, already keeps its interval.

4. In `ClockComponent.cpp`, under the function `setFormat()`, after its closing `}`, add:

   ```cpp
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

   A setter that refuses a value keeps the former one, and emits nothing.

5. Close NebulaStudio, build, and start NebulaStudio with your project:

   ```powershell
   cmake --build C:/work/Acme/build --config Release
   ```

   ```powershell
   & "C:\Nebula\NebulaStudio\Release\bin\NebulaStudio.exe" --plugin-dir C:\work\Acme\build\bin\Release\plugins C:\work\Acme\Clock.nbp
   ```

6. Click `ClockComponent1`.

   *You should see:* two properties, `format` and `interval`, whose value is `1000`.

7. Set `format` to `hh:mm:ss.zzz` and `interval` to `100`, then **Run** > **Start**.

   *You should see:* the time with its milliseconds, which change ten times a second. **Run** > **Stop**.

8. Set `interval` to `0`, then click another box, and `ClockComponent1` again.

   *You should see:* `interval` is still `100`: your setter refused the `0`.

9. **File** > **Save**, then open `C:\work\Acme\Clock.nbp` with the Notepad.

   *You should see:* `"interval": 100` among the properties of `ClockComponent1`. A project saves every property that
   can be written.

## Exercise 6.2 · Refuse to start

With an empty `format`, the clock shows nothing. Rather than run for nothing, it refuses to start.

**Steps**

1. In `ClockComponent.cpp`, replace the function `onStart()` with:

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

2. Close NebulaStudio, build, start NebulaStudio with your project (step 5 of exercise 6.1).
3. Click `ClockComponent1`, click the value of `format`, erase it, press Enter.
4. **Run** > **Start**.

   *You should see:* a box, **Start**, that says `The application cannot be started:` and `ClockComponent1 could not
   be started`. Click **OK**: the application is edited again, and no window stayed open.

5. Set `format` back to `hh:mm:ss`, and **Run** > **Start**.

   *You should see:* the clock runs. **Run** > **Stop**.

The message names the instance, not the reason. [Lesson 8](Course-8-Log-and-History.md) has the clock tell why.

## Exercise 6.3 · A choice in a list: the style of the clock

A property whose type is an **enumeration**, a list of named values, is edited in NebulaStudio as a list to choose
from.

**Goal**: a property `style`, `Plain`, `Bold` or `Large`, that changes how the time is written.

**Steps**

1. In `ClockComponent.h`, under the line `Q_PROPERTY(int interval ...)`, add:

   ```cpp
       Q_PROPERTY(Style style READ style WRITE setStyle NOTIFY styleChanged)
   ```

2. In `ClockComponent.h`, just under the line `public:`, before `ClockComponent();`, add the enumeration:

   ```cpp
       enum class Style
       {
           Plain,
           Bold,
           Large,
       };
       Q_ENUM(Style)

   ```

   `Q_ENUM(Style)` tells Qt the names of the values: that is what lets NebulaStudio list them, and a project save
   them by name.

3. In `ClockComponent.h`, under the line `void setInterval(int interval);`, add:

   ```cpp
       [[nodiscard]] Style style() const;
       void setStyle(Style style);
   ```

4. In `ClockComponent.h`, under the line `void intervalChanged(int interval);`, add:

   ```cpp
       void styleChanged(ClockComponent::Style style);
   ```

   In a signal, write the type in full, with `ClockComponent::` before it.

5. In `ClockComponent.h`, under the line `void showTime();`, add:

   ```cpp
       void applyStyle();
   ```

   and under the line `QString m_format = QStringLiteral("hh:mm:ss");`, add:

   ```cpp
       Style m_style = Style::Plain;
   ```

6. In `ClockComponent.cpp`, under the function `setInterval()`, add:

   ```cpp
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
   ```

   and at the very end of the file:

   ```cpp
   void ClockComponent::applyStyle()
   {
       QFont font = m_display.font();
       font.setBold(m_style != Style::Plain);
       font.setPointSize(m_style == Style::Large ? 28 : 11);
       m_display.setFont(font);
   }
   ```

7. Close NebulaStudio, build, start NebulaStudio with your project.
8. Click `ClockComponent1`, then click the value of `style`.

   *You should see:* a list with `Plain`, `Bold` and `Large`.

9. Choose `Large`, then **Run** > **Start**.

   *You should see:* the time, large and bold. **Run** > **Stop**, then **File** > **Save**.

**Check**: in `Clock.nbp`, opened with the Notepad, `"style": "Large"`.

**If it does not work**

- *An error says `Style` is not declared*: the enumeration is under the functions that use it. It has to be just
  under `public:`.
- *The value of `style` is a number, not a list*: the line `Q_ENUM(Style)` is missing.

## The two files, whole

<details>
<summary>ClockComponent.h, at the end of this lesson</summary>

```cpp
// Acme/TimePlugin/src/ClockComponent.h (complete)
#pragma once

#include <QLabel>
#include <QString>
#include <QTimer>

#include "Nebula/Component/Component.h"

// Shows the time in a label it provides as "display", refreshed while it runs.
class ClockComponent : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Shows the time, refreshed while it runs")
    Q_CLASSINFO("category", "Time")
    Q_PROPERTY(QString format READ format WRITE setFormat NOTIFY formatChanged)
    Q_PROPERTY(int interval READ interval WRITE setInterval NOTIFY intervalChanged)
    Q_PROPERTY(Style style READ style WRITE setStyle NOTIFY styleChanged)

  public:
    enum class Style
    {
        Plain,
        Bold,
        Large,
    };
    Q_ENUM(Style)

    ClockComponent();

    [[nodiscard]] QString format() const;
    void setFormat(const QString& format);
    [[nodiscard]] int interval() const;
    void setInterval(int interval);
    [[nodiscard]] Style style() const;
    void setStyle(Style style);

  signals:
    void formatChanged(const QString& format);
    void intervalChanged(int interval);
    void styleChanged(ClockComponent::Style style);

  protected:
    bool onStart() override;
    void onStop() override;

  private:
    void showTime();
    void applyStyle();

    QLabel m_display;
    QTimer m_timer;
    QString m_format = QStringLiteral("hh:mm:ss");
    Style m_style = Style::Plain;
};
```

</details>

<details>
<summary>ClockComponent.cpp, at the end of this lesson</summary>

```cpp
// Acme/TimePlugin/src/ClockComponent.cpp (complete)
#include "ClockComponent.h"

#include <QTime>

#include "TimePlugin.h" // Written for you by nebula_add_plugin(NAME TimePlugin)

NEBULA_REGISTER_COMPONENT(TimePlugin, ClockComponent);

ClockComponent::ClockComponent()
{
    m_display.setAlignment(Qt::AlignCenter);
    showTime();
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &ClockComponent::showTime);
    provideInterface<QWidget>(QStringLiteral("display"), &m_display);
}

QString ClockComponent::format() const
{
    return m_format;
}

void ClockComponent::setFormat(const QString& format)
{
    if(m_format == format)
    {
        return;
    }
    m_format = format;
    showTime();
    emit formatChanged(m_format);
}

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

void ClockComponent::onStop()
{
    m_timer.stop();
}

void ClockComponent::showTime()
{
    m_display.setText(QTime::currentTime().toString(m_format));
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

## Your turn

Give the clock a property `prefix`, a text written before the time, empty at first: with `Paris `, the clock shows
`Paris 14:03:27`.

<details>
<summary>Solution</summary>

The five additions of the recipe. In `ClockComponent.h`:

```cpp
    Q_PROPERTY(QString prefix READ prefix WRITE setPrefix NOTIFY prefixChanged)
```

```cpp
    [[nodiscard]] QString prefix() const;
    void setPrefix(const QString& prefix);
```

```cpp
    void prefixChanged(const QString& prefix);
```

```cpp
    QString m_prefix;
```

In `ClockComponent.cpp`:

```cpp
QString ClockComponent::prefix() const
{
    return m_prefix;
}

void ClockComponent::setPrefix(const QString& prefix)
{
    if(m_prefix == prefix)
    {
        return;
    }
    m_prefix = prefix;
    showTime();
    emit prefixChanged(m_prefix);
}
```

and in `showTime()`, the text becomes `m_prefix + QTime::currentTime().toString(m_format)`.

The rest of the course does not use it: keep it or remove it.

</details>

## What you learned

- A property is five additions, always the same.
- NebulaStudio edits numbers, texts, booleans, dates and times, colors, and enumerations declared with `Q_ENUM`; a
  project saves them.
- A setter can refuse a value; `onStart()` can refuse to start by returning `false`.

**Next**: [Lesson 7 · Interfaces of your own](Course-7-Interfaces-of-Your-Own.md)
