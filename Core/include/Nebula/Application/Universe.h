// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QMetaProperty>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <map>
#include <memory>
#include <optional>
#include <vector>

#include "Core_export.h"
#include "Nebula/Component/Component.h"
#include "Nebula/Galaxy/Galaxy.h"
#include "Nebula/Galaxy/GalaxyInfo.h"
#include "Nebula/Galaxy/Stargate.h"
#include "Nebula/Plugin/PluginManager.h"
#include "Nebula/Settings/SettingsFile.h"

class QCommandLineParser;

namespace Nebula
{

// An application assembled from the components of the plugins, which it runs. It owns the instances of its components,
// each named, as its objectName, after its component and a number, e.g. Greeter1. It is read from a project file, as
// NebulaStudio writes them, see README.md, or assembled with instantiate() and Component::bindInterface().
// A project file is a galaxy, named after its file: readGalaxy() tells what it is without loading it, and
// missingPlugins() whether it can be loaded. instantiateGalaxy() places it in the application, as one component, a
// Galaxy, as many times as wanted. What its stargates expose, see addStargate(), is what the galaxy has once placed.
// A galaxy placed is assembled as the application is, through an instance of it, which saveGalaxy() writes as its file
// holds it: once the file is written, reloadGalaxy() gives the other instances, see galaxies(), what it holds now.
// It is either edited or running, as its components are. While it runs, it is not assembled: its components, and those
// of its galaxies, are neither instantiated, renamed nor removed. What changes their properties meanwhile, its settings
// files or its components themselves, is undone once it stops: they have the values of the edition again, the ones
// save() writes.
// Its settings files hold the properties its user changes while it runs, and finds again the next time: see
// SettingsFile.
// It needs a QCoreApplication, and a QApplication as soon as one of its components shows a widget.
class CORE_EXPORT Universe : public QObject
{
    Q_OBJECT

  public:
    // Finds the plugins of PluginManager::defaultDirectory().
    explicit Universe(QObject* parent = nullptr);
    // Stops it.
    ~Universe() override;

    // The plugins its components are from: those it found, and those the projects it loaded name.
    [[nodiscard]] PluginManager& plugins();
    [[nodiscard]] const PluginManager& plugins() const;

    // The directory the files of the plugins are relative to in a project: the one of the application until it is set.
    [[nodiscard]] QString pluginRoot() const;
    void setPluginRoot(const QString& directory);

    // Adds to parser, before it processes the command line, the options every application assembled with it
    // understands: --plugin-dir <directory>, as many times as needed, and --plugin-root <directory>.
    static void addCommandLineOptions(QCommandLineParser& parser);
    // Does what the options addCommandLineOptions() added to parser ask, once it processed the command line: scans
    // each --plugin-dir after plugins/, in their order, skipping with a warning one that is not a directory, then sets
    // the plugin root to --plugin-root when it is given. Before a project is loaded, for its plugins to be found there.
    void applyCommandLine(const QCommandLineParser& parser);

    // The project the file filePath holds; nullopt, with error set, when it cannot be read or is not a project of the
    // version this Core reads.
    [[nodiscard]] static std::optional<QJsonObject> read(const QString& filePath, QString& error);
    // What the project file filePath tells of its galaxy, its name, its description and its plugins, without loading
    // it; nullopt, with error set, when read() cannot read it.
    [[nodiscard]] static std::optional<GalaxyInfo> readGalaxy(const QString& filePath, QString& error);
    // The plugins of galaxy none of plugins() is, by id, in its order: it can be loaded when there are none. Unlike
    // load(), which looks for a plugin it does not know in the file its project gives it, the file is not looked at.
    [[nodiscard]] QStringList missingPlugins(const GalaxyInfo& galaxy) const;
    // Replaces its components with those of project, as save() wrote it, and its description with the one of project:
    // loads its plugins, instantiates its components, sets their properties, places its galaxies, see
    // instantiateGalaxy(), binds their interfaces and adds its settings files. Returns what could not be restored, a
    // sentence each, those of its galaxies included: none when everything was.
    QStringList load(const QJsonObject& project);
    // load() of the project the file filePath holds, from then on its filePath(). Returns false, with errorString()
    // set, when the file cannot be read, nothing being changed then, and when anything could not be restored.
    bool open(const QString& filePath);
    // Its components as a project: its "version", its "description" when it has one, the "plugins" they are from,
    // those of its galaxies included, the "components" with their properties, or the "galaxy" file of a galaxy with
    // the values of what it exposes, or what a stargate exposes, and their "bindings", and its "settings" files when it
    // has some. While it runs, the properties have the values they had when it started, those of the edition.
    [[nodiscard]] QJsonObject save() const;
    // galaxy, one of its galaxies or of those they hold, as its file is to hold it, as save() writes the application:
    // its description, its components, their values and bindings, its settings files, under the names its file gives
    // them, and the files of its galaxies, relative to its own. The values of what it exposes are those of galaxy,
    // which the other instances, read again, do not take: see reloadGalaxy().
    [[nodiscard]] QJsonObject saveGalaxy(const Galaxy& galaxy) const;
    // Reads the file of galaxy, one of its galaxies or of those they hold, again, as it holds it now, in place of what
    // galaxy holds: its components and settings files are destroyed, see Galaxy::aboutToBeReloaded(), then those of
    // its file are placed in it, see instantiateGalaxy(). It keeps its name, the values of the properties it exposes
    // that it still exposes, and what is bound to its interfaces, and what they are bound to, under the names they
    // still have. Returns what could not be restored, a sentence each, those of Galaxy::problems() included: none when
    // everything was. Nothing is changed while it runs, nor when its file cannot be read, or one of its plugins is
    // unknown or cannot be loaded, which is told.
    QStringList reloadGalaxy(Galaxy& galaxy);

    // The project file it is read from and saved to, absolute; empty until it is set, or opened. The file of a galaxy
    // is written relative to its directory, and read from it, from the current directory when it has none; a galaxy
    // that holds it, directly or not, cannot be placed in it.
    [[nodiscard]] QString filePath() const;
    void setFilePath(const QString& filePath);

    // What it is for, in a few words, that its project holds: the description of the galaxy its file is. Empty until
    // it is given one, and once it is cleared.
    [[nodiscard]] QString description() const;
    void setDescription(const QString& description);

    // Creates componentId, in the Editing state, in galaxy, one of its galaxies or of those they hold, or in the
    // application itself when galaxy is nullptr; nullptr when it is not registered, when galaxy is not one of its
    // galaxies, or while it runs. It is named name, as one read from a project is, unless name is empty or the name of
    // another instance where it is.
    Component* instantiate(const QString& componentId, const QString& name = {}, Galaxy* galaxy = nullptr);
    // Places the galaxy of the project file filePath in it, a Galaxy named name, unless name is empty or the name of
    // another instance: after its galaxy and a number then, Clock1 for Clock.nbp. Its plugins are found by their id
    // among plugins(), not by their file, and loaded when they are not yet; the galaxies it holds, at a path relative
    // to its file, are placed in it the same way. Its settings files are among those of the application, named after
    // it, as each instance of the galaxy has them: settings.json of Clock1 is Clock1.settings.json, and the one of a
    // galaxy Clock1 in Home1 is Home1.Clock1.settings.json. What could not be restored is told by Galaxy::problems().
    // It is placed in galaxy, one of its galaxies or of those they hold, as instantiate() places a component.
    // Returns nullptr, with errorString() set, while it runs, when galaxy is not one of its galaxies, when the file
    // cannot be read, when one of its plugins is unknown or cannot be loaded, and when it would hold itself, the file
    // of the application, or the file of a galaxy it is placed in, directly or not.
    // The galaxy provides and consumes what its stargates expose, and its properties are those they expose, see
    // Galaxy::exposedProperties(): the ones of its file until they are set on it.
    Galaxy* instantiateGalaxy(const QString& filePath, const QString& name = {}, Galaxy* galaxy = nullptr);
    // Adds a stargate, which exposes nothing yet, named name, unless name is empty or the name of another instance:
    // Stargate1, Stargate2... then. What it exposes is what the galaxy its file is has once placed: see Stargate. It is
    // added to galaxy, as instantiate() adds a component, whose galaxy has what it exposes at once. Returns nullptr
    // when galaxy is not one of its galaxies, and while it runs.
    Stargate* addStargate(const QString& name = {}, Galaxy* galaxy = nullptr);
    // Names instance, one of its components or of those of its galaxies, name: what component() finds it by, and a
    // project holds it as; the settings files of a galaxy are named after it again. Returns false, changing nothing,
    // when name is empty or the name of another instance where it is, and while it runs.
    bool rename(Component& instance, const QString& name);
    // Destroys instance, one of its components or of those of its galaxies: nothing may refer to it any longer. What
    // was bound to its interfaces is unbound from them. Returns false, doing nothing, when it is none of them, and
    // while it runs.
    bool remove(Component& instance);
    // In the order they were instantiated, its galaxies among them: not the components they hold.
    [[nodiscard]] const std::vector<std::unique_ptr<Component>>& components() const;
    // Those of galaxy, one of its galaxies or of those they hold, see Galaxy::components(); components() when galaxy is
    // nullptr.
    [[nodiscard]] const std::vector<std::unique_ptr<Component>>& components(const Galaxy* galaxy) const;
    // The galaxy instance is one of the components of, directly; nullptr when it is one of its own, or of none.
    [[nodiscard]] Galaxy* galaxyOf(const Component& instance) const;
    // Its galaxies of the file filePath, and those of the galaxies they hold, in the order of the application: the
    // instances saveGalaxy() and reloadGalaxy() are for.
    [[nodiscard]] QList<Galaxy*> galaxies(const QString& filePath) const;
    // The one named name; nullptr when none is.
    [[nodiscard]] Component* component(const QString& name) const;
    // Those of the plugin pluginId, and the galaxies that hold one, directly or not, in the order they were
    // instantiated.
    [[nodiscard]] QList<Component*> componentsOf(const QString& pluginId) const;
    // Stops it, destroys its components and its settings files, and empties its description: nothing may refer to them
    // any longer.
    void clear();

    // Adds a settings file, which holds no property yet, named settings.json, or settings2.json, settings3.json... when
    // another one is: see SettingsFile.
    SettingsFile& addSettingsFile();
    // Adds a settings file to galaxy, one of its galaxies or of those they hold, named as the file of galaxy names it,
    // see SettingsFile::name(), settings.json, or settings2.json... when another file of galaxy is, or when the one it
    // has in galaxy is taken; nullptr when galaxy is not one of its galaxies.
    SettingsFile* addSettingsFile(Galaxy& galaxy);
    // Destroys file, one of its settings files or of those of its galaxies, written and stopped first when it runs: its
    // properties have their default values again. Does nothing when it is none of them.
    void removeSettingsFile(SettingsFile& file);
    // In the order they were added; not those of its galaxies, see Galaxy::settingsFiles().
    [[nodiscard]] const std::vector<std::unique_ptr<SettingsFile>>& settingsFiles() const;
    // The one of its settings files, or of those of its galaxies, that holds entry; nullptr when none does.
    [[nodiscard]] SettingsFile* settingsFile(const SettingsEntry& entry) const;
    // The one of its settings files, or of those of its galaxies, named fileName, whatever its case; nullptr when
    // none is.
    [[nodiscard]] SettingsFile* settingsFile(const QString& fileName) const;

    // Keeps the values the properties of its components, of their objects and of the components of its galaxies have:
    // those of the edition. Then gives the properties its settings files, and those of its galaxies, hold the values
    // of their files, and starts its components, in the order they were instantiated. Returns false, with
    // errorString() set, when one refuses: those it started are stopped again, and it is edited again, as stop()
    // leaves it. Returns true, doing nothing, while it runs.
    bool start();
    // Stops its components, the last one instantiated first, then its settings files, which write the values of the
    // user, and gives the properties the values of the edition again. Does nothing while it does not run.
    void stop();
    // Whether start() was called with success, and stop() not since.
    [[nodiscard]] bool isRunning() const;

    // Removes its components of the plugin pluginId, as remove() does, then unloads it, see PluginManager::unload().
    // Returns false, with errorString() set, while it runs, nothing being changed then, and when the plugin cannot be
    // unloaded, its components being removed all the same.
    bool unloadPlugin(const QString& pluginId);

    // Why open(), start() or unloadPlugin() returned false, or instantiateGalaxy() nullptr.
    [[nodiscard]] QString errorString() const;

  signals:
    void runningChanged(bool running);
    // A component was instantiated, removed, or all of them were: components() no longer holds those removed, which
    // are destroyed once this returns.
    void componentsChanged();
    // A settings file was added or removed, or one of them changed, see SettingsFile::changed().
    void settingsFilesChanged();
    // A settings file could not be read or written, see SettingsFile::failed(); why is a sentence to show.
    void settingsFileFailed(const QString& why);

  private:
    // The properties a project holds of one of its components, or of one of their objects, with the values they had
    // when it started.
    struct EditedValues
    {
        QPointer<QObject> object; // nullptr once it is destroyed
        QList<QMetaProperty> properties;
        QJsonObject values; // By name
    };

    // Where a project is loaded: the application itself, or one of its galaxies.
    struct Scope
    {
        Galaxy* galaxy = nullptr; // nullptr for the application
        QDir directory;           // The files of its galaxies are relative to it
        QStringList files;        // Canonical: those of the galaxies it is in, and of the application, it cannot hold
        QString prefix;           // Of the names of its settings files: the galaxies it is in, each with a dot
        QString* cycle = nullptr; // Set, in a galaxy, when one it holds would hold itself: it is refused then
    };

    // The stargates of a project, each with its JSON: what they expose is read once every component is there.
    using StargateJsons = QList<std::pair<Stargate*, QJsonObject>>;

    // Sets errorString() to why and returns false.
    bool refuse(const QString& why);
    // Scans directory, given on the command line; skips it, with a warning, when it is not one.
    void scanDirectory(const QString& directory);
    // Loads the plugin pluginId, unless it is loaded already; false, with PluginManager::errorString() set, when it
    // cannot be.
    bool loadPlugin(const QString& pluginId);

    // The directory of its file, canonical when it exists; empty when it has no file.
    [[nodiscard]] QString projectDirectory() const;

    // The scope of its own project.
    [[nodiscard]] Scope applicationScope() const;
    // The scope of galaxy, being filled in parent; cycle is set when a galaxy it holds would hold itself.
    [[nodiscard]] static Scope innerScope(Galaxy& galaxy, const Scope& parent, QString* cycle);
    // The galaxies from the application down to the one that holds instance, directly, in that order: none when the
    // application does. nullopt when none does.
    [[nodiscard]] std::optional<QList<Galaxy*>> pathTo(const Component& instance) const;
    // The scope of the last galaxy of path, see pathTo(), in the one before; the one of the application when path is
    // empty.
    [[nodiscard]] Scope scope(const QList<Galaxy*>& path) const;
    // The scope instance is in; nullopt when it is none of those of the application, at any depth.
    [[nodiscard]] std::optional<Scope> scopeOf(const Component& instance) const;
    // The scope of galaxy, one of its galaxies or of those they hold, or of the application when galaxy is nullptr;
    // nullopt when it is none of them.
    [[nodiscard]] std::optional<Scope> galaxyScope(Galaxy* galaxy) const;
    [[nodiscard]] std::vector<std::unique_ptr<Component>>& scopeComponents(const Scope& scope);
    // The one of scope named name; nullptr when none is.
    [[nodiscard]] Component* scopeComponent(const Scope& scope, const QString& name) const;
    // name, unless it is empty or the name of one of scope: base and the first number none is named with then.
    [[nodiscard]] QString freeName(const Scope& scope, const QString& name, const QString& base) const;
    // Whether name is free for instance in scope: none of scope but instance is named so.
    [[nodiscard]] bool isFreeFor(const Scope& scope, const QString& name, const Component& instance) const;
    // Names instance, of scope, name: the settings files of a galaxy are named after it.
    void setName(Component& instance, const QString& name, const Scope& scope);
    // A component of the type componentId in scope, named name or after its type when it is taken; nullptr when no
    // loaded plugin provides it.
    Component* create(const Scope& scope, const QString& componentId, const QString& name);
    // A stargate of scope, named name or after the stargates when it is taken.
    Stargate* newStargate(const Scope& scope, const QString& name);
    // Adds component to scope, named after freeName(); nullptr when component is nullptr.
    Component* adopt(const Scope& scope, std::unique_ptr<Component> component, const QString& name,
                     const QString& base);
    // adopt() for a galaxy, named already.
    Galaxy* adoptGalaxy(const Scope& scope, std::unique_ptr<Galaxy> galaxy);
    // adoptGalaxy() while the application is edited: its settings files change when the galaxy has some.
    Galaxy* placeGalaxy(const Scope& scope, std::unique_ptr<Galaxy> galaxy);
    // Tells, when scope is the application, that its components changed.
    void tellComponentsChanged(const Scope& scope);
    // Tells, when scope is the application, that its components changed, and its settings files too when adopted is a
    // galaxy that has some.
    void tellAdopted(const Scope& scope, const Component& adopted);
    // Tells that the settings files of the application changed when component is a galaxy that has some: they come or
    // go with it.
    void tellSettingsFilesOf(const Component* component);
    // Destroys its components, out of components() first, as remove() does.
    void removeComponents();
    // Removes the instances of the plugin pluginId, and the galaxies that hold one.
    void removeComponentsOf(const QString& pluginId);

    // The plugins of project, loaded unless they are; returns those that could not be.
    QStringList loadProjectPlugins(const QJsonArray& plugins);
    // Loads plugin, one of a project, opened from its file when it is not known already; why it could not, empty when
    // it could.
    QString loadProjectPlugin(const QJsonObject& plugin);
    // Fills scope with the components of project, its galaxies, their links and its settings files, as load() does,
    // the plugins being loaded. Returns what could not be restored.
    QStringList loadScope(const Scope& scope, const QJsonObject& project);
    // Places in scope components, those of a project, galaxies and stargates included; problems tells what could not
    // be. Returns the stargates.
    StargateJsons loadComponents(const Scope& scope, const QJsonArray& components, QStringList& problems);
    // Places in scope the component json of a project, a galaxy or a stargate included; problems tells what could not
    // be. Returns it; nullptr when it is missing.
    Component* loadComponent(const Scope& scope, const QJsonObject& json, QStringList& problems);
    // Places in scope the component json of a project, of a plugin, with its properties and those of its objects;
    // problems tells what could not be. Returns it; nullptr when no loaded plugin provides it.
    Component* loadInstance(const Scope& scope, const QJsonObject& json, QStringList& problems);
    // Places in scope the galaxy json, a component of a project, see createGalaxy(), with the values json gives what it
    // exposes; problems tells what could not be restored, it missing included. Returns it; nullptr when it is missing.
    Galaxy* loadGalaxy(const Scope& scope, const QJsonObject& json, QStringList& problems);
    // Has stargates, of scope, expose the interfaces their JSON gives them, of the types of the components of scope;
    // problems tells those they could not.
    void loadStargateInterfaces(const Scope& scope, const StargateJsons& stargates, QStringList& problems) const;
    // Has stargate expose, the way exposure tells, interfaces, from its JSON; problems tells those it could not.
    void loadInterfaces(const Scope& scope, Stargate& stargate, Exposure exposure, const QJsonArray& interfaces,
                        QStringList& problems) const;
    // Has stargate expose, the way exposure tells, the interface json; why it could not, empty when it could.
    QString loadInterface(const Scope& scope, Stargate& stargate, Exposure exposure, const QJsonObject& json) const;
    // Binds the components of scope as bindings, those of a project, tell; problems tells those it could not.
    void loadBindings(const Scope& scope, const QJsonArray& bindings, QStringList& problems) const;
    // Binds the components of scope as json, a binding of a project, tells; why it could not, empty when it could.
    QString loadBinding(const Scope& scope, const QJsonObject& json) const;
    // Has stargates expose the properties their JSON gives them; problems tells those they could not.
    void loadStargateProperties(const Scope& scope, const StargateJsons& stargates, QStringList& problems) const;
    // Has stargate expose the property json; why it could not, empty when it could.
    QString loadExposedProperty(const Scope& scope, Stargate& stargate, const QJsonObject& json) const;
    // Adds to scope settings, the settings files of a project; problems tells what could not be restored.
    void loadSettingsFiles(const Scope& scope, const QJsonArray& settings, QStringList& problems);
    // Adds to scope json, a settings file of a project; problems tells what could not be restored.
    void loadSettingsFile(const Scope& scope, const QJsonObject& json, QStringList& problems);
    // Adds to file, of scope, the entry json, of a project; problems tells when it could not.
    void loadEntry(const Scope& scope, SettingsFile& file, const QJsonObject& json, QStringList& problems) const;
    // The property json, an entry of a settings file or a property a stargate exposes, tells of a component of scope;
    // its component or its object nullptr when there is none. owner is set to what the property is of, as it is told.
    [[nodiscard]] SettingsEntry readEntry(const Scope& scope, const QJsonObject& json, QString& owner) const;
    // The type named typeName, QWidget* say, of an interface a component of scope provides or consumes, or registered
    // under that name otherwise; not valid when there is none.
    [[nodiscard]] QMetaType interfaceType(const Scope& scope, const QString& typeName) const;

    // The project of the galaxy of filePath, read for it to be placed in parent, whose plugins are loaded; nullopt,
    // with error set, when it cannot be placed there, see instantiateGalaxy().
    std::optional<QJsonObject> readGalaxyProject(const QString& filePath, const Scope& parent, QString& error);
    // Whether the galaxy of info, whose file is nativePath, can be placed in parent: it does not hold itself, and its
    // plugins are known and loaded. error tells why otherwise.
    bool canPlace(const QString& nativePath, const GalaxyInfo& info, const Scope& parent, QString& error);
    // Whether the galaxy of the file nativePath, canonicalPath canonical, would hold itself in parent, directly or
    // not; error and the cycle of parent tell it then.
    static bool isCycle(const QString& nativePath, const QString& canonicalPath, const Scope& parent, QString& error);
    // Tells the galaxy of parent that one it holds would hold itself, as cycle tells: it is refused as well.
    static void tellCycle(const Scope& parent, const QString& cycle);
    // Whether the application knows the plugins of the galaxy of info, whose file is nativePath; error tells those it
    // does not.
    bool knowsPlugins(const QString& nativePath, const GalaxyInfo& info, QString& error) const;
    // Loads the plugins of the galaxy of info, whose file is nativePath; false, with error set, at the first that
    // cannot be.
    bool loadPlugins(const QString& nativePath, const GalaxyInfo& info, QString& error);
    // The galaxy of filePath, named name in parent, see instantiateGalaxy(); nullptr, with error set, when it cannot
    // be placed there. Not added to parent yet.
    std::unique_ptr<Galaxy> createGalaxy(const QString& filePath, const QString& name, const Scope& parent,
                                         QString& error);
    // An empty galaxy of the file filePath, named name in parent, or after its file when it is taken.
    std::unique_ptr<Galaxy> emptyGalaxy(const QString& filePath, const QString& name, const Scope& parent) const;
    // Fills galaxy, an empty one named as it is to be in parent, of its file, with project, the one of its file, as
    // loadScope() does. Returns false, with error set and galaxy empty, when a galaxy of project would hold itself.
    bool fillGalaxy(Galaxy& galaxy, const QJsonObject& project, const Scope& parent, QString& error);
    // Fills galaxy with project, as fillGalaxy() does; returns why it would hold itself, empty when it would not.
    QString loadGalaxyContent(Galaxy& galaxy, const QJsonObject& project, const Scope& parent);
    // Empties galaxy, which would hold itself as cycle tells, and tells parent.
    static void refuseCycle(Galaxy& galaxy, const QString& cycle, const Scope& parent);
    // Follows the settings files of galaxy, once it is filled, and attaches its stargates, once they are bound.
    void settleGalaxy(Galaxy& galaxy);
    // The scope a galaxy is placed in, the one of galaxy; nullopt, with errorString() set, when none can be.
    std::optional<Scope> placingScope(Galaxy* galaxy);
    // Why galaxy is not read again; empty when it can be.
    [[nodiscard]] QString reloadRefusal(const Galaxy& galaxy) const;
    // The project of the file of galaxy, read again for it to be filled again in parent; nullopt, with error set, when
    // it can no longer be placed there.
    std::optional<QJsonObject> readAgain(const Galaxy& galaxy, const Scope& parent, QString& error);
    // Fills galaxy again with project, the one of its file, in parent, keeping what is bound to its interfaces, either
    // way, and the values of what it exposes; returns what is lost.
    QStringList reload(Galaxy& galaxy, const QJsonObject& project, const Scope& parent);
    // Fills galaxy, emptied, with project; returns its problems, or why it is left empty.
    QStringList refill(Galaxy& galaxy, const QJsonObject& project, const Scope& parent);
    // Tells galaxy was read again, and that the settings files of the application changed when it had or has some.
    void tellReloaded(Galaxy& galaxy, bool hadSettingsFiles);
    // The values of what galaxy exposes, by the names they are exposed as, as a project holds them.
    [[nodiscard]] QJsonObject exposedValues(const Galaxy& galaxy) const;

    // What save() writes of components and files, the files of their galaxies relative to directory, whole when it is
    // empty, with description.
    [[nodiscard]] QJsonObject saveProject(const std::vector<std::unique_ptr<Component>>& components,
                                          const std::vector<std::unique_ptr<SettingsFile>>& files,
                                          const QString& directory, const QString& description) const;
    // The plugins of components, and of those of their galaxies, as a project holds them: those to load before it is
    // read again.
    [[nodiscard]] QJsonArray pluginsJson(const std::vector<std::unique_ptr<Component>>& components) const;
    // The files of the plugins of components, and of those of their galaxies, by plugin id.
    [[nodiscard]] std::map<QString, QString>
    pluginFiles(const std::vector<std::unique_ptr<Component>>& components) const;
    // Adds the plugin of component, or those of the components of a galaxy, to files, by id.
    void addPluginFiles(const Component& component, std::map<QString, QString>& files) const;
    // The plugin of component; nullopt when it is no longer known.
    [[nodiscard]] std::optional<PluginInfo> pluginOf(const Component& component) const;
    // components as a project holds them, the files of their galaxies relative to directory.
    [[nodiscard]] QJsonArray componentsJson(const std::vector<std::unique_ptr<Component>>& components,
                                            const QString& directory) const;
    // component as a project holds it: its name, its type, and what contentJson() writes.
    [[nodiscard]] QJsonObject componentJson(const Component& component, const QString& directory) const;
    // What a project holds of component beyond its name and its type: what galaxyJson(), stargateJson() or
    // propertiesJson() writes.
    [[nodiscard]] QJsonObject contentJson(const Component& component, const QString& directory) const;
    // galaxy as a project holds it: its file, relative to directory, and the values of what it exposes.
    [[nodiscard]] QJsonObject galaxyJson(const Galaxy& galaxy, const QString& directory) const;
    // The properties of component, and those of its objects, as a project holds them.
    [[nodiscard]] QJsonObject propertiesJson(const Component& component) const;
    // added, an object of a component, as a project holds it.
    [[nodiscard]] QJsonObject objectJson(const PropertyObject& added) const;

    // The settings files of galaxy, or its own when galaxy is nullptr.
    [[nodiscard]] std::vector<std::unique_ptr<SettingsFile>>& settingsFilesOf(Galaxy* galaxy);
    // A settings file of scope, named fileName when it is not empty, after its galaxy in a galaxy; when the name is
    // taken, it is named as addSettingsFile() names it, or with a number after its name in a galaxy, which problems
    // tells.
    SettingsFile& addSettingsFile(const Scope& scope, const QString& fileName, QStringList& problems);
    // addSettingsFile() in the application.
    SettingsFile& addApplicationSettingsFile(const QString& fileName, QStringList& problems);
    // addSettingsFile() in the galaxy of scope.
    SettingsFile& addGalaxySettingsFile(const Scope& scope, const QString& fileName, QStringList& problems);
    // base, or the first name numbered after it that no other file of the galaxy of scope has, whatever its case, nor
    // another file of the application the one it would have then.
    [[nodiscard]] QString freeGalaxyFileName(const Scope& scope, const QString& base) const;
    // Its settings files, then those of its galaxies, in the order they start.
    [[nodiscard]] QList<SettingsFile*> allSettingsFiles() const;
    // Names the settings files of galaxy, and of the galaxies it holds, after prefix, as createGalaxy() does.
    void nameSettingsFiles(Galaxy& galaxy, const QString& prefix);
    // settings.json, or settings<n>.json with the first number from 2 no settings file is named with.
    [[nodiscard]] QString newSettingsFileName() const;

    // The properties a project holds of object, among properties: those of the edition while it runs.
    [[nodiscard]] QJsonObject savedValues(const QObject& object, const QList<QMetaProperty>& properties) const;
    // The value a project holds of property, one a galaxy exposes: the one of the edition while it runs. nullopt when
    // JSON cannot hold it.
    [[nodiscard]] std::optional<QJsonValue> savedValue(const ExposedProperty& property) const;
    // Starts its settings files, in their order.
    void startSettingsFiles();
    // Stops its settings files, in the reverse order.
    void stopSettingsFiles();
    // Starts its components in their order; at the first that refuses, returns to the edition, errorString() telling
    // it, and returns false.
    bool startComponents();
    // Keeps the values of the edition, see EditedValues.
    void keepEditedValues();
    // keepEditedValues() of each of components.
    void keepEditedValues(const std::vector<std::unique_ptr<Component>>& components);
    // Those of component and of its objects, or of the components of a galaxy.
    void keepEditedValues(Component& component);
    // Those of component and of its objects.
    void keepPropertyValues(Component& component);
    // Those of object, among properties.
    void keepValues(QObject& object, const QList<QMetaProperty>& properties);
    // Once its components no longer run: stops its settings files, then gives the properties the values of the
    // edition again, and forgets them.
    void returnToEdition();
    // Gives the properties of edited the values of the edition again, unless its object is gone.
    static void giveBack(const EditedValues& edited);
    void setRunning(bool running);

    PluginManager m_plugins; // Before m_components: it unloads its plugins once their components are destroyed
    QDir m_pluginRoot;
    QString m_filePath;
    QString m_description;
    std::vector<std::unique_ptr<Component>> m_components;
    std::vector<std::unique_ptr<SettingsFile>> m_settingsFiles; // Destroyed first: their entries are of m_components
    bool m_running = false;
    std::vector<EditedValues> m_editedValues; // While it runs, in the order of the components and of their objects
    QString m_errorString;
};

} // namespace Nebula
