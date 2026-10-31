// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QString>
#include <QStringList>
#include <QtGlobal>
#include <functional>

namespace Nebula
{

// What an executable tells of itself, which NebulaApplication gives QCoreApplication: its name, its version, and the
// organization that makes it, with its domain, as CMakeUtils writes them in the <NAME>_info.h of the executable. Each
// one left empty stays as Qt has it: the name of the executable file for its name, none for the others.
struct ExecutableInfo
{
    QString name;
    QString version;
    QString organization; // Part of the paths of QStandardPaths::AppConfigLocation, where settings files are written
    QString organizationDomain;
};

// What a message of the application is given to, as a QtMessageHandler is: its type, where it comes from, its category
// among them, and its text. It is called in the thread that logs the message, one message at a time: it does not wait
// for another thread that logs. A message it logs itself is written by Qt to the error output, not given to it again.
using Logger = std::function<void(QtMsgType type, const QMessageLogContext& context, const QString& message)>;

// How a NebulaApplication finds its galaxy and its plugins, and writes its messages, as its code tells it. Its command
// line, when it reads one, adds to them or replaces them: see NebulaApplication::addCommandLineOptions(). A relative
// directory or file is read from the directory of the executable here, and from the current directory on the command
// line.
struct ApplicationOptions
{
    // Scanned for plugins after plugins/ next to the executable, in their order, then those of --plugin-dir: a plugin
    // found again is skipped, and a directory that does not exist is skipped with a warning.
    QStringList pluginDirectories;
    // The directory the files of the plugins are relative to in a project file, see Universe::setPluginRoot(); the
    // directory of the executable when it is empty. --plugin-root replaces it.
    QString pluginRoot;
    // Where the file of the galaxy is looked for, in their order, after those of --galaxy-dir: the first that has it
    // is the one.
    QStringList galaxyDirectories = {QStringLiteral("scene")};
    ExecutableInfo executable;
    // The least severe messages written: QtDebugMsg writes all those Qt writes, QtInfoMsg leaves the debug ones out,
    // QtWarningMsg the information too, then QtCriticalMsg the warnings and QtFatalMsg the critical ones. The rules of
    // QT_LOGGING_RULES take precedence over it. --log-level replaces it.
    QtMsgType logLevel = QtDebugMsg;
    // Whether the messages are written to the error output rather than where Qt writes them, the debugger of an
    // application without a console of its own on Windows say: that one writes them to the console of the terminal it
    // is started from. --console sets it.
    bool console = false;
    // The file the messages are added to as well; none when it is empty. --log-file replaces it.
    QString logFile;
    // What the messages are given to as well, to be shown in a window, say; none when it is empty.
    Logger logger;
};

} // namespace Nebula
