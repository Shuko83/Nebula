// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QApplication>
#include <QCommandLineParser>
#include <QTextStream>

#include "Nebula/Application/Universe.h"

namespace
{

// Declares what the command line gives: the options of Universe, and the project file to run.
void declareOptions(QCommandLineParser& parser)
{
    parser.setApplicationDescription(QStringLiteral(
        "Runs the application of a project file, as NebulaStudio saved it, until its last window is closed."));
    parser.addHelpOption();
    Nebula::Universe::addCommandLineOptions(parser);
    parser.addPositionalArgument(QStringLiteral("project"), QStringLiteral("Project file to run."));
}

// The project file the command line of app gives, its only argument; exits, telling how to call it, otherwise.
QString projectFile(QCommandLineParser& parser, const QApplication& app)
{
    declareOptions(parser);
    parser.process(app);
    if(parser.positionalArguments().size() != 1)
    {
        parser.showHelp(1);
    }
    return parser.positionalArguments().constFirst();
}

// Opens project in application and starts it, the plugins of the directories of the command line, which parser read,
// found before the project looks for its own. open() loads the plugins of the project, instantiates its components,
// sets their properties and binds their interfaces; start() starts them. Returns false, telling why on the error
// output, when it cannot.
bool start(Nebula::Universe& application, const QCommandLineParser& parser, const QString& project)
{
    application.applyCommandLine(parser);
    if(application.open(project) && application.start())
    {
        return true;
    }
    QTextStream(stderr) << application.errorString() << '\n';
    return false;
}

} // namespace

// ExampleUniverseApp [--plugin-dir <directory>]... [--plugin-root <directory>] <project file>: the application
// NebulaStudio saved there, running until its last window is closed.
int main(int argc, char* argv[])
{
    // A QApplication rather than a QCoreApplication: the components of the project may show widgets.
    const QApplication app(argc, argv);
    QCommandLineParser parser;
    const QString project = projectFile(parser, app);
    Nebula::Universe application; // Which stops its components when it is destroyed
    return start(application, parser, project) ? QApplication::exec() : 1;
}
