// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "MessageLog.h"

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QMutex>
#include <QMutexLocker>
#include <algorithm>
#include <array>
#include <cstdio>
#include <iterator>
#include <utility>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace Nebula
{

namespace
{

using Level = std::pair<QtMsgType, const char*>;

// The levels, from the least severe, under the names the command line and the filter rules give them.
constexpr std::array Levels = {Level{QtDebugMsg, "debug"}, Level{QtInfoMsg, "info"}, Level{QtWarningMsg, "warning"},
                               Level{QtCriticalMsg, "critical"}, Level{QtFatalMsg, "fatal"}};

QMutex mutex;                        // Guards current, and has a message written whole before the next one
MessageLog* current = nullptr;       // The one installed, while there is one
QtMessageHandler previous = nullptr; // Where Qt wrote the messages before it was installed

// The rank of level among Levels, from the least severe.
std::ptrdiff_t rankOf(QtMsgType level)
{
    return std::distance(Levels.begin(), std::ranges::find(Levels, level, &Level::first));
}

// The filter rules that leave out the messages less severe than level: none for debug, which leaves them as Qt writes
// them. A fatal message is never left out.
QString filterRules(QtMsgType level)
{
    QStringList rules;
    for(std::ptrdiff_t rank = 0; rank < rankOf(level); ++rank)
    {
        rules.append(QStringLiteral("*.%1=false").arg(QLatin1StringView(Levels.at(rank).second)));
    }
    return rules.join(u'\n');
}

#ifdef Q_OS_WIN
// Whether the error output goes somewhere already: to a console, or to a file it is redirected to.
bool hasErrorOutput()
{
    const HANDLE output = GetStdHandle(STD_ERROR_HANDLE);
    return output != nullptr && output != INVALID_HANDLE_VALUE;
}
#endif

// Has an application without a console of its own, a Windows one, write its error output to the console of the
// terminal it is started from, when it is started from one and its error output goes nowhere else.
void attachConsole()
{
#ifdef Q_OS_WIN
    if(!hasErrorOutput() && AttachConsole(ATTACH_PARENT_PROCESS) != 0)
    {
        FILE* stream = nullptr;
        freopen_s(&stream, "CONOUT$", "w", stderr);
    }
#endif
}

} // namespace

MessageLog::MessageLog(const ApplicationOptions& options, QString& error)
    : m_console(options.console), m_logger(options.logger)
{
    QLoggingCategory::setFilterRules(filterRules(options.logLevel));
    openFile(options.logFile, error);
    if(m_console)
    {
        attachConsole();
    }
    install();
}

MessageLog::~MessageLog()
{
    if(m_installed)
    {
        const QMutexLocker locker(&mutex);
        qInstallMessageHandler(previous);
        current = nullptr;
    }
    QLoggingCategory::setFilterRules({});
}

std::optional<QtMsgType> MessageLog::levelNamed(const QString& name)
{
    for(const auto& [level, levelName] : Levels)
    {
        if(name.compare(QLatin1StringView(levelName), Qt::CaseInsensitive) == 0)
        {
            return level;
        }
    }
    return std::nullopt;
}

QStringList MessageLog::levelNames()
{
    QStringList names;
    for(const auto& [level, name] : Levels)
    {
        names.append(QLatin1StringView(name));
    }
    return names;
}

void MessageLog::openFile(const QString& filePath, QString& error)
{
    if(filePath.isEmpty())
    {
        return;
    }
    m_file.setFileName(filePath);
    QDir().mkpath(QFileInfo(filePath).absolutePath()); // Its directory may not exist yet
    if(!m_file.open(QIODevice::Append | QIODevice::Text))
    {
        error = QStringLiteral("Cannot open the log file %1: %2")
                    .arg(QDir::toNativeSeparators(filePath), m_file.errorString());
    }
}

void MessageLog::install()
{
    if(!m_console && !m_file.isOpen() && !m_logger)
    {
        return; // Qt writes them as it does
    }
    const QMutexLocker locker(&mutex);
    current = this;
    previous = qInstallMessageHandler(&MessageLog::handle);
    m_installed = true;
}

void MessageLog::handle(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    // Qt does not call it again for a message logged meanwhile in the same thread, by the logger say: it writes it to
    // the error output itself.
    const QMutexLocker locker(&mutex);
    if(current != nullptr)
    {
        current->write(type, context, message);
    }
    else
    {
        previous(type, context, message); // Uninstalled meanwhile
    }
}

void MessageLog::write(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    const QString text = qFormatLogMessage(type, context, message);
    writeToOutput(type, context, message, text);
    writeToFile(text);
    if(m_logger)
    {
        m_logger(type, context, message);
    }
}

void MessageLog::writeToOutput(QtMsgType type, const QMessageLogContext& context, const QString& message,
                               const QString& text)
{
    if(!m_console)
    {
        previous(type, context, message);
        return;
    }
    std::fprintf(stderr, "%s\n", text.toLocal8Bit().constData());
    std::fflush(stderr);
}

void MessageLog::writeToFile(const QString& text)
{
    if(m_file.isOpen())
    {
        m_file.write((text + u'\n').toUtf8());
        m_file.flush(); // Whole, even when the application ends on a fatal message
    }
}

} // namespace Nebula
