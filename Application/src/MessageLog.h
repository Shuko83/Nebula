// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QFile>
#include <QString>
#include <QStringList>
#include <QtGlobal>
#include <optional>

#include "Nebula/Application/ApplicationOptions.h"

namespace Nebula
{

// Where the messages of the process go while it exists, as the options of a NebulaApplication tell: those less
// severe than their level are left out, by the filter rules of QLoggingCategory, and the others are written to the
// error output, or where Qt writes them, then to the log file and given to the logger, when there are some. Each one
// is written whole before the next, whichever thread logs it. Once it is destroyed, Qt writes them as it did before.
// There is one at a time.
class MessageLog
{
  public:
    // Writes the messages as options tell from now on, the log file being added to; without it, error set, when it
    // cannot be opened.
    MessageLog(const ApplicationOptions& options, QString& error);
    ~MessageLog();

    MessageLog(const MessageLog&) = delete;
    MessageLog& operator=(const MessageLog&) = delete;

    // The level named name, as the command line names it: debug, info, warning, critical or fatal, whatever its case;
    // nullopt when none is.
    [[nodiscard]] static std::optional<QtMsgType> levelNamed(const QString& name);
    // Those names, from the least severe level.
    [[nodiscard]] static QStringList levelNames();

  private:
    // Opens filePath to add the messages to it, unless it is empty; error tells why it cannot be.
    void openFile(const QString& filePath, QString& error);
    // Has the messages written by handle(), unless it would only pass them on to where Qt writes them.
    void install();
    // What Qt calls with each message, from any thread.
    static void handle(QtMsgType type, const QMessageLogContext& context, const QString& message);
    // Writes message, which is not left out, as options tell.
    void write(QtMsgType type, const QMessageLogContext& context, const QString& message);
    // Writes text, message as it is formatted, to the error output when the console is asked for; has Qt write message
    // where it wrote it otherwise.
    void writeToOutput(QtMsgType type, const QMessageLogContext& context, const QString& message, const QString& text);
    // Adds text, a message as it is formatted, to the log file, when there is one.
    void writeToFile(const QString& text);

    bool m_console = false;
    QFile m_file; // Not open when there is none
    Logger m_logger;
    bool m_installed = false;
};

} // namespace Nebula
