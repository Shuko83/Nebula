// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Nebula/Component/Component.h"
#include "Studio.h"

// What the parts of NebulaStudio share, see Studio: the application they assemble, with the plugins of plugins/ next
// to the application that runs it, and of the directories its command line gives with --plugin-dir, the preferences
// of the user of NebulaStudio, and what the parts tell one another. It provides it as the interface "studio", at once,
// to each part, a PluginListComponent or an EditionZoneComponent say. The first one created gives the application
// that runs it the theme of NebulaStudio; stopping it stops the application it assembles, when it runs. It needs a
// QApplication.
class StudioComponent : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "What the parts of NebulaStudio share: the application they assemble, and its plugins")
    Q_CLASSINFO("category", "NebulaStudio")

  public:
    StudioComponent();

  protected:
    void onStop() override;

  private:
    // Lists the plugins of the directories the command line of the application that runs it gives, and sets the
    // plugin root it gives: the options of Universe::addCommandLineOptions(), its others left out.
    void applyCommandLine();

    Studio m_studio;
};
