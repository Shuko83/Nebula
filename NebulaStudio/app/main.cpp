// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QCommandLineParser>
#include <QFileInfo>
#include <QIcon>
#include <QMessageBox>

#include "Nebula/Application/NebulaApplication.h"
#include "NebulaStudio_info.h"
#include "SplashScreen.h"
#include "Theme.h"

namespace
{

// The galaxy NebulaStudio runs, which assembles its window from the components of StudioPlugin: in scene/ next to it,
// where each build copies the one of StudioPlugin/.
const QString Galaxy = QStringLiteral("NebulaStudio.nbp");

// What NebulaStudio tells of itself, its name and its version, which --version shows; not its organization, which would
// move the AppConfigLocation the applications run in it write their settings files to.
Nebula::ApplicationOptions options()
{
    return {.executable = {.name = QString::fromUtf8(NebulaStudioInfo::name),
                           .version = QString::fromUtf8(NebulaStudioInfo::version)}};
}

// Declares what the command line of NebulaStudio gives beyond the options of NebulaApplication: a project file to open.
void declareProject(QCommandLineParser& parser)
{
    parser.setApplicationDescription(
        QCoreApplication::translate("main", "Assembles applications from the components of the plugins, live."));
    parser.addPositionalArgument(QStringLiteral("project"),
                                 QCoreApplication::translate("main", "Project file to open."),
                                 QStringLiteral("[project]"));
}

// Has the galaxy of app open the first of projects, given on the command line, as it starts: the file of its Project.
void openFirst(Nebula::NebulaApplication& app, const QStringList& projects)
{
    Nebula::Component* project = app.universe().component(QStringLiteral("Project"));
    if(project != nullptr && !projects.isEmpty())
    {
        project->setProperty("file", QFileInfo(projects.constFirst()).absoluteFilePath());
    }
}

// Runs the galaxy of app, the splash screen shown over its window until it is read. Returns what app.exec() returns.
int runWithSplash(Nebula::NebulaApplication& app)
{
    SplashScreen splash;
    QObject::connect(&splash, &SplashScreen::read, &splash, &QWidget::close);
    splash.show();
    const int code = app.exec();
    splash.close(); // Before a box tells why the galaxy did not run
    return code;
}

// Tells in a box why the galaxy of app did not run, when it did not, then returns code, the one app.exec() returned.
int tellFailure(const Nebula::NebulaApplication& app, int code)
{
    if(!app.errorString().isEmpty())
    {
        QMessageBox::critical(nullptr, QCoreApplication::translate("main", "NebulaStudio"), app.errorString());
    }
    return code;
}

} // namespace

// NebulaStudio [options of NebulaApplication] [project]: its galaxy, whose window opens the project given, if any.
int main(int argc, char* argv[])
{
    QCommandLineParser parser;
    declareProject(parser);
    Nebula::NebulaApplication app(argc, argv, parser, Galaxy, options()); // Its StudioComponent applies the theme
    QApplication::setWindowIcon(QIcon(logoPath()));                       // Of all its windows, and on the taskbar
    openFirst(app, parser.positionalArguments());
    return tellFailure(app, app.isLoaded() ? runWithSplash(app) : app.exec());
}
