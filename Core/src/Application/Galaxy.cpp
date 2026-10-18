// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Nebula/Application/Galaxy.h"

#include <QFileInfo>
#include <algorithm>
#include <ranges>

namespace Nebula
{

Galaxy::Galaxy() = default;

Galaxy::~Galaxy() = default;

QString Galaxy::componentId() const
{
    return Id;
}

QString Galaxy::filePath() const
{
    return m_filePath;
}

QString Galaxy::galaxyName() const
{
    return QFileInfo(m_filePath).completeBaseName();
}

QString Galaxy::description() const
{
    return m_description;
}

const QJsonObject& Galaxy::project() const
{
    return m_project;
}

const QStringList& Galaxy::problems() const
{
    return m_problems;
}

const std::vector<std::unique_ptr<Component>>& Galaxy::components() const
{
    return m_components;
}

Component* Galaxy::component(const QString& name) const
{
    const auto found = std::ranges::find(m_components, name, &Component::objectName);
    return found != m_components.end() ? found->get() : nullptr;
}

const std::vector<std::unique_ptr<SettingsFile>>& Galaxy::settingsFiles() const
{
    return m_settingsFiles;
}

bool Galaxy::onStart()
{
    std::vector<Component*> started;
    for(const std::unique_ptr<Component>& component : m_components)
    {
        if(!component->start())
        {
            for(Component* running : started | std::views::reverse)
            {
                running->stop();
            }
            return false;
        }
        started.push_back(component.get());
    }
    return true;
}

void Galaxy::onStop()
{
    for(const std::unique_ptr<Component>& component : m_components | std::views::reverse)
    {
        component->stop(); // Does nothing to one that does not run
    }
}

void Galaxy::builtByPlugin() {}

} // namespace Nebula
