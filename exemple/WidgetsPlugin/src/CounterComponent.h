// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QLabel>
#include <QMetaObject>
#include <QString>

#include "Nebula/Component/Component.h"

class QAction;

// Counts the times the action it consumes as "action", which it cannot do without, is triggered while it runs, and
// shows the count in a QLabel it creates and provides as the interface "display", a QWidget. It needs a QApplication.
class CounterComponent : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Counts the times an action is triggered while it runs")
    Q_CLASSINFO("category", "Widgets")
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)

  public:
    CounterComponent();

    [[nodiscard]] int count() const;
    void setCount(int count);
    // What its display shows, %1 standing for the count.
    [[nodiscard]] QString text() const;
    void setText(const QString& text);

  signals:
    void countChanged(int count);
    void textChanged(const QString& text);

  private:
    void countTrigger();
    void showCount();

    QLabel m_display;
    QAction* m_action = nullptr;         // Consumed
    QMetaObject::Connection m_triggered; // To m_action, while it has it
    int m_count = 0;
    QString m_text = QStringLiteral("Triggered %1 times");
};
