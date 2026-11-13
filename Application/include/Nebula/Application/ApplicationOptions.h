// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
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
    QString description; // What it is for, which --help shows
    QString icon;        // The file of the icon of its windows, a resource say; relative, from its directory
};

// An argument of the command line of an application, which a NebulaApplication gives to a property of an instance of
// its galaxy once it is loaded: the file a window opens as it starts, say. --help shows it as [name], with its
// description: none has to be given, and the first argument given is the first one declared.
struct CommandLineArgument
{
    QString name;
    QString description;
    QString instance;  // The instance of the galaxy it is given to, by its name
    QString property;  // The property of that instance it is given to, converted to its type
    bool path = false; // Whether it is a file or a directory: given absolute then, read from the current directory
};

// What a message of the application is given to, as a QtMessageHandler is: its type, where it comes from, its category
// among them, and its text. It is called in the thread that logs the message, one message at a time: it does not wait
// for another thread that logs. A message it logs itself is written by Qt to the error output, not given to it again.
using Logger = std::function<void(QtMsgType type, const QMessageLogContext& context, const QString& message)>;

// How a NebulaApplication finds its galaxy and its plugins, reads its command line, writes its messages and tells what
// fails, as its code tells it. Its command line adds to them or replaces them: see
// NebulaApplication::addCommandLineOptions(). A relative directory or file is read from the directory of the
// executable here, and from the current directory on the command line.
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
    // Whether the command line is read, with the options of the application and its arguments: an option none
    // declared is refused then. An application that reads its command line all by itself sets it to false; one that
    // only adds options of its own gives its QCommandLineParser to the application instead.
    bool commandLine = true;
    // What the command line gives the instances of the galaxy, in the order they are given in.
    QList<CommandLineArgument> arguments;
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
    // Whether exec() tells in a box why the galaxy does not run, which is only logged otherwise: for an application
    // with windows, whose messages nobody reads.
    bool failureBox = false;
};

} // namespace Nebula
