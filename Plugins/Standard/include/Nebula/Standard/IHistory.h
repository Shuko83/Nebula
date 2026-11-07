// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <memory>

#include "Nebula/Standard/Command.h"

namespace Nebula
{

// What an application did that can be undone: the interface "history" of a HistoryComponent of StandardPlugin. An
// action is pushed to it as a Command, rather than done directly: it is executed, then kept as the last one to undo.
// undo() undoes the last one kept, one step at a time, and redo() executes again the last one undone. Pushing an action
// after an undo forgets those undone: they can no longer be redone. It keeps maxSize commands at most, forgetting the
// oldest ones beyond. Undoing, or redoing, can be turned off: it does nothing then, see isUndoActive() and
// isRedoActive().
// It does not tell what it holds: whether there is anything to undo is told by undo() itself.
class IHistory
{
  public:
    virtual ~IHistory() = default;

    // Executes command, then keeps it as the last one to undo, forgetting those undone; nothing when it is nullptr. One
    // pushed while another is executed or undone is executed without being kept, with a warning.
    virtual void push(std::unique_ptr<Command> command) = 0;
    // Undoes the last command done, dropping those that are obsolete on the way, see Command::isObsolete(). Returns
    // whether one was undone: none is when none is left, while undoing is turned off, and while another one is
    // executed or undone.
    virtual bool undo() = 0;
    // Executes again the last command undone, dropping those that are obsolete on the way. Returns whether one was
    // executed: none is when none was undone since the last push, while redoing is turned off, and while another one is
    // executed or undone.
    virtual bool redo() = 0;
    // Forgets every command, those undone included: when what they act on is replaced, a document opened say.
    virtual void clear() = 0;
    // Whether undo() and redo() do anything: what offers them can be disabled while they do not.
    [[nodiscard]] virtual bool isUndoActive() const = 0;
    [[nodiscard]] virtual bool isRedoActive() const = 0;
};

} // namespace Nebula
