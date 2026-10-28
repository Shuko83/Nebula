// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>

#include "MainWindow.h"
#include "SplashScreen.h"
#include "Theme.h"

namespace
{

// Reads what the command line of app gives: the options of Universe, and a project file to open.
void readCommandLine(QCommandLineParser& parser, const QApplication& app)
{
    parser.addHelpOption();
    Nebula::Universe::addCommandLineOptions(parser);
    parser.addPositionalArgument(QStringLiteral("project"),
                                 QCoreApplication::translate("main", "Project file to open."),
                                 QStringLiteral("[project]"));
    parser.process(app);
}

// Shows splash while the plugins are scanned, then until it is read: painted before the scan, a click waits for it to
// end, when read() can be received.
void showSplash(SplashScreen& splash)
{
    splash.show();
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

// Shows window once splash is read, and opens the first of projects, given on the command line, as soon as it shows.
void showOnceRead(SplashScreen& splash, MainWindow& window, const QStringList& projects)
{
    QObject::connect(&splash, &SplashScreen::read, &window,
                     [&splash, &window, projects]
                     {
                         window.show();
                         splash.finish(&window);
                         if(!projects.isEmpty())
                         {
                             window.openProjectFile(projects.constFirst());
                         }
                     });
}

} // namespace

int main(int argc, char* argv[])
{
    const QApplication app(argc, argv);
    applyTheme();                                   // Before the splash screen, which is painted in its colors
    QApplication::setWindowIcon(QIcon(logoPath())); // Of all its windows, and on the taskbar
    QCommandLineParser parser;
    readCommandLine(parser, app);
    SplashScreen splash;
    showSplash(splash);
    MainWindow window(&parser);
    showOnceRead(splash, window, parser.positionalArguments());
    return QApplication::exec();
}
