// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QTest>
#include <algorithm>
#include <memory>

#include "Nebula/Component/ComponentFactory.h"
#include "Nebula/Galaxy/Galaxy.h"
#include "Probe.h"
#include "TestComponents.h"

using Nebula::ComponentFactory;
using Nebula::ComponentInfo;

namespace
{

// The names of properties, in their order.
QStringList names(const QList<QMetaProperty>& properties)
{
    QStringList names;
    for(const QMetaProperty& property : properties)
    {
        names.append(QString::fromLatin1(property.name()));
    }
    return names;
}

} // namespace

// ComponentFactory: what it reads of the components of a plugin, and how it creates them.
class TestComponentFactory : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();

    void component_data();
    void component();
    void componentsAreSortedById();
    void create_data();
    void create();
    void addingTheSamePluginAgainDoesNothing();
    void addingAKnownIdIsSkipped();
    void remove();
    void componentNameHasNoNamespace();
    void pluginComponents();
    void pluginCreate_data();
    void pluginCreate();
};

void TestComponentFactory::initTestCase()
{
    registerTestComponents();
}

void TestComponentFactory::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
}

void TestComponentFactory::component_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("description");
    QTest::addColumn<QString>("category");
    QTest::addColumn<QStringList>("properties");

    QTest::newRow("Probe") << "Probe" << "Provides and consumes what the test tells it to" << "Test"
                           << QStringList{"refusesToStart", "count",  "ratio", "text",  "date",
                                          "time",           "moment", "point", "starts"};
    QTest::newRow("no category") << "Source" << "Provides an object while it runs" << "" << QStringList{"label"};
    QTest::newRow("no property") << "Sink" << "Consumes an object, a timer and a list of objects" << "Test"
                                 << QStringList{};
    QTest::newRow("no class info") << "Panel" << "" << "" << QStringList{"title"};
}

void TestComponentFactory::component()
{
    QFETCH(QString, name);
    QFETCH(QString, description);
    QFETCH(QString, category);
    QFETCH(QStringList, properties);

    const std::optional<ComponentInfo> info = ComponentFactory::instance().component(testComponentId(name));
    QVERIFY(info.has_value());
    QCOMPARE(info->id, testComponentId(name));
    QCOMPARE(info->name, name);
    QCOMPARE(info->pluginId, QString(TestComponents::Id));
    QCOMPARE(info->description, description);
    QCOMPARE(info->category, category);
    // Not those of QObject and Component: objectName, componentId.
    QCOMPARE(names(info->properties), properties);
}

void TestComponentFactory::componentsAreSortedById()
{
    const QList<ComponentInfo> components = ComponentFactory::instance().components();
    QCOMPARE(components.size(), 4);
    QVERIFY(std::ranges::is_sorted(components, {}, &ComponentInfo::id));
}

void TestComponentFactory::create_data()
{
    QTest::addColumn<QString>("componentId");
    QTest::addColumn<bool>("created");

    QTest::newRow("registered") << testComponentId(QStringLiteral("Probe")) << true;
    QTest::newRow("another one") << testComponentId(QStringLiteral("Panel")) << true;
    QTest::newRow("unknown") << testComponentId(QStringLiteral("Unknown")) << false;
    QTest::newRow("name without plugin") << "Probe" << false;
    QTest::newRow("empty") << "" << false;
}

void TestComponentFactory::create()
{
    QFETCH(QString, componentId);
    QFETCH(bool, created);

    const std::unique_ptr<Nebula::Component> component = ComponentFactory::instance().create(componentId);
    QCOMPARE(component != nullptr, created);
    if(created)
    {
        QCOMPARE(component->componentId(), componentId);
        QCOMPARE(component->property("componentId").toString(), componentId);
        QCOMPARE(component->state(), Nebula::ComponentState::Editing);
    }
}

void TestComponentFactory::addingTheSamePluginAgainDoesNothing()
{
    ComponentFactory::instance().add(QStringLiteral("Again"), registerTestComponents());
    QCOMPARE(ComponentFactory::instance().components().size(), 4);
    QVERIFY(!ComponentFactory::instance().component(QStringLiteral("Again.Probe")));
}

void TestComponentFactory::addingAKnownIdIsSkipped()
{
    TestComponents other;
    for(const char* name : {"Probe", "Source", "Sink", "Panel"})
    {
        QTest::ignoreMessage(
            QtWarningMsg,
            qPrintable(QStringLiteral("Ignoring Test.%1 - already registered").arg(QLatin1StringView(name))));
    }
    ComponentFactory::instance().add(TestComponents::Id, other);
    QCOMPARE(ComponentFactory::instance().components().size(), 4);
}

void TestComponentFactory::remove()
{
    const QString otherId = QStringLiteral("Other");
    TestComponents other;
    ComponentFactory::instance().add(otherId, other);
    QCOMPARE(ComponentFactory::instance().components().size(), 8);
    QVERIFY(ComponentFactory::instance().create(QStringLiteral("Other.Probe")) != nullptr);

    ComponentFactory::instance().remove(otherId);
    QCOMPARE(ComponentFactory::instance().components().size(), 4);
    QVERIFY(!ComponentFactory::instance().component(QStringLiteral("Other.Probe")));
    QVERIFY(ComponentFactory::instance().create(QStringLiteral("Other.Probe")) == nullptr);
    QVERIFY(ComponentFactory::instance().component(testComponentId(QStringLiteral("Probe"))));
}

void TestComponentFactory::componentNameHasNoNamespace()
{
    QCOMPARE(Nebula::componentName(Probe::staticMetaObject), QStringLiteral("Probe"));
    QCOMPARE(Nebula::componentName(Nebula::Galaxy::staticMetaObject), QStringLiteral("Galaxy"));
}

// The plugin itself, which the factory has create its components.
void TestComponentFactory::pluginComponents()
{
    QStringList names;
    for(const QMetaObject* metaObject : registerTestComponents().components())
    {
        names.append(Nebula::componentName(*metaObject));
    }
    // In the order of NEBULA_REGISTER_COMPONENT, in a single source file.
    QCOMPARE(names, (QStringList{"Probe", "Source", "Sink", "Panel"}));
}

void TestComponentFactory::pluginCreate_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<bool>("created");

    QTest::newRow("registered") << "Probe" << true;
    QTest::newRow("another one") << "Panel" << true;
    QTest::newRow("unknown") << "Unknown" << false;
    QTest::newRow("id") << testComponentId(QStringLiteral("Probe")) << false;
    QTest::newRow("empty") << "" << false;
}

void TestComponentFactory::pluginCreate()
{
    QFETCH(QString, name);
    QFETCH(bool, created);

    const std::unique_ptr<Nebula::Component> component = registerTestComponents().create(name);
    QCOMPARE(component != nullptr, created);
    if(created)
    {
        QCOMPARE(Nebula::componentName(*component->metaObject()), name);
        // Given by the factory, which knows the id of the plugin.
        QVERIFY(component->componentId().isEmpty());
        QCOMPARE(component->state(), Nebula::ComponentState::Editing);
    }
}

QTEST_GUILESS_MAIN(TestComponentFactory)
#include "TestComponentFactory.moc"
