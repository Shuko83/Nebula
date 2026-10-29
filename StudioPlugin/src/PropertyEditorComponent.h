// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "StudioPartComponent.h"

class EditionZone;
class PropertyEditor;

// The editor of the properties of the component selected in the edition zone it consumes as "zone", see
// PropertyEditor, one of the application of the Studio it consumes as "studio": it cannot do without either, and
// provides the editor as "widget". It needs a QApplication.
class PropertyEditorComponent : public StudioPartComponent
{
    Q_OBJECT
    Q_CLASSINFO("description", "The properties of the component selected in the edition zone of NebulaStudio")
    Q_CLASSINFO("category", "NebulaStudio")

  public:
    PropertyEditorComponent();

  protected:
    [[nodiscard]] std::unique_ptr<QWidget> createPart(Studio& studio) override;

  private:
    // The editor; nullptr while there is no studio.
    [[nodiscard]] PropertyEditor* editor() const;
    // Has the editor show the component selected in the zone, from now on, once it has both.
    void followZone();
    // No longer has the editor follow the zone, about to go: it shows nothing.
    void forgetZone();

    EditionZone* m_zone = nullptr; // Consumed
};
