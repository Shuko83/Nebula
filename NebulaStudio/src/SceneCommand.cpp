// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "SceneCommand.h"

#include <utility>

#include "EditionScene.h"

SceneCommand::SceneCommand(EditionScene& scene, const QString& label, QJsonObject before, QJsonObject after)
    : Command(label), m_scene(&scene), m_before(std::move(before)), m_after(std::move(after))
{
}

void SceneCommand::execute()
{
    if(m_scene != nullptr)
    {
        m_scene->restore(m_after, tr("Redone: %1").arg(label()));
    }
}

void SceneCommand::undo()
{
    if(m_scene != nullptr)
    {
        m_scene->restore(m_before, tr("Undone: %1").arg(label()));
    }
}

bool SceneCommand::isObsolete() const
{
    return m_scene == nullptr;
}
