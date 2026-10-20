// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QAction>
#include <QString>

#include "Nebula/Component/Component.h"

// An action the user triggers, from a bar or with its shortcut: a QAction it creates and provides as the interface
// "action", which a TopBarComponent shows and a CounterComponent counts, say. It needs a QApplication.
class ActionComponent : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "An action the user triggers, from a bar or with its shortcut")
    Q_CLASSINFO("category", "Widgets")
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QString shortcut READ shortcut WRITE setShortcut NOTIFY shortcutChanged)

  public:
    ActionComponent();

    [[nodiscard]] QString text() const;
    void setText(const QString& text);
    // As it is written whatever the system, "Ctrl+N" say; none when empty, or when it is not a shortcut.
    [[nodiscard]] QString shortcut() const;
    void setShortcut(const QString& shortcut);

  signals:
    void textChanged(const QString& text);
    void shortcutChanged(const QString& shortcut);

  private:
    QAction m_action;
};
