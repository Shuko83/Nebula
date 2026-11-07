// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "History.h"

#include <QLoggingCategory>
#include <algorithm>
#include <iterator>
#include <utility>

using Nebula::Command;

namespace
{

Q_LOGGING_CATEGORY(lcStandard, "nebula.standard")

} // namespace

void History::push(std::unique_ptr<Command> command)
{
    if(command == nullptr)
    {
        return;
    }
    if(m_running)
    {
        executeAside(*command);
        return;
    }
    forgetUndone();
    Command& pushed = *m_commands.emplace_back(std::move(command));
    ++m_done;                       // Before it runs: what it does may clear the history
    run(pushed, &Command::execute); // Then forgets the oldest ones beyond its size
}

void History::executeAside(Command& command)
{
    qCWarning(lcStandard).noquote()
        << QStringLiteral("\"%1\" is pushed to the history while another command runs: it is done, but not kept")
               .arg(command.label());
    command.execute();
}

bool History::undo()
{
    if(!m_undoActive || isBusy("undone") || !dropObsoleteDone())
    {
        return false;
    }
    --m_done;
    run(*m_commands.at(m_done), &Command::undo);
    return true;
}

bool History::redo()
{
    if(!m_redoActive || isBusy("redone") || !dropObsoleteUndone())
    {
        return false;
    }
    run(*m_commands.at(m_done++), &Command::execute);
    return true;
}

void History::clear()
{
    if(m_running)
    {
        m_clearPending = true; // The command running is still needed
        return;
    }
    m_commands.clear();
    m_done = 0;
}

bool History::isUndoActive() const
{
    return m_undoActive;
}

bool History::isRedoActive() const
{
    return m_redoActive;
}

int History::maxSize() const
{
    return m_maxSize;
}

void History::setMaxSize(int maxSize)
{
    m_maxSize = std::max(maxSize, 0);
    if(!m_running) // Otherwise once the command running is done
    {
        trim();
    }
}

void History::setUndoActive(bool active)
{
    m_undoActive = active;
}

void History::setRedoActive(bool active)
{
    m_redoActive = active;
}

void History::forgetUndone()
{
    m_commands.erase(std::next(m_commands.begin(), static_cast<std::ptrdiff_t>(m_done)), m_commands.end());
}

void History::trim()
{
    const auto maxSize = static_cast<std::size_t>(m_maxSize);
    while(m_commands.size() > maxSize)
    {
        // The oldest done first; once none is left, the undone the furthest from being redone, the last one pushed.
        if(m_done > 0)
        {
            m_commands.erase(m_commands.begin());
            --m_done;
        }
        else
        {
            m_commands.pop_back();
        }
    }
}

bool History::dropObsoleteDone()
{
    while(m_done > 0 && m_commands.at(m_done - 1)->isObsolete())
    {
        --m_done;
        m_commands.erase(std::next(m_commands.begin(), static_cast<std::ptrdiff_t>(m_done)));
    }
    return m_done > 0;
}

bool History::dropObsoleteUndone()
{
    const auto next = std::next(m_commands.begin(), static_cast<std::ptrdiff_t>(m_done));
    const auto obsolete = std::find_if_not(next, m_commands.end(), [](const std::unique_ptr<Command>& command)
                                           { return command->isObsolete(); });
    m_commands.erase(next, obsolete);
    return m_done < m_commands.size();
}

void History::run(Command& command, void (Command::*step)())
{
    m_running = true;
    (command.*step)();
    m_running = false;
    if(std::exchange(m_clearPending, false))
    {
        clear();
    }
    trim(); // When its size was set meanwhile
}

bool History::isBusy(const char* what) const
{
    if(m_running)
    {
        qCWarning(lcStandard).noquote()
            << QStringLiteral("Nothing is %1 while a command of the history runs").arg(QLatin1StringView(what));
    }
    return m_running;
}
