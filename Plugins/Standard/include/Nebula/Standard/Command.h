// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QString>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <ranges>
#include <utility>
#include <vector>

namespace Nebula
{

// An action an application can undo, which a history keeps once it is done, see IHistory: what does it, execute(), what
// undoes it, undo(), and what it is, label(), a few words to show, such as "Add Sensor1". The history executes it once
// as it is pushed, then undoes it and executes it again as many times as asked, each time from the state the other one
// left: undo() from the one execute() left, execute() from the one undo() left. What it acts on is to be found again
// by what does not change across those, its name say, rather than by a pointer a step may make obsolete.
// One whose subject is gone, a document closed say, tells it is obsolete: the history drops it rather than undo or
// execute it.
class Command
{
  public:
    explicit Command(QString label) : m_label(std::move(label)) {}
    virtual ~Command() = default;
    Command(const Command&) = delete;
    Command& operator=(const Command&) = delete;
    Command(Command&&) = delete;
    Command& operator=(Command&&) = delete;

    [[nodiscard]] const QString& label() const
    {
        return m_label;
    }
    virtual void execute() = 0;
    virtual void undo() = 0;
    // Whether it can no longer be executed nor undone: the history drops it then. Never, unless a subclass tells
    // otherwise.
    [[nodiscard]] virtual bool isObsolete() const
    {
        return false;
    }

  private:
    QString m_label;
};

// Commands that are one action: one step of the history, which one undo() undoes entirely, the removal of a component
// and of its links say. execute() executes them in the order they were added, undo() undoes them the other way round.
// Obsolete as soon as one of them is.
class CompositeCommand : public Command
{
  public:
    using Command::Command;

    // Adds command after the others; nothing when it is nullptr.
    void add(std::unique_ptr<Command> command)
    {
        if(command != nullptr)
        {
            m_commands.push_back(std::move(command));
        }
    }
    [[nodiscard]] std::size_t count() const
    {
        return m_commands.size();
    }
    [[nodiscard]] bool isEmpty() const
    {
        return m_commands.empty();
    }

    void execute() override
    {
        std::ranges::for_each(m_commands, [](const std::unique_ptr<Command>& command) { command->execute(); });
    }
    void undo() override
    {
        std::ranges::for_each(m_commands | std::views::reverse,
                              [](const std::unique_ptr<Command>& command) { command->undo(); });
    }
    [[nodiscard]] bool isObsolete() const override
    {
        return std::ranges::any_of(m_commands,
                                   [](const std::unique_ptr<Command>& command) { return command->isObsolete(); });
    }

  private:
    std::vector<std::unique_ptr<Command>> m_commands;
};

} // namespace Nebula
