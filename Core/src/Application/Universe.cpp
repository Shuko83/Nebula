// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Nebula/Application/Universe.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QMetaProperty>
#include <QSignalBlocker>
#include <algorithm>
#include <map>
#include <ranges>
#include <utility>

#include "Nebula/Component/ComponentFactory.h"
#include "ProjectJson.h"

namespace Nebula
{

namespace
{

using ProjectJson::writableProperties;
using ProjectJson::writeProperty;

Q_LOGGING_CATEGORY(lcPlugins, "nebula.plugins")

// The version of the projects written, the only one read.
constexpr int ProjectVersion = 1;

// The names of the options addCommandLineOptions() adds.
const QString PluginDirOption = QStringLiteral("plugin-dir");
const QString PluginRootOption = QStringLiteral("plugin-root");

// What the stargates are named after, with a number: Stargate1.
const QString StargateName = QStringLiteral("Stargate");

// The key under which a stargate of a project lists the interfaces it exposes as exposure tells.
QString exposureKey(Exposure exposure)
{
    return exposure == Exposure::Provided ? QStringLiteral("provided") : QStringLiteral("consumed");
}

// The property name of object; not valid when it has none of that name.
QMetaProperty metaProperty(const QObject& object, const QString& name)
{
    const QMetaObject* metaObject = object.metaObject();
    return metaObject->property(metaObject->indexOfProperty(name.toLatin1().constData()));
}

// entry as a project holds it: the name of its component, the key of its object when it is not the component, see
// ProjectJson::ObjectKey, its "index" only when it is not the first object of its name, and its property.
QJsonObject entryJson(const SettingsEntry& entry)
{
    QJsonObject json;
    json.insert(u"component", entry.component->objectName());
    if(const std::optional<ProjectJson::ObjectKey> key = ProjectJson::objectKey(*entry.component, entry.object))
    {
        json.insert(u"object", key->name);
        if(key->index > 0)
        {
            json.insert(u"index", key->index);
        }
    }
    json.insert(u"property", entry.property);
    return json;
}

// What project, read from the file filePath, tells of its galaxy.
GalaxyInfo galaxyInfo(const QString& filePath, const QJsonObject& project)
{
    GalaxyInfo galaxy{.name = QFileInfo(filePath).completeBaseName(),
                      .description = project.value(u"description").toString(),
                      .plugins = {},
                      .filePath = filePath};
    const QJsonArray plugins = project.value(u"plugins").toArray();
    for(const QJsonValue& plugin : plugins)
    {
        galaxy.plugins.append(plugin.toObject().value(u"id").toString());
    }
    return galaxy;
}

// Adds the settings files of galaxy, then those of the galaxies it holds, to files.
void collectSettingsFiles(const Galaxy& galaxy, QList<SettingsFile*>& files)
{
    for(const std::unique_ptr<SettingsFile>& file : galaxy.settingsFiles())
    {
        files.append(file.get());
    }
    for(const std::unique_ptr<Component>& component : galaxy.components())
    {
        if(const auto* inner = qobject_cast<const Galaxy*>(component.get()))
        {
            collectSettingsFiles(*inner, files);
        }
    }
}

// Whether galaxy, or a galaxy it holds, has settings files.
bool hasSettingsFiles(const Galaxy& galaxy)
{
    QList<SettingsFile*> files;
    collectSettingsFiles(galaxy, files);
    return !files.isEmpty();
}

// Appends to path the galaxies of components down to the one that holds instance, directly; false, leaving path as it
// was, when none does.
bool findPath(const std::vector<std::unique_ptr<Component>>& components, const Component& instance,
              QList<Galaxy*>& path)
{
    if(std::ranges::any_of(components, [&instance](const std::unique_ptr<Component>& component)
                           { return component.get() == &instance; }))
    {
        return true;
    }
    for(const std::unique_ptr<Component>& component : components)
    {
        if(auto* galaxy = qobject_cast<Galaxy*>(component.get()))
        {
            path.append(galaxy);
            if(findPath(galaxy->components(), instance, path))
            {
                return true;
            }
            path.removeLast();
        }
    }
    return false;
}

// Adds the galaxies of components of the file filePath, canonical, and those of the galaxies they hold, to found.
void collectGalaxies(const std::vector<std::unique_ptr<Component>>& components, const QString& filePath,
                     QList<Galaxy*>& found)
{
    for(const std::unique_ptr<Component>& component : components)
    {
        if(auto* galaxy = qobject_cast<Galaxy*>(component.get()))
        {
            if(galaxy->filePath() == filePath)
            {
                found.append(galaxy);
            }
            collectGalaxies(galaxy->components(), filePath, found);
        }
    }
}

// fileName with number before its suffix: settings2.json for settings.json.
QString numbered(const QString& fileName, int number)
{
    const QFileInfo info(fileName);
    const QString suffix = info.suffix();
    return info.completeBaseName() + QString::number(number) + (suffix.isEmpty() ? QString() : u'.' + suffix);
}

// Whether component is one of the plugin pluginId or, a galaxy, holds one, directly or not.
bool isOf(const Component& component, const QString& pluginId)
{
    if(const auto* galaxy = qobject_cast<const Galaxy*>(&component))
    {
        return std::ranges::any_of(galaxy->components(), [&pluginId](const std::unique_ptr<Component>& inner)
                                   { return isOf(*inner, pluginId); });
    }
    const std::optional<ComponentInfo> info = ComponentFactory::instance().component(component.componentId());
    return info && info->pluginId == pluginId;
}

} // namespace

Universe::Universe(QObject* parent) : QObject(parent), m_pluginRoot(QCoreApplication::applicationDirPath())
{
    m_plugins.scan();
}

Universe::~Universe()
{
    const QSignalBlocker blocker(this); // Not told to anyone: it is going
    clear();
}

PluginManager& Universe::plugins()
{
    return m_plugins;
}

const PluginManager& Universe::plugins() const
{
    return m_plugins;
}

QString Universe::pluginRoot() const
{
    return m_pluginRoot.absolutePath();
}

void Universe::setPluginRoot(const QString& directory)
{
    m_pluginRoot.setPath(directory);
}

void Universe::addCommandLineOptions(QCommandLineParser& parser)
{
    parser.addOption(
        {PluginDirOption,
         QStringLiteral("Directory of plugins to scan, after plugins/ next to the application. Can be given "
                        "several times: the directories are scanned in their order, and a plugin found "
                        "again is skipped."),
         QStringLiteral("directory")});
    parser.addOption({PluginRootOption,
                      QStringLiteral("Directory the files of the plugins are relative to in a project file (default: "
                                     "the directory of the application)."),
                      QStringLiteral("directory")});
}

void Universe::applyCommandLine(const QCommandLineParser& parser)
{
    for(const QString& directory : parser.values(PluginDirOption))
    {
        if(!QFileInfo(directory).isDir())
        {
            qCWarning(lcPlugins).noquote()
                << "Skipping" << QDir::toNativeSeparators(directory) << "- it is not a directory";
            continue;
        }
        m_plugins.scan(directory);
    }
    if(parser.isSet(PluginRootOption))
    {
        setPluginRoot(parser.value(PluginRootOption));
    }
}

std::optional<QJsonObject> Universe::read(const QString& filePath, QString& error)
{
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
    {
        error = file.errorString();
        return std::nullopt;
    }
    QJsonParseError parsed;
    QJsonObject project = QJsonDocument::fromJson(file.readAll(), &parsed).object();
    if(parsed.error != QJsonParseError::NoError)
    {
        error = parsed.errorString();
        return std::nullopt;
    }
    if(project.value(u"version").toInt() != ProjectVersion)
    {
        error = QStringLiteral("It is not a Nebula project of version %1").arg(ProjectVersion);
        return std::nullopt;
    }
    return project;
}

std::optional<GalaxyInfo> Universe::readGalaxy(const QString& filePath, QString& error)
{
    const std::optional<QJsonObject> project = read(filePath, error);
    if(!project)
    {
        return std::nullopt;
    }
    return galaxyInfo(filePath, *project);
}

QStringList Universe::missingPlugins(const GalaxyInfo& galaxy) const
{
    QStringList missing;
    for(const QString& pluginId : galaxy.plugins)
    {
        if(!m_plugins.plugin(pluginId) && !missing.contains(pluginId))
        {
            missing.append(pluginId);
        }
    }
    return missing;
}

QStringList Universe::load(const QJsonObject& project)
{
    clear();
    m_description = project.value(u"description").toString();
    QStringList problems;

    const QJsonArray plugins = project.value(u"plugins").toArray();
    for(const QJsonValue& value : plugins)
    {
        const QJsonObject plugin = value.toObject();
        const QString pluginId = plugin.value(u"id").toString();
        // Where its file is from the plugin root; as it is when it is not relative.
        const QString filePath = QDir::cleanPath(m_pluginRoot.absoluteFilePath(plugin.value(u"file").toString()));
        // Opened from its file when it is not found already, as the ones of plugins/ are.
        if((!m_plugins.plugin(pluginId) && !m_plugins.addFile(filePath)) ||
           (!m_plugins.isLoaded(pluginId) && !m_plugins.load(pluginId)))
        {
            problems.append(QStringLiteral("The plugin %1 is not loaded: %2").arg(pluginId, m_plugins.errorString()));
        }
    }
    problems.append(loadScope(applicationScope(), project));
    return problems;
}

QStringList Universe::loadScope(const Scope& scope, const QJsonObject& project)
{
    QStringList problems;
    QList<std::pair<Stargate*, QJsonObject>> stargates; // What each of them exposes, once every component is there
    const QJsonArray components = project.value(u"components").toArray();
    for(const QJsonValue& value : components)
    {
        const QJsonObject json = value.toObject();
        const QString name = json.value(u"name").toString();
        const QString componentId = json.value(u"type").toString();
        if(componentId == Galaxy::Id)
        {
            problems.append(loadGalaxy(scope, json));
            continue;
        }
        if(componentId == Stargate::Id)
        {
            // Not made with std::make_unique: its constructor is only for the application.
            Component* stargate =
                adopt(scope, std::unique_ptr<Component>(new Stargate(*this, scope.galaxy)), name, StargateName);
            stargates.append({static_cast<Stargate*>(stargate), json});
            continue;
        }
        const std::optional<ComponentInfo> info = ComponentFactory::instance().component(componentId);
        Component* instance =
            info ? adopt(scope, ComponentFactory::instance().create(componentId), name, info->name) : nullptr;
        if(instance == nullptr)
        {
            problems.append(QStringLiteral("%1 is missing: no loaded plugin provides %2").arg(name, componentId));
            continue;
        }
        const QJsonObject properties = json.value(u"properties").toObject();
        for(auto property = properties.begin(); property != properties.end(); ++property)
        {
            if(!writeProperty(*instance, info->properties, property.key(), property.value()))
            {
                problems.append(QStringLiteral("The property %1 of %2 could not be set").arg(property.key(), name));
            }
        }

        // Each to the first object of its name that none was given yet: those of a name are held in the order they
        // were added.
        QList<PropertyObject> remaining = instance->propertyObjects();
        const QJsonArray objects = json.value(u"objects").toArray();
        for(const QJsonValue& objectValue : objects)
        {
            const QJsonObject objectJson = objectValue.toObject();
            const QString objectName = objectJson.value(u"name").toString();
            const auto found = std::ranges::find(remaining, objectName, &PropertyObject::name);
            if(found == remaining.end())
            {
                problems.append(QStringLiteral("The object %1 of %2 is missing").arg(objectName, name));
                continue;
            }
            const PropertyObject restored = *found;
            remaining.erase(found);
            const QJsonObject objectProperties = objectJson.value(u"properties").toObject();
            for(auto property = objectProperties.begin(); property != objectProperties.end(); ++property)
            {
                if(!writeProperty(*restored.object, restored.properties(), property.key(), property.value()))
                {
                    problems.append(QStringLiteral("The property %1 of the object %2 of %3 could not be set")
                                        .arg(property.key(), objectName, name));
                }
            }
        }
    }
    // Of the types of the components, which can come after them; before the bindings, which bind to them.
    for(const auto& [stargate, json] : std::as_const(stargates))
    {
        problems.append(loadStargateInterfaces(scope, *stargate, json));
    }

    const QJsonArray bindings = project.value(u"bindings").toArray();
    for(const QJsonValue& value : bindings)
    {
        const QJsonObject json = value.toObject();
        const QString consumerName = json.value(u"consumer").toString();
        const QString consumedName = json.value(u"consumed").toString();
        const QString providerName = json.value(u"provider").toString();
        const QString providedName = json.value(u"provided").toString();
        Component* consumer = scopeComponent(scope, consumerName);
        Component* provider = scopeComponent(scope, providerName);
        if(consumer == nullptr || provider == nullptr ||
           !consumer->bindInterface(consumedName, *provider, providedName))
        {
            problems.append(QStringLiteral("%1 of %2 could not be bound to %3 of %4")
                                .arg(consumedName, consumerName, providedName, providerName));
        }
    }
    for(const auto& [stargate, json] : std::as_const(stargates))
    {
        problems.append(loadStargateProperties(scope, *stargate, json));
    }

    const QJsonArray settings = project.value(u"settings").toArray();
    for(const QJsonValue& value : settings)
    {
        const QJsonObject json = value.toObject();
        SettingsFile& file = addSettingsFile(scope, json.value(u"file").toString(), problems);
        file.setDirectory(json.value(u"directory").toString());
        const QJsonArray entries = json.value(u"entries").toArray();
        for(const QJsonValue& entryValue : entries)
        {
            QString owner;
            const SettingsEntry entry = readEntry(scope, entryValue.toObject(), owner);
            if(entry.object == nullptr || !file.add(entry))
            {
                problems.append(QStringLiteral("The property %1 of %2 is not held by %3")
                                    .arg(entry.property, owner, file.fileName()));
            }
        }
    }
    return problems;
}

QStringList Universe::loadStargateInterfaces(const Scope& scope, Stargate& stargate, const QJsonObject& json) const
{
    QStringList problems;
    for(const Exposure exposure : {Exposure::Provided, Exposure::Consumed})
    {
        const QJsonArray interfaces = json.value(exposureKey(exposure)).toArray();
        for(const QJsonValue& value : interfaces)
        {
            const QJsonObject interfaceJson = value.toObject();
            const QString name = interfaceJson.value(u"name").toString();
            const QString typeName = interfaceJson.value(u"type").toString();
            const QMetaType type = interfaceType(scope, typeName);
            if(!type.isValid())
            {
                problems.append(QStringLiteral("The interface %1 of %2 is not exposed: no component has its type %3")
                                    .arg(name, stargate.objectName(), typeName));
            }
            else if(!stargate.exposeInterface(exposure, name, type))
            {
                problems.append(
                    QStringLiteral("The interface %1 of %2 is not exposed: another one is exposed under its name")
                        .arg(name, stargate.objectName()));
            }
        }
    }
    return problems;
}

QStringList Universe::loadStargateProperties(const Scope& scope, Stargate& stargate, const QJsonObject& json) const
{
    QStringList problems;
    const QJsonArray properties = json.value(u"exposed").toArray();
    for(const QJsonValue& value : properties)
    {
        const QJsonObject propertyJson = value.toObject();
        QString owner;
        const SettingsEntry entry = readEntry(scope, propertyJson, owner);
        const QString name = propertyJson.value(u"name").toString();
        if(entry.object == nullptr ||
           !stargate.exposeProperty(
               {.name = name, .component = entry.component, .object = entry.object, .property = entry.property}))
        {
            problems.append(QStringLiteral("The property %1 of %2 is not exposed by %3 as %4")
                                .arg(entry.property, owner, stargate.objectName(), name));
        }
    }
    return problems;
}

SettingsEntry Universe::readEntry(const Scope& scope, const QJsonObject& json, QString& owner) const
{
    owner = json.value(u"component").toString(); // What the property is of, as it is told
    Component* instance = scopeComponent(scope, owner);
    QObject* object = instance;
    if(json.contains(u"object"))
    {
        const ProjectJson::ObjectKey key{.name = json.value(u"object").toString(),
                                         .index = json.value(u"index").toInteger()};
        object = instance != nullptr ? ProjectJson::object(*instance, key) : nullptr;
        owner = QStringLiteral("the object %1 of %2").arg(key.name, owner);
    }
    return {.component = instance, .object = object, .property = json.value(u"property").toString()};
}

QMetaType Universe::interfaceType(const Scope& scope, const QString& typeName) const
{
    // Those of the components first: the type of an interface of a plugin is not registered by its name until it is.
    const std::vector<std::unique_ptr<Component>>& components =
        scope.galaxy != nullptr ? scope.galaxy->components() : m_components;
    for(const std::unique_ptr<Component>& component : components)
    {
        const QList<InterfaceInfo> interfaces = component->providedInterfaces() + component->consumedInterfaces();
        const auto found = std::ranges::find_if(interfaces, [&typeName](const InterfaceInfo& info)
                                                { return QLatin1StringView(info.type.name()) == typeName; });
        if(found != interfaces.end())
        {
            return found->type;
        }
    }
    return QMetaType::fromName(typeName.toLatin1());
}

QStringList Universe::loadGalaxy(const Scope& scope, const QJsonObject& json)
{
    const QString name = json.value(u"name").toString();
    // Where its file is from the directory of the project; as it is when it is not relative.
    const QString filePath = QDir::cleanPath(scope.directory.absoluteFilePath(json.value(u"galaxy").toString()));
    QString error;
    std::unique_ptr<Galaxy> galaxy = createGalaxy(filePath, name, scope, error);
    if(galaxy == nullptr)
    {
        return {QStringLiteral("%1 is missing: %2").arg(name, error)};
    }
    QStringList problems;
    for(const QString& problem : galaxy->problems())
    {
        problems.append(QStringLiteral("In %1: %2").arg(galaxy->objectName(), problem));
    }
    // The values of this instance, in place of those of its file.
    setExposedValues(*galaxy, json.value(u"properties").toObject(), problems);
    const QString placedName = galaxy->objectName();
    adopt(scope, std::move(galaxy), placedName, placedName);
    return problems;
}

void Universe::setExposedValues(const Galaxy& galaxy, const QJsonObject& values, QStringList& problems)
{
    const QList<ExposedProperty> exposed = galaxy.exposedProperties();
    for(auto value = values.begin(); value != values.end(); ++value)
    {
        const auto found = std::ranges::find(exposed, value.key(), &ExposedProperty::name);
        if(found == exposed.end() || found->object == nullptr ||
           !metaProperty(*found->object, found->property).write(found->object, value.value().toVariant()))
        {
            problems.append(
                QStringLiteral("The property %1 of %2 could not be set").arg(value.key(), galaxy.objectName()));
        }
    }
}

std::optional<QJsonObject> Universe::readGalaxyProject(const QString& filePath, const Scope& parent, QString& error)
{
    const QString nativePath = QDir::toNativeSeparators(filePath);
    std::optional<QJsonObject> project = read(filePath, error);
    if(!project)
    {
        error = QStringLiteral("%1 cannot be read: %2").arg(nativePath, error);
        return std::nullopt;
    }
    // Its own file, as the files of the galaxies it is placed in are: a link to it is the same file.
    const QString canonicalPath = QFileInfo(filePath).canonicalFilePath();
    if(parent.files.contains(canonicalPath))
    {
        error = QStringLiteral("%1 would hold itself, directly or not").arg(nativePath);
        if(parent.cycle != nullptr)
        {
            *parent.cycle = error; // The galaxies it is in are refused as well
        }
        return std::nullopt;
    }
    const GalaxyInfo info = galaxyInfo(canonicalPath, *project);
    const QStringList missing = missingPlugins(info);
    if(!missing.isEmpty())
    {
        error = QStringLiteral("%1 needs plugins the application does not know: %2")
                    .arg(nativePath, missing.join(QStringLiteral(", ")));
        return std::nullopt;
    }
    for(const QString& pluginId : info.plugins)
    {
        if(!m_plugins.isLoaded(pluginId) && !m_plugins.load(pluginId))
        {
            error = QStringLiteral("%1 needs the plugin %2, which cannot be loaded: %3")
                        .arg(nativePath, pluginId, m_plugins.errorString());
            return std::nullopt;
        }
    }
    return project;
}

std::unique_ptr<Galaxy> Universe::createGalaxy(const QString& filePath, const QString& name, const Scope& parent,
                                               QString& error)
{
    const std::optional<QJsonObject> project = readGalaxyProject(filePath, parent, error);
    if(!project)
    {
        return nullptr;
    }
    // Not made with std::make_unique: its constructor is only for the application.
    std::unique_ptr<Galaxy> galaxy(new Galaxy);
    // Its own file, as the files of the galaxies it is placed in are: a link to it is the same file.
    galaxy->m_filePath = QFileInfo(filePath).canonicalFilePath();
    // Its name before its content: the names of its settings files start with it.
    galaxy->setObjectName(freeName(parent, name, galaxy->galaxyName()));
    if(!fillGalaxy(*galaxy, *project, parent, error))
    {
        return nullptr;
    }
    return galaxy;
}

bool Universe::fillGalaxy(Galaxy& galaxy, const QJsonObject& project, const Scope& parent, QString& error)
{
    galaxy.m_description = project.value(u"description").toString();
    galaxy.m_project = project;
    QString cycle; // Why it would hold itself, which a galaxy it holds tells
    const Scope scope{.galaxy = &galaxy,
                      .directory = QFileInfo(galaxy.m_filePath).absoluteDir(),
                      .files = parent.files + QStringList{galaxy.m_filePath},
                      .prefix = parent.prefix + galaxy.objectName() + u'.',
                      .cycle = &cycle};
    galaxy.m_problems = loadScope(scope, project);
    if(!cycle.isEmpty())
    {
        galaxy.empty(); // What was placed before the galaxy that holds it
        galaxy.m_problems = {cycle};
        error = cycle;
        if(parent.cycle != nullptr)
        {
            *parent.cycle = cycle;
        }
        return false;
    }
    // Once it is filled: adding it tells that its files are among those of the application, see adopt().
    for(const std::unique_ptr<SettingsFile>& file : galaxy.m_settingsFiles)
    {
        connect(file.get(), &SettingsFile::changed, this, &Universe::settingsFilesChanged);
    }
    // Once they are bound, which tells what it needs: what they expose is its own from now on.
    for(const std::unique_ptr<Component>& component : galaxy.m_components)
    {
        if(auto* stargate = qobject_cast<Stargate*>(component.get()))
        {
            stargate->attach();
        }
    }
    return true;
}

bool Universe::open(const QString& filePath)
{
    const std::optional<QJsonObject> project = read(filePath, m_errorString);
    if(!project)
    {
        return false;
    }
    setFilePath(filePath); // Before it is loaded: its galaxies are relative to its directory
    const QStringList problems = load(*project);
    m_errorString = problems.join(u'\n');
    return problems.isEmpty();
}

QJsonObject Universe::save() const
{
    // The files of its galaxies relative to its own, whole without one.
    return saveProject(m_components, m_settingsFiles, projectDirectory(), m_description);
}

QJsonObject Universe::saveGalaxy(const Galaxy& galaxy) const
{
    // The files of its galaxies relative to its own, as it reads them.
    return saveProject(galaxy.m_components, galaxy.m_settingsFiles, QFileInfo(galaxy.filePath()).absolutePath(),
                       galaxy.description());
}

QStringList Universe::reloadGalaxy(Galaxy& galaxy)
{
    const QString name = galaxy.objectName();
    if(m_running)
    {
        return {QStringLiteral("%1 is not read again while the application runs").arg(name)};
    }
    const std::optional<QList<Galaxy*>> path = pathTo(galaxy);
    if(!path)
    {
        return {QStringLiteral("%1 is not read again: it is not a galaxy of the application").arg(name)};
    }
    const Scope parent = scope(*path);
    QString error;
    const std::optional<QJsonObject> project = readGalaxyProject(galaxy.filePath(), parent, error);
    if(!project)
    {
        return {QStringLiteral("%1 is not read again: %2").arg(name, error)};
    }

    emit galaxy.aboutToBeReloaded();
    // What it keeps: what is bound to its interfaces, either way, and the values of what it exposes.
    struct Binding
    {
        QPointer<Component> consumer;
        QString consumed;
        QPointer<Component> provider;
        QString provided;
    };
    QList<Binding> bindings;
    const QList<InterfaceInfo> consumedInterfaces = galaxy.consumedInterfaces();
    for(const InterfaceInfo& consumed : consumedInterfaces)
    {
        const QList<InterfaceBinding> bound = galaxy.bindings(consumed.name);
        for(const InterfaceBinding& to : bound)
        {
            bindings.append(
                {.consumer = &galaxy, .consumed = consumed.name, .provider = to.provider, .provided = to.providedName});
        }
    }
    // A list bound to it is bound again after what else it is bound to.
    for(const std::unique_ptr<Component>& component : scopeComponents(parent))
    {
        const QList<InterfaceInfo> consumed = component->consumedInterfaces();
        for(const InterfaceInfo& info : consumed)
        {
            const QList<InterfaceBinding> bound = component->bindings(info.name);
            for(const InterfaceBinding& to : bound)
            {
                if(to.provider == &galaxy)
                {
                    bindings.append({.consumer = component.get(),
                                     .consumed = info.name,
                                     .provider = &galaxy,
                                     .provided = to.providedName});
                }
            }
        }
    }
    QJsonObject values;
    const QList<ExposedProperty> exposed = galaxy.exposedProperties();
    for(const ExposedProperty& property : exposed)
    {
        if(const std::optional<QJsonValue> value = savedValue(property))
        {
            values.insert(property.name, *value);
        }
    }
    const bool hadSettingsFiles = hasSettingsFiles(galaxy);

    galaxy.empty();
    QStringList problems;
    if(fillGalaxy(galaxy, *project, parent, error))
    {
        for(const QString& problem : galaxy.problems())
        {
            problems.append(QStringLiteral("In %1: %2").arg(name, problem));
        }
    }
    else
    {
        problems.append(QStringLiteral("%1 is left empty: %2").arg(name, error));
    }
    for(const Binding& bound : std::as_const(bindings))
    {
        if(bound.consumer == nullptr || bound.provider == nullptr ||
           !bound.consumer->bindInterface(bound.consumed, *bound.provider, bound.provided))
        {
            problems.append(
                QStringLiteral("%1 of %2 is no longer bound to %3 of %4")
                    .arg(bound.consumed, bound.consumer != nullptr ? bound.consumer->objectName() : QString(),
                         bound.provided, bound.provider != nullptr ? bound.provider->objectName() : QString()));
        }
    }
    // Those of this instance, in place of those of its file, but for what it no longer exposes.
    for(const QString& property : values.keys())
    {
        if(!galaxy.exposedProperty(property))
        {
            values.remove(property);
            problems.append(QStringLiteral("%1 no longer exposes %2: the value it had is lost").arg(name, property));
        }
    }
    setExposedValues(galaxy, values, problems);
    emit galaxy.reloaded();
    if(hadSettingsFiles || hasSettingsFiles(galaxy))
    {
        emit settingsFilesChanged(); // Its files are others
    }
    return problems;
}

QJsonObject Universe::saveProject(const std::vector<std::unique_ptr<Component>>& components,
                                  const std::vector<std::unique_ptr<SettingsFile>>& files, const QString& directory,
                                  const QString& description) const
{
    std::map<QString, QString> pluginFiles; // By plugin id: each of them once
    QJsonArray componentsJson;
    QJsonArray bindings;
    for(const std::unique_ptr<Component>& instance : components)
    {
        addPluginFiles(*instance, pluginFiles);
        QJsonObject json;
        json.insert(u"name", instance->objectName());
        json.insert(u"type", instance->componentId());
        if(const auto* galaxy = qobject_cast<const Galaxy*>(instance.get()))
        {
            // Its file, and the values of what it exposes: its content is the one of its file. Relative to the
            // directory of the project, for both to be moved together; whole without a project file, or on another
            // drive.
            json.insert(u"galaxy", directory.isEmpty() ? galaxy->filePath()
                                                       : QDir(directory).relativeFilePath(galaxy->filePath()));
            QJsonObject properties;
            const QList<ExposedProperty> exposed = galaxy->exposedProperties();
            for(const ExposedProperty& property : exposed)
            {
                if(const std::optional<QJsonValue> value = savedValue(property))
                {
                    properties.insert(property.name, *value);
                }
            }
            if(!properties.isEmpty()) // Only for those that expose some: the others are written as they were
            {
                json.insert(u"properties", properties);
            }
        }
        else if(const auto* stargate = qobject_cast<const Stargate*>(instance.get()))
        {
            // What it exposes, without properties of its own.
            const QList<ExposedInterface> interfaces = stargate->interfaces();
            for(const Exposure exposure : {Exposure::Provided, Exposure::Consumed})
            {
                QJsonArray exposed;
                for(const ExposedInterface& interfaceInfo : interfaces)
                {
                    if(interfaceInfo.exposure == exposure)
                    {
                        QJsonObject interfaceJson;
                        interfaceJson.insert(u"name", interfaceInfo.name);
                        interfaceJson.insert(u"type", QString::fromLatin1(interfaceInfo.type.name()));
                        exposed.append(interfaceJson);
                    }
                }
                if(!exposed.isEmpty())
                {
                    json.insert(exposureKey(exposure), exposed);
                }
            }
            QJsonArray properties;
            for(const ExposedProperty& property : stargate->properties())
            {
                QJsonObject propertyJson = entryJson(
                    {.component = property.component, .object = property.object, .property = property.property});
                propertyJson.insert(u"name", property.name);
                properties.append(propertyJson);
            }
            if(!properties.isEmpty())
            {
                json.insert(u"exposed", properties);
            }
        }
        else
        {
            const std::optional<ComponentInfo> info = ComponentFactory::instance().component(instance->componentId());
            json.insert(u"properties", savedValues(*instance, info ? info->properties : QList<QMetaProperty>()));
            QJsonArray objects;
            const QList<PropertyObject> propertyObjects = instance->propertyObjects();
            for(const PropertyObject& added : propertyObjects)
            {
                QJsonObject object;
                object.insert(u"name", added.name);
                object.insert(u"properties", savedValues(*added.object, added.properties()));
                objects.append(object);
            }
            if(!objects.isEmpty())
            {
                json.insert(u"objects", objects);
            }
        }
        componentsJson.append(json);

        const QList<InterfaceInfo> consumedInterfaces = instance->consumedInterfaces();
        for(const InterfaceInfo& consumed : consumedInterfaces)
        {
            // One for each of what a list is bound to, in its order, which binding them again keeps.
            const QList<InterfaceBinding> bound = instance->bindings(consumed.name);
            for(const InterfaceBinding& to : bound)
            {
                QJsonObject binding;
                binding.insert(u"consumer", instance->objectName());
                binding.insert(u"consumed", consumed.name);
                binding.insert(u"provider", to.provider->objectName());
                binding.insert(u"provided", to.providedName);
                bindings.append(binding);
            }
        }
    }

    // The plugins of its components, and of those of its galaxies: those to load before it is read again.
    QJsonArray plugins;
    for(const auto& [pluginId, filePath] : pluginFiles)
    {
        QJsonObject plugin;
        plugin.insert(u"id", pluginId);
        // Relative, for the project to be read wherever its plugins are moved to, with their root; it cannot be when
        // the file is on another drive than the root.
        plugin.insert(u"file", m_pluginRoot.relativeFilePath(filePath));
        plugins.append(plugin);
    }

    QJsonArray settings;
    for(const std::unique_ptr<SettingsFile>& file : files)
    {
        QJsonObject json;
        json.insert(u"file", file->name()); // Without the galaxies it is in: each instance names its own after it
        json.insert(u"directory", file->directory());
        QJsonArray entries;
        for(const SettingsEntry& entry : file->entries())
        {
            entries.append(entryJson(entry));
        }
        json.insert(u"entries", entries);
        settings.append(json);
    }

    QJsonObject project;
    project.insert(u"version", ProjectVersion);
    if(!description.isEmpty()) // As settings: the projects without one are written as they were
    {
        project.insert(u"description", description);
    }
    project.insert(u"plugins", plugins);
    project.insert(u"components", componentsJson);
    project.insert(u"bindings", bindings);
    if(!settings.isEmpty()) // Only for those who use them: the others are written as they were
    {
        project.insert(u"settings", settings);
    }
    return project;
}

QString Universe::description() const
{
    return m_description;
}

void Universe::setDescription(const QString& description)
{
    m_description = description;
}

QString Universe::filePath() const
{
    return m_filePath;
}

void Universe::setFilePath(const QString& filePath)
{
    m_filePath = filePath.isEmpty() ? QString() : QFileInfo(filePath).absoluteFilePath();
}

Component* Universe::instantiate(const QString& componentId, const QString& name, Galaxy* galaxy)
{
    const std::optional<ComponentInfo> info = ComponentFactory::instance().component(componentId);
    const std::optional<Scope> scope = galaxyScope(galaxy);
    if(m_running || !info || !scope)
    {
        return nullptr;
    }
    return adopt(*scope, ComponentFactory::instance().create(componentId), name, info->name);
}

Galaxy* Universe::instantiateGalaxy(const QString& filePath, const QString& name, Galaxy* galaxy)
{
    if(m_running)
    {
        m_errorString = QStringLiteral("A galaxy cannot be placed while the application runs");
        return nullptr;
    }
    // Not holding the file of galaxy is enough for each instance of that file to hold it once it is saved: the galaxies
    // another instance is in hold that file too.
    const std::optional<Scope> scope = galaxyScope(galaxy);
    if(!scope)
    {
        m_errorString = QStringLiteral("A galaxy is only placed in the application, or in one of its galaxies");
        return nullptr;
    }
    std::unique_ptr<Galaxy> placed = createGalaxy(filePath, name, *scope, m_errorString);
    if(placed == nullptr)
    {
        return nullptr;
    }
    m_errorString.clear();
    const QString placedName = placed->objectName();
    auto* adopted = static_cast<Galaxy*>(adopt(*scope, std::move(placed), placedName, placedName));
    if(galaxy != nullptr && hasSettingsFiles(*adopted))
    {
        emit settingsFilesChanged(); // As adopt() tells it in the application: its files are among its own
    }
    return adopted;
}

Stargate* Universe::addStargate(const QString& name, Galaxy* galaxy)
{
    const std::optional<Scope> scope = galaxyScope(galaxy);
    if(m_running || !scope)
    {
        return nullptr;
    }
    // Not made with std::make_unique: its constructor is only for the application.
    auto* stargate = static_cast<Stargate*>(
        adopt(*scope, std::unique_ptr<Component>(new Stargate(*this, galaxy)), name, StargateName));
    stargate->attach(); // Its galaxy has what it exposes from now on; nothing in the application
    return stargate;
}

bool Universe::rename(Component& instance, const QString& name)
{
    const std::optional<QList<Galaxy*>> path = pathTo(instance);
    if(m_running || name.isEmpty() || !path)
    {
        return false;
    }
    const Scope where = scope(*path);
    const Component* named = scopeComponent(where, name);
    if(named != nullptr && named != &instance)
    {
        return false;
    }
    instance.setObjectName(name);
    if(auto* galaxy = qobject_cast<Galaxy*>(&instance))
    {
        nameSettingsFiles(*galaxy, where.prefix + name + u'.'); // Its files are named after it
    }
    return true;
}

bool Universe::remove(Component& instance)
{
    const std::optional<QList<Galaxy*>> path = pathTo(instance);
    if(m_running || !path)
    {
        return false;
    }
    std::vector<std::unique_ptr<Component>>& components =
        path->isEmpty() ? m_components : path->constLast()->m_components;
    // There, as pathTo() found it.
    const auto found = std::ranges::find_if(components, [&instance](const std::unique_ptr<Component>& owned)
                                            { return owned.get() == &instance; });
    // Out of components() before it is destroyed: what its links falling tell can look at the others.
    const std::unique_ptr<Component> removed = std::move(*found);
    components.erase(found);
    if(path->isEmpty())
    {
        emit componentsChanged();
    }
    const auto* galaxy = qobject_cast<const Galaxy*>(removed.get());
    if(galaxy != nullptr && hasSettingsFiles(*galaxy))
    {
        emit settingsFilesChanged(); // Its files are no longer among those of the application
    }
    return true;
}

const std::vector<std::unique_ptr<Component>>& Universe::components() const
{
    return m_components;
}

const std::vector<std::unique_ptr<Component>>& Universe::components(const Galaxy* galaxy) const
{
    return galaxy != nullptr ? galaxy->components() : m_components;
}

Galaxy* Universe::galaxyOf(const Component& instance) const
{
    const std::optional<QList<Galaxy*>> path = pathTo(instance);
    return path && !path->isEmpty() ? path->constLast() : nullptr;
}

QList<Galaxy*> Universe::galaxies(const QString& filePath) const
{
    // As the galaxies know their file: a link to it is the same file.
    const QString canonicalPath = QFileInfo(filePath).canonicalFilePath();
    QList<Galaxy*> found;
    if(!canonicalPath.isEmpty())
    {
        collectGalaxies(m_components, canonicalPath, found);
    }
    return found;
}

Component* Universe::component(const QString& name) const
{
    const auto found = std::ranges::find(m_components, name, &Component::objectName);
    return found != m_components.end() ? found->get() : nullptr;
}

QList<Component*> Universe::componentsOf(const QString& pluginId) const
{
    QList<Component*> found;
    for(const std::unique_ptr<Component>& instance : m_components)
    {
        if(isOf(*instance, pluginId))
        {
            found.append(instance.get());
        }
    }
    return found;
}

void Universe::clear()
{
    stop();
    const bool hadSettingsFiles = !allSettingsFiles().isEmpty();
    if(!m_components.empty())
    {
        // Out of components() before they are destroyed, as remove() does.
        const std::vector<std::unique_ptr<Component>> removed = std::exchange(m_components, {});
        emit componentsChanged();
    }
    m_settingsFiles.clear();
    if(hadSettingsFiles)
    {
        emit settingsFilesChanged();
    }
    m_description.clear();
}

SettingsFile& Universe::addSettingsFile()
{
    // Not made with std::make_unique: its constructor is only for the application.
    SettingsFile& file = *m_settingsFiles.emplace_back(new SettingsFile(*this, newSettingsFileName()));
    connect(&file, &SettingsFile::changed, this, &Universe::settingsFilesChanged);
    connect(&file, &SettingsFile::failed, this, &Universe::settingsFileFailed);
    emit settingsFilesChanged();
    return file;
}

SettingsFile* Universe::addSettingsFile(Galaxy& galaxy)
{
    const std::optional<Scope> scope = galaxyScope(&galaxy);
    if(!scope)
    {
        return nullptr;
    }
    QStringList problems; // None: it is given a name no file has
    SettingsFile& file = addSettingsFile(*scope, {}, problems);
    connect(&file, &SettingsFile::changed, this, &Universe::settingsFilesChanged);
    emit settingsFilesChanged();
    return &file;
}

void Universe::removeSettingsFile(SettingsFile& file)
{
    std::vector<std::unique_ptr<SettingsFile>>& files =
        file.m_galaxy != nullptr ? file.m_galaxy->m_settingsFiles : m_settingsFiles;
    const auto found = std::ranges::find_if(files, [&file](const std::unique_ptr<SettingsFile>& owned)
                                            { return owned.get() == &file; });
    if(found == files.end())
    {
        return;
    }
    if(m_running)
    {
        file.stop();
    }
    files.erase(found);
    emit settingsFilesChanged();
}

const std::vector<std::unique_ptr<SettingsFile>>& Universe::settingsFiles() const
{
    return m_settingsFiles;
}

SettingsFile* Universe::settingsFile(const SettingsEntry& entry) const
{
    const QList<SettingsFile*> files = allSettingsFiles();
    const auto found =
        std::ranges::find_if(files, [&entry](const SettingsFile* file) { return file->contains(entry); });
    return found != files.end() ? *found : nullptr;
}

SettingsFile* Universe::settingsFile(const QString& fileName) const
{
    // Whatever its case: two names that only differ by it are the same file where file names do not tell it apart.
    const QList<SettingsFile*> files = allSettingsFiles();
    const auto found = std::ranges::find_if(files, [&fileName](const SettingsFile* file)
                                            { return file->fileName().compare(fileName, Qt::CaseInsensitive) == 0; });
    return found != files.end() ? *found : nullptr;
}

bool Universe::start()
{
    if(m_running)
    {
        m_errorString.clear();
        return true;
    }
    // Before the settings files give the properties the values of the user.
    keepEditedValues();
    const QList<SettingsFile*> files = allSettingsFiles();
    for(SettingsFile* file : files)
    {
        file->start();
    }
    std::vector<Component*> started;
    for(const std::unique_ptr<Component>& instance : m_components)
    {
        if(!instance->start())
        {
            m_errorString = QStringLiteral("%1 could not be started").arg(instance->objectName());
            for(Component* running : started | std::views::reverse)
            {
                running->stop();
            }
            returnToEdition();
            return false;
        }
        started.push_back(instance.get());
    }
    m_errorString.clear();
    setRunning(true);
    return true;
}

void Universe::stop()
{
    for(const std::unique_ptr<Component>& instance : m_components | std::views::reverse)
    {
        instance->stop(); // Does nothing to one that does not run
    }
    if(m_running)
    {
        returnToEdition();
    }
    setRunning(false);
}

bool Universe::isRunning() const
{
    return m_running;
}

bool Universe::unloadPlugin(const QString& pluginId)
{
    if(m_running)
    {
        m_errorString = QStringLiteral("A plugin cannot be unloaded while the application runs");
        return false;
    }
    // None of them outlives what built it.
    const QList<Component*> instances = componentsOf(pluginId);
    for(Component* instance : instances)
    {
        remove(*instance);
    }
    if(!m_plugins.unload(pluginId))
    {
        m_errorString = m_plugins.errorString();
        return false;
    }
    m_errorString.clear();
    return true;
}

QString Universe::errorString() const
{
    return m_errorString;
}

QJsonObject Universe::savedValues(const QObject& object, const QList<QMetaProperty>& properties) const
{
    const auto edited = std::ranges::find(m_editedValues, &object,
                                          [](const EditedValues& values) -> const QObject* { return values.object; });
    return edited != m_editedValues.end() ? edited->values : writableProperties(object, properties);
}

std::optional<QJsonValue> Universe::savedValue(const ExposedProperty& property) const
{
    if(property.object == nullptr)
    {
        return std::nullopt;
    }
    const QJsonObject values = savedValues(*property.object, {metaProperty(*property.object, property.property)});
    const QJsonValue value = values.value(property.property);
    return value.isUndefined() ? std::nullopt : std::optional(value);
}

void Universe::keepEditedValues()
{
    m_editedValues.clear();
    for(const std::unique_ptr<Component>& instance : m_components)
    {
        keepEditedValues(*instance);
    }
}

void Universe::keepEditedValues(Component& component)
{
    // Those of its galaxies too: what they do while they run is undone as well.
    if(const auto* galaxy = qobject_cast<const Galaxy*>(&component))
    {
        for(const std::unique_ptr<Component>& inner : galaxy->components())
        {
            keepEditedValues(*inner);
        }
        return;
    }
    if(qobject_cast<const Stargate*>(&component) != nullptr)
    {
        return; // No property of its own
    }
    const auto keep = [this](QObject& object, const QList<QMetaProperty>& properties)
    {
        m_editedValues.push_back(EditedValues{
            .object = &object, .properties = properties, .values = writableProperties(object, properties)});
    };
    const std::optional<ComponentInfo> info = ComponentFactory::instance().component(component.componentId());
    keep(component, info ? info->properties : QList<QMetaProperty>());
    const QList<PropertyObject> objects = component.propertyObjects();
    for(const PropertyObject& added : objects)
    {
        keep(*added.object, added.properties());
    }
}

void Universe::returnToEdition()
{
    const QList<SettingsFile*> files = allSettingsFiles();
    for(SettingsFile* file : files | std::views::reverse)
    {
        file->stop();
    }
    // Once the files wrote what the user changed: it is undone here.
    for(const EditedValues& edited : m_editedValues)
    {
        if(edited.object == nullptr)
        {
            continue; // Destroyed while it ran
        }
        for(auto value = edited.values.begin(); value != edited.values.end(); ++value)
        {
            writeProperty(*edited.object, edited.properties, value.key(), value.value());
        }
    }
    m_editedValues.clear();
}

QString Universe::projectDirectory() const
{
    if(m_filePath.isEmpty())
    {
        return {};
    }
    // As the files of the galaxies are: relative to it, they are written without detour.
    const QFileInfo directory(QFileInfo(m_filePath).absolutePath());
    const QString canonicalPath = directory.canonicalFilePath();
    return canonicalPath.isEmpty() ? directory.absoluteFilePath() : canonicalPath;
}

Universe::Scope Universe::applicationScope() const
{
    Scope scope;
    if(!m_filePath.isEmpty())
    {
        scope.directory.setPath(projectDirectory());
        if(const QString canonicalPath = QFileInfo(m_filePath).canonicalFilePath(); !canonicalPath.isEmpty())
        {
            scope.files.append(canonicalPath); // A galaxy of the application cannot hold it
        }
    }
    return scope;
}

std::optional<QList<Galaxy*>> Universe::pathTo(const Component& instance) const
{
    QList<Galaxy*> path;
    return findPath(m_components, instance, path) ? std::optional(path) : std::nullopt;
}

Universe::Scope Universe::scope(const QList<Galaxy*>& path) const
{
    Scope inner = applicationScope();
    for(Galaxy* galaxy : path)
    {
        inner.galaxy = galaxy;
        inner.directory = QFileInfo(galaxy->filePath()).absoluteDir();
        inner.files.append(galaxy->filePath());
        inner.prefix += galaxy->objectName() + u'.';
    }
    return inner;
}

std::optional<Universe::Scope> Universe::galaxyScope(Galaxy* galaxy) const
{
    if(galaxy == nullptr)
    {
        return applicationScope();
    }
    std::optional<QList<Galaxy*>> path = pathTo(*galaxy);
    if(!path)
    {
        return std::nullopt;
    }
    path->append(galaxy);
    return scope(*path);
}

std::vector<std::unique_ptr<Component>>& Universe::scopeComponents(const Scope& scope)
{
    return scope.galaxy != nullptr ? scope.galaxy->m_components : m_components;
}

Component* Universe::scopeComponent(const Scope& scope, const QString& name) const
{
    return scope.galaxy != nullptr ? scope.galaxy->component(name) : component(name);
}

QString Universe::freeName(const Scope& scope, const QString& name, const QString& base) const
{
    if(!name.isEmpty() && scopeComponent(scope, name) == nullptr)
    {
        return name;
    }
    for(int number = 1;; ++number)
    {
        const QString numbered = base + QString::number(number);
        if(scopeComponent(scope, numbered) == nullptr)
        {
            return numbered;
        }
    }
}

Component* Universe::adopt(const Scope& scope, std::unique_ptr<Component> component, const QString& name,
                           const QString& base)
{
    if(component == nullptr)
    {
        return nullptr;
    }
    component->setObjectName(freeName(scope, name, base));
    Component* adopted = scopeComponents(scope).emplace_back(std::move(component)).get();
    if(scope.galaxy == nullptr)
    {
        emit componentsChanged();
        const auto* galaxy = qobject_cast<const Galaxy*>(adopted);
        if(galaxy != nullptr && hasSettingsFiles(*galaxy))
        {
            emit settingsFilesChanged(); // Its files are among those of the application
        }
    }
    return adopted;
}

SettingsFile& Universe::addSettingsFile(const Scope& scope, const QString& fileName, QStringList& problems)
{
    if(scope.galaxy == nullptr)
    {
        SettingsFile& file = addSettingsFile();
        // Not the name of one read before: they would be written over each other.
        if(!fileName.isEmpty() && !file.setFileName(fileName))
        {
            problems.append(QStringLiteral("The settings file %1 is named %2: another one has its name")
                                .arg(fileName, file.fileName()));
        }
        return file;
    }
    // After its galaxy: each instance of it has the same files. With a number when another file of the galaxy has that
    // name, or another file of the application the one it would have.
    const QString base = fileName.isEmpty() ? QStringLiteral("settings.json") : fileName;
    const auto isTaken = [this, &scope](const QString& name)
    {
        return settingsFile(scope.prefix + name) != nullptr ||
               std::ranges::any_of(scope.galaxy->m_settingsFiles, [&name](const std::unique_ptr<SettingsFile>& file)
                                   { return file->name().compare(name, Qt::CaseInsensitive) == 0; });
    };
    QString name = base;
    for(int number = 2; isTaken(name); ++number)
    {
        name = numbered(base, number);
    }
    if(!fileName.isEmpty() && name != fileName)
    {
        problems.append(QStringLiteral("The settings file %1 is named %2: another one has its name")
                            .arg(scope.prefix + fileName, scope.prefix + name));
    }
    // Not made with std::make_unique: its constructor is only for the application.
    SettingsFile& file =
        *scope.galaxy->m_settingsFiles.emplace_back(new SettingsFile(*this, scope.prefix + name, scope.galaxy, name));
    connect(&file, &SettingsFile::failed, this, &Universe::settingsFileFailed);
    return file; // Followed once its galaxy is filled, see fillGalaxy(), or by who adds it
}

QList<SettingsFile*> Universe::allSettingsFiles() const
{
    QList<SettingsFile*> files;
    for(const std::unique_ptr<SettingsFile>& file : m_settingsFiles)
    {
        files.append(file.get());
    }
    for(const std::unique_ptr<Component>& component : m_components)
    {
        if(const auto* galaxy = qobject_cast<const Galaxy*>(component.get()))
        {
            collectSettingsFiles(*galaxy, files);
        }
    }
    return files;
}

void Universe::nameSettingsFiles(Galaxy& galaxy, const QString& prefix)
{
    for(const std::unique_ptr<SettingsFile>& file : galaxy.m_settingsFiles)
    {
        file->rename(prefix + file->name(), file->name()); // Keeps its file name when the new one is taken
    }
    for(const std::unique_ptr<Component>& component : galaxy.m_components)
    {
        if(auto* inner = qobject_cast<Galaxy*>(component.get()))
        {
            nameSettingsFiles(*inner, prefix + inner->objectName() + u'.');
        }
    }
}

void Universe::addPluginFiles(const Component& component, std::map<QString, QString>& pluginFiles) const
{
    if(const auto* galaxy = qobject_cast<const Galaxy*>(&component))
    {
        for(const std::unique_ptr<Component>& inner : galaxy->components())
        {
            addPluginFiles(*inner, pluginFiles);
        }
        return;
    }
    const std::optional<ComponentInfo> info = ComponentFactory::instance().component(component.componentId());
    const std::optional<PluginInfo> plugin = info ? m_plugins.plugin(info->pluginId) : std::nullopt;
    if(plugin)
    {
        pluginFiles.emplace(plugin->id, plugin->filePath);
    }
}

QString Universe::newSettingsFileName() const
{
    QString name = QStringLiteral("settings.json");
    for(int number = 2; settingsFile(name) != nullptr; ++number)
    {
        name = QStringLiteral("settings%1.json").arg(number);
    }
    return name;
}

void Universe::setRunning(bool running)
{
    if(m_running == running)
    {
        return;
    }
    m_running = running;
    emit runningChanged(running);
}

} // namespace Nebula
