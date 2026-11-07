// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "Nebula/Standard/IHistory.h"

// What a HistoryComponent provides as "history": its commands, those done then those undone, the last one undone
// first. Its size and whether it undoes and redoes are the properties of the component.
class History : public Nebula::IHistory
{
  public:
    // The size a history has until it is given another one.
    static constexpr int DefaultMaxSize = 100;

    void push(std::unique_ptr<Nebula::Command> command) override;
    bool undo() override;
    bool redo() override;
    void clear() override;
    [[nodiscard]] bool isUndoActive() const override;
    [[nodiscard]] bool isRedoActive() const override;

    // How many commands it keeps at most, those undone included: the oldest ones go beyond. 0 keeps none.
    [[nodiscard]] int maxSize() const;
    // Forgets the commands beyond maxSize, at least 0, at once.
    void setMaxSize(int maxSize);
    void setUndoActive(bool active);
    void setRedoActive(bool active);

  private:
    // Pushes command while another one is executed or undone: executes it without keeping it.
    static void executeAside(Nebula::Command& command);
    // Forgets the commands undone: they can no longer be redone.
    void forgetUndone();
    // Forgets the oldest commands done, then the last ones undone, beyond maxSize().
    void trim();
    // Drops the last commands done while they are obsolete; returns whether one is left.
    bool dropObsoleteDone();
    // Drops the next commands to redo while they are obsolete; returns whether one is left.
    bool dropObsoleteUndone();
    // Has command do step, execute or undo, none other being run meanwhile; clears it afterwards when clear() was
    // called meanwhile.
    void run(Nebula::Command& command, void (Nebula::Command::*step)());
    // Whether a command is executed or undone, which nothing else is meanwhile; warns when one is.
    [[nodiscard]] bool isBusy(const char* what) const;

    std::vector<std::unique_ptr<Nebula::Command>> m_commands; // The oldest first
    std::size_t m_done = 0; // Those before are done, the others undone: m_commands[m_done] is the next to redo
    int m_maxSize = DefaultMaxSize;
    bool m_undoActive = true;
    bool m_redoActive = true;
    bool m_running = false;      // While a command is executed or undone
    bool m_clearPending = false; // clear() was called while one was: it is cleared once it is done
};
