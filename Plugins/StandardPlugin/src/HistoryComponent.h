// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "History.h"
#include "Nebula/Component/Component.h"

// The history of what the application does that can be undone. It provides "history", a Nebula::IHistory, from
// construction on: an action pushed there as a Nebula::Command is executed, then undone and done again as asked, one
// step at a time. It keeps maxSize commands at most, and undoes and redoes while undoActive and redoActive are true.
// It works whatever its state; once it stops, it forgets what was done while it ran.
class HistoryComponent : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Keeps what the application does, for it to be undone and redone one step at a time")
    Q_CLASSINFO("category", "Edition")
    Q_PROPERTY(int maxSize READ maxSize WRITE setMaxSize NOTIFY maxSizeChanged)
    Q_PROPERTY(bool undoActive READ isUndoActive WRITE setUndoActive NOTIFY undoActiveChanged)
    Q_PROPERTY(bool redoActive READ isRedoActive WRITE setRedoActive NOTIFY redoActiveChanged)

  public:
    HistoryComponent();

    // How many commands the history keeps at most, those undone included: the oldest go beyond, at once. 0 keeps
    // none; a negative one is refused with a warning.
    [[nodiscard]] int maxSize() const;
    void setMaxSize(int maxSize);
    // Whether the history undoes, and redoes: undo(), or redo(), does nothing otherwise.
    [[nodiscard]] bool isUndoActive() const;
    void setUndoActive(bool active);
    [[nodiscard]] bool isRedoActive() const;
    void setRedoActive(bool active);

  signals:
    void maxSizeChanged(int maxSize);
    void undoActiveChanged(bool active);
    void redoActiveChanged(bool active);

  protected:
    void onStop() override;

  private:
    History m_history;
};
