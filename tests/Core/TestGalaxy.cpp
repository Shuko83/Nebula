// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPointer>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include "Nebula/Application/Universe.h"
#include "Nebula/Component/ComponentFactory.h"
#include "Nebula/Galaxy/Galaxy.h"
#include "Probe.h"
#include "Sink.h"
#include "Source.h"
#include "TestComponents.h"

using Nebula::Galaxy;
using Nebula::Universe;
using namespace Qt::StringLiterals;

namespace
{

const QString FirstPluginId = QStringLiteral("FabienDG.Nebula.FirstPlugin");

// A galaxy of a Source, a Sink bound to it, and a Probe counting 7.
const QByteArray Clock = R"({"version": 1, "description": "Ticks",
    "components": [{"name": "Source", "type": "Test.Source", "properties": {"label": "tick"}},
                   {"name": "Sink", "type": "Test.Sink"},
                   {"name": "Probe", "type": "Test.Probe", "properties": {"count": 7}}],
    "bindings": [{"consumer": "Sink", "consumed": "object", "provider": "Source", "provided": "object"}]})";

// A galaxy whose stargate exposes what its Source provides, as "out", gives what it consumes, as "in", to its Sink,
// which needs it, and exposes the count of its Probe, as "rate".
const QByteArray Gated = R"({"version": 1,
    "components": [{"name": "Source", "type": "Test.Source"},
                   {"name": "Sink", "type": "Test.Sink"},
                   {"name": "Probe", "type": "Test.Probe", "properties": {"count": 7}},
                   {"name": "Gate", "type": "Nebula.Stargate",
                    "provided": [{"name": "out", "type": "QObject*"}],
                    "consumed": [{"name": "in", "type": "QObject*"}],
                    "exposed": [{"name": "rate", "component": "Probe", "property": "count"}]}],
    "bindings": [{"consumer": "Gate", "consumed": "out", "provider": "Source", "provided": "object"},
                 {"consumer": "Sink", "consumed": "object", "provider": "Gate", "provided": "in"}]})";

// A project that places the galaxy of the file path, relative to it, as name.
QByteArray placing(const QString& path, const QString& name = u"Clock1"_s)
{
    return R"({"version": 1, "components": [{"name": ")" + name.toUtf8() +
           R"(", "type": "Nebula.Galaxy", "galaxy": ")" + path.toUtf8() + R"("}]})";
}

// Writes content to path, relative to directory, with the directories it is in; returns its path, empty when it could
// not be written.
QString write(const QTemporaryDir& directory, const QString& path, const QByteArray& content)
{
    const QString filePath = directory.filePath(path);
    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly) || file.write(content) != content.size())
    {
        return {};
    }
    return filePath;
}

// The JSON object the file filePath holds.
QJsonObject readJson(const QString& filePath)
{
    QFile file(filePath);
    return file.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(file.readAll()).object() : QJsonObject();
}

// The names of components, in their order.
QStringList namesOf(const std::vector<std::unique_ptr<Nebula::Component>>& components)
{
    QStringList names;
    for(const std::unique_ptr<Nebula::Component>& component : components)
    {
        names.append(component->objectName());
    }
    return names;
}

// The names of interfaces, in their order.
QStringList interfaceNames(const QList<Nebula::InterfaceInfo>& interfaces)
{
    QStringList names;
    for(const Nebula::InterfaceInfo& info : interfaces)
    {
        names.append(info.name);
    }
    return names;
}

// Writes project to the file filePath in place of what it holds; false when it cannot.
bool rewrite(const QString& filePath, const QJsonObject& project)
{
    QFile file(filePath);
    const QByteArray content = QJsonDocument(project).toJson();
    return file.open(QIODevice::WriteOnly | QIODevice::Truncate) && file.write(content) == content.size();
}

// The one of interfaces named name; a default one when none is.
Nebula::InterfaceInfo interfaceNamed(const QList<Nebula::InterfaceInfo>& interfaces, const QString& name)
{
    const auto found = std::ranges::find(interfaces, name, &Nebula::InterfaceInfo::name);
    return found != interfaces.end() ? *found : Nebula::InterfaceInfo{};
}

// The value of the property galaxy exposes as name; not valid when it exposes none.
QVariant exposedValue(const Galaxy& galaxy, const QString& name)
{
    const std::optional<Nebula::ExposedProperty> exposed = galaxy.exposedProperty(name);
    return exposed ? exposed->object->property(exposed->property.toLatin1().constData()) : QVariant();
}

// Sets the property galaxy exposes as name to value; false when it exposes none, or it cannot be set.
bool setExposedValue(const Galaxy& galaxy, const QString& name, const QVariant& value)
{
    const std::optional<Nebula::ExposedProperty> exposed = galaxy.exposedProperty(name);
    return exposed && exposed->object->setProperty(exposed->property.toLatin1().constData(), value);
}

} // namespace

// Galaxy, and how Universe places it: a project file as one component, its plugins, the galaxies it holds, how it is
// saved, runs, and keeps its settings files; how an instance is edited, saved as its file holds it, and how the others
// are read again.
class TestGalaxy : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();

    void instantiateGalaxy();
    void names_data();
    void names();
    void refused_data();
    void refused();
    void loadsItsPlugins();
    void nested();
    void problems();
    void saveAndLoad_data();
    void saveAndLoad();
    void loadReportsMissingGalaxies();
    void startAndStop();
    void startRefused();
    void runningRestoresEditedValues();
    void settingsFiles();
    void settingsFilesRenamed();
    void remove();
    void componentsOfAndUnloadPlugin();

    void stargateInterfaces();
    void passesWhatItProvides();
    void passesWhatItConsumes();
    void consumedRequirement_data();
    void consumedRequirement();
    void bindingsSavedAndLoaded();
    void removedWhileBound();
    void exposedProperties();
    void exposedValuesSavedAndLoaded();
    void exposedValuesSavedWhileRunning();
    void reexposed();

    void editedInPlace();
    void stargateChangedOncePlaced();
    void placedInGalaxy();
    void settingsFilesInGalaxy();
    void saveGalaxy();
    void galaxiesOfFile();
    void reloadGalaxy();
    void reloadLosesWhatIsGone();
    void reloadRefused_data();
    void reloadRefused();
    void reloadLeftEmpty();
};

void TestGalaxy::initTestCase()
{
    registerTestComponents();
    QStandardPaths::setTestModeEnabled(true);
}

void TestGalaxy::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
}

void TestGalaxy::instantiateGalaxy()
{
    const QTemporaryDir directory;
    const QString filePath = write(directory, u"Clock.nbp"_s, Clock);
    QVERIFY(!filePath.isEmpty());

    Universe universe;
    const QSignalSpy componentsChanged(&universe, &Universe::componentsChanged);
    Galaxy* galaxy = universe.instantiateGalaxy(filePath);
    QVERIFY2(galaxy != nullptr, qPrintable(universe.errorString()));
    QVERIFY(universe.errorString().isEmpty());
    QCOMPARE(componentsChanged.count(), 1);
    QCOMPARE(namesOf(universe.components()), QStringList{"Clock1"});
    QCOMPARE(universe.component(u"Clock1"_s), galaxy);

    QCOMPARE(galaxy->componentId(), QString(Galaxy::Id));
    QCOMPARE(galaxy->galaxyName(), u"Clock"_s);
    QCOMPARE(galaxy->description(), u"Ticks"_s);
    QCOMPARE(galaxy->filePath(), QFileInfo(filePath).canonicalFilePath());
    QCOMPARE(galaxy->project().value(u"description").toString(), u"Ticks"_s);
    QCOMPARE(galaxy->problems(), QStringList());
    QCOMPARE(galaxy->state(), Nebula::ComponentState::Editing);

    // Its components, with the values and the links of its file.
    QCOMPARE(namesOf(galaxy->components()), (QStringList{"Source", "Sink", "Probe"}));
    QCOMPARE(galaxy->component(u"Probe"_s)->property("count").toInt(), 7);
    QCOMPARE(galaxy->component(u"Source"_s)->property("label").toString(), u"tick"_s);
    const Nebula::InterfaceBinding bound = galaxy->component(u"Sink"_s)->binding(u"object"_s);
    QCOMPARE(bound.provider, galaxy->component(u"Source"_s));
    QCOMPARE(bound.providedName, u"object"_s);
    QVERIFY(galaxy->component(u"Missing"_s) == nullptr);
    // Not those of the application.
    QVERIFY(universe.component(u"Probe"_s) == nullptr);

    // As many times as wanted, each with its own components.
    const Galaxy* second = universe.instantiateGalaxy(filePath);
    QVERIFY(second != nullptr);
    QCOMPARE(second->objectName(), u"Clock2"_s);
    QVERIFY(second->component(u"Probe"_s) != galaxy->component(u"Probe"_s));
}

void TestGalaxy::names_data()
{
    QTest::addColumn<QStringList>("existing"); // Probes of the application
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("named");

    QTest::newRow("after the galaxy") << QStringList{} << "" << "Clock1";
    QTest::newRow("first free number") << QStringList{"Clock1"} << "" << "Clock2";
    QTest::newRow("given") << QStringList{} << "Kitchen" << "Kitchen";
    QTest::newRow("given, taken") << QStringList{"Kitchen"} << "Kitchen" << "Clock1";
}

void TestGalaxy::names()
{
    QFETCH(QStringList, existing);
    QFETCH(QString, name);
    QFETCH(QString, named);

    const QTemporaryDir directory;
    const QString filePath = write(directory, u"Clock.nbp"_s, Clock);
    Universe universe;
    for(const QString& probe : std::as_const(existing))
    {
        QVERIFY(universe.instantiate(testComponentId(u"Probe"_s), probe) != nullptr);
    }
    const Galaxy* galaxy = universe.instantiateGalaxy(filePath, name);
    QVERIFY(galaxy != nullptr);
    QCOMPARE(galaxy->objectName(), named);
}

void TestGalaxy::refused_data()
{
    QTest::addColumn<QString>("galaxy");          // The file placed, in the directory of the test
    QTest::addColumn<QString>("applicationFile"); // The one of the application; none when empty
    QTest::addColumn<bool>("running");
    QTest::addColumn<QString>("error"); // Part of it

    QTest::newRow("missing file") << "Missing.nbp" << "" << false << "Missing.nbp cannot be read";
    QTest::newRow("not a project") << "Broken.nbp" << "" << false << "Broken.nbp cannot be read";
    QTest::newRow("unknown plugin") << "Unknown.nbp" << "" << false
                                    << "Unknown.nbp needs plugins the application does not know: Other.Plugin";
    QTest::newRow("holds itself") << "Self.nbp" << "" << false << "Self.nbp would hold itself, directly or not";
    QTest::newRow("holds itself, not directly") << "A.nbp" << "" << false << "A.nbp would hold itself, directly or not";
    QTest::newRow("is the application") << "App.nbp" << "App.nbp" << false
                                        << "App.nbp would hold itself, directly or not";
    QTest::newRow("holds the application")
        << "HoldsApp.nbp" << "App.nbp" << false << "App.nbp would hold itself, directly or not";
    QTest::newRow("while running") << "Clock.nbp" << "" << true
                                   << "A galaxy cannot be placed while the application runs";
}

void TestGalaxy::refused()
{
    QFETCH(QString, galaxy);
    QFETCH(QString, applicationFile);
    QFETCH(bool, running);
    QFETCH(QString, error);

    const QTemporaryDir directory;
    QVERIFY(!write(directory, u"Clock.nbp"_s, Clock).isEmpty());
    QVERIFY(!write(directory, u"Broken.nbp"_s, "{ version").isEmpty());
    QVERIFY(!write(directory, u"Unknown.nbp"_s,
                   R"({"version": 1, "plugins": [{"id": "Other.Plugin", "file": "other.nbdl"}]})")
                 .isEmpty());
    QVERIFY(!write(directory, u"Self.nbp"_s, placing(u"Self.nbp"_s, u"Me"_s)).isEmpty());
    QVERIFY(!write(directory, u"A.nbp"_s, placing(u"B.nbp"_s, u"B1"_s)).isEmpty());
    QVERIFY(!write(directory, u"B.nbp"_s, placing(u"A.nbp"_s, u"A1"_s)).isEmpty());
    QVERIFY(!write(directory, u"App.nbp"_s, R"({"version": 1})").isEmpty());
    QVERIFY(!write(directory, u"HoldsApp.nbp"_s, placing(u"App.nbp"_s, u"App1"_s)).isEmpty());

    Universe universe;
    if(!applicationFile.isEmpty())
    {
        universe.setFilePath(directory.filePath(applicationFile));
    }
    if(running)
    {
        QVERIFY(universe.start());
    }
    QVERIFY(universe.instantiateGalaxy(directory.filePath(galaxy)) == nullptr);
    QVERIFY2(universe.errorString().contains(error), qPrintable(universe.errorString()));
    QVERIFY(universe.components().empty());
}

void TestGalaxy::loadsItsPlugins()
{
    const QTemporaryDir directory;
    const QString filePath =
        write(directory, u"Hello.nbp"_s,
              R"({"version": 1, "plugins": [{"id": "FabienDG.Nebula.FirstPlugin", "file": "nowhere.nbdl"}],
            "components": [{"name": "Alpha1", "type": "FabienDG.Nebula.FirstPlugin.Alpha"}]})");

    Universe universe;
    universe.plugins().scan(QStringLiteral(NEBULA_TEST_PLUGINS_DIR));
    QVERIFY(!universe.plugins().isLoaded(FirstPluginId));
    // Found by its id, not by its file, and loaded.
    const Galaxy* galaxy = universe.instantiateGalaxy(filePath);
    QVERIFY2(galaxy != nullptr, qPrintable(universe.errorString()));
    QVERIFY(universe.plugins().isLoaded(FirstPluginId));
    QCOMPARE(namesOf(galaxy->components()), QStringList{"Alpha1"});

    // Saved with the plugins of the components of its galaxies.
    const QJsonArray plugins = universe.save().value(u"plugins").toArray();
    QCOMPARE(plugins.size(), 1);
    QCOMPARE(plugins.first().toObject().value(u"id").toString(), FirstPluginId);
}

void TestGalaxy::nested()
{
    const QTemporaryDir directory;
    QVERIFY(!write(directory, u"parts/Clock.nbp"_s, Clock).isEmpty());
    const QString filePath = write(directory, u"Home.nbp"_s, placing(u"parts/Clock.nbp"_s));

    Universe universe;
    const Galaxy* home = universe.instantiateGalaxy(filePath);
    QVERIFY2(home != nullptr, qPrintable(universe.errorString()));
    QCOMPARE(home->objectName(), u"Home1"_s);
    // Another component among its own, from a file relative to its own.
    const auto* clock = qobject_cast<const Galaxy*>(home->component(u"Clock1"_s));
    QVERIFY(clock != nullptr);
    QCOMPARE(clock->filePath(), QFileInfo(directory.filePath(u"parts/Clock.nbp"_s)).canonicalFilePath());
    QCOMPARE(namesOf(clock->components()), (QStringList{"Source", "Sink", "Probe"}));
}

void TestGalaxy::problems()
{
    const QTemporaryDir directory;
    QVERIFY(!write(directory, u"Partial.nbp"_s,
                   R"({"version": 1, "components": [{"name": "P", "type": "Test.Probe", "properties": {"nope": 1}},
                                                    {"name": "X", "type": "Test.Unknown"}]})")
                 .isEmpty());
    const QString filePath = write(directory, u"Home.nbp"_s,
                                   R"({"version": 1, "components": [
                                       {"name": "Part1", "type": "Nebula.Galaxy", "galaxy": "Partial.nbp"},
                                       {"name": "Gone1", "type": "Nebula.Galaxy", "galaxy": "Gone.nbp"}]})");

    // Placed all the same, telling what it misses.
    Universe universe;
    const Galaxy* home = universe.instantiateGalaxy(filePath);
    QVERIFY(home != nullptr);
    QCOMPARE(namesOf(home->components()), QStringList{"Part1"});
    const QStringList problems = home->problems();
    QCOMPARE(problems.size(), 3);
    QCOMPARE(problems.at(0), u"In Part1: The property nope of P could not be set"_s);
    QCOMPARE(problems.at(1), u"In Part1: X is missing: no loaded plugin provides Test.Unknown"_s);
    QVERIFY2(problems.at(2).startsWith(u"Gone1 is missing: "_s) && problems.at(2).contains(u"cannot be read"_s),
             qPrintable(problems.at(2)));
}

void TestGalaxy::saveAndLoad_data()
{
    QTest::addColumn<QString>("applicationFile"); // Relative to the directory of the test; none when empty
    QTest::addColumn<QString>("galaxy");          // Its file, relative to the directory of the test
    QTest::addColumn<QString>("written");         // What the project holds; the absolute path when empty

    QTest::newRow("beside the project") << "App.nbp" << "Clock.nbp" << "Clock.nbp";
    QTest::newRow("under the project") << "App.nbp" << "galaxies/Clock.nbp" << "galaxies/Clock.nbp";
    QTest::newRow("above the project") << "apps/App.nbp" << "Clock.nbp" << "../Clock.nbp";
    QTest::newRow("no project file") << "" << "Clock.nbp" << "";
}

void TestGalaxy::saveAndLoad()
{
    QFETCH(QString, applicationFile);
    QFETCH(QString, galaxy);
    QFETCH(QString, written);

    const QTemporaryDir directory;
    const QString galaxyPath = QFileInfo(write(directory, galaxy, Clock)).canonicalFilePath();
    QVERIFY(!galaxyPath.isEmpty());
    const QString applicationPath = applicationFile.isEmpty() ? QString() : directory.filePath(applicationFile);
    if(!applicationPath.isEmpty())
    {
        QVERIFY(QDir().mkpath(QFileInfo(applicationPath).absolutePath()));
    }

    QJsonObject saved;
    {
        Universe universe;
        universe.setFilePath(applicationPath);
        QVERIFY(universe.instantiateGalaxy(galaxyPath) != nullptr);
        saved = universe.save();
    }
    const QJsonObject json = saved.value(u"components").toArray().first().toObject();
    QCOMPARE(json.value(u"name").toString(), u"Clock1"_s);
    QCOMPARE(json.value(u"type").toString(), QString(Galaxy::Id));
    QCOMPARE(json.value(u"galaxy").toString(), written.isEmpty() ? galaxyPath : written);
    // Its file only: its content is the one of its file.
    QVERIFY(!json.contains(u"properties"));
    QVERIFY(saved.value(u"bindings").toArray().isEmpty());

    Universe universe;
    universe.setFilePath(applicationPath);
    QCOMPARE(universe.load(saved), QStringList());
    const auto* loaded = qobject_cast<const Galaxy*>(universe.component(u"Clock1"_s));
    QVERIFY(loaded != nullptr);
    QCOMPARE(loaded->filePath(), galaxyPath);
    QCOMPARE(namesOf(loaded->components()), (QStringList{"Source", "Sink", "Probe"}));
}

void TestGalaxy::loadReportsMissingGalaxies()
{
    const QTemporaryDir directory;
    QVERIFY(
        !write(directory, u"Partial.nbp"_s, R"({"version": 1, "components": [{"name": "X", "type": "Test.Unknown"}]})")
             .isEmpty());
    const QString filePath = write(directory, u"App.nbp"_s,
                                   R"({"version": 1, "components": [
                                       {"name": "Gone1", "type": "Nebula.Galaxy", "galaxy": "Gone.nbp"},
                                       {"name": "Part1", "type": "Nebula.Galaxy", "galaxy": "Partial.nbp"},
                                       {"name": "Probe1", "type": "Test.Probe"}]})");

    // Opened from its file: the galaxies are relative to it, and what they miss is told with what the project misses.
    Universe universe;
    QVERIFY(!universe.open(filePath));
    QCOMPARE(universe.filePath(), QFileInfo(filePath).absoluteFilePath());
    QCOMPARE(namesOf(universe.components()), (QStringList{"Part1", "Probe1"}));
    const QStringList problems = universe.errorString().split(u'\n');
    QCOMPARE(problems.size(), 2);
    QVERIFY2(problems.at(0).startsWith(u"Gone1 is missing: "_s), qPrintable(problems.at(0)));
    QCOMPARE(problems.at(1), u"In Part1: X is missing: no loaded plugin provides Test.Unknown"_s);
}

void TestGalaxy::startAndStop()
{
    const QTemporaryDir directory;
    Universe universe;
    auto* galaxy = universe.instantiateGalaxy(write(directory, u"Clock.nbp"_s, Clock));
    QVERIFY(galaxy != nullptr);
    auto* probe = qobject_cast<Probe*>(galaxy->component(u"Probe"_s));
    auto* sink = qobject_cast<Sink*>(galaxy->component(u"Sink"_s));

    QVERIFY(universe.start());
    QCOMPARE(galaxy->state(), Nebula::ComponentState::Running);
    QCOMPARE(probe->state(), Nebula::ComponentState::Running);
    QCOMPARE(probe->events(), QStringList{"start"});
    QVERIFY(sink->object() != nullptr); // What the Source provides while it runs

    universe.stop();
    QCOMPARE(galaxy->state(), Nebula::ComponentState::Editing);
    QCOMPARE(probe->state(), Nebula::ComponentState::Editing);
    QCOMPARE(probe->events(), (QStringList{"start", "stop"}));
    QVERIFY(sink->object() == nullptr);
}

void TestGalaxy::startRefused()
{
    const QTemporaryDir directory;
    Universe universe;
    auto* galaxy =
        universe.instantiateGalaxy(write(directory, u"Refusing.nbp"_s,
                                         R"({"version": 1, "components": [{"name": "First", "type": "Test.Probe"},
                                         {"name": "Refusing", "type": "Test.Probe",
                                          "properties": {"refusesToStart": true}}]})"));
    QVERIFY(galaxy != nullptr);
    auto* first = qobject_cast<Probe*>(galaxy->component(u"First"_s));

    QVERIFY(!universe.start());
    QCOMPARE(universe.errorString(), u"Refusing1 could not be started"_s);
    QVERIFY(!universe.isRunning());
    QCOMPARE(galaxy->state(), Nebula::ComponentState::Editing);
    QCOMPARE(first->events(), (QStringList{"start", "stop"}));
}

void TestGalaxy::runningRestoresEditedValues()
{
    const QTemporaryDir directory;
    Universe universe;
    const Galaxy* galaxy = universe.instantiateGalaxy(write(directory, u"Clock.nbp"_s, Clock));
    QVERIFY(galaxy != nullptr);
    Nebula::Component* probe = galaxy->component(u"Probe"_s);

    QVERIFY(universe.start());
    QVERIFY(probe->setProperty("count", 99));
    universe.stop();
    QCOMPARE(probe->property("count").toInt(), 7);
}

void TestGalaxy::settingsFiles()
{
    const QTemporaryDir directory;
    const QString settingsDirectory = directory.filePath(u"settings"_s);
    const QString filePath =
        write(directory, u"Clock.nbp"_s,
              R"({"version": 1,
        "components": [{"name": "P", "type": "Test.Probe", "properties": {"count": 7}}],
        "settings": [{"file": "clock.json", "directory": ")" +
                  settingsDirectory.toUtf8() + R"(", "entries": [{"component": "P", "property": "count"}]}]})");

    Universe universe;
    const QSignalSpy settingsFilesChanged(&universe, &Universe::settingsFilesChanged);
    const Galaxy* first = universe.instantiateGalaxy(filePath);
    const Galaxy* second = universe.instantiateGalaxy(filePath);
    QVERIFY(first != nullptr && second != nullptr);
    QCOMPARE(settingsFilesChanged.count(), 2);

    // Named after their galaxy: each instance has its own.
    QCOMPARE(first->settingsFiles().size(), std::size_t{1});
    Nebula::SettingsFile& firstFile = *first->settingsFiles().front();
    QCOMPARE(firstFile.fileName(), u"Clock1.clock.json"_s);
    QCOMPARE(second->settingsFiles().front()->fileName(), u"Clock2.clock.json"_s);
    QCOMPARE(firstFile.directory(), settingsDirectory);
    // Among those of the application, not in its own list.
    QVERIFY(universe.settingsFiles().empty());
    QCOMPARE(universe.settingsFile(u"clock1.CLOCK.json"_s), &firstFile);
    Nebula::Component* probe = first->component(u"P"_s);
    QCOMPARE(universe.settingsFile({.component = probe, .object = probe, .property = u"count"_s}), &firstFile);
    // The application cannot give another file their names.
    QVERIFY(!universe.addSettingsFile().setFileName(u"Clock2.clock.json"_s));

    // Started with the application: the values of the user of each instance in its own file.
    QVERIFY(universe.start());
    QVERIFY(probe->setProperty("count", 9));
    universe.stop();
    QCOMPARE(probe->property("count").toInt(), 7);
    const QJsonObject firstValues = readJson(QDir(settingsDirectory).filePath(u"Clock1.clock.json"_s));
    QCOMPARE(firstValues.value(u"P").toObject().value(u"properties").toObject().value(u"count").toInt(), 9);
    const QJsonObject secondValues = readJson(QDir(settingsDirectory).filePath(u"Clock2.clock.json"_s));
    QCOMPARE(secondValues.value(u"P").toObject().value(u"properties").toObject().value(u"count").toInt(), 7);
    QVERIFY(universe.start());
    QCOMPARE(probe->property("count").toInt(), 9);
    universe.stop();

    // The project holds the galaxies, not their files.
    QCOMPARE(universe.save().value(u"settings").toArray().size(), 1); // The one added above
}

void TestGalaxy::settingsFilesRenamed()
{
    const QTemporaryDir directory;
    QVERIFY(!write(directory, u"Clock.nbp"_s,
                   R"({"version": 1, "components": [{"name": "P", "type": "Test.Probe"}],
                       "settings": [{"file": "clock.json", "entries": [{"component": "P", "property": "count"}]}]})")
                 .isEmpty());
    const QString filePath = write(directory, u"Home.nbp"_s, placing(u"Clock.nbp"_s));

    Universe universe;
    auto* home = universe.instantiateGalaxy(filePath);
    QVERIFY(home != nullptr);
    const auto* clock = qobject_cast<const Galaxy*>(home->component(u"Clock1"_s));
    QVERIFY(clock != nullptr);
    // After every galaxy it is in.
    QCOMPARE(clock->settingsFiles().front()->fileName(), u"Home1.Clock1.clock.json"_s);

    QVERIFY(universe.rename(*home, u"House"_s));
    QCOMPARE(clock->settingsFiles().front()->fileName(), u"House.Clock1.clock.json"_s);
}

void TestGalaxy::remove()
{
    const QTemporaryDir directory;
    const QString filePath = write(directory, u"Clock.nbp"_s,
                                   R"({"version": 1, "components": [{"name": "P", "type": "Test.Probe"}],
        "settings": [{"file": "clock.json", "entries": [{"component": "P", "property": "count"}]}]})");
    Universe universe;
    Galaxy* galaxy = universe.instantiateGalaxy(filePath);
    QVERIFY(galaxy != nullptr);
    const QPointer<Nebula::Component> probe = galaxy->component(u"P"_s);
    const QSignalSpy settingsFilesChanged(&universe, &Universe::settingsFilesChanged);

    QVERIFY(universe.remove(*galaxy));
    QVERIFY(probe.isNull());
    QVERIFY(universe.components().empty());
    QVERIFY(universe.settingsFile(u"Clock1.clock.json"_s) == nullptr);
    QCOMPARE(settingsFilesChanged.count(), 1);
}

void TestGalaxy::componentsOfAndUnloadPlugin()
{
    const QTemporaryDir directory;
    QVERIFY(!write(directory, u"Hello.nbp"_s,
                   R"({"version": 1, "plugins": [{"id": "FabienDG.Nebula.FirstPlugin", "file": "x.nbdl"}],
                       "components": [{"name": "Alpha1", "type": "FabienDG.Nebula.FirstPlugin.Alpha"}]})")
                 .isEmpty());
    const QString filePath = write(directory, u"Home.nbp"_s, placing(u"Hello.nbp"_s, u"Hello1"_s));

    Universe universe;
    universe.plugins().scan(QStringLiteral(NEBULA_TEST_PLUGINS_DIR));
    const Galaxy* home = universe.instantiateGalaxy(filePath);
    QVERIFY(home != nullptr);
    QVERIFY(universe.instantiate(testComponentId(u"Probe"_s)) != nullptr);

    // A galaxy that holds one of its components, directly or not, is of the plugin too: it goes with it.
    QCOMPARE(universe.componentsOf(FirstPluginId), QList<Nebula::Component*>{universe.component(u"Home1"_s)});
    QVERIFY(universe.unloadPlugin(FirstPluginId));
    QCOMPARE(namesOf(universe.components()), QStringList{"Probe1"});
    QVERIFY(!universe.plugins().isLoaded(FirstPluginId));
}

void TestGalaxy::stargateInterfaces()
{
    const QTemporaryDir directory;
    Universe universe;
    const Galaxy* galaxy = universe.instantiateGalaxy(write(directory, u"Gated.nbp"_s, Gated));
    QVERIFY2(galaxy != nullptr, qPrintable(universe.errorString()));
    QCOMPARE(galaxy->problems(), QStringList());

    // What its stargate exposes, the other way round: it provides what the stargate consumes, and the other way.
    const QList<Nebula::InterfaceInfo> provided = galaxy->providedInterfaces();
    QCOMPARE(provided.size(), 1);
    QCOMPARE(provided.first().name, u"out"_s);
    QCOMPARE(provided.first().type, QMetaType::fromType<QObject*>());
    QVERIFY(!provided.first().available); // The Source provides it while it runs
    const QList<Nebula::InterfaceInfo> consumed = galaxy->consumedInterfaces();
    QCOMPARE(consumed.size(), 1);
    QCOMPARE(consumed.first().name, u"in"_s);
    QCOMPARE(consumed.first().requirement, Nebula::InterfaceRequirement::Mandatory); // The Sink needs it
    QCOMPARE(galaxy->unboundMandatoryInterfaces(), QStringList{"in"});

    // Placed, its stargate is still changed, which the galaxy has at once: see stargateChangedOncePlaced().
    const auto* stargate = qobject_cast<const Nebula::Stargate*>(galaxy->component(u"Gate"_s));
    QVERIFY(stargate != nullptr);
    QVERIFY(stargate->isChangeable());
}

void TestGalaxy::passesWhatItProvides()
{
    const QTemporaryDir directory;
    Universe universe;
    Galaxy* galaxy = universe.instantiateGalaxy(write(directory, u"Gated.nbp"_s, Gated));
    QVERIFY(galaxy != nullptr);
    auto* probe = qobject_cast<Probe*>(universe.instantiate(testComponentId(u"Probe"_s)));
    QVERIFY(probe->consumeObject(u"got"_s));
    QVERIFY(probe->bindInterface(u"got"_s, *galaxy, u"out"_s));
    QVERIFY(probe->consumed(u"got"_s) == nullptr);

    // The pointer of the Source, as it is, while it provides it.
    QVERIFY(universe.start());
    auto* source = qobject_cast<Source*>(galaxy->component(u"Source"_s));
    QCOMPARE(probe->consumed(u"got"_s), &source->object());
    QVERIFY(interfaceNamed(galaxy->providedInterfaces(), u"out"_s).available);
    probe->clearEvents();

    // Withdrawn: told while it still has it.
    universe.stop();
    QVERIFY(probe->consumed(u"got"_s) == nullptr);
    QVERIFY2(probe->events().contains(u"unavailable got (set)"_s), qPrintable(probe->events().join(u", ")));
    QVERIFY(!interfaceNamed(galaxy->providedInterfaces(), u"out"_s).available);
}

void TestGalaxy::passesWhatItConsumes()
{
    const QTemporaryDir directory;
    QObject object; // Outlives what provides it
    Universe universe;
    Galaxy* galaxy = universe.instantiateGalaxy(write(directory, u"Gated.nbp"_s, Gated));
    QVERIFY(galaxy != nullptr);
    auto* probe = qobject_cast<Probe*>(universe.instantiate(testComponentId(u"Probe"_s)));
    QVERIFY(probe->provideObject(u"made"_s, &object));
    const auto* sink = qobject_cast<const Sink*>(galaxy->component(u"Sink"_s));

    QVERIFY(galaxy->bindInterface(u"in"_s, *probe, u"made"_s));
    QVERIFY(galaxy->unboundMandatoryInterfaces().isEmpty());
    QCOMPARE(sink->object(), &object); // As it is, inside

    // Withdrawn on one side, gone on the other before the provider returns.
    QVERIFY(probe->provideObject(u"made"_s, nullptr));
    QVERIFY(sink->object() == nullptr);
    QVERIFY(probe->provideObject(u"made"_s, &object));
    QCOMPARE(sink->object(), &object);
    galaxy->unbindInterface(u"in"_s);
    QVERIFY(sink->object() == nullptr);
}

void TestGalaxy::consumedRequirement_data()
{
    QTest::addColumn<QString>("consumer"); // What the stargate gives what the galaxy consumes to; nothing when empty
    QTest::addColumn<bool>("mandatory");

    QTest::newRow("needed") << "object" << true;
    QTest::newRow("not needed") << "timer" << false;
    QTest::newRow("given to nothing") << "" << false;
}

void TestGalaxy::consumedRequirement()
{
    QFETCH(QString, consumer);
    QFETCH(bool, mandatory);

    const QByteArray bindings = consumer.isEmpty() ? QByteArray()
                                                   : R"({"consumer": "Sink", "consumed": ")" + consumer.toUtf8() +
                                                         R"(", "provider": "Gate", "provided": "in"})";
    const QString type = consumer == u"timer"_s ? u"QTimer*"_s : u"QObject*"_s;
    const QTemporaryDir directory;
    Universe universe;
    const Galaxy* galaxy =
        universe.instantiateGalaxy(write(directory, u"Gated.nbp"_s,
                                         R"({"version": 1, "components": [{"name": "Sink", "type": "Test.Sink"},
                  {"name": "Gate", "type": "Nebula.Stargate", "consumed": [{"name": "in", "type": ")" +
                                             type.toUtf8() + R"("}]}], "bindings": [)" + bindings + "]}"));
    QVERIFY(galaxy != nullptr);
    QCOMPARE(galaxy->problems(), QStringList());
    QCOMPARE(interfaceNamed(galaxy->consumedInterfaces(), u"in"_s).requirement,
             mandatory ? Nebula::InterfaceRequirement::Mandatory : Nebula::InterfaceRequirement::Optional);
}

void TestGalaxy::bindingsSavedAndLoaded()
{
    const QTemporaryDir directory;
    const QString applicationPath = directory.filePath(u"App.nbp"_s);
    QVERIFY(!write(directory, u"Gated.nbp"_s, Gated).isEmpty());

    QJsonObject saved;
    {
        Universe universe;
        universe.setFilePath(applicationPath);
        Galaxy* galaxy = universe.instantiateGalaxy(directory.filePath(u"Gated.nbp"_s));
        QVERIFY(galaxy != nullptr);
        Nebula::Component* source = universe.instantiate(testComponentId(u"Source"_s));
        Nebula::Component* sink = universe.instantiate(testComponentId(u"Sink"_s));
        QVERIFY(galaxy->bindInterface(u"in"_s, *source, u"object"_s));
        QVERIFY(sink->bindInterface(u"object"_s, *galaxy, u"out"_s));
        saved = universe.save();
    }
    // Those of the galaxy, as those of any component.
    const QJsonArray bindings = saved.value(u"bindings").toArray();
    QCOMPARE(bindings.size(), 2);
    QCOMPARE(bindings.at(0).toObject(), QJsonDocument::fromJson(R"({"consumer": "Gated1", "consumed": "in",
                                                                    "provider": "Source1", "provided": "object"})")
                                            .object());
    QCOMPARE(bindings.at(1).toObject(), QJsonDocument::fromJson(R"({"consumer": "Sink1", "consumed": "object",
                                                                    "provider": "Gated1", "provided": "out"})")
                                            .object());

    Universe universe;
    universe.setFilePath(applicationPath);
    QCOMPARE(universe.load(saved), QStringList());
    Nebula::Component* galaxy = universe.component(u"Gated1"_s);
    QCOMPARE(galaxy->binding(u"in"_s).provider, universe.component(u"Source1"_s));
    QCOMPARE(universe.component(u"Sink1"_s)->binding(u"object"_s).provider, galaxy);
    QCOMPARE(universe.component(u"Sink1"_s)->binding(u"object"_s).providedName, u"out"_s);
}

void TestGalaxy::removedWhileBound()
{
    const QTemporaryDir directory;
    QObject object; // Outlives what provides it
    Universe universe;
    const QString filePath = write(directory, u"Gated.nbp"_s, Gated);
    Galaxy* galaxy = universe.instantiateGalaxy(filePath);
    QVERIFY(galaxy != nullptr);
    auto* probe = qobject_cast<Probe*>(universe.instantiate(testComponentId(u"Probe"_s)));
    QVERIFY(probe->consumeObject(u"got"_s));
    QVERIFY(probe->provideObject(u"made"_s, &object));
    QVERIFY(probe->bindInterface(u"got"_s, *galaxy, u"out"_s));
    QVERIFY(galaxy->bindInterface(u"in"_s, *probe, u"made"_s));
    QVERIFY(universe.start());
    QVERIFY(probe->consumed(u"got"_s) != nullptr);
    universe.stop();

    // What it provides goes with it, and what it consumes lets go.
    QVERIFY(universe.remove(*galaxy));
    QVERIFY(probe->binding(u"got"_s).provider == nullptr);

    // The other way: its provider goes, what it gave inside is withdrawn.
    galaxy = universe.instantiateGalaxy(filePath);
    QVERIFY(galaxy != nullptr);
    QVERIFY(galaxy->bindInterface(u"in"_s, *probe, u"made"_s));
    const auto* sink = qobject_cast<const Sink*>(galaxy->component(u"Sink"_s));
    QCOMPARE(sink->object(), &object);
    QVERIFY(universe.remove(*probe));
    QVERIFY(sink->object() == nullptr);
    QVERIFY(galaxy->binding(u"in"_s).provider == nullptr);
}

void TestGalaxy::exposedProperties()
{
    const QTemporaryDir directory;
    const QString filePath = write(directory, u"Gated.nbp"_s, Gated);
    Universe universe;
    const Galaxy* first = universe.instantiateGalaxy(filePath);
    const Galaxy* second = universe.instantiateGalaxy(filePath);
    QVERIFY(first != nullptr && second != nullptr);

    // The property of its own Probe, the same one on the same object.
    const QList<Nebula::ExposedProperty> exposed = first->exposedProperties();
    QCOMPARE(exposed.size(), 1);
    Nebula::Component* probe = first->component(u"Probe"_s);
    QCOMPARE(exposed.first(),
             (Nebula::ExposedProperty{.name = u"rate"_s, .component = probe, .object = probe, .property = u"count"_s}));
    QVERIFY(!first->exposedProperty(u"count"_s)); // Under the name it exposes it as only
    QCOMPARE(exposedValue(*first, u"rate"_s).toInt(), 7);

    // Set on the galaxy, it is on its component, and the other way round; each instance its own.
    QVERIFY(setExposedValue(*first, u"rate"_s, 9));
    QCOMPARE(probe->property("count").toInt(), 9);
    QVERIFY(probe->setProperty("count", 11));
    QCOMPARE(exposedValue(*first, u"rate"_s).toInt(), 11);
    QCOMPARE(exposedValue(*second, u"rate"_s).toInt(), 7);
}

void TestGalaxy::exposedValuesSavedAndLoaded()
{
    const QTemporaryDir directory;
    const QString applicationPath = directory.filePath(u"App.nbp"_s);
    QVERIFY(!write(directory, u"Gated.nbp"_s, Gated).isEmpty());
    QJsonObject saved;
    {
        Universe universe;
        universe.setFilePath(applicationPath);
        const Galaxy* first = universe.instantiateGalaxy(directory.filePath(u"Gated.nbp"_s));
        QVERIFY(universe.instantiateGalaxy(directory.filePath(u"Gated.nbp"_s)) != nullptr);
        QVERIFY(setExposedValue(*first, u"rate"_s, 9));
        saved = universe.save();
    }
    // Under the name each galaxy exposes them as, once.
    const QJsonArray components = saved.value(u"components").toArray();
    QCOMPARE(components.at(0).toObject().value(u"properties").toObject(),
             QJsonDocument::fromJson(R"({"rate": 9})").object());
    QCOMPARE(components.at(1).toObject().value(u"properties").toObject(),
             QJsonDocument::fromJson(R"({"rate": 7})").object());

    Universe universe;
    universe.setFilePath(applicationPath);
    QCOMPARE(universe.load(saved), QStringList());
    QCOMPARE(exposedValue(*qobject_cast<const Galaxy*>(universe.component(u"Gated1"_s)), u"rate"_s).toInt(), 9);
    QCOMPARE(exposedValue(*qobject_cast<const Galaxy*>(universe.component(u"Gated2"_s)), u"rate"_s).toInt(), 7);

    // One it does not expose, or that cannot have the value, is told.
    QJsonObject wrong = components.at(0).toObject();
    wrong.insert(u"properties", QJsonDocument::fromJson(R"({"nope": 1, "rate": "fast"})").object());
    QJsonObject project = saved;
    project.insert(u"components", QJsonArray{wrong});
    QCOMPARE(universe.load(project), (QStringList{u"The property nope of Gated1 could not be set"_s,
                                                  u"The property rate of Gated1 could not be set"_s}));
}

void TestGalaxy::exposedValuesSavedWhileRunning()
{
    const QTemporaryDir directory;
    Universe universe;
    const Galaxy* galaxy = universe.instantiateGalaxy(write(directory, u"Gated.nbp"_s, Gated));
    QVERIFY(galaxy != nullptr);
    QVERIFY(setExposedValue(*galaxy, u"rate"_s, 9));

    // Those of the edition, which it has again once stopped.
    QVERIFY(universe.start());
    QVERIFY(setExposedValue(*galaxy, u"rate"_s, 12));
    const QJsonObject json = universe.save().value(u"components").toArray().first().toObject();
    QCOMPARE(json.value(u"properties").toObject().value(u"rate").toInt(), 9);
    universe.stop();
    QCOMPARE(exposedValue(*galaxy, u"rate"_s).toInt(), 9);
}

void TestGalaxy::reexposed()
{
    const QTemporaryDir directory;
    QVERIFY(!write(directory, u"Gated.nbp"_s, Gated).isEmpty());
    // A galaxy that exposes again what the galaxy it holds exposes.
    const QString filePath = write(directory, u"Home.nbp"_s,
                                   R"({"version": 1, "components": [
                  {"name": "Gated1", "type": "Nebula.Galaxy", "galaxy": "Gated.nbp"},
                  {"name": "Door", "type": "Nebula.Stargate",
                   "provided": [{"name": "homeOut", "type": "QObject*"}],
                   "consumed": [{"name": "homeIn", "type": "QObject*"}],
                   "exposed": [{"name": "homeRate", "component": "Gated1", "property": "rate"}]}],
                  "bindings": [{"consumer": "Door", "consumed": "homeOut", "provider": "Gated1", "provided": "out"},
                               {"consumer": "Gated1", "consumed": "in", "provider": "Door", "provided": "homeIn"}]})");

    QObject object; // Outlives what provides it
    Universe universe;
    Galaxy* home = universe.instantiateGalaxy(filePath);
    QVERIFY2(home != nullptr, qPrintable(universe.errorString()));
    QCOMPARE(home->problems(), QStringList());
    const auto* gated = qobject_cast<const Galaxy*>(home->component(u"Gated1"_s));
    QVERIFY(gated != nullptr);

    // The property of the Probe of the galaxy it holds, the same one.
    const std::optional<Nebula::ExposedProperty> rate = home->exposedProperty(u"homeRate"_s);
    QVERIFY(rate);
    QCOMPARE(rate->object, gated->component(u"Probe"_s));
    QCOMPARE(rate->property, u"count"_s);
    QVERIFY(setExposedValue(*home, u"homeRate"_s, 3));
    QCOMPARE(exposedValue(*gated, u"rate"_s).toInt(), 3);

    // Its interfaces through both, needed as the Sink needs it.
    QCOMPARE(interfaceNamed(home->consumedInterfaces(), u"homeIn"_s).requirement,
             Nebula::InterfaceRequirement::Mandatory);
    auto* probe = qobject_cast<Probe*>(universe.instantiate(testComponentId(u"Probe"_s)));
    QVERIFY(probe->provideObject(u"made"_s, &object));
    QVERIFY(probe->consumeObject(u"got"_s));
    QVERIFY(home->bindInterface(u"homeIn"_s, *probe, u"made"_s));
    QVERIFY(probe->bindInterface(u"got"_s, *home, u"homeOut"_s));
    QCOMPARE(qobject_cast<const Sink*>(gated->component(u"Sink"_s))->object(), &object);
    QVERIFY(universe.start());
    QCOMPARE(probe->consumed(u"got"_s), &qobject_cast<Source*>(gated->component(u"Source"_s))->object());
    universe.stop();
    QVERIFY(probe->consumed(u"got"_s) == nullptr);
}

void TestGalaxy::editedInPlace()
{
    const QTemporaryDir directory;
    Universe universe;
    Galaxy* galaxy = universe.instantiateGalaxy(write(directory, u"Clock.nbp"_s, Clock));
    QVERIFY(galaxy != nullptr);
    const Nebula::Component* applicationProbe = universe.instantiate(testComponentId(u"Probe"_s), u"Probe"_s);
    QVERIFY(applicationProbe != nullptr);
    const QSignalSpy componentsChanged(&universe, &Universe::componentsChanged);

    // Instantiated in it, named among its own components.
    Nebula::Component* probe = universe.instantiate(testComponentId(u"Probe"_s), {}, galaxy);
    QVERIFY(probe != nullptr);
    QCOMPARE(probe->objectName(), u"Probe1"_s);
    QCOMPARE(namesOf(galaxy->components()), (QStringList{"Source", "Sink", "Probe", "Probe1"}));
    QCOMPARE(namesOf(universe.components()), (QStringList{"Clock1", "Probe"}));
    QCOMPARE(universe.galaxyOf(*probe), galaxy);
    QVERIFY(universe.galaxyOf(*applicationProbe) == nullptr);
    QVERIFY(universe.galaxyOf(*galaxy) == nullptr);
    QCOMPARE(&universe.components(galaxy), &galaxy->components());
    QCOMPARE(&universe.components(nullptr), &universe.components());

    // Renamed among them: the name of another component of the galaxy is taken, not one of the application.
    QVERIFY(!universe.rename(*probe, u"Sink"_s));
    QVERIFY(universe.rename(*probe, u"Clock1"_s));
    QCOMPARE(galaxy->component(u"Clock1"_s), probe);

    // A stargate, whose galaxy has what it exposes at once.
    Nebula::Stargate* stargate = universe.addStargate({}, galaxy);
    QVERIFY(stargate != nullptr);
    QCOMPARE(stargate->objectName(), u"Stargate1"_s);
    QVERIFY(stargate->exposeInterface(Nebula::Exposure::Provided, u"out"_s, QMetaType::fromType<QObject*>()));
    QCOMPARE(interfaceNames(galaxy->providedInterfaces()), QStringList{"out"});

    // Removed from it.
    const QPointer<Nebula::Component> removed = probe;
    QVERIFY(universe.remove(*probe));
    QVERIFY(removed.isNull());
    QCOMPARE(namesOf(galaxy->components()), (QStringList{"Source", "Sink", "Probe", "Stargate1"}));
    // The components of the application did not change.
    QCOMPARE(componentsChanged.count(), 0);

    // Nothing in a galaxy of another application, nor while it runs.
    Nebula::Component* kept = galaxy->component(u"Probe"_s);
    Universe other;
    QVERIFY(other.instantiate(testComponentId(u"Probe"_s), {}, galaxy) == nullptr);
    QVERIFY(other.addStargate({}, galaxy) == nullptr);
    QVERIFY(!other.rename(*kept, u"P"_s));
    QVERIFY(!other.remove(*kept));
    QVERIFY(other.galaxyOf(*kept) == nullptr);
    QVERIFY(universe.start());
    QVERIFY(universe.instantiate(testComponentId(u"Probe"_s), {}, galaxy) == nullptr);
    QVERIFY(universe.addStargate({}, galaxy) == nullptr);
    QVERIFY(!universe.rename(*kept, u"P"_s));
    QVERIFY(!universe.remove(*kept));
    universe.stop();
    QCOMPARE(namesOf(galaxy->components()), (QStringList{"Source", "Sink", "Probe", "Stargate1"}));
}

void TestGalaxy::stargateChangedOncePlaced()
{
    const QTemporaryDir directory;
    QObject object; // Outlives what provides it
    Universe universe;
    Galaxy* galaxy = universe.instantiateGalaxy(write(directory, u"Gated.nbp"_s, Gated));
    QVERIFY(galaxy != nullptr);
    auto* gate = qobject_cast<Nebula::Stargate*>(galaxy->component(u"Gate"_s));
    auto* sink = qobject_cast<Sink*>(galaxy->component(u"Sink"_s));
    auto* probe = qobject_cast<Probe*>(universe.instantiate(testComponentId(u"Probe"_s)));
    QVERIFY(probe->consumeObject(u"got"_s));
    QVERIFY(probe->provideObject(u"made"_s, &object));
    QVERIFY(probe->bindInterface(u"got"_s, *galaxy, u"out"_s));

    // Exposed: the galaxy provides and consumes it at once, what it consumes being needed by nothing yet.
    const QMetaType objectType = QMetaType::fromType<QObject*>();
    QVERIFY(gate->exposeInterface(Nebula::Exposure::Provided, u"more"_s, objectType));
    QVERIFY(gate->exposeInterface(Nebula::Exposure::Consumed, u"extra"_s, objectType));
    QCOMPARE(interfaceNames(galaxy->providedInterfaces()), (QStringList{"out", "more"}));
    QCOMPARE(interfaceNames(galaxy->consumedInterfaces()), (QStringList{"in", "extra"}));
    QCOMPARE(interfaceNamed(galaxy->consumedInterfaces(), u"extra"_s).requirement,
             Nebula::InterfaceRequirement::Optional);
    // What it is given passes at once.
    QVERIFY(sink->bindInterface(u"object"_s, *gate, u"extra"_s));
    QVERIFY(galaxy->bindInterface(u"extra"_s, *probe, u"made"_s));
    QCOMPARE(sink->object(), &object);

    // Renamed: what is bound to it stays bound, on both sides, and what it passes still passes.
    QVERIFY(gate->renameInterface(Nebula::Exposure::Provided, u"out"_s, u"exit"_s));
    QCOMPARE(interfaceNames(galaxy->providedInterfaces()), (QStringList{"exit", "more"}));
    QCOMPARE(probe->binding(u"got"_s).provider, galaxy);
    QCOMPARE(probe->binding(u"got"_s).providedName, u"exit"_s);
    QVERIFY(gate->renameInterface(Nebula::Exposure::Consumed, u"extra"_s, u"entry"_s));
    QCOMPARE(interfaceNames(galaxy->consumedInterfaces()), (QStringList{"in", "entry"}));
    QCOMPARE(galaxy->binding(u"entry"_s).provider, probe);
    QCOMPARE(sink->object(), &object);
    QVERIFY(universe.start());
    QCOMPARE(probe->consumed(u"got"_s), &qobject_cast<Source*>(galaxy->component(u"Source"_s))->object());
    universe.stop();

    // Removed: what was bound to it no longer is, once it let go of what it had.
    gate->removeInterface(Nebula::Exposure::Consumed, u"entry"_s);
    QVERIFY(sink->object() == nullptr);
    QVERIFY(sink->binding(u"object"_s).provider == nullptr);
    QCOMPARE(interfaceNames(galaxy->consumedInterfaces()), QStringList{"in"});
    gate->removeInterface(Nebula::Exposure::Provided, u"exit"_s);
    QVERIFY(probe->binding(u"got"_s).provider == nullptr);
    QCOMPARE(interfaceNames(galaxy->providedInterfaces()), QStringList{"more"});
}

void TestGalaxy::placedInGalaxy()
{
    const QTemporaryDir directory;
    const QString clockPath = write(directory, u"parts/Clock.nbp"_s, Clock);
    const QString homePath = write(directory, u"Home.nbp"_s, placing(u"parts/Clock.nbp"_s));
    Universe universe;
    Galaxy* home = universe.instantiateGalaxy(homePath);
    QVERIFY(home != nullptr);
    auto* clock = qobject_cast<Galaxy*>(home->component(u"Clock1"_s));
    QVERIFY(clock != nullptr);

    // In it, as in the application: named among its components.
    const Galaxy* placed = universe.instantiateGalaxy(clockPath, {}, home);
    QVERIFY2(placed != nullptr, qPrintable(universe.errorString()));
    QCOMPARE(placed->objectName(), u"Clock2"_s);
    QCOMPARE(universe.galaxyOf(*placed), home);
    QCOMPARE(namesOf(universe.components()), QStringList{"Home1"});

    // Not where it would hold itself, directly or not.
    QVERIFY(universe.instantiateGalaxy(homePath, {}, home) == nullptr);
    QVERIFY2(universe.errorString().contains(u"would hold itself"_s), qPrintable(universe.errorString()));
    QVERIFY(universe.instantiateGalaxy(homePath, {}, clock) == nullptr);
    QVERIFY2(universe.errorString().contains(u"would hold itself"_s), qPrintable(universe.errorString()));
    QCOMPARE(namesOf(home->components()), (QStringList{"Clock1", "Clock2"}));
    // Nor in a galaxy of another application.
    Universe other;
    QVERIFY(other.instantiateGalaxy(clockPath, {}, home) == nullptr);
    QCOMPARE(other.errorString(), u"A galaxy is only placed in the application, or in one of its galaxies"_s);
}

void TestGalaxy::settingsFilesInGalaxy()
{
    const QTemporaryDir directory;
    QVERIFY(!write(directory, u"Clock.nbp"_s,
                   R"({"version": 1, "components": [{"name": "P", "type": "Test.Probe"}],
                       "settings": [{"file": "settings.json", "entries": [{"component": "P", "property": "count"}]}]})")
                 .isEmpty());
    Universe universe;
    Galaxy* home = universe.instantiateGalaxy(write(directory, u"Home.nbp"_s, placing(u"Clock.nbp"_s)));
    QVERIFY(home != nullptr);
    auto* clock = qobject_cast<Galaxy*>(home->component(u"Clock1"_s));
    QVERIFY(clock != nullptr);
    const QSignalSpy settingsFilesChanged(&universe, &Universe::settingsFilesChanged);

    // Named as the file of its galaxy names it, with the first number its other files are not named with, after the
    // galaxies it is in.
    Nebula::SettingsFile* file = universe.addSettingsFile(*clock);
    QVERIFY(file != nullptr);
    QCOMPARE(file->name(), u"settings2.json"_s);
    QCOMPARE(file->fileName(), u"Home1.Clock1.settings2.json"_s);
    QCOMPARE(file->galaxy(), clock);
    QCOMPARE(clock->settingsFiles().size(), std::size_t{2});
    QCOMPARE(universe.settingsFile(u"Home1.Clock1.settings2.json"_s), file);
    QVERIFY(universe.settingsFiles().empty());
    QCOMPARE(settingsFilesChanged.count(), 1);

    // Renamed in its galaxy, still after the galaxies it is in.
    QVERIFY(file->setName(u"user.json"_s));
    QCOMPARE(file->fileName(), u"Home1.Clock1.user.json"_s);
    QVERIFY(!file->setName(u"SETTINGS.json"_s)); // The name of the other file of the galaxy, whatever its case
    QVERIFY(!file->setName({}));
    QVERIFY(!file->setFileName(u"other.json"_s)); // Its name, not its file name
    QCOMPARE(file->fileName(), u"Home1.Clock1.user.json"_s);
    QVERIFY(universe.rename(*clock, u"Alarm"_s));
    QCOMPARE(file->fileName(), u"Home1.Alarm.user.json"_s);
    QCOMPARE(file->name(), u"user.json"_s);

    // It holds properties of the components of its galaxy, not of those of the application.
    Nebula::Component* probe = clock->component(u"P"_s);
    QVERIFY(file->add({.component = probe, .object = probe, .property = u"ratio"_s}));
    Nebula::Component* outside = universe.instantiate(testComponentId(u"Probe"_s));
    QVERIFY(!file->add({.component = outside, .object = outside, .property = u"ratio"_s}));

    // Removed from its galaxy.
    universe.removeSettingsFile(*file);
    QCOMPARE(clock->settingsFiles().size(), std::size_t{1});
    QVERIFY(universe.settingsFile(u"Home1.Alarm.user.json"_s) == nullptr);
    // Not added to a galaxy of another application.
    Universe other;
    QVERIFY(other.addSettingsFile(*clock) == nullptr);
}

void TestGalaxy::saveGalaxy()
{
    const QTemporaryDir directory;
    QVERIFY(!write(directory, u"parts/Clock.nbp"_s, Clock).isEmpty());
    QVERIFY(!write(directory, u"parts/Gated.nbp"_s, Gated).isEmpty());
    const QString homePath = write(directory, u"Home.nbp"_s,
                                   R"({"version": 1, "description": "A home", "components": [
                                       {"name": "Clock1", "type": "Nebula.Galaxy", "galaxy": "parts/Clock.nbp"}]})");
    Universe universe;
    // Not what the files of its galaxies are relative to.
    universe.setFilePath(directory.filePath(u"elsewhere/App.nbp"_s));
    Galaxy* home = universe.instantiateGalaxy(homePath);
    QVERIFY(home != nullptr);

    // Edited: a component, a galaxy and a settings file more.
    Nebula::Component* probe = universe.instantiate(testComponentId(u"Probe"_s), {}, home);
    QVERIFY(probe != nullptr);
    QVERIFY(probe->setProperty("count", 4));
    Galaxy* gated = universe.instantiateGalaxy(directory.filePath(u"parts/Gated.nbp"_s), {}, home);
    QVERIFY(gated != nullptr);
    QVERIFY(setExposedValue(*gated, u"rate"_s, 3));
    Nebula::SettingsFile* file = universe.addSettingsFile(*home);
    QVERIFY(file != nullptr);
    QVERIFY(file->add({.component = probe, .object = probe, .property = u"count"_s}));

    // As its file is to hold it.
    const QJsonObject saved = universe.saveGalaxy(*home);
    QCOMPARE(saved.value(u"version").toInt(), 1);
    QCOMPARE(saved.value(u"description").toString(), u"A home"_s);
    const QJsonArray components = saved.value(u"components").toArray();
    QCOMPARE(components.size(), 3);
    // The files of its galaxies relative to its own.
    QCOMPARE(components.at(0).toObject().value(u"galaxy").toString(), u"parts/Clock.nbp"_s);
    QCOMPARE(components.at(1).toObject().value(u"name").toString(), u"Probe1"_s);
    QCOMPARE(components.at(1).toObject().value(u"properties").toObject().value(u"count").toInt(), 4);
    QCOMPARE(components.at(2).toObject().value(u"name").toString(), u"Gated1"_s);
    QCOMPARE(components.at(2).toObject().value(u"galaxy").toString(), u"parts/Gated.nbp"_s);
    QCOMPARE(components.at(2).toObject().value(u"properties").toObject(),
             QJsonDocument::fromJson(R"({"rate": 3})").object());
    // Its settings files under the names its file gives them.
    const QJsonArray settings = saved.value(u"settings").toArray();
    QCOMPARE(settings.size(), 1);
    QCOMPARE(settings.first().toObject().value(u"file").toString(), u"settings.json"_s);
    QCOMPARE(settings.first().toObject().value(u"entries").toArray(),
             QJsonDocument::fromJson(R"([{"component": "Probe1", "property": "count"}])").array());

    // What another instance has once read from it.
    QVERIFY(rewrite(homePath, saved));
    const Galaxy* placed = universe.instantiateGalaxy(homePath);
    QVERIFY(placed != nullptr);
    QCOMPARE(placed->problems(), QStringList());
    QCOMPARE(namesOf(placed->components()), (QStringList{"Clock1", "Probe1", "Gated1"}));
    QCOMPARE(placed->component(u"Probe1"_s)->property("count").toInt(), 4);
    QCOMPARE(exposedValue(*qobject_cast<const Galaxy*>(placed->component(u"Gated1"_s)), u"rate"_s).toInt(), 3);
    QCOMPARE(placed->settingsFiles().front()->fileName(), u"Home2.settings.json"_s);
}

void TestGalaxy::galaxiesOfFile()
{
    const QTemporaryDir directory;
    const QString clockPath = write(directory, u"Clock.nbp"_s, Clock);
    const QString homePath = write(directory, u"Home.nbp"_s, placing(u"Clock.nbp"_s));
    Universe universe;
    Galaxy* clock = universe.instantiateGalaxy(clockPath);
    Galaxy* home = universe.instantiateGalaxy(homePath);
    QVERIFY(clock != nullptr && home != nullptr);

    // Wherever they are, in the order of the application.
    auto* inner = qobject_cast<Galaxy*>(home->component(u"Clock1"_s));
    QCOMPARE(universe.galaxies(clockPath), (QList<Galaxy*>{clock, inner}));
    QCOMPARE(universe.galaxies(homePath), QList<Galaxy*>{home});
    QCOMPARE(universe.galaxies(directory.filePath(u"./Home.nbp"_s)), QList<Galaxy*>{home});
    QVERIFY(universe.galaxies(directory.filePath(u"Missing.nbp"_s)).isEmpty());
}

void TestGalaxy::reloadGalaxy()
{
    const QTemporaryDir directory;
    const QString filePath = write(directory, u"Gated.nbp"_s, Gated);
    QObject object; // Outlives what provides it
    Universe universe;
    Galaxy* edited = universe.instantiateGalaxy(filePath);
    Galaxy* other = universe.instantiateGalaxy(filePath);
    QVERIFY(edited != nullptr && other != nullptr);
    // The other: a value of its own, and bound both ways.
    QVERIFY(setExposedValue(*other, u"rate"_s, 3));
    auto* probe = qobject_cast<Probe*>(universe.instantiate(testComponentId(u"Probe"_s)));
    QVERIFY(probe->consumeObject(u"got"_s));
    QVERIFY(probe->provideObject(u"made"_s, &object));
    QVERIFY(probe->bindInterface(u"got"_s, *other, u"out"_s));
    QVERIFY(other->bindInterface(u"in"_s, *probe, u"made"_s));

    // The one edited: a component and a settings file more, and other values, saved to its file.
    Nebula::Component* extra = universe.instantiate(testComponentId(u"Probe"_s), u"Extra"_s, edited);
    QVERIFY(extra != nullptr);
    Nebula::SettingsFile* file = universe.addSettingsFile(*edited);
    QVERIFY(file->add({.component = extra, .object = extra, .property = u"count"_s}));
    QVERIFY(edited->component(u"Source"_s)->setProperty("label", u"tock"_s));
    QVERIFY(setExposedValue(*edited, u"rate"_s, 9));
    QVERIFY(rewrite(filePath, universe.saveGalaxy(*edited)));

    // Read again: what its file holds now, in the same instance.
    const QPointer<Nebula::Component> formerSource = other->component(u"Source"_s);
    const QSignalSpy aboutToBeReloaded(other, &Galaxy::aboutToBeReloaded);
    const QSignalSpy reloaded(other, &Galaxy::reloaded);
    const QSignalSpy settingsFilesChanged(&universe, &Universe::settingsFilesChanged);
    QCOMPARE(universe.reloadGalaxy(*other), QStringList());
    QCOMPARE(aboutToBeReloaded.count(), 1);
    QCOMPARE(reloaded.count(), 1);
    QCOMPARE(settingsFilesChanged.count(), 1);
    QVERIFY(formerSource.isNull());
    QCOMPARE(universe.component(u"Gated2"_s), other);
    QCOMPARE(namesOf(other->components()), (QStringList{"Source", "Sink", "Probe", "Gate", "Extra"}));
    QCOMPARE(other->component(u"Source"_s)->property("label").toString(), u"tock"_s);
    QCOMPARE(other->project(), readJson(filePath));
    QCOMPARE(other->problems(), QStringList());
    QCOMPARE(other->settingsFiles().front()->fileName(), u"Gated2.settings.json"_s);
    // With its own value, and bound as it was: it passes what it is given again.
    QCOMPARE(exposedValue(*other, u"rate"_s).toInt(), 3);
    QCOMPARE(probe->binding(u"got"_s).provider, other);
    QCOMPARE(other->binding(u"in"_s).provider, probe);
    QCOMPARE(qobject_cast<const Sink*>(other->component(u"Sink"_s))->object(), &object);

    // The one edited, read again, is as it was; a new instance has the values of the file.
    QCOMPARE(universe.reloadGalaxy(*edited), QStringList());
    QCOMPARE(namesOf(edited->components()), (QStringList{"Source", "Sink", "Probe", "Gate", "Extra"}));
    QCOMPARE(exposedValue(*edited, u"rate"_s).toInt(), 9);
    const Galaxy* placed = universe.instantiateGalaxy(filePath);
    QVERIFY(placed != nullptr);
    QCOMPARE(exposedValue(*placed, u"rate"_s).toInt(), 9);
}

void TestGalaxy::reloadLosesWhatIsGone()
{
    const QTemporaryDir directory;
    const QString filePath = write(directory, u"Gated.nbp"_s, Gated);
    Universe universe;
    Galaxy* galaxy = universe.instantiateGalaxy(filePath);
    QVERIFY(galaxy != nullptr);
    auto* probe = qobject_cast<Probe*>(universe.instantiate(testComponentId(u"Probe"_s)));
    QVERIFY(probe->consumeObject(u"got"_s));
    QVERIFY(probe->bindInterface(u"got"_s, *galaxy, u"out"_s));
    QVERIFY(setExposedValue(*galaxy, u"rate"_s, 3));

    // Its file no longer exposes what it was bound to, nor its value.
    QVERIFY(!write(directory, u"Gated.nbp"_s, R"({"version": 1, "components": [{"name": "Gate",
        "type": "Nebula.Stargate", "consumed": [{"name": "in", "type": "QObject*"}]}]})")
                 .isEmpty());
    QCOMPARE(universe.reloadGalaxy(*galaxy),
             (QStringList{u"got of Probe1 is no longer bound to out of Gated1"_s,
                          u"Gated1 no longer exposes rate: the value it had is lost"_s}));
    QVERIFY(probe->binding(u"got"_s).provider == nullptr);
    QCOMPARE(interfaceNames(galaxy->providedInterfaces()), QStringList());
    QCOMPARE(interfaceNames(galaxy->consumedInterfaces()), QStringList{"in"});
}

void TestGalaxy::reloadRefused_data()
{
    QTest::addColumn<QString>("content"); // What its file holds now; it is removed when empty
    QTest::addColumn<bool>("running");
    QTest::addColumn<bool>("ofAnother");  // Whether another application is asked
    QTest::addColumn<QString>("problem"); // How it starts

    QTest::newRow("file removed") << "" << false << false << "Gated1 is not read again: ";
    QTest::newRow("not a project") << "{ version" << false << false << "Gated1 is not read again: ";
    QTest::newRow("unknown plugin") << R"({"version": 1, "plugins": [{"id": "Other.Plugin", "file": "o.nbdl"}]})"
                                    << false << false << "Gated1 is not read again: ";
    QTest::newRow("while running") << "" << true << false << "Gated1 is not read again while the application runs";
    QTest::newRow("of another application")
        << "" << false << true << "Gated1 is not read again: it is not a galaxy of the application";
}

void TestGalaxy::reloadRefused()
{
    QFETCH(QString, content);
    QFETCH(bool, running);
    QFETCH(bool, ofAnother);
    QFETCH(QString, problem);

    const QTemporaryDir directory;
    const QString filePath = write(directory, u"Gated.nbp"_s, Gated);
    Universe universe;
    Galaxy* galaxy = universe.instantiateGalaxy(filePath);
    QVERIFY(galaxy != nullptr);
    if(!running && !ofAnother)
    {
        QVERIFY(content.isEmpty() ? QFile::remove(filePath)
                                  : !write(directory, u"Gated.nbp"_s, content.toUtf8()).isEmpty());
    }
    if(running)
    {
        QVERIFY(universe.start());
    }
    const QPointer<Nebula::Component> source = galaxy->component(u"Source"_s);
    const QSignalSpy aboutToBeReloaded(galaxy, &Galaxy::aboutToBeReloaded);

    Universe other;
    const QStringList problems = ofAnother ? other.reloadGalaxy(*galaxy) : universe.reloadGalaxy(*galaxy);
    QCOMPARE(problems.size(), 1);
    QVERIFY2(problems.first().startsWith(problem), qPrintable(problems.first()));
    // Nothing changed.
    QCOMPARE(aboutToBeReloaded.count(), 0);
    QVERIFY(!source.isNull());
    QCOMPARE(namesOf(galaxy->components()), (QStringList{"Source", "Sink", "Probe", "Gate"}));
    universe.stop();
}

void TestGalaxy::reloadLeftEmpty()
{
    const QTemporaryDir directory;
    const QString filePath = write(directory, u"Gated.nbp"_s, Gated);
    Universe universe;
    Galaxy* galaxy = universe.instantiateGalaxy(filePath);
    QVERIFY(galaxy != nullptr);

    // Its file holds itself now: only found once it is read.
    QVERIFY(!write(directory, u"Gated.nbp"_s, placing(u"Gated.nbp"_s, u"Me"_s)).isEmpty());
    const QSignalSpy reloaded(galaxy, &Galaxy::reloaded);
    const QStringList problems = universe.reloadGalaxy(*galaxy);
    QCOMPARE(problems.size(), 2);
    QVERIFY2(problems.first().startsWith(u"Gated1 is left empty: "_s) &&
                 problems.first().contains(u"would hold itself"_s),
             qPrintable(problems.first()));
    QCOMPARE(problems.last(), u"Gated1 no longer exposes rate: the value it had is lost"_s);
    QCOMPARE(reloaded.count(), 1);
    QVERIFY(galaxy->components().empty());
    QCOMPARE(galaxy->problems().size(), 1);
    QCOMPARE(universe.component(u"Gated1"_s), galaxy);
}

QTEST_GUILESS_MAIN(TestGalaxy)
#include "TestGalaxy.moc"
