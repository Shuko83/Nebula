// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "PropertyEditorComponent.h"

#include "EditionZone.h"
#include "Nebula/Component/ComponentFactory.h"
#include "PropertyEditor.h"
#include "Studio.h"
#include "StudioPlugin.h"

NEBULA_REGISTER_COMPONENT(StudioPlugin, PropertyEditorComponent);

namespace
{

// Has editor show the component selected in zone, from now on.
void showSelection(const EditionZone& zone, PropertyEditor& editor)
{
    QObject::connect(&zone, &EditionZone::selectionChanged, &editor, &PropertyEditor::setComponent);
    editor.setComponent(zone.selectedComponent());
}

} // namespace

PropertyEditorComponent::PropertyEditorComponent()
{
    consumeInterface<EditionZone>(
        QStringLiteral("zone"), m_zone, Nebula::InterfaceRequirement::Mandatory, [this] { followZone(); },
        [this] { forgetZone(); });
}

std::unique_ptr<QWidget> PropertyEditorComponent::createPart(Studio& studio)
{
    auto editor = std::make_unique<PropertyEditor>(studio.application(), Nebula::ComponentFactory::instance());
    if(m_zone != nullptr)
    {
        showSelection(*m_zone, *editor);
    }
    return editor;
}

PropertyEditor* PropertyEditorComponent::editor() const
{
    return qobject_cast<PropertyEditor*>(part());
}

void PropertyEditorComponent::followZone()
{
    if(PropertyEditor* shown = editor())
    {
        showSelection(*m_zone, *shown);
    }
}

void PropertyEditorComponent::forgetZone()
{
    if(PropertyEditor* shown = editor())
    {
        m_zone->disconnect(shown);
        shown->setComponent(nullptr);
    }
}
