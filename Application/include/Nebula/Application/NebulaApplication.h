// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QApplication>
#include <QString>
#include <QStringList>
#include <memory>

#include "Application_export.h"
#include "Nebula/Application/ApplicationOptions.h"
#include "Nebula/Application/Universe.h"

class QCommandLineParser;

namespace Nebula
{

class MessageLog;

// The application of an executable whose content is a galaxy, a project file NebulaStudio saved: it loads it as it is
// built, and runs it with exec(). The main of the executable is then no more than:
//
//     QCommandLineParser parser;
//     Nebula::NebulaApplication app(argc, argv, parser, QStringLiteral("Clock.nbp"));
//     return app.exec();
//
// Built, it gives QCoreApplication what the executable tells of itself, writes the messages as it is asked to, lists
// the plugins of plugins/ next to the executable and of the directories it is given, then opens the galaxy, found in
// the directories it is given, scene/ next to the executable unless told otherwise: its plugins are loaded, its
// components instantiated, set and bound, see Universe::open(). What could not be done is logged as critical, told by
// errorString(), and exec() returns at once. ApplicationOptions tell how, and so does its command line when it is
// given a parser, see addCommandLineOptions().
// It is the QApplication of the process: there is one at a time.
class APPLICATION_EXPORT NebulaApplication : public QApplication
{
    Q_OBJECT

  public:
    // What exec() returns when the galaxy does not run: it was not loaded, or it refused to start.
    static constexpr int LoadFailed = 1;
    static constexpr int StartFailed = 2;

    // Loads galaxy, the name of its file or its path, as options tell; the command line is not read.
    NebulaApplication(int& argc, char** argv, const QString& galaxy, const ApplicationOptions& options = {});
    // Loads galaxy as options tell, and as the command line tells, which parser processes once
    // addCommandLineOptions() added to it the options of the application, after those the main declared: --help and
    // --version show what they show and exit, and an option none declared is refused. The main reads its own options
    // and arguments from parser then.
    NebulaApplication(int& argc, char** argv, QCommandLineParser& parser, const QString& galaxy,
                      ApplicationOptions options = {});
    // Stops its galaxy and destroys it, then leaves the messages to Qt again.
    ~NebulaApplication() override;

    // Adds to parser the options of the application, each one adding to what ApplicationOptions tell or replacing it:
    // --help, --version, --plugin-dir <directory>, as many times as needed, and --plugin-root <directory>, see
    // Universe::addCommandLineOptions(), --galaxy-dir <directory>, as many times as needed, --log-level <level>,
    // debug, info, warning, critical or fatal, --log-file <file> and --console. The constructor adds them itself.
    static void addCommandLineOptions(QCommandLineParser& parser);

    // What its galaxy is loaded in and runs in: its components, by their names, say.
    [[nodiscard]] Universe& universe();
    [[nodiscard]] const Universe& universe() const;
    // The file of its galaxy, absolute; empty when it was not found.
    [[nodiscard]] QString galaxyFile() const;
    // Whether its galaxy is loaded, all of it: exec() runs it then.
    [[nodiscard]] bool isLoaded() const;
    // Why its galaxy was not loaded, or refused to start; empty while neither happened.
    [[nodiscard]] QString errorString() const;

    // Starts its galaxy, processes the events until the application quits, see QCoreApplication::exit(), then stops
    // it, which has its settings files written. Returns the code the application quit with; LoadFailed, at once, when
    // its galaxy is not loaded, and StartFailed when it refuses to start, which is logged. QCoreApplication::exec(),
    // which it hides, runs nothing.
    int exec();

  private:
    // Gives QCoreApplication what info tells of the executable, each part that is not empty.
    static void setExecutableInfo(const ExecutableInfo& info);
    // Adds the options of the application to parser, then processes the command line with it.
    void processCommandLine(QCommandLineParser& parser);
    // Adds to options what the command line, processed by parser, adds to them, or replaces what they tell with it.
    void readCommandLine(const QCommandLineParser& parser, ApplicationOptions& options);
    // readCommandLine() of the options of the messages.
    void readLogOptions(const QCommandLineParser& parser, ApplicationOptions& options);
    // Sets the log level of options to the one named name; refuses a name it does not know.
    void readLogLevel(const QString& name, ApplicationOptions& options);
    // Writes the messages as options tell, then, unless something is refused already, loads galaxy as they tell, the
    // plugin directories of commandLine, when there is one, scanned after theirs.
    void load(const QString& galaxy, const ApplicationOptions& options, const QCommandLineParser* commandLine);
    // Writes the messages as options tell from now on; refuses a log file that cannot be opened.
    void startLog(ApplicationOptions options);
    // Lists the plugins of the directories of options, then of those of commandLine, and sets the plugin root.
    void addPlugins(const ApplicationOptions& options, const QCommandLineParser* commandLine);
    // Opens galaxy, found in the first of directories that has it; false, refused, when it cannot.
    bool openGalaxy(const QString& galaxy, const QStringList& directories);
    // Starts the galaxy; false, refused, when it refuses to start.
    bool startGalaxy();
    // Sets errorString() to why, logs it as critical, and returns false.
    bool refuse(const QString& why);

    std::unique_ptr<MessageLog> m_log;    // Before m_universe: what it logs as it is destroyed is written as asked
    std::unique_ptr<Universe> m_universe; // Built once the messages are written as asked: it lists its plugins then
    QString m_galaxyFile;
    bool m_loaded = false;
    QString m_errorString;
};

} // namespace Nebula
