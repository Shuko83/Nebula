// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Nebula/Application/NebulaApplication.h"
#include "NebulaStudio_info.h"
#include "SplashScreen.h"
#include "Theme.h"

namespace
{

// The galaxy NebulaStudio runs, which assembles its window from the components of StudioPlugin: in scene/ next to it,
// where each build copies the one of StudioPlugin/.
const QString Galaxy = QStringLiteral("NebulaStudio.nbp");

// What NebulaStudio tells of itself: its name and its version, which --version shows, not its organization, which
// would move the AppConfigLocation the applications run in it write their settings files to. Then what its command
// line gives beyond the options of NebulaApplication, the project its Project opens as it starts, and a box that
// tells why its galaxy does not run.
Nebula::ApplicationOptions options()
{
    return {.executable = {.name = QString::fromUtf8(NebulaStudioInfo::name),
                           .version = QString::fromUtf8(NebulaStudioInfo::version),
                           .description = QCoreApplication::translate(
                               "main", "Assembles applications from the components of the plugins, live."),
                           .icon = logoPath()},
            .arguments = {{.name = QStringLiteral("project"),
                           .description = QCoreApplication::translate("main", "Project file to open."),
                           .instance = QStringLiteral("Project"),
                           .property = QStringLiteral("file"),
                           .path = true}},
            .failureBox = true};
}

// Shows splash over the window the galaxy of app shows as it starts, until it is read; not at all when the galaxy is
// not loaded, and no longer when it refuses to start: a box tells why then.
void showUntilRead(SplashScreen& splash, const Nebula::NebulaApplication& app)
{
    QObject::connect(&splash, &SplashScreen::read, &splash, &QWidget::close);
    QObject::connect(&app, &Nebula::NebulaApplication::failed, &splash, &QWidget::close);
    splash.setVisible(app.isLoaded());
}

} // namespace

// NebulaStudio [options of NebulaApplication] [project]: its galaxy, whose window opens the project given, if any.
int main(int argc, char* argv[])
{
    Nebula::NebulaApplication app(argc, argv, Galaxy, options()); // Its StudioComponent applies the theme
    SplashScreen splash;                                          // In the colors of that theme
    showUntilRead(splash, app);
    return app.exec();
}
