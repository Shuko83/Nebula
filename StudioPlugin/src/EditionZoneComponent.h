// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QLabel>
#include <QMenu>
#include <memory>

#include "EditionZone.h"
#include "StudioPartComponent.h"

// The edition zone of NebulaStudio, see EditionZone, built around the Studio it consumes as "studio", whose
// application it shows and edits, and the galaxies placed there: it provides it as "widget", a QWidget, and as "zone",
// for the parts that follow what is selected there or show a component there, a PropertyEditorComponent say. It also
// provides its View menu, as "viewMenu", which zooms it and chooses how the links are drawn, its Run menu, as
// "runMenu", which starts and stops the application, and the label that tells its zoom level, as "zoomLabel": a
// StudioWindowComponent shows them. All of them for as long as it has the studio. It needs a QApplication.
class EditionZoneComponent : public StudioPartComponent
{
    Q_OBJECT
    Q_CLASSINFO("description", "The edition zone of NebulaStudio, with its View and Run menus")
    Q_CLASSINFO("category", "NebulaStudio")

  public:
    EditionZoneComponent();

  protected:
    [[nodiscard]] std::unique_ptr<QWidget> createPart(Studio& studio) override;
    void withdrawPart() override;

  private:
    // Provides the menus and the label of zone, the part just built.
    void provideMenus(EditionZone& zone);

    std::unique_ptr<QMenu> m_viewMenu;
    std::unique_ptr<QMenu> m_runMenu;
    std::unique_ptr<QLabel> m_zoomLabel;
};
