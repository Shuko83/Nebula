// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Nebula/Galaxy/Galaxy.h"

#include <QFileInfo>
#include <algorithm>
#include <utility>

#include "Component/Components.h"

namespace Nebula
{

namespace
{

// The properties the stargates among components expose, as they expose them: maybe through a galaxy inside.
QList<ExposedProperty> stargateProperties(ComponentSpan components)
{
    QList<ExposedProperty> properties;
    for(const Stargate* stargate : componentsOfType<const Stargate>(components))
    {
        properties.append(stargate->properties());
    }
    return properties;
}

// Where property, exposed by a stargate, is: through the galaxy that exposes it in turn; nullopt when that galaxy no
// longer exposes it.
std::optional<ExposedProperty> resolve(const ExposedProperty& property)
{
    const auto* inner = qobject_cast<const Galaxy*>(property.component);
    std::optional<ExposedProperty> target = inner != nullptr ? inner->exposedProperty(property.property) : property;
    if(target)
    {
        target->name = property.name;
    }
    return target;
}

} // namespace

Galaxy::Galaxy() = default;

Galaxy::~Galaxy()
{
    detachStargates();
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
    for(const ExposedProperty& property : stargateProperties(m_components))
    {
        if(const std::optional<ExposedProperty> target = resolve(property))
        {
            exposed.append(*target);
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
    return startInOrder(m_components) == nullptr;
}

void Galaxy::onStop()
{
    stopInReverse(m_components);
}

void Galaxy::builtByPlugin() {}

void Galaxy::empty()
{
    detachStargates(); // As when it is destroyed
    removeInterfaces();
    m_settingsFiles.clear(); // Before the components their entries are of
    // Out of components() before they are destroyed, as Universe::clear() does.
    const std::vector<std::unique_ptr<Component>> removed = std::exchange(m_components, {});
}

void Galaxy::detachStargates()
{
    for(Stargate* stargate : componentsOfType<Stargate>(m_components))
    {
        stargate->detach();
    }
}

void Galaxy::removeInterfaces()
{
    const QList<InterfaceInfo> provided = providedInterfaces(); // Copies: each removal can change the lists
    for(const InterfaceInfo& info : provided)
    {
        removeProvidedInterface(info.name);
    }
    const QList<InterfaceInfo> consumed = consumedInterfaces();
    for(const InterfaceInfo& info : consumed)
    {
        removeConsumedInterface(info.name);
    }
}

} // namespace Nebula
