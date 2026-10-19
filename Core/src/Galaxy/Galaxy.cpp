// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Nebula/Galaxy/Galaxy.h"

#include <QFileInfo>
#include <algorithm>
#include <ranges>
#include <utility>

namespace Nebula
{

Galaxy::Galaxy() = default;

Galaxy::~Galaxy()
{
    // Its stargates no longer give it anything: what they are bound to is about to go with them.
    for(const std::unique_ptr<Component>& component : m_components)
    {
        if(auto* stargate = qobject_cast<Stargate*>(component.get()))
        {
            stargate->detach();
        }
    }
    releaseInterfaces(); // While its components, which what it consumes is passed to, are still there
}

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

QList<ExposedProperty> Galaxy::exposedProperties() const
{
    QList<ExposedProperty> exposed;
    for(const std::unique_ptr<Component>& component : m_components)
    {
        const auto* stargate = qobject_cast<const Stargate*>(component.get());
        if(stargate == nullptr)
        {
            continue;
        }
        for(const ExposedProperty& property : stargate->properties())
        {
            // Where it is: through the galaxy that exposes it in turn.
            const auto* inner = qobject_cast<const Galaxy*>(property.component);
            const std::optional<ExposedProperty> target =
                inner != nullptr ? inner->exposedProperty(property.property) : std::optional(property);
            if(target)
            {
                exposed.append(ExposedProperty{.name = property.name,
                                               .component = target->component,
                                               .object = target->object,
                                               .property = target->property});
            }
        }
    }
    return exposed;
}

std::optional<ExposedProperty> Galaxy::exposedProperty(const QString& name) const
{
    const QList<ExposedProperty> exposed = exposedProperties();
    const auto found = std::ranges::find(exposed, name, &ExposedProperty::name);
    return found != exposed.end() ? std::optional(*found) : std::nullopt;
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

void Galaxy::empty()
{
    // Its stargates no longer give it anything, as when it is destroyed.
    for(const std::unique_ptr<Component>& component : m_components)
    {
        if(auto* stargate = qobject_cast<Stargate*>(component.get()))
        {
            stargate->detach();
        }
    }
    // From copies of the lists: each removal tells what was bound to it, which can change them.
    const QList<InterfaceInfo> provided = providedInterfaces();
    for(const InterfaceInfo& info : provided)
    {
        removeProvidedInterface(info.name);
    }
    const QList<InterfaceInfo> consumed = consumedInterfaces();
    for(const InterfaceInfo& info : consumed)
    {
        removeConsumedInterface(info.name);
    }
    m_settingsFiles.clear(); // Before the components their entries are of
    // Out of components() before they are destroyed, as Universe::clear() does.
    const std::vector<std::unique_ptr<Component>> removed = std::exchange(m_components, {});
}

} // namespace Nebula
