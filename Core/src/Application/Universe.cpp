// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Nebula/Application/Universe.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QMetaProperty>
#include <QSignalBlocker>
#include <algorithm>
#include <map>
#include <ranges>
#include <utility>

#include "Component/Components.h"
#include "JsonFile.h"
#include "Nebula/Component/ComponentFactory.h"
#include "ProjectJson.h"

namespace Nebula
{

namespace
{

using ProjectJson::writableProperties;

Q_LOGGING_CATEGORY(lcPlugins, "nebula.plugins")

// The version of the projects written, the only one read.
constexpr int ProjectVersion = 1;

// The names of the options addCommandLineOptions() adds.
const QString PluginDirOption = QStringLiteral("plugin-dir");
const QString PluginRootOption = QStringLiteral("plugin-root");

// What the stargates are named after, with a number: Stargate1.
const QString StargateName = QStringLiteral("Stargate");

// A binding of an interface: what a project holds of it, and what a galaxy read again keeps.
struct Binding
{
    QPointer<Component> consumer;
    QString consumed;
    QPointer<Component> provider;
    QString provided;
};

// Appends problem to problems, unless it is empty.
void appendProblem(QStringList& problems, const QString& problem)
{
    if(!problem.isEmpty())
    {
        problems.append(problem);
    }
}

// Appends to problems that each property of unset, of what of tells, could not be set.
void tellUnset(const QStringList& unset, const QString& of, QStringList& problems)
{
    for(const QString& property : unset)
    {
        problems.append(QStringLiteral("The property %1 of %2 could not be set").arg(property, of));
    }
}

// That the settings file former, of a project, is named name: another one has its name.
QString renamedFile(const QString& former, const QString& name)
{
    return QStringLiteral("The settings file %1 is named %2: another one has its name").arg(former, name);
}

// The name of object; empty when it is gone.
QString nameOf(const QObject* object)
{
    return object != nullptr ? object->objectName() : QString();
}

// Adds value to json under key, unless it is empty: the projects without it are written as they were.
template <typename Value> void insertUnlessEmpty(QJsonObject& json, QStringView key, const Value& value)
{
    if(!value.isEmpty())
    {
        json.insert(key, value);
    }
}

// What toJson makes of each of items, in their order.
template <typename Items, typename ToJson> QJsonArray jsonArray(const Items& items, ToJson toJson)
{
    QJsonArray array;
    for(const auto& item : items)
    {
        array.append(toJson(item));
    }
    return array;
}

// The objects owners owns, in their order.
template <typename T> QList<T*> pointers(const std::vector<std::unique_ptr<T>>& owners)
{
    QList<T*> found;
    for(const std::unique_ptr<T>& owned : owners)
    {
        found.append(owned.get());
    }
    return found;
}

// Takes item out of owners, the vector that owns it; nullptr when it does not.
template <typename T> std::unique_ptr<T> take(std::vector<std::unique_ptr<T>>& owners, const T& item)
{
    const auto found =
        std::ranges::find_if(owners, [&item](const std::unique_ptr<T>& owned) { return owned.get() == &item; });
    if(found == owners.end())
    {
        return nullptr;
    }
    std::unique_ptr<T> taken = std::move(*found);
    owners.erase(found);
    return taken;
}

// fileName with number before its suffix: settings2.json for settings.json.
QString numbered(const QString& fileName, int number)
{
    const QFileInfo info(fileName);
    const QString suffix = info.suffix();
    return info.completeBaseName() + QString::number(number) + (suffix.isEmpty() ? QString() : u'.' + suffix);
}

// fileName, or the first of numbered(fileName, 2), numbered(fileName, 3)... that isFree accepts.
template <typename IsFree> QString freeFileName(const QString& fileName, IsFree isFree)
{
    QString name = fileName;
    for(int number = 2; !isFree(name); ++number)
    {
        name = numbered(fileName, number);
    }
    return name;
}

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

// Adds key, the one of an object, to json: its "index" only when it is not the first object of its name.
void insertObjectKey(QJsonObject& json, const ProjectJson::ObjectKey& key)
{
    json.insert(u"object", key.name);
    if(key.index > 0)
    {
        json.insert(u"index", key.index);
    }
}

// The key of an object, as insertObjectKey() wrote it in json.
ProjectJson::ObjectKey readObjectKey(const QJsonObject& json)
{
    return {.name = json.value(u"object").toString(), .index = json.value(u"index").toInteger()};
}

// entry as a project holds it: the name of its component, the key of its object when it is not the component, see
// insertObjectKey(), and its property.
QJsonObject entryJson(const SettingsEntry& entry)
{
    QJsonObject json{{QStringLiteral("component"), entry.component->objectName()}};
    if(const std::optional<ProjectJson::ObjectKey> key = ProjectJson::objectKey(*entry.component, entry.object))
    {
        insertObjectKey(json, *key);
    }
    json.insert(u"property", entry.property);
    return json;
}

// The object of instance json, an entry, tells: instance itself, or one of its objects; nullptr when instance is.
QObject* entryObject(Component* instance, const QJsonObject& json)
{
    if(instance == nullptr || !json.contains(u"object"))
    {
        return instance;
    }
    return ProjectJson::object(*instance, readObjectKey(json));
}

// property, exposed by a stargate, as a project holds it.
QJsonObject exposedPropertyJson(const ExposedProperty& property)
{
    QJsonObject json =
        entryJson({.component = property.component, .object = property.object, .property = property.property});
    json.insert(u"name", property.name);
    return json;
}

// info, an interface a stargate exposes, as a project holds it.
QJsonObject interfaceJson(const ExposedInterface& info)
{
    return {{QStringLiteral("name"), info.name}, {QStringLiteral("type"), QString::fromLatin1(info.type.name())}};
}

// The interfaces stargate exposes as exposure tells, as a project holds them.
QJsonArray interfacesJson(const Stargate& stargate, Exposure exposure)
{
    QJsonArray json;
    const QList<ExposedInterface> interfaces = stargate.interfaces();
    for(const ExposedInterface& info : interfaces)
    {
        if(info.exposure == exposure)
        {
            json.append(interfaceJson(info));
        }
    }
    return json;
}

// That the interface name of stargate, in a project, is not exposed, for why.
QString notExposed(const QString& name, const Stargate& stargate, const QString& why)
{
    return QStringLiteral("The interface %1 of %2 is not exposed: %3").arg(name, stargate.objectName(), why);
}

// What a stargate exposes of entry, a property, as name.
ExposedProperty exposedAs(const SettingsEntry& entry, const QString& name)
{
    return {.name = name, .component = entry.component, .object = entry.object, .property = entry.property};
}

// What a project holds of stargate beyond its name and its type: what it exposes, without properties of its own.
QJsonObject stargateJson(const Stargate& stargate)
{
    QJsonObject json;
    for(const Exposure exposure : {Exposure::Provided, Exposure::Consumed})
    {
        insertUnlessEmpty(json, exposureKey(exposure), interfacesJson(stargate, exposure));
    }
    insertUnlessEmpty(json, u"exposed", jsonArray(stargate.properties(), exposedPropertyJson));
    return json;
}

// The bindings of name, which component consumes, in their order.
QList<Binding> bindingsOf(Component& component, const QString& name)
{
    QList<Binding> bindings;
    for(const InterfaceBinding& to : component.bindings(name))
    {
        bindings.append(
            {.consumer = &component, .consumed = name, .provider = to.provider, .provided = to.providedName});
    }
    return bindings;
}

// The bindings of component, in the order of the interfaces it consumes, then of the bindings of a list.
QList<Binding> bindingsOf(Component& component)
{
    QList<Binding> bindings;
    const QList<InterfaceInfo> consumed = component.consumedInterfaces();
    for(const InterfaceInfo& info : consumed)
    {
        bindings.append(bindingsOf(component, info.name));
    }
    return bindings;
}

// The bindings of component to provider.
QList<Binding> bindingsTo(Component& component, const Component* provider)
{
    QList<Binding> bindings = bindingsOf(component);
    bindings.removeIf([provider](const Binding& binding) { return binding.provider != provider; });
    return bindings;
}

// binding as a project holds it.
QJsonObject bindingJson(const Binding& binding)
{
    return {{QStringLiteral("consumer"), binding.consumer->objectName()},
            {QStringLiteral("consumed"), binding.consumed},
            {QStringLiteral("provider"), binding.provider->objectName()},
            {QStringLiteral("provided"), binding.provided}};
}

// The bindings of components as a project holds them: one for each of what a list is bound to, in its order, which
// binding them again keeps.
QJsonArray bindingsJson(const std::vector<std::unique_ptr<Component>>& components)
{
    QJsonArray json;
    for(const std::unique_ptr<Component>& component : components)
    {
        for(const Binding& binding : bindingsOf(*component))
        {
            json.append(bindingJson(binding));
        }
    }
    return json;
}

// Binds what binding tells; false when its consumer or its provider is gone, or they cannot be bound.
bool bind(const Binding& binding)
{
    return binding.consumer != nullptr && binding.provider != nullptr &&
           binding.consumer->bindInterface(binding.consumed, *binding.provider, binding.provided);
}

// That binding, kept as a galaxy is read again, could not be bound again.
QString noLongerBound(const Binding& binding)
{
    return QStringLiteral("%1 of %2 is no longer bound to %3 of %4")
        .arg(binding.consumed, nameOf(binding.consumer), binding.provided, nameOf(binding.provider));
}

// Binds again each of bindings; returns those that could not be.
QStringList bindAgain(const QList<Binding>& bindings)
{
    QStringList problems;
    for(const Binding& binding : bindings)
    {
        if(!bind(binding))
        {
            problems.append(noLongerBound(binding));
        }
    }
    return problems;
}

// What is bound to the interfaces of galaxy, either way, among components, those it is in: a list bound to it is bound
// again after what else it is bound to.
QList<Binding> bindingsWith(Galaxy& galaxy, const std::vector<std::unique_ptr<Component>>& components)
{
    QList<Binding> bindings = bindingsOf(galaxy);
    for(const std::unique_ptr<Component>& component : components)
    {
        bindings.append(bindingsTo(*component, &galaxy));
    }
    return bindings;
}

// file as a project holds it.
QJsonObject settingsFileJson(const std::unique_ptr<SettingsFile>& file)
{
    // Without the galaxies it is in: each instance names its own after it.
    return {{QStringLiteral("file"), file->name()},
            {QStringLiteral("directory"), file->directory()},
            {QStringLiteral("entries"), jsonArray(file->entries(), entryJson)}};
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

// The file of the galaxy json, a component of a project, from directory, the one of the project; as it is when it is
// not relative.
QString galaxyFilePath(const QDir& directory, const QJsonObject& json)
{
    return QDir::cleanPath(directory.absoluteFilePath(json.value(u"galaxy").toString()));
}

// The problems of galaxy, each saying it is in it.
QStringList galaxyProblems(const Galaxy& galaxy)
{
    QStringList problems;
    for(const QString& problem : galaxy.problems())
    {
        problems.append(QStringLiteral("In %1: %2").arg(galaxy.objectName(), problem));
    }
    return problems;
}

// files, then those of the galaxies among components and of those they hold, in the order they start.
QList<SettingsFile*> allFiles(const std::vector<std::unique_ptr<SettingsFile>>& files, ComponentSpan components)
{
    QList<SettingsFile*> all = pointers(files);
    for(const Galaxy* galaxy : componentsOfType<const Galaxy>(components))
    {
        all.append(allFiles(galaxy->settingsFiles(), galaxy->components()));
    }
    return all;
}

// Whether galaxy, or a galaxy it holds, has settings files.
bool hasSettingsFiles(const Galaxy& galaxy)
{
    return !allFiles(galaxy.settingsFiles(), galaxy.components()).isEmpty();
}

std::optional<QList<Galaxy*>> findPath(ComponentSpan components, const Component& instance);

// The galaxies down to instance through one of galaxies: the first that holds it, then those in it; nullopt when none
// holds it.
std::optional<QList<Galaxy*>> findPathThrough(const QList<Galaxy*>& galaxies, const Component& instance)
{
    for(Galaxy* galaxy : galaxies)
    {
        if(const std::optional<QList<Galaxy*>> path = findPath(galaxy->components(), instance))
        {
            return QList<Galaxy*>{galaxy} + *path;
        }
    }
    return std::nullopt;
}

// The galaxies of components down to the one that holds instance, directly: none when one of components is instance.
// nullopt when none holds it.
std::optional<QList<Galaxy*>> findPath(ComponentSpan components, const Component& instance)
{
    if(isAmong(&instance, components))
    {
        return QList<Galaxy*>();
    }
    return findPathThrough(componentsOfType<Galaxy>(components), instance);
}

// The galaxies of components of the file filePath, canonical, and those of the galaxies they hold.
QList<Galaxy*> galaxiesOf(ComponentSpan components, const QString& filePath)
{
    QList<Galaxy*> found;
    for(Galaxy* galaxy : componentsOfType<Galaxy>(components))
    {
        if(galaxy->filePath() == filePath)
        {
            found.append(galaxy);
        }
        found.append(galaxiesOf(galaxy->components(), filePath));
    }
    return found;
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

// Gives object, one of those of the component named name in its project, the values json, the object as the project
// holds it, gives its properties; problems tells what could not be, object missing included.
void restoreObject(QObject* object, const QJsonObject& json, const QString& name, QStringList& problems)
{
    const QString objectName = json.value(u"name").toString();
    if(object == nullptr)
    {
        problems.append(QStringLiteral("The object %1 of %2 is missing").arg(objectName, name));
        return;
    }
    const QStringList unset = ProjectJson::writeProperties(*object, PropertyObject{.object = object}.properties(),
                                                           json.value(u"properties").toObject());
    tellUnset(unset, QStringLiteral("the object %1 of %2").arg(objectName, name), problems);
}

// Gives the objects of instance, named name in its project, the values objects, its "objects" there, give their
// properties: each to the first object of its name none was given yet, as those of a name are held in the order they
// were added. problems tells what could not be.
void restoreObjects(const Component& instance, const QJsonArray& objects, const QString& name, QStringList& problems)
{
    QHash<QString, qsizetype> counts; // Of the objects of each name met so far
    for(const QJsonValue& value : objects)
    {
        const QJsonObject json = value.toObject();
        const QString objectName = json.value(u"name").toString();
        QObject* object = ProjectJson::object(instance, {.name = objectName, .index = counts[objectName]++});
        restoreObject(object, json, name, problems);
    }
}

// Gives instance, named name in its project, the values json, the component there, gives its properties and those of
// its objects; problems tells those that could not be set.
void restoreProperties(Component& instance, const QJsonObject& json, const QString& name, QStringList& problems)
{
    const QStringList unset = ProjectJson::writeProperties(instance, ProjectJson::componentProperties(instance),
                                                           json.value(u"properties").toObject());
    tellUnset(unset, name, problems);
    restoreObjects(instance, json.value(u"objects").toArray(), name, problems);
}

// The type named typeName of an interface component provides or consumes; nullopt when it has none of that type.
std::optional<QMetaType> interfaceTypeOf(const Component& component, const QString& typeName)
{
    const QList<InterfaceInfo> interfaces = component.providedInterfaces() + component.consumedInterfaces();
    const auto found = std::ranges::find_if(interfaces, [&typeName](const InterfaceInfo& info)
                                            { return QLatin1StringView(info.type.name()) == typeName; });
    return found != interfaces.end() ? std::optional(found->type) : std::nullopt;
}

// Sets the property galaxy exposes as name to value; false when it exposes none, or it could not be set. Found again
// for each value: setting one can destroy the object of another, one of the widgets a count tells, say.
bool writeExposed(const Galaxy& galaxy, const QString& name, const QJsonValue& value)
{
    const std::optional<ExposedProperty> found = galaxy.exposedProperty(name);
    return found && found->object != nullptr &&
           metaProperty(*found->object, found->property).write(found->object, value.toVariant());
}

// The names galaxy exposes its properties as, in the order it exposes them.
QStringList exposedNames(const Galaxy& galaxy)
{
    QStringList names;
    for(const ExposedProperty& exposed : galaxy.exposedProperties())
    {
        names.append(exposed.name);
    }
    return names;
}

// Sets the properties galaxy exposes to values, by the names they are exposed as, in the order it exposes them, as a
// component has its properties set in the order it declares them. problems tells those it does not expose, or that
// could not be set.
void setExposedValues(const Galaxy& galaxy, const QJsonObject& values, QStringList& problems)
{
    QStringList unset = values.keys();
    for(const QString& name : exposedNames(galaxy))
    {
        if(unset.contains(name) && writeExposed(galaxy, name, values.value(name)))
        {
            unset.removeOne(name);
        }
    }
    tellUnset(unset, galaxy.objectName(), problems);
}

// Gives galaxy, just loaded, values, those of its instance in place of those of its file; problems tells the problems
// of galaxy, then the values it could not be given.
void giveInstanceValues(const Galaxy& galaxy, const QJsonObject& values, QStringList& problems)
{
    problems.append(galaxyProblems(galaxy));
    setExposedValues(galaxy, values, problems);
}

// The names among values of what galaxy no longer exposes.
QStringList unexposed(const Galaxy& galaxy, const QJsonObject& values)
{
    QStringList names = values.keys();
    names.removeIf([&galaxy](const QString& name) { return galaxy.exposedProperty(name).has_value(); });
    return names;
}

// Gives galaxy, read again, values, those of what it exposed; returns those it no longer exposes, and those it could
// not be given.
QStringList giveBackValues(const Galaxy& galaxy, QJsonObject values)
{
    QStringList problems;
    for(const QString& property : unexposed(galaxy, values))
    {
        values.remove(property);
        problems.append(
            QStringLiteral("%1 no longer exposes %2: the value it had is lost").arg(galaxy.objectName(), property));
    }
    setExposedValues(galaxy, values, problems);
    return problems;
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
        scanDirectory(directory);
    }
    if(parser.isSet(PluginRootOption))
    {
        setPluginRoot(parser.value(PluginRootOption));
    }
}

std::optional<QJsonObject> Universe::read(const QString& filePath, QString& error)
{
    const std::optional<QJsonDocument> document = JsonFile::read(filePath, error);
    if(document && document->object().value(u"version").toInt() != ProjectVersion)
    {
        error = QStringLiteral("It is not a Nebula project of version %1").arg(ProjectVersion);
        return std::nullopt;
    }
    return document ? std::optional(document->object()) : std::nullopt;
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
    QStringList problems = loadProjectPlugins(project.value(u"plugins").toArray());
    problems.append(loadScope(applicationScope(), project));
    return problems;
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
    if(const QString refusal = reloadRefusal(galaxy); !refusal.isEmpty())
    {
        return {refusal};
    }
    const Scope parent = *scopeOf(galaxy);
    QString error;
    const std::optional<QJsonObject> project = readAgain(galaxy, parent, error);
    return project ? reload(galaxy, *project, parent) : QStringList{error};
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
    const std::optional<Scope> scope = galaxyScope(galaxy);
    return !m_running && scope ? create(*scope, componentId, name) : nullptr;
}

Galaxy* Universe::instantiateGalaxy(const QString& filePath, const QString& name, Galaxy* galaxy)
{
    const std::optional<Scope> scope = placingScope(galaxy);
    std::unique_ptr<Galaxy> placed = scope ? createGalaxy(filePath, name, *scope, m_errorString) : nullptr;
    if(placed == nullptr)
    {
        return nullptr;
    }
    m_errorString.clear();
    return placeGalaxy(*scope, std::move(placed));
}

Stargate* Universe::addStargate(const QString& name, Galaxy* galaxy)
{
    const std::optional<Scope> scope = galaxyScope(galaxy);
    if(m_running || !scope)
    {
        return nullptr;
    }
    Stargate* stargate = newStargate(*scope, name);
    stargate->attach(); // Its galaxy has what it exposes from now on; nothing in the application
    return stargate;
}

bool Universe::rename(Component& instance, const QString& name)
{
    const std::optional<Scope> where = m_running || name.isEmpty() ? std::nullopt : scopeOf(instance);
    if(!where || !isFreeFor(*where, name, instance))
    {
        return false;
    }
    setName(instance, name, *where);
    return true;
}

bool Universe::remove(Component& instance)
{
    const std::optional<Scope> where = m_running ? std::nullopt : scopeOf(instance);
    if(!where)
    {
        return false;
    }
    // Out of components() before it is destroyed: what its links falling tell can look at the others.
    const std::unique_ptr<Component> removed = take(scopeComponents(*where), instance);
    tellComponentsChanged(*where);
    tellSettingsFilesOf(removed.get()); // Its files are no longer among those of the application
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
    return canonicalPath.isEmpty() ? QList<Galaxy*>() : galaxiesOf(m_components, canonicalPath);
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
    removeComponents();
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
    if(take(settingsFilesOf(file.m_galaxy), file) != nullptr) // Stopped first while it runs, see ~SettingsFile()
    {
        emit settingsFilesChanged();
    }
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
    m_errorString.clear();
    if(!m_running)
    {
        keepEditedValues(); // Before the settings files give the properties the values of the user
        startSettingsFiles();
        setRunning(startComponents());
    }
    return m_running;
}

void Universe::stop()
{
    stopInReverse(m_components);
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
        return refuse(QStringLiteral("A plugin cannot be unloaded while the application runs"));
    }
    removeComponentsOf(pluginId); // None of them outlives what built it
    const bool unloaded = m_plugins.unload(pluginId);
    m_errorString = unloaded ? QString() : m_plugins.errorString();
    return unloaded;
}

QString Universe::errorString() const
{
    return m_errorString;
}

bool Universe::refuse(const QString& why)
{
    m_errorString = why;
    return false;
}

void Universe::scanDirectory(const QString& directory)
{
    if(!QFileInfo(directory).isDir())
    {
        qCWarning(lcPlugins).noquote() << "Skipping" << QDir::toNativeSeparators(directory)
                                       << "- it is not a directory";
        return;
    }
    m_plugins.scan(directory);
}

bool Universe::loadPlugin(const QString& pluginId)
{
    return m_plugins.isLoaded(pluginId) || m_plugins.load(pluginId);
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

Universe::Scope Universe::innerScope(Galaxy& galaxy, const Scope& parent, QString* cycle)
{
    return {.galaxy = &galaxy,
            .directory = QFileInfo(galaxy.m_filePath).absoluteDir(),
            .files = parent.files + QStringList{galaxy.m_filePath},
            .prefix = parent.prefix + galaxy.objectName() + u'.',
            .cycle = cycle};
}

std::optional<QList<Galaxy*>> Universe::pathTo(const Component& instance) const
{
    return findPath(m_components, instance);
}

Universe::Scope Universe::scope(const QList<Galaxy*>& path) const
{
    Scope inner = applicationScope();
    for(Galaxy* galaxy : path)
    {
        inner = innerScope(*galaxy, inner, nullptr);
    }
    return inner;
}

std::optional<Universe::Scope> Universe::scopeOf(const Component& instance) const
{
    const std::optional<QList<Galaxy*>> path = pathTo(instance);
    return path ? std::optional(scope(*path)) : std::nullopt;
}

std::optional<Universe::Scope> Universe::galaxyScope(Galaxy* galaxy) const
{
    if(galaxy == nullptr)
    {
        return applicationScope();
    }
    const std::optional<Scope> parent = scopeOf(*galaxy);
    return parent ? std::optional(innerScope(*galaxy, *parent, nullptr)) : std::nullopt;
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
    int number = 1;
    while(scopeComponent(scope, base + QString::number(number)) != nullptr)
    {
        ++number;
    }
    return base + QString::number(number);
}

bool Universe::isFreeFor(const Scope& scope, const QString& name, const Component& instance) const
{
    const Component* named = scopeComponent(scope, name);
    return named == nullptr || named == &instance;
}

void Universe::setName(Component& instance, const QString& name, const Scope& scope)
{
    instance.setObjectName(name);
    if(auto* galaxy = qobject_cast<Galaxy*>(&instance))
    {
        nameSettingsFiles(*galaxy, scope.prefix + name + u'.'); // Its files are named after it
    }
}

Component* Universe::create(const Scope& scope, const QString& componentId, const QString& name)
{
    const std::optional<ComponentInfo> info = ComponentFactory::instance().component(componentId);
    return info ? adopt(scope, ComponentFactory::instance().create(componentId), name, info->name) : nullptr;
}

Stargate* Universe::newStargate(const Scope& scope, const QString& name)
{
    // Not made with std::make_unique: its constructor is only for the application.
    return static_cast<Stargate*>(
        adopt(scope, std::unique_ptr<Component>(new Stargate(*this, scope.galaxy)), name, StargateName));
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
    tellAdopted(scope, *adopted);
    return adopted;
}

Galaxy* Universe::adoptGalaxy(const Scope& scope, std::unique_ptr<Galaxy> galaxy)
{
    const QString name = galaxy->objectName();
    return static_cast<Galaxy*>(adopt(scope, std::move(galaxy), name, name));
}

Galaxy* Universe::placeGalaxy(const Scope& scope, std::unique_ptr<Galaxy> galaxy)
{
    Galaxy* placed = adoptGalaxy(scope, std::move(galaxy));
    if(scope.galaxy != nullptr)
    {
        tellSettingsFilesOf(placed); // As adopt() tells it in the application: its files are among its own
    }
    return placed;
}

void Universe::tellComponentsChanged(const Scope& scope)
{
    if(scope.galaxy == nullptr)
    {
        emit componentsChanged();
    }
}

void Universe::tellAdopted(const Scope& scope, const Component& adopted)
{
    if(scope.galaxy == nullptr)
    {
        emit componentsChanged();
        tellSettingsFilesOf(&adopted); // Its files are among those of the application
    }
}

void Universe::tellSettingsFilesOf(const Component* component)
{
    const auto* galaxy = qobject_cast<const Galaxy*>(component);
    if(galaxy != nullptr && hasSettingsFiles(*galaxy))
    {
        emit settingsFilesChanged();
    }
}

void Universe::removeComponents()
{
    if(m_components.empty())
    {
        return;
    }
    const std::vector<std::unique_ptr<Component>> removed = std::exchange(m_components, {});
    emit componentsChanged();
}

void Universe::removeComponentsOf(const QString& pluginId)
{
    const QList<Component*> instances = componentsOf(pluginId);
    for(Component* instance : instances)
    {
        remove(*instance);
    }
}

QStringList Universe::loadProjectPlugins(const QJsonArray& plugins)
{
    QStringList problems;
    for(const QJsonValue& plugin : plugins)
    {
        appendProblem(problems, loadProjectPlugin(plugin.toObject()));
    }
    return problems;
}

QString Universe::loadProjectPlugin(const QJsonObject& plugin)
{
    const QString pluginId = plugin.value(u"id").toString();
    // Where its file is from the plugin root; as it is when it is not relative.
    const QString filePath = QDir::cleanPath(m_pluginRoot.absoluteFilePath(plugin.value(u"file").toString()));
    // Opened from its file when it is not found already, as the ones of plugins/ are.
    if((m_plugins.plugin(pluginId) || m_plugins.addFile(filePath)) && loadPlugin(pluginId))
    {
        return {};
    }
    return QStringLiteral("The plugin %1 is not loaded: %2").arg(pluginId, m_plugins.errorString());
}

QStringList Universe::loadScope(const Scope& scope, const QJsonObject& project)
{
    QStringList problems;
    const StargateJsons stargates = loadComponents(scope, project.value(u"components").toArray(), problems);
    // Of the types of the components, which can come after them; before the bindings, which bind to them.
    loadStargateInterfaces(scope, stargates, problems);
    loadBindings(scope, project.value(u"bindings").toArray(), problems);
    loadStargateProperties(scope, stargates, problems);
    loadSettingsFiles(scope, project.value(u"settings").toArray(), problems);
    return problems;
}

Universe::StargateJsons Universe::loadComponents(const Scope& scope, const QJsonArray& components,
                                                 QStringList& problems)
{
    StargateJsons stargates;
    for(const QJsonValue& value : components)
    {
        const QJsonObject json = value.toObject();
        if(auto* stargate = qobject_cast<Stargate*>(loadComponent(scope, json, problems)))
        {
            stargates.append({stargate, json});
        }
    }
    return stargates;
}

Component* Universe::loadComponent(const Scope& scope, const QJsonObject& json, QStringList& problems)
{
    const QString componentId = json.value(u"type").toString();
    if(componentId == Galaxy::Id)
    {
        return loadGalaxy(scope, json, problems);
    }
    if(componentId == Stargate::Id)
    {
        return newStargate(scope, json.value(u"name").toString());
    }
    return loadInstance(scope, json, problems);
}

Component* Universe::loadInstance(const Scope& scope, const QJsonObject& json, QStringList& problems)
{
    const QString name = json.value(u"name").toString();
    const QString componentId = json.value(u"type").toString();
    Component* instance = create(scope, componentId, name);
    if(instance == nullptr)
    {
        problems.append(QStringLiteral("%1 is missing: no loaded plugin provides %2").arg(name, componentId));
        return nullptr;
    }
    restoreProperties(*instance, json, name, problems);
    return instance;
}

Galaxy* Universe::loadGalaxy(const Scope& scope, const QJsonObject& json, QStringList& problems)
{
    const QString name = json.value(u"name").toString();
    QString error;
    std::unique_ptr<Galaxy> galaxy = createGalaxy(galaxyFilePath(scope.directory, json), name, scope, error);
    if(galaxy == nullptr)
    {
        problems.append(QStringLiteral("%1 is missing: %2").arg(name, error));
        return nullptr;
    }
    giveInstanceValues(*galaxy, json.value(u"properties").toObject(), problems);
    return adoptGalaxy(scope, std::move(galaxy));
}

void Universe::loadStargateInterfaces(const Scope& scope, const StargateJsons& stargates, QStringList& problems) const
{
    for(const auto& [stargate, json] : stargates)
    {
        for(const Exposure exposure : {Exposure::Provided, Exposure::Consumed})
        {
            loadInterfaces(scope, *stargate, exposure, json.value(exposureKey(exposure)).toArray(), problems);
        }
    }
}

void Universe::loadInterfaces(const Scope& scope, Stargate& stargate, Exposure exposure, const QJsonArray& interfaces,
                              QStringList& problems) const
{
    for(const QJsonValue& value : interfaces)
    {
        appendProblem(problems, loadInterface(scope, stargate, exposure, value.toObject()));
    }
}

QString Universe::loadInterface(const Scope& scope, Stargate& stargate, Exposure exposure,
                                const QJsonObject& json) const
{
    const QString name = json.value(u"name").toString();
    const QString typeName = json.value(u"type").toString();
    const QMetaType type = interfaceType(scope, typeName);
    if(!type.isValid())
    {
        return notExposed(name, stargate, QStringLiteral("no component has its type %1").arg(typeName));
    }
    return stargate.exposeInterface(exposure, name, type)
               ? QString()
               : notExposed(name, stargate, QStringLiteral("another one is exposed under its name"));
}

void Universe::loadBindings(const Scope& scope, const QJsonArray& bindings, QStringList& problems) const
{
    for(const QJsonValue& value : bindings)
    {
        appendProblem(problems, loadBinding(scope, value.toObject()));
    }
}

QString Universe::loadBinding(const Scope& scope, const QJsonObject& json) const
{
    const auto text = [&json](QStringView key) { return json.value(key).toString(); };
    const Binding binding{.consumer = scopeComponent(scope, text(u"consumer")),
                          .consumed = text(u"consumed"),
                          .provider = scopeComponent(scope, text(u"provider")),
                          .provided = text(u"provided")};
    return bind(binding) ? QString()
                         : QStringLiteral("%1 of %2 could not be bound to %3 of %4")
                               .arg(binding.consumed, text(u"consumer"), binding.provided, text(u"provider"));
}

void Universe::loadStargateProperties(const Scope& scope, const StargateJsons& stargates, QStringList& problems) const
{
    for(const auto& [stargate, json] : stargates)
    {
        const QJsonArray properties = json.value(u"exposed").toArray();
        for(const QJsonValue& value : properties)
        {
            appendProblem(problems, loadExposedProperty(scope, *stargate, value.toObject()));
        }
    }
}

QString Universe::loadExposedProperty(const Scope& scope, Stargate& stargate, const QJsonObject& json) const
{
    QString owner;
    const SettingsEntry entry = readEntry(scope, json, owner);
    const QString name = json.value(u"name").toString();
    if(entry.object != nullptr && stargate.exposeProperty(exposedAs(entry, name)))
    {
        return {};
    }
    return QStringLiteral("The property %1 of %2 is not exposed by %3 as %4")
        .arg(entry.property, owner, stargate.objectName(), name);
}

void Universe::loadSettingsFiles(const Scope& scope, const QJsonArray& settings, QStringList& problems)
{
    for(const QJsonValue& value : settings)
    {
        loadSettingsFile(scope, value.toObject(), problems);
    }
}

void Universe::loadSettingsFile(const Scope& scope, const QJsonObject& json, QStringList& problems)
{
    SettingsFile& file = addSettingsFile(scope, json.value(u"file").toString(), problems);
    file.setDirectory(json.value(u"directory").toString());
    const QJsonArray entries = json.value(u"entries").toArray();
    for(const QJsonValue& value : entries)
    {
        loadEntry(scope, file, value.toObject(), problems);
    }
}

void Universe::loadEntry(const Scope& scope, SettingsFile& file, const QJsonObject& json, QStringList& problems) const
{
    QString owner;
    const SettingsEntry entry = readEntry(scope, json, owner);
    if(entry.object == nullptr || !file.add(entry))
    {
        problems.append(
            QStringLiteral("The property %1 of %2 is not held by %3").arg(entry.property, owner, file.fileName()));
    }
}

SettingsEntry Universe::readEntry(const Scope& scope, const QJsonObject& json, QString& owner) const
{
    owner = json.value(u"component").toString(); // What the property is of, as it is told
    Component* instance = scopeComponent(scope, owner);
    if(json.contains(u"object"))
    {
        owner = QStringLiteral("the object %1 of %2").arg(json.value(u"object").toString(), owner);
    }
    return {
        .component = instance, .object = entryObject(instance, json), .property = json.value(u"property").toString()};
}

QMetaType Universe::interfaceType(const Scope& scope, const QString& typeName) const
{
    // Those of the components first: the type of an interface of a plugin is not registered by its name until it is.
    for(const std::unique_ptr<Component>& component : components(scope.galaxy))
    {
        if(const std::optional<QMetaType> type = interfaceTypeOf(*component, typeName))
        {
            return *type;
        }
    }
    return QMetaType::fromName(typeName.toLatin1());
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
    const GalaxyInfo info = galaxyInfo(QFileInfo(filePath).canonicalFilePath(), *project);
    return canPlace(nativePath, info, parent, error) ? project : std::nullopt;
}

bool Universe::canPlace(const QString& nativePath, const GalaxyInfo& info, const Scope& parent, QString& error)
{
    return !isCycle(nativePath, info.filePath, parent, error) && knowsPlugins(nativePath, info, error) &&
           loadPlugins(nativePath, info, error);
}

bool Universe::isCycle(const QString& nativePath, const QString& canonicalPath, const Scope& parent, QString& error)
{
    if(!parent.files.contains(canonicalPath))
    {
        return false;
    }
    error = QStringLiteral("%1 would hold itself, directly or not").arg(nativePath);
    tellCycle(parent, error);
    return true;
}

void Universe::tellCycle(const Scope& parent, const QString& cycle)
{
    if(parent.cycle != nullptr)
    {
        *parent.cycle = cycle;
    }
}

bool Universe::knowsPlugins(const QString& nativePath, const GalaxyInfo& info, QString& error) const
{
    const QStringList missing = missingPlugins(info);
    if(!missing.isEmpty())
    {
        error = QStringLiteral("%1 needs plugins the application does not know: %2")
                    .arg(nativePath, missing.join(QStringLiteral(", ")));
    }
    return missing.isEmpty();
}

bool Universe::loadPlugins(const QString& nativePath, const GalaxyInfo& info, QString& error)
{
    const auto unloadable =
        std::ranges::find_if_not(info.plugins, [this](const QString& pluginId) { return loadPlugin(pluginId); });
    if(unloadable == info.plugins.end())
    {
        return true;
    }
    error = QStringLiteral("%1 needs the plugin %2, which cannot be loaded: %3")
                .arg(nativePath, *unloadable, m_plugins.errorString());
    return false;
}

std::unique_ptr<Galaxy> Universe::createGalaxy(const QString& filePath, const QString& name, const Scope& parent,
                                               QString& error)
{
    const std::optional<QJsonObject> project = readGalaxyProject(filePath, parent, error);
    if(!project)
    {
        return nullptr;
    }
    std::unique_ptr<Galaxy> galaxy = emptyGalaxy(filePath, name, parent);
    return fillGalaxy(*galaxy, *project, parent, error) ? std::move(galaxy) : nullptr;
}

std::unique_ptr<Galaxy> Universe::emptyGalaxy(const QString& filePath, const QString& name, const Scope& parent) const
{
    // Not made with std::make_unique: its constructor is only for the application.
    std::unique_ptr<Galaxy> galaxy(new Galaxy);
    // Its own file, as the files of the galaxies it is placed in are: a link to it is the same file.
    galaxy->m_filePath = QFileInfo(filePath).canonicalFilePath();
    // Its name before its content: the names of its settings files start with it.
    galaxy->setObjectName(freeName(parent, name, galaxy->galaxyName()));
    return galaxy;
}

bool Universe::fillGalaxy(Galaxy& galaxy, const QJsonObject& project, const Scope& parent, QString& error)
{
    error = loadGalaxyContent(galaxy, project, parent);
    if(!error.isEmpty())
    {
        refuseCycle(galaxy, error, parent);
        return false;
    }
    settleGalaxy(galaxy);
    return true;
}

QString Universe::loadGalaxyContent(Galaxy& galaxy, const QJsonObject& project, const Scope& parent)
{
    galaxy.m_description = project.value(u"description").toString();
    galaxy.m_project = project;
    QString cycle; // Why it would hold itself, which a galaxy it holds tells
    galaxy.m_problems = loadScope(innerScope(galaxy, parent, &cycle), project);
    return cycle;
}

void Universe::refuseCycle(Galaxy& galaxy, const QString& cycle, const Scope& parent)
{
    galaxy.empty(); // What was placed before the galaxy that holds it
    galaxy.m_problems = {cycle};
    tellCycle(parent, cycle);
}

void Universe::settleGalaxy(Galaxy& galaxy)
{
    // Once it is filled: adding it tells that its files are among those of the application, see adopt().
    for(const std::unique_ptr<SettingsFile>& file : galaxy.m_settingsFiles)
    {
        connect(file.get(), &SettingsFile::changed, this, &Universe::settingsFilesChanged);
    }
    // Once they are bound, which tells what it needs: what they expose is its own from now on.
    for(Stargate* stargate : componentsOfType<Stargate>(galaxy.m_components))
    {
        stargate->attach();
    }
}

std::optional<Universe::Scope> Universe::placingScope(Galaxy* galaxy)
{
    // Not holding the file of galaxy is enough for each instance of that file to hold it once it is saved: the galaxies
    // another instance is in hold that file too.
    const std::optional<Scope> scope = m_running ? std::nullopt : galaxyScope(galaxy);
    if(!scope)
    {
        m_errorString = m_running
                            ? QStringLiteral("A galaxy cannot be placed while the application runs")
                            : QStringLiteral("A galaxy is only placed in the application, or in one of its galaxies");
    }
    return scope;
}

QString Universe::reloadRefusal(const Galaxy& galaxy) const
{
    if(m_running)
    {
        return QStringLiteral("%1 is not read again while the application runs").arg(galaxy.objectName());
    }
    return pathTo(galaxy)
               ? QString()
               : QStringLiteral("%1 is not read again: it is not a galaxy of the application").arg(galaxy.objectName());
}

std::optional<QJsonObject> Universe::readAgain(const Galaxy& galaxy, const Scope& parent, QString& error)
{
    std::optional<QJsonObject> project = readGalaxyProject(galaxy.filePath(), parent, error);
    if(!project)
    {
        error = QStringLiteral("%1 is not read again: %2").arg(galaxy.objectName(), error);
    }
    return project;
}

QStringList Universe::reload(Galaxy& galaxy, const QJsonObject& project, const Scope& parent)
{
    emit galaxy.aboutToBeReloaded();
    const QList<Binding> bindings = bindingsWith(galaxy, components(parent.galaxy));
    const QJsonObject values = exposedValues(galaxy);
    const bool hadSettingsFiles = hasSettingsFiles(galaxy);
    galaxy.empty();
    QStringList problems = refill(galaxy, project, parent);
    problems.append(bindAgain(bindings));
    problems.append(giveBackValues(galaxy, values)); // Those of this instance, in place of those of its file
    tellReloaded(galaxy, hadSettingsFiles);
    return problems;
}

QStringList Universe::refill(Galaxy& galaxy, const QJsonObject& project, const Scope& parent)
{
    QString error;
    if(!fillGalaxy(galaxy, project, parent, error))
    {
        return {QStringLiteral("%1 is left empty: %2").arg(galaxy.objectName(), error)};
    }
    return galaxyProblems(galaxy);
}

void Universe::tellReloaded(Galaxy& galaxy, bool hadSettingsFiles)
{
    emit galaxy.reloaded();
    if(hadSettingsFiles || hasSettingsFiles(galaxy))
    {
        emit settingsFilesChanged(); // Its files are others
    }
}

QJsonObject Universe::exposedValues(const Galaxy& galaxy) const
{
    QJsonObject values;
    const QList<ExposedProperty> exposed = galaxy.exposedProperties();
    for(const ExposedProperty& property : exposed)
    {
        if(const std::optional<QJsonValue> value = savedValue(property))
        {
            values.insert(property.name, *value);
        }
    }
    return values;
}

QJsonObject Universe::saveProject(const std::vector<std::unique_ptr<Component>>& components,
                                  const std::vector<std::unique_ptr<SettingsFile>>& files, const QString& directory,
                                  const QString& description) const
{
    QJsonObject project{{QStringLiteral("version"), ProjectVersion}};
    insertUnlessEmpty(project, u"description", description);
    project.insert(u"plugins", pluginsJson(components));
    project.insert(u"components", componentsJson(components, directory));
    project.insert(u"bindings", bindingsJson(components));
    insertUnlessEmpty(project, u"settings", jsonArray(files, settingsFileJson));
    return project;
}

QJsonArray Universe::pluginsJson(const std::vector<std::unique_ptr<Component>>& components) const
{
    QJsonArray plugins;
    for(const auto& [pluginId, filePath] : pluginFiles(components))
    {
        // Relative, for the project to be read wherever its plugins are moved to, with their root; it cannot be when
        // the file is on another drive than the root.
        plugins.append(QJsonObject{{QStringLiteral("id"), pluginId},
                                   {QStringLiteral("file"), m_pluginRoot.relativeFilePath(filePath)}});
    }
    return plugins;
}

std::map<QString, QString> Universe::pluginFiles(const std::vector<std::unique_ptr<Component>>& components) const
{
    std::map<QString, QString> files; // By plugin id: each of them once
    for(const std::unique_ptr<Component>& component : components)
    {
        addPluginFiles(*component, files);
    }
    return files;
}

void Universe::addPluginFiles(const Component& component, std::map<QString, QString>& files) const
{
    if(const auto* galaxy = qobject_cast<const Galaxy*>(&component))
    {
        files.merge(pluginFiles(galaxy->components()));
    }
    else if(const std::optional<PluginInfo> plugin = pluginOf(component))
    {
        files.emplace(plugin->id, plugin->filePath);
    }
}

std::optional<PluginInfo> Universe::pluginOf(const Component& component) const
{
    const std::optional<ComponentInfo> info = ComponentFactory::instance().component(component.componentId());
    return info ? m_plugins.plugin(info->pluginId) : std::nullopt;
}

QJsonArray Universe::componentsJson(const std::vector<std::unique_ptr<Component>>& components,
                                    const QString& directory) const
{
    return jsonArray(components, [this, &directory](const std::unique_ptr<Component>& component)
                     { return componentJson(*component, directory); });
}

QJsonObject Universe::componentJson(const Component& component, const QString& directory) const
{
    QJsonObject json = contentJson(component, directory);
    json.insert(u"name", component.objectName());
    json.insert(u"type", component.componentId());
    return json;
}

QJsonObject Universe::contentJson(const Component& component, const QString& directory) const
{
    if(const auto* galaxy = qobject_cast<const Galaxy*>(&component))
    {
        return galaxyJson(*galaxy, directory);
    }
    if(const auto* stargate = qobject_cast<const Stargate*>(&component))
    {
        return stargateJson(*stargate);
    }
    return propertiesJson(component);
}

QJsonObject Universe::galaxyJson(const Galaxy& galaxy, const QString& directory) const
{
    // Its file, and the values of what it exposes: its content is the one of its file. Relative to the directory of the
    // project, for both to be moved together; whole without a project file, or on another drive.
    QJsonObject json{{QStringLiteral("galaxy"),
                      directory.isEmpty() ? galaxy.filePath() : QDir(directory).relativeFilePath(galaxy.filePath())}};
    insertUnlessEmpty(json, u"properties", exposedValues(galaxy));
    return json;
}

QJsonObject Universe::propertiesJson(const Component& component) const
{
    QJsonObject json{
        {QStringLiteral("properties"), savedValues(component, ProjectJson::componentProperties(component))}};
    const auto toJson = [this](const PropertyObject& added) { return objectJson(added); };
    insertUnlessEmpty(json, u"objects", jsonArray(component.propertyObjects(), toJson));
    return json;
}

QJsonObject Universe::objectJson(const PropertyObject& added) const
{
    return {{QStringLiteral("name"), added.name},
            {QStringLiteral("properties"), savedValues(*added.object, added.properties())}};
}

std::vector<std::unique_ptr<SettingsFile>>& Universe::settingsFilesOf(Galaxy* galaxy)
{
    return galaxy != nullptr ? galaxy->m_settingsFiles : m_settingsFiles;
}

SettingsFile& Universe::addSettingsFile(const Scope& scope, const QString& fileName, QStringList& problems)
{
    return scope.galaxy == nullptr ? addApplicationSettingsFile(fileName, problems)
                                   : addGalaxySettingsFile(scope, fileName, problems);
}

SettingsFile& Universe::addApplicationSettingsFile(const QString& fileName, QStringList& problems)
{
    SettingsFile& file = addSettingsFile();
    // Not the name of one read before: they would be written over each other.
    if(!fileName.isEmpty() && !file.setFileName(fileName))
    {
        problems.append(renamedFile(fileName, file.fileName()));
    }
    return file;
}

SettingsFile& Universe::addGalaxySettingsFile(const Scope& scope, const QString& fileName, QStringList& problems)
{
    const QString name = freeGalaxyFileName(scope, fileName.isEmpty() ? QStringLiteral("settings.json") : fileName);
    if(!fileName.isEmpty() && name != fileName)
    {
        problems.append(renamedFile(scope.prefix + fileName, scope.prefix + name));
    }
    // Not made with std::make_unique: its constructor is only for the application.
    SettingsFile& file =
        *scope.galaxy->m_settingsFiles.emplace_back(new SettingsFile(*this, scope.prefix + name, scope.galaxy, name));
    connect(&file, &SettingsFile::failed, this, &Universe::settingsFileFailed);
    return file; // Followed once its galaxy is filled, see fillGalaxy(), or by who adds it
}

QString Universe::freeGalaxyFileName(const Scope& scope, const QString& base) const
{
    // After its galaxy: each instance of it has the same files.
    const auto isFree = [this, &scope](const QString& name)
    {
        return settingsFile(scope.prefix + name) == nullptr &&
               std::ranges::none_of(scope.galaxy->m_settingsFiles, [&name](const std::unique_ptr<SettingsFile>& file)
                                    { return file->name().compare(name, Qt::CaseInsensitive) == 0; });
    };
    return freeFileName(base, isFree);
}

QList<SettingsFile*> Universe::allSettingsFiles() const
{
    return allFiles(m_settingsFiles, m_components);
}

void Universe::nameSettingsFiles(Galaxy& galaxy, const QString& prefix)
{
    for(const std::unique_ptr<SettingsFile>& file : galaxy.m_settingsFiles)
    {
        file->rename(prefix + file->name(), file->name()); // Keeps its file name when the new one is taken
    }
    for(Galaxy* inner : componentsOfType<Galaxy>(galaxy.m_components))
    {
        nameSettingsFiles(*inner, prefix + inner->objectName() + u'.');
    }
}

QString Universe::newSettingsFileName() const
{
    return freeFileName(QStringLiteral("settings.json"),
                        [this](const QString& name) { return settingsFile(name) == nullptr; });
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

void Universe::startSettingsFiles()
{
    for(SettingsFile* file : allSettingsFiles())
    {
        file->start();
    }
}

void Universe::stopSettingsFiles()
{
    const QList<SettingsFile*> files = allSettingsFiles();
    for(SettingsFile* file : files | std::views::reverse)
    {
        file->stop();
    }
}

bool Universe::startComponents()
{
    const Component* refused = startInOrder(m_components);
    if(refused != nullptr)
    {
        m_errorString = QStringLiteral("%1 could not be started").arg(refused->objectName());
        returnToEdition();
    }
    return refused == nullptr;
}

void Universe::keepEditedValues()
{
    m_editedValues.clear();
    keepEditedValues(m_components);
}

void Universe::keepEditedValues(const std::vector<std::unique_ptr<Component>>& components)
{
    for(const std::unique_ptr<Component>& component : components)
    {
        keepEditedValues(*component);
    }
}

void Universe::keepEditedValues(Component& component)
{
    // Those of its galaxies too: what they do while they run is undone as well.
    if(const auto* galaxy = qobject_cast<const Galaxy*>(&component))
    {
        keepEditedValues(galaxy->components());
    }
    else if(qobject_cast<const Stargate*>(&component) == nullptr) // No property of its own
    {
        keepPropertyValues(component);
    }
}

void Universe::keepPropertyValues(Component& component)
{
    keepValues(component, ProjectJson::componentProperties(component));
    const QList<PropertyObject> objects = component.propertyObjects();
    for(const PropertyObject& added : objects)
    {
        keepValues(*added.object, added.properties());
    }
}

void Universe::keepValues(QObject& object, const QList<QMetaProperty>& properties)
{
    m_editedValues.push_back(
        EditedValues{.object = &object, .properties = properties, .values = writableProperties(object, properties)});
}

void Universe::returnToEdition()
{
    stopSettingsFiles();
    // Once the files wrote what the user changed: it is undone here.
    for(const EditedValues& edited : m_editedValues)
    {
        giveBack(edited);
    }
    m_editedValues.clear();
}

void Universe::giveBack(const EditedValues& edited)
{
    if(edited.object != nullptr) // Not when it was destroyed while it ran
    {
        ProjectJson::writeProperties(*edited.object, edited.properties, edited.values);
    }
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
