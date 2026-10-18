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

const QString ExamplePluginId = QStringLiteral("FabienDG.Nebula.ExamplePlugin");

// A galaxy of a Source, a Sink bound to it, and a Probe counting 7.
const QByteArray Clock = R"({"version": 1, "description": "Ticks",
    "components": [{"name": "Source", "type": "Test.Source", "properties": {"label": "tick"}},
                   {"name": "Sink", "type": "Test.Sink"},
                   {"name": "Probe", "type": "Test.Probe", "properties": {"count": 7}}],
    "bindings": [{"consumer": "Sink", "consumed": "object", "provider": "Source", "provided": "object"}]})";

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

} // namespace

// Galaxy, and how Universe places it: a project file as one component, its plugins, the galaxies it holds, how it is
// saved, runs, and keeps its settings files.
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
              R"({"version": 1, "plugins": [{"id": "FabienDG.Nebula.ExamplePlugin", "file": "nowhere.nbdl"}],
            "components": [{"name": "Greeter1", "type": "FabienDG.Nebula.ExamplePlugin.Greeter"}]})");

    Universe universe;
    universe.plugins().scan(QStringLiteral(NEBULA_EXAMPLE_PLUGINS_DIR));
    QVERIFY(!universe.plugins().isLoaded(ExamplePluginId));
    // Found by its id, not by its file, and loaded.
    const Galaxy* galaxy = universe.instantiateGalaxy(filePath);
    QVERIFY2(galaxy != nullptr, qPrintable(universe.errorString()));
    QVERIFY(universe.plugins().isLoaded(ExamplePluginId));
    QCOMPARE(namesOf(galaxy->components()), QStringList{"Greeter1"});

    // Saved with the plugins of the components of its galaxies.
    const QJsonArray plugins = universe.save().value(u"plugins").toArray();
    QCOMPARE(plugins.size(), 1);
    QCOMPARE(plugins.first().toObject().value(u"id").toString(), ExamplePluginId);
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
                   R"({"version": 1, "plugins": [{"id": "FabienDG.Nebula.ExamplePlugin", "file": "x.nbdl"}],
                       "components": [{"name": "Greeter1", "type": "FabienDG.Nebula.ExamplePlugin.Greeter"}]})")
                 .isEmpty());
    const QString filePath = write(directory, u"Home.nbp"_s, placing(u"Hello.nbp"_s, u"Hello1"_s));

    Universe universe;
    universe.plugins().scan(QStringLiteral(NEBULA_EXAMPLE_PLUGINS_DIR));
    const Galaxy* home = universe.instantiateGalaxy(filePath);
    QVERIFY(home != nullptr);
    QVERIFY(universe.instantiate(testComponentId(u"Probe"_s)) != nullptr);

    // A galaxy that holds one of its components, directly or not, is of the plugin too: it goes with it.
    QCOMPARE(universe.componentsOf(ExamplePluginId), QList<Nebula::Component*>{universe.component(u"Home1"_s)});
    QVERIFY(universe.unloadPlugin(ExamplePluginId));
    QCOMPARE(namesOf(universe.components()), QStringList{"Probe1"});
    QVERIFY(!universe.plugins().isLoaded(ExamplePluginId));
}

QTEST_GUILESS_MAIN(TestGalaxy)
#include "TestGalaxy.moc"
