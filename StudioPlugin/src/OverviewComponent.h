// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QAction>
#include <memory>

#include "Nebula/Component/Component.h"
#include "SceneOverview.h"

class EditionZone;
class QMenu;
class Studio;

// The whole scene the edition zone it consumes as "zone" shows, small, see SceneOverview: it cannot do without the
// zone, and provides the overview as "widget" for as long as it has it. Overview, its entry at the end of the menu it
// consumes as "viewMenu", shows it or hides it; the Studio it consumes as "studio", when it has one, keeps that choice
// for its user, from one launch to the next. It needs a QApplication.
class OverviewComponent : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description",
                "The whole scene the edition zone of NebulaStudio shows, small, shown or hidden from the View menu")
    Q_CLASSINFO("category", "NebulaStudio")

  public:
    OverviewComponent();

  protected:
    bool onStart() override;

  private:
    // Declares what it consumes: the studio, the zone and the menu.
    void consumeParts();
    // Builds the overview of the zone, just consumed, and provides it.
    void createOverview();
    // Withdraws the overview, then destroys it: the zone is about to go.
    void destroyOverview();
    // Adds its entry at the end of the menu, just consumed, after a separator.
    void addEntry();
    // Takes its entry out of the menu, about to go.
    void removeEntry();
    // Checks its entry as the studio, just consumed, keeps it, from now on.
    void followStudio();
    // Has the studio keep shown, the overview just shown or hidden from its entry, when it has one.
    void keepChoice(bool shown);
    // Shows or hides the overview as its entry is checked, once it is provided where it is shown.
    void showAsChecked();

    QAction m_separator; // Before its entry
    QAction m_entry;     // Checked while the overview is shown
    std::unique_ptr<SceneOverview> m_overview;
    Studio* m_studio = nullptr;    // Consumed
    EditionZone* m_zone = nullptr; // Consumed
    QMenu* m_viewMenu = nullptr;   // Consumed
};
