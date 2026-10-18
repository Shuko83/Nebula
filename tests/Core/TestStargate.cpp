// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QJsonArray>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>
#include <memory>

#include "Nebula/Application/Universe.h"
#include "Nebula/Component/ComponentFactory.h"
#include "Nebula/Galaxy/Stargate.h"
#include "Options.h"
#include "Panel.h"
#include "Probe.h"
#include "TestComponents.h"

using Nebula::ExposedProperty;
using Nebula::Exposure;
using Nebula::Stargate;
using Nebula::Universe;
using namespace Qt::StringLiterals;

namespace
{

const QMetaType ObjectType = QMetaType::fromType<QObject*>();
const QMetaType TimerType = QMetaType::fromType<QTimer*>();

// An instance of the component of the tests T in universe, named name, or after its component when empty.
template <typename T> T* instantiate(Universe& universe, const QString& name = {})
{
    return qobject_cast<T*>(
        universe.instantiate(testComponentId(QLatin1StringView(T::staticMetaObject.className())), name));
}

// The names of interfaces, in their order.
QStringList namesOf(const QList<Nebula::InterfaceInfo>& interfaces)
{
    QStringList names;
    for(const Nebula::InterfaceInfo& info : interfaces)
    {
        names.append(info.name);
    }
    return names;
}

// The one of interfaces named name; a default one when none is.
Nebula::InterfaceInfo interfaceNamed(const QList<Nebula::InterfaceInfo>& interfaces, const QString& name)
{
    const auto found = std::ranges::find(interfaces, name, &Nebula::InterfaceInfo::name);
    return found != interfaces.end() ? *found : Nebula::InterfaceInfo{};
}

} // namespace

// Stargate, in the application that edits it: what it exposes, how it is bound, saved and loaded. What a placed galaxy
// makes of it is in TestGalaxy.
class TestStargate : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();

    void addStargate_data();
    void addStargate();
    void addStargateRefusedWhileRunning();
    void exposeInterface_data();
    void exposeInterface();
    void exposedInterfacesAreItsOwnTheOtherWayRound();
    void renameInterface_data();
    void renameInterface();
    void renameKeepsBindings();
    void removeInterface();
    void passesInTheApplication();
    void exposeProperty_data();
    void exposeProperty();
    void renameAndRemoveProperty();
    void forgetsWhatIsDestroyed();
    void notChangedWhileRunning();
    void saveAndLoad();
    void loadProblems();
};

void TestStargate::initTestCase()
{
    registerTestComponents();
}

void TestStargate::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
}

void TestStargate::addStargate_data()
{
    QTest::addColumn<QStringList>("existing"); // Probes of the application
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("named");

    QTest::newRow("first") << QStringList{} << "" << "Stargate1";
    QTest::newRow("first free number") << QStringList{"Stargate1"} << "" << "Stargate2";
    QTest::newRow("given") << QStringList{} << "Door" << "Door";
    QTest::newRow("given, taken") << QStringList{"Door"} << "Door" << "Stargate1";
}

void TestStargate::addStargate()
{
    QFETCH(QStringList, existing);
    QFETCH(QString, name);
    QFETCH(QString, named);

    Universe universe;
    for(const QString& probe : std::as_const(existing))
    {
        QVERIFY(instantiate<Probe>(universe, probe) != nullptr);
    }
    const QSignalSpy componentsChanged(&universe, &Universe::componentsChanged);
    Stargate* stargate = universe.addStargate(name);
    QVERIFY(stargate != nullptr);
    QCOMPARE(componentsChanged.count(), 1);
    QCOMPARE(stargate->objectName(), named);
    QCOMPARE(universe.component(named), stargate);
    QCOMPARE(stargate->componentId(), QString(Stargate::Id));
    QVERIFY(stargate->interfaces().isEmpty());
    QVERIFY(stargate->properties().isEmpty());
    QVERIFY(stargate->isChangeable());
    // Core builds it: it is not one ComponentFactory creates.
    QVERIFY(!Nebula::ComponentFactory::instance().component(QString(Stargate::Id)));
}

void TestStargate::addStargateRefusedWhileRunning()
{
    Universe universe;
    QVERIFY(universe.start());
    QVERIFY(universe.addStargate() == nullptr);
    QVERIFY(universe.components().empty());
}

void TestStargate::exposeInterface_data()
{
    QTest::addColumn<int>("exposure");   // Exposure, of the one exposed
    QTest::addColumn<QString>("name");   // The name it is exposed as
    QTest::addColumn<bool>("validType"); // Whether it is of a type, rather than of none
    QTest::addColumn<bool>("onOther");   // On the other stargate, which exposes "taken" both ways, not on the first
    QTest::addColumn<bool>("exposed");

    const auto provided = static_cast<int>(Exposure::Provided);
    const auto consumed = static_cast<int>(Exposure::Consumed);
    QTest::newRow("provided") << provided << "display" << true << false << true;
    QTest::newRow("consumed") << consumed << "display" << true << false << true;
    QTest::newRow("no name") << provided << "" << true << false << false;
    QTest::newRow("no type") << provided << "display" << false << false << false;
    QTest::newRow("name another stargate provides") << provided << "taken" << true << false << false;
    QTest::newRow("name another stargate consumes") << consumed << "taken" << true << false << false;
    QTest::newRow("name it provides") << provided << "taken" << true << true << false;
}

void TestStargate::exposeInterface()
{
    QFETCH(int, exposure);
    QFETCH(QString, name);
    QFETCH(bool, validType);
    QFETCH(bool, onOther);
    QFETCH(bool, exposed);

    Universe universe;
    Stargate* first = universe.addStargate();
    Stargate* other = universe.addStargate();
    QVERIFY(other->exposeInterface(Exposure::Provided, u"taken"_s, ObjectType));
    QVERIFY(other->exposeInterface(Exposure::Consumed, u"taken"_s, ObjectType)); // The other way: its own name
    Stargate* stargate = onOther ? other : first;
    const QList<Nebula::ExposedInterface> before = stargate->interfaces();

    const QSignalSpy changed(stargate, &Stargate::changed);
    const auto way = static_cast<Exposure>(exposure);
    QCOMPARE(stargate->exposeInterface(way, name, validType ? ObjectType : QMetaType()), exposed);
    QCOMPARE(changed.count(), exposed ? 1 : 0);
    if(!exposed)
    {
        QCOMPARE(stargate->interfaces(), before);
        return;
    }
    QCOMPARE(stargate->interfaces().size(), 1);
    QCOMPARE(stargate->interfaces().first(),
             (Nebula::ExposedInterface{.name = name, .type = ObjectType, .exposure = way}));
    QVERIFY(stargate->exposedInterface(way, name) != nullptr);
    const Exposure otherWay = way == Exposure::Provided ? Exposure::Consumed : Exposure::Provided;
    QVERIFY(stargate->exposedInterface(otherWay, name) == nullptr);
}

void TestStargate::exposedInterfacesAreItsOwnTheOtherWayRound()
{
    Universe universe;
    Stargate* stargate = universe.addStargate();
    const QSignalSpy interfacesChanged(stargate, &Nebula::Component::interfacesChanged);
    QVERIFY(stargate->exposeInterface(Exposure::Provided, u"out"_s, ObjectType));
    QVERIFY(stargate->exposeInterface(Exposure::Consumed, u"in"_s, TimerType));
    QCOMPARE(interfacesChanged.count(), 2);

    // What its galaxy provides, it consumes, needing it; what its galaxy consumes, it provides.
    const QList<Nebula::InterfaceInfo> consumed = stargate->consumedInterfaces();
    QCOMPARE(namesOf(consumed), QStringList{"out"});
    QCOMPARE(consumed.first().type, ObjectType);
    QCOMPARE(consumed.first().requirement, Nebula::InterfaceRequirement::Mandatory);
    const QList<Nebula::InterfaceInfo> provided = stargate->providedInterfaces();
    QCOMPARE(namesOf(provided), QStringList{"in"});
    QCOMPARE(provided.first().type, TimerType);
    QVERIFY(!provided.first().available);
    // It exposes nothing until it is bound.
    QCOMPARE(stargate->unboundMandatoryInterfaces(), QStringList{"out"});
}

void TestStargate::renameInterface_data()
{
    QTest::addColumn<int>("exposure");
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("newName");
    QTest::addColumn<bool>("renamed");
    QTest::addColumn<QStringList>("names"); // Of those it exposes that way, after it

    const auto provided = static_cast<int>(Exposure::Provided);
    const auto consumed = static_cast<int>(Exposure::Consumed);
    QTest::newRow("provided") << provided << "a" << "c" << true << QStringList{"c", "b"};
    QTest::newRow("consumed") << consumed << "a" << "c" << true << QStringList{"c"};
    QTest::newRow("same name") << provided << "a" << "a" << true << QStringList{"a", "b"};
    QTest::newRow("no name") << provided << "a" << "" << false << QStringList{"a", "b"};
    QTest::newRow("one of its own") << provided << "a" << "b" << false << QStringList{"a", "b"};
    QTest::newRow("one of another stargate") << provided << "a" << "other" << false << QStringList{"a", "b"};
    QTest::newRow("the one it consumes, the other way") << consumed << "a" << "b" << true << QStringList{"b"};
    QTest::newRow("not exposed") << provided << "missing" << "c" << false << QStringList{"a", "b"};
}

void TestStargate::renameInterface()
{
    QFETCH(int, exposure);
    QFETCH(QString, name);
    QFETCH(QString, newName);
    QFETCH(bool, renamed);
    QFETCH(QStringList, names);

    Universe universe;
    Stargate* stargate = universe.addStargate();
    QVERIFY(stargate->exposeInterface(Exposure::Provided, u"a"_s, ObjectType));
    QVERIFY(stargate->exposeInterface(Exposure::Provided, u"b"_s, ObjectType));
    QVERIFY(stargate->exposeInterface(Exposure::Consumed, u"a"_s, ObjectType));
    QVERIFY(universe.addStargate()->exposeInterface(Exposure::Provided, u"other"_s, ObjectType));

    const auto way = static_cast<Exposure>(exposure);
    QCOMPARE(stargate->renameInterface(way, name, newName), renamed);
    QStringList exposed;
    for(const Nebula::ExposedInterface& info : stargate->interfaces())
    {
        if(info.exposure == way)
        {
            exposed.append(info.name);
        }
    }
    QCOMPARE(exposed, names);
    // As its own interfaces are.
    QCOMPARE(namesOf(way == Exposure::Provided ? stargate->consumedInterfaces() : stargate->providedInterfaces()),
             names);
}

void TestStargate::renameKeepsBindings()
{
    QObject object; // Outlives what provides it
    Universe universe;
    auto* probe = instantiate<Probe>(universe);
    QVERIFY(probe->provideObject(u"made"_s, &object));
    QVERIFY(probe->consumeObject(u"wanted"_s));
    Stargate* stargate = universe.addStargate();
    QVERIFY(stargate->exposeInterface(Exposure::Provided, u"out"_s, ObjectType));
    QVERIFY(stargate->exposeInterface(Exposure::Consumed, u"in"_s, ObjectType));
    QVERIFY(stargate->bindInterface(u"out"_s, *probe, u"made"_s));
    QVERIFY(probe->bindInterface(u"wanted"_s, *stargate, u"in"_s));

    const QSignalSpy interfaceRenamed(stargate, &Nebula::Component::interfaceRenamed);
    QVERIFY(stargate->renameInterface(Exposure::Provided, u"out"_s, u"shown"_s));
    QVERIFY(stargate->renameInterface(Exposure::Consumed, u"in"_s, u"given"_s));
    // Only what it provides is told: what is bound to it follows.
    QCOMPARE(interfaceRenamed.count(), 1);
    QCOMPARE(interfaceRenamed.first(), (QVariantList{u"in"_s, u"given"_s}));

    const Nebula::InterfaceBinding out = stargate->binding(u"shown"_s);
    QCOMPARE(out.provider, probe);
    QCOMPARE(out.providedName, u"made"_s);
    QVERIFY(interfaceNamed(stargate->consumedInterfaces(), u"shown"_s).available); // Still what it is given
    const Nebula::InterfaceBinding in = probe->binding(u"wanted"_s);
    QCOMPARE(in.provider, stargate);
    QCOMPARE(in.providedName, u"given"_s);

    // And it still follows them under their new names.
    QVERIFY(probe->provideObject(u"made"_s, nullptr));
    QVERIFY(!interfaceNamed(stargate->consumedInterfaces(), u"shown"_s).available);
}

void TestStargate::removeInterface()
{
    QObject object; // Outlives what provides it
    Universe universe;
    auto* probe = instantiate<Probe>(universe);
    QVERIFY(probe->provideObject(u"made"_s, &object));
    QVERIFY(probe->consumeObject(u"wanted"_s));
    Stargate* stargate = universe.addStargate();
    QVERIFY(stargate->exposeInterface(Exposure::Provided, u"out"_s, ObjectType));
    QVERIFY(stargate->exposeInterface(Exposure::Consumed, u"in"_s, ObjectType));
    QVERIFY(stargate->bindInterface(u"out"_s, *probe, u"made"_s));
    QVERIFY(probe->bindInterface(u"wanted"_s, *stargate, u"in"_s));
    const QSignalSpy interfaceRemoved(stargate, &Nebula::Component::interfaceRemoved);
    const QSignalSpy changed(stargate, &Stargate::changed);

    stargate->removeInterface(Exposure::Consumed, u"in"_s);
    QCOMPARE(interfaceRemoved.count(), 1);
    QVERIFY(stargate->providedInterfaces().isEmpty());
    QVERIFY(probe->binding(u"wanted"_s).provider == nullptr); // What was bound to it no longer is

    stargate->removeInterface(Exposure::Provided, u"out"_s);
    QVERIFY(stargate->consumedInterfaces().isEmpty());
    QVERIFY(stargate->interfaces().isEmpty());
    QCOMPARE(changed.count(), 2);

    // The same name can be exposed again.
    stargate->removeInterface(Exposure::Provided, u"out"_s);
    QCOMPARE(changed.count(), 2);
    QVERIFY(stargate->exposeInterface(Exposure::Provided, u"out"_s, ObjectType));
}

void TestStargate::passesInTheApplication()
{
    QObject object; // Outlives what provides it
    Universe universe;
    auto* probe = instantiate<Probe>(universe);
    QVERIFY(probe->provideObject(u"made"_s, &object));
    Stargate* stargate = universe.addStargate();
    QVERIFY(stargate->exposeInterface(Exposure::Provided, u"out"_s, ObjectType));
    QVERIFY(stargate->bindInterface(u"out"_s, *probe, u"made"_s));
    // What its components give it, as any component that consumes it; nothing places the application.
    QVERIFY(interfaceNamed(stargate->consumedInterfaces(), u"out"_s).available);
    QVERIFY(stargate->unboundMandatoryInterfaces().isEmpty());
}

void TestStargate::exposeProperty_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("component"); // Probe1, Panel1, Stargate1 (itself), or outside: a Probe of another one
    QTest::addColumn<QString>("object");    // general, the Options of Panel1; the component itself when empty
    QTest::addColumn<QString>("property");
    QTest::addColumn<bool>("exposed");

    QTest::newRow("of a component") << "rate" << "Probe1" << "" << "count" << true;
    QTest::newRow("of one of its objects") << "level" << "Panel1" << "general" << "level" << true;
    QTest::newRow("no name") << "" << "Probe1" << "" << "count" << false;
    QTest::newRow("name exposed by another stargate") << "taken" << "Probe1" << "" << "count" << false;
    QTest::newRow("exposed by another stargate") << "again" << "Probe1" << "" << "text" << false;
    QTest::newRow("unknown") << "nope" << "Probe1" << "" << "nope" << false;
    QTest::newRow("JSON does not hold it") << "point" << "Probe1" << "" << "point" << false;
    QTest::newRow("read only") << "starts" << "Probe1" << "" << "starts" << false;
    QTest::newRow("not saved") << "hint" << "Panel1" << "general" << "hint" << false;
    QTest::newRow("of a stargate") << "name" << "Stargate1" << "" << "objectName" << false;
    QTest::newRow("of another application") << "rate" << "outside" << "" << "count" << false;
}

void TestStargate::exposeProperty()
{
    QFETCH(QString, name);
    QFETCH(QString, component);
    QFETCH(QString, object);
    QFETCH(QString, property);
    QFETCH(bool, exposed);

    Universe universe;
    auto* probe = instantiate<Probe>(universe);
    auto* panel = instantiate<Panel>(universe);
    Stargate* stargate = universe.addStargate();
    Stargate* other = universe.addStargate();
    QVERIFY(other->exposeProperty({.name = u"taken"_s, .component = panel, .object = panel, .property = u"title"_s}));
    QVERIFY(other->exposeProperty({.name = u"text"_s, .component = probe, .object = probe, .property = u"text"_s}));
    Universe outside;
    auto* outsider = instantiate<Probe>(outside);

    Nebula::Component* owner = component == u"outside"_s ? outsider : universe.component(component);
    QVERIFY(owner != nullptr);
    QObject* held = object.isEmpty() ? static_cast<QObject*>(owner) : &panel->general();
    const ExposedProperty exposedProperty{.name = name, .component = owner, .object = held, .property = property};
    const QSignalSpy changed(stargate, &Stargate::changed);
    QCOMPARE(stargate->exposeProperty(exposedProperty), exposed);
    QCOMPARE(changed.count(), exposed ? 1 : 0);
    QCOMPARE(stargate->properties(), exposed ? QList{exposedProperty} : QList<ExposedProperty>());
    QCOMPARE(stargate->property(name) != nullptr, exposed);
}

void TestStargate::renameAndRemoveProperty()
{
    Universe universe;
    auto* probe = instantiate<Probe>(universe);
    Stargate* stargate = universe.addStargate();
    QVERIFY(stargate->exposeProperty({.name = u"rate"_s, .component = probe, .object = probe, .property = u"count"_s}));
    QVERIFY(stargate->exposeProperty({.name = u"label"_s, .component = probe, .object = probe, .property = u"text"_s}));
    QVERIFY(universe.addStargate()->exposeProperty(
        {.name = u"other"_s, .component = probe, .object = probe, .property = u"ratio"_s}));
    const QSignalSpy changed(stargate, &Stargate::changed);

    QVERIFY(!stargate->renameProperty(u"rate"_s, u"label"_s)); // Its own
    QVERIFY(!stargate->renameProperty(u"rate"_s, u"other"_s)); // Of another stargate of its galaxy
    QVERIFY(!stargate->renameProperty(u"rate"_s, QString()));
    QVERIFY(!stargate->renameProperty(u"missing"_s, u"speed"_s));
    QCOMPARE(changed.count(), 0);
    QVERIFY(stargate->renameProperty(u"rate"_s, u"speed"_s));
    QCOMPARE(changed.count(), 1);
    QVERIFY(stargate->property(u"speed"_s) != nullptr);
    QCOMPARE(stargate->property(u"speed"_s)->property, u"count"_s);

    stargate->removeProperty(u"speed"_s);
    QCOMPARE(changed.count(), 2);
    QVERIFY(stargate->property(u"speed"_s) == nullptr);
    stargate->removeProperty(u"speed"_s);
    QCOMPARE(changed.count(), 2);
    // Exposed again, under another name if need be.
    QVERIFY(stargate->exposeProperty({.name = u"rate"_s, .component = probe, .object = probe, .property = u"count"_s}));
}

void TestStargate::forgetsWhatIsDestroyed()
{
    Universe universe;
    auto* probe = instantiate<Probe>(universe);
    auto* panel = instantiate<Panel>(universe);
    auto* options = new Options(probe); // Destroyed with it, or before
    probe->addObject(options, u"extra"_s);
    Stargate* stargate = universe.addStargate();
    QVERIFY(stargate->exposeProperty({.name = u"rate"_s, .component = probe, .object = probe, .property = u"count"_s}));
    QVERIFY(
        stargate->exposeProperty({.name = u"level"_s, .component = probe, .object = options, .property = u"level"_s}));
    QVERIFY(
        stargate->exposeProperty({.name = u"title"_s, .component = panel, .object = panel, .property = u"title"_s}));
    const QSignalSpy changed(stargate, &Stargate::changed);

    delete options;
    QCOMPARE(changed.count(), 1);
    QCOMPARE(stargate->properties().size(), 2);
    QVERIFY(universe.remove(*probe));
    QCOMPARE(changed.count(), 2);
    QCOMPARE(stargate->properties().size(), 1);
    QCOMPARE(stargate->properties().first().name, u"title"_s);
}

void TestStargate::notChangedWhileRunning()
{
    Universe universe;
    auto* probe = instantiate<Probe>(universe);
    Stargate* stargate = universe.addStargate();
    QVERIFY(stargate->exposeInterface(Exposure::Consumed, u"in"_s, ObjectType));
    QVERIFY(stargate->exposeProperty({.name = u"rate"_s, .component = probe, .object = probe, .property = u"count"_s}));

    QVERIFY(universe.start());
    QCOMPARE(stargate->state(), Nebula::ComponentState::Running);
    QVERIFY(!stargate->isChangeable());
    QVERIFY(!stargate->exposeInterface(Exposure::Consumed, u"other"_s, ObjectType));
    QVERIFY(!stargate->renameInterface(Exposure::Consumed, u"in"_s, u"other"_s));
    stargate->removeInterface(Exposure::Consumed, u"in"_s);
    QVERIFY(
        !stargate->exposeProperty({.name = u"label"_s, .component = probe, .object = probe, .property = u"text"_s}));
    QVERIFY(!stargate->renameProperty(u"rate"_s, u"speed"_s));
    stargate->removeProperty(u"rate"_s);
    QCOMPARE(stargate->interfaces().size(), 1);
    QCOMPARE(stargate->properties().size(), 1);

    universe.stop();
    QVERIFY(stargate->isChangeable());
}

void TestStargate::saveAndLoad()
{
    QJsonObject saved;
    {
        Universe universe;
        auto* probe = instantiate<Probe>(universe);
        QVERIFY(probe->consumeObject(u"wanted"_s));
        QObject object;
        QVERIFY(probe->provideObject(u"made"_s, &object));
        auto* panel = instantiate<Panel>(universe);
        Stargate* stargate = universe.addStargate(u"Gate"_s);
        QVERIFY(stargate->exposeInterface(Exposure::Provided, u"out"_s, ObjectType));
        QVERIFY(stargate->exposeInterface(Exposure::Consumed, u"in"_s, ObjectType));
        QVERIFY(stargate->bindInterface(u"out"_s, *probe, u"made"_s));
        QVERIFY(probe->bindInterface(u"wanted"_s, *stargate, u"in"_s));
        QVERIFY(
            stargate->exposeProperty({.name = u"rate"_s, .component = probe, .object = probe, .property = u"count"_s}));
        QVERIFY(stargate->exposeProperty(
            {.name = u"second"_s, .component = panel, .object = panel->pages().at(1), .property = u"level"_s}));
        saved = universe.save();
        QVERIFY(probe->provideObject(u"made"_s, nullptr)); // Before object goes
    }

    // What it exposes, in place of properties of its own.
    const QJsonArray components = saved.value(u"components").toArray();
    QCOMPARE(components.size(), 3);
    const QJsonObject gate = components.at(2).toObject();
    QCOMPARE(gate.value(u"name").toString(), u"Gate"_s);
    QCOMPARE(gate.value(u"type").toString(), QString(Stargate::Id));
    QVERIFY(!gate.contains(u"properties"));
    QCOMPARE(gate.value(u"provided").toArray(),
             QJsonDocument::fromJson(R"([{"name": "out", "type": "QObject*"}])").array());
    QCOMPARE(gate.value(u"consumed").toArray(),
             QJsonDocument::fromJson(R"([{"name": "in", "type": "QObject*"}])").array());
    QCOMPARE(gate.value(u"exposed").toArray(), QJsonDocument::fromJson(R"([
        {"name": "rate", "component": "Probe1", "property": "count"},
        {"name": "second", "component": "Panel1", "object": "page", "index": 1, "property": "level"}])")
                                                   .array());
    // Its bindings, as those of any component.
    QCOMPARE(saved.value(u"bindings").toArray().size(), 2);

    // Without the bindings: the interfaces of the Probe are declared by the test, not as it is built.
    Universe universe;
    QJsonObject project = saved;
    project.insert(u"bindings", QJsonArray());
    QCOMPARE(universe.load(project), QStringList());
    const auto* stargate = qobject_cast<const Stargate*>(universe.component(u"Gate"_s));
    QVERIFY(stargate != nullptr);
    QCOMPARE(stargate->interfaces(),
             (QList<Nebula::ExposedInterface>{{.name = u"out"_s, .type = ObjectType, .exposure = Exposure::Provided},
                                              {.name = u"in"_s, .type = ObjectType, .exposure = Exposure::Consumed}}));
    auto* panel = qobject_cast<Panel*>(universe.component(u"Panel1"_s));
    QCOMPARE(stargate->properties(),
             (QList<ExposedProperty>{
                 {.name = u"rate"_s,
                  .component = universe.component(u"Probe1"_s),
                  .object = universe.component(u"Probe1"_s),
                  .property = u"count"_s},
                 {.name = u"second"_s, .component = panel, .object = panel->pages().at(1), .property = u"level"_s}}));
    // Saved again as it was.
    QCOMPARE(universe.save().value(u"components").toArray().at(2), gate);
}

void TestStargate::loadProblems()
{
    Universe universe;
    const QStringList problems = universe.load(QJsonDocument::fromJson(R"({"version": 1, "components": [
        {"name": "Probe1", "type": "Test.Probe"},
        {"name": "A", "type": "Nebula.Stargate", "provided": [{"name": "out", "type": "QObject*"},
                                                             {"name": "odd", "type": "Unknown*"}],
         "exposed": [{"name": "rate", "component": "Probe1", "property": "count"},
                     {"name": "gone", "component": "Missing", "property": "count"},
                     {"name": "point", "component": "Probe1", "property": "point"}]},
        {"name": "B", "type": "Nebula.Stargate", "provided": [{"name": "out", "type": "QObject*"}],
         "exposed": [{"name": "rate", "component": "Probe1", "property": "text"}]}]})")
                                                   .object());
    QCOMPARE(problems, (QStringList{
                           u"The interface odd of A is not exposed: no component has its type Unknown*"_s,
                           u"The interface out of B is not exposed: another one is exposed under its name"_s,
                           u"The property count of Missing is not exposed by A as gone"_s,
                           u"The property point of Probe1 is not exposed by A as point"_s,
                           u"The property text of Probe1 is not exposed by B as rate"_s,
                       }));
    // What could be restored is.
    const auto* first = qobject_cast<const Stargate*>(universe.component(u"A"_s));
    QCOMPARE(first->interfaces().size(), 1);
    QCOMPARE(first->properties().size(), 1);
    QVERIFY(qobject_cast<const Stargate*>(universe.component(u"B"_s))->interfaces().isEmpty());
}

QTEST_GUILESS_MAIN(TestStargate)
#include "TestStargate.moc"
