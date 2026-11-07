// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QJsonObject>
#include <QLoggingCategory>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <memory>
#include <utility>

#include "Nebula/Application/Universe.h"
#include "Nebula/Component/Component.h"
#include "Nebula/Settings/SettingsFile.h"
#include "Nebula/Standard/ILog.h"
#include "Nebula/Standard/ILogControl.h"

using namespace Qt::StringLiterals;
using Nebula::Component;
using Nebula::LogLevel;
using Nebula::Universe;

namespace
{

// A category of this binary, not of the plugin: the level of a running LogComponent holds for it too.
Q_LOGGING_CATEGORY(lcOther, "test.other")

const QString StandardPluginId = u"FabienDG.Nebula.StandardPlugin"_s;

// A message as Qt's journal received it.
struct Message
{
    QtMsgType type = QtDebugMsg;
    QString category;
    QString text;

    [[nodiscard]] bool operator==(const Message& other) const = default;
};

// For QCOMPARE to tell which messages differ.
QDebug operator<<(QDebug debug, const Message& message)
{
    const QDebugStateSaver saver(debug);
    debug.nospace() << static_cast<int>(message.type) << ' ' << message.category << ' ' << message.text;
    return debug;
}

QList<Message> captured;

// The messages of Qt's journal, in captured while it is alive, in place of where they went.
class Capture
{
  public:
    Capture()
    {
        captured.clear();
        m_previous = qInstallMessageHandler(&Capture::handle);
    }
    ~Capture()
    {
        qInstallMessageHandler(m_previous);
    }
    Capture(const Capture&) = delete;
    Capture& operator=(const Capture&) = delete;

  private:
    static void handle(QtMsgType type, const QMessageLogContext& context, const QString& text)
    {
        captured.append({.type = type, .category = QString::fromUtf8(context.category), .text = text});
    }

    QtMessageHandler m_previous = nullptr;
};

// The names of the levels from level on.
QStringList levelsFrom(LogLevel level)
{
    QStringList names;
    for(const LogLevel each : Nebula::LogLevels)
    {
        if(each >= level)
        {
            names.append(Nebula::logLevelName(each));
        }
    }
    return names;
}

const QStringList AllLevels = levelsFrom(LogLevel::Debug);

// The names of the levels category writes.
QStringList enabledLevels(const QLoggingCategory& category)
{
    QStringList names;
    for(const LogLevel level : Nebula::LogLevels)
    {
        if(category.isEnabled(Nebula::qtMsgType(level)))
        {
            names.append(Nebula::logLevelName(level));
        }
    }
    return names;
}

// The names of the levels log writes.
QStringList writtenLevels(const Nebula::ILog& log)
{
    QStringList names;
    for(const LogLevel level : Nebula::LogLevels)
    {
        if(log.isWritten(level))
        {
            names.append(Nebula::logLevelName(level));
        }
    }
    return names;
}

Nebula::ILog& logOf(const Component& component)
{
    return *component.providedInterface<Nebula::ILog>(u"log"_s);
}

Nebula::ILogControl& controlOf(const Component& component)
{
    return *component.providedInterface<Nebula::ILogControl>(u"logControl"_s);
}

// The filter under disableInfo(), installed on top of the one of a running LogComponent.
QLoggingCategory::CategoryFilter belowDisableInfo = nullptr;

// A filter of another binary, which calls the one below it, then disables the information messages; nothing while it
// is installed, before Qt tells the one below.
void disableInfo(QLoggingCategory* category)
{
    if(belowDisableInfo != nullptr)
    {
        belowDisableInfo(category);
        category->setEnabled(QtInfoMsg, false);
    }
}

} // namespace

// The LogComponent of StandardPlugin, loaded from the plugin as an application loads it: the messages it writes, and
// the log of the whole process it drives while it runs.
class TestLogComponent : public QObject
{
    Q_OBJECT

  private slots:
    void init();
    void cleanup();

    void interfacesFromConstruction();
    void writes_data();
    void writes();
    void shortcuts();
    void category();
    void unknownLevel();
    void emptyCategory();
    void filtersNothingWhileEdited();
    void levelHoldsForTheProcess_data();
    void levelHoldsForTheProcess();
    void stoppedLoggingDisablesAll();
    void rulesOfTheApplicationStill();
    void controlDrivesIt();
    void controlWhileRunning();
    void runningInstancesShare();
    void filterInstalledOnTop();
    void settingsFileKeepsTheLevel();
    void projectRoundTrip();

  private:
    // An instance of LogComponent.
    Component* instantiate();

    std::unique_ptr<Universe> m_universe;
};

void TestLogComponent::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
    m_universe = std::make_unique<Universe>();
    m_universe->plugins().scan(QStringLiteral(NEBULA_STANDARD_PLUGINS_DIR));
    QVERIFY2(m_universe->plugins().load(StandardPluginId), qPrintable(m_universe->plugins().errorString()));
}

void TestLogComponent::cleanup()
{
    m_universe.reset();
    QLoggingCategory::setFilterRules({});
}

Component* TestLogComponent::instantiate()
{
    return m_universe->instantiate(StandardPluginId + u".LogComponent"_s);
}

void TestLogComponent::interfacesFromConstruction()
{
    Component* component = instantiate();
    QVERIFY(component != nullptr);
    QCOMPARE(component->property("logging").toBool(), true);
    QCOMPARE(component->property("level").toString(), u"debug"_s);
    QCOMPARE(component->property("category").toString(), u"log"_s);

    // Of the types this binary knows them by.
    const QList<Nebula::InterfaceInfo> provided = component->providedInterfaces();
    QCOMPARE(provided.size(), 2);
    QCOMPARE(provided.at(0).name, u"log"_s);
    QCOMPARE(provided.at(0).type, QMetaType::fromType<Nebula::ILog*>());
    QCOMPARE(provided.at(1).name, u"logControl"_s);
    QCOMPARE(provided.at(1).type, QMetaType::fromType<Nebula::ILogControl*>());
    QVERIFY(component->providedInterface<Nebula::ILog>(u"log"_s) != nullptr);
    QVERIFY(component->providedInterface<Nebula::ILogControl>(u"logControl"_s) != nullptr);
}

void TestLogComponent::writes_data()
{
    QTest::addColumn<QString>("level");
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("text");

    QTest::newRow("debug") << u"debug"_s << u"Greeter1"_s << u"Greeter1: Ready"_s;
    QTest::newRow("info") << u"info"_s << u"Greeter1"_s << u"Greeter1: Ready"_s;
    QTest::newRow("warning") << u"warning"_s << u"Greeter1"_s << u"Greeter1: Ready"_s;
    QTest::newRow("critical") << u"critical"_s << u"Greeter1"_s << u"Greeter1: Ready"_s;
    QTest::newRow("no source") << u"info"_s << QString() << u"Ready"_s;
}

void TestLogComponent::writes()
{
    QFETCH(QString, level);
    QFETCH(QString, source);
    QFETCH(QString, text);
    const LogLevel written = Nebula::logLevelNamed(level).value();
    Nebula::ILog& log = logOf(*instantiate());

    const Capture capture;
    log.write(written, u"Ready"_s, source);
    QCOMPARE(captured, (QList<Message>{{.type = Nebula::qtMsgType(written), .category = u"log"_s, .text = text}}));
}

void TestLogComponent::shortcuts()
{
    Nebula::ILog& log = logOf(*instantiate());

    const Capture capture;
    log.debug(u"One"_s);
    log.info(u"Two"_s, u"Source"_s);
    log.warning(u"Three"_s);
    log.critical(u"Four"_s);
    const QList<Message> expected{
        {.type = QtDebugMsg, .category = u"log"_s, .text = u"One"_s},
        {.type = QtInfoMsg, .category = u"log"_s, .text = u"Source: Two"_s},
        {.type = QtWarningMsg, .category = u"log"_s, .text = u"Three"_s},
        {.type = QtCriticalMsg, .category = u"log"_s, .text = u"Four"_s},
    };
    QCOMPARE(captured, expected);
}

void TestLogComponent::category()
{
    Component* component = instantiate();
    Nebula::ILog& log = logOf(*component);
    QSignalSpy changed(component, SIGNAL(categoryChanged(QString)));

    QVERIFY(component->setProperty("category", u"app.events"_s));
    QVERIFY(component->setProperty("category", u"app.events"_s)); // The same: nothing changes
    QCOMPARE(changed.count(), 1);
    QCOMPARE(changed.at(0).at(0).toString(), u"app.events"_s);
    {
        const Capture capture;
        log.info(u"Opened"_s);
        QCOMPARE(captured, (QList<Message>{{.type = QtInfoMsg, .category = u"app.events"_s, .text = u"Opened"_s}}));
    }

    // One set while it runs is filtered as the others.
    QVERIFY(component->setProperty("level", u"warning"_s));
    QVERIFY(m_universe->start());
    QVERIFY(component->setProperty("category", u"app.other"_s));
    QCOMPARE(writtenLevels(log), levelsFrom(LogLevel::Warning));
    m_universe->stop();
}

void TestLogComponent::unknownLevel()
{
    Component* component = instantiate();
    QSignalSpy changed(component, SIGNAL(levelChanged(QString)));

    QTest::ignoreMessage(
        QtWarningMsg, "LogComponent1: \"verbose\" is not a log level (debug, info, warning, critical), debug is kept");
    QVERIFY(component->setProperty("level", u"verbose"_s));
    QCOMPARE(component->property("level").toString(), u"debug"_s);
    QCOMPARE(changed.count(), 0);

    // Whatever its case.
    QVERIFY(component->setProperty("level", u"WARNING"_s));
    QCOMPARE(component->property("level").toString(), u"warning"_s);
    QCOMPARE(changed.count(), 1);
    QCOMPARE(changed.at(0).at(0).toString(), u"warning"_s);
}

void TestLogComponent::emptyCategory()
{
    Component* component = instantiate();
    const QSignalSpy changed(component, SIGNAL(categoryChanged(QString)));

    QTest::ignoreMessage(QtWarningMsg, "LogComponent1: a log category needs a name, log is kept");
    QVERIFY(component->setProperty("category", QString()));
    QCOMPARE(component->property("category").toString(), u"log"_s);
    QCOMPARE(changed.count(), 0);
}

void TestLogComponent::filtersNothingWhileEdited()
{
    Component* component = instantiate();
    QVERIFY(component->setProperty("level", u"critical"_s));
    QVERIFY(component->setProperty("logging", false));

    QCOMPARE(enabledLevels(lcOther()), AllLevels);
    QCOMPARE(writtenLevels(logOf(*component)), AllLevels);
}

void TestLogComponent::levelHoldsForTheProcess_data()
{
    QTest::addColumn<QString>("level");

    for(const LogLevel level : Nebula::LogLevels)
    {
        QTest::newRow(qPrintable(Nebula::logLevelName(level))) << Nebula::logLevelName(level);
    }
}

void TestLogComponent::levelHoldsForTheProcess()
{
    QFETCH(QString, level);
    const QStringList expected = levelsFrom(Nebula::logLevelNamed(level).value());
    Component* component = instantiate();
    QVERIFY(component->setProperty("level", level));

    QVERIFY(m_universe->start());
    QCOMPARE(enabledLevels(lcOther()), expected);
    QCOMPARE(writtenLevels(logOf(*component)), expected);
    {
        // As it is created.
        const QLoggingCategory created("test.created");
        QCOMPARE(enabledLevels(created), expected);
    }
    m_universe->stop();
    QCOMPARE(enabledLevels(lcOther()), AllLevels);
}

void TestLogComponent::stoppedLoggingDisablesAll()
{
    Component* component = instantiate();
    QVERIFY(component->setProperty("logging", false));

    QVERIFY(m_universe->start());
    QCOMPARE(enabledLevels(lcOther()), QStringList());
    {
        const Capture capture;
        logOf(*component).critical(u"Lost"_s);
        qCCritical(lcOther) << "Lost too";
        QCOMPARE(captured, QList<Message>());
    }
    m_universe->stop();
    QCOMPARE(enabledLevels(lcOther()), AllLevels);
}

void TestLogComponent::rulesOfTheApplicationStill()
{
    Component* component = instantiate();
    QLoggingCategory::setFilterRules(u"test.other.info=false"_s);

    QVERIFY(m_universe->start());
    QCOMPARE(enabledLevels(lcOther()), (QStringList{u"debug"_s, u"warning"_s, u"critical"_s}));
    // Changed while it runs, they are still applied first.
    QLoggingCategory::setFilterRules(u"test.other.warning=false"_s);
    QCOMPARE(enabledLevels(lcOther()), (QStringList{u"debug"_s, u"info"_s, u"critical"_s}));
    QVERIFY(component->setProperty("level", u"info"_s));
    QCOMPARE(enabledLevels(lcOther()), (QStringList{u"info"_s, u"critical"_s}));
    m_universe->stop();
    QCOMPARE(enabledLevels(lcOther()), (QStringList{u"debug"_s, u"info"_s, u"critical"_s}));
}

void TestLogComponent::controlDrivesIt()
{
    Component* component = instantiate();
    Nebula::ILogControl& control = controlOf(*component);
    QSignalSpy logging(component, SIGNAL(loggingChanged(bool)));
    QSignalSpy level(component, SIGNAL(levelChanged(QString)));

    control.stopLogging();
    control.stopLogging(); // Already stopped: nothing changes
    QVERIFY(!control.isLogging());
    QCOMPARE(component->property("logging").toBool(), false);
    QCOMPARE(logging.count(), 1);
    QCOMPARE(logging.at(0).at(0).toBool(), false);
    control.startLogging();
    QVERIFY(control.isLogging());
    QCOMPARE(logging.count(), 2);

    control.setLevel(LogLevel::Warning);
    control.setLevel(LogLevel::Warning);
    QCOMPARE(control.level(), LogLevel::Warning);
    QCOMPARE(component->property("level").toString(), u"warning"_s);
    QCOMPARE(level.count(), 1);
    QCOMPARE(level.at(0).at(0).toString(), u"warning"_s);
}

void TestLogComponent::controlWhileRunning()
{
    Component* component = instantiate();
    QVERIFY(component->setProperty("level", u"info"_s));
    Nebula::ILogControl& control = controlOf(*component);

    QVERIFY(m_universe->start());
    control.setLevel(LogLevel::Critical);
    QCOMPARE(enabledLevels(lcOther()), QStringList{u"critical"_s});
    control.stopLogging();
    QCOMPARE(enabledLevels(lcOther()), QStringList());
    control.startLogging();
    QCOMPARE(enabledLevels(lcOther()), QStringList{u"critical"_s});
    control.setLevel(LogLevel::Debug);
    QCOMPARE(enabledLevels(lcOther()), AllLevels);
    control.setLevel(LogLevel::Warning);
    m_universe->stop();

    // Its values of edition, given back; and the process filters nothing any longer.
    QCOMPARE(control.level(), LogLevel::Info);
    QCOMPARE(enabledLevels(lcOther()), AllLevels);
}

void TestLogComponent::runningInstancesShare()
{
    Component* first = instantiate();
    Component* second = instantiate();
    QVERIFY(second->setProperty("level", u"warning"_s));
    const QSignalSpy firstLevel(first, SIGNAL(levelChanged(QString)));
    const QSignalSpy secondLevel(second, SIGNAL(levelChanged(QString)));
    const QSignalSpy secondLogging(second, SIGNAL(loggingChanged(bool)));

    // Each gives its values as it starts: the second one last.
    QVERIFY(m_universe->start());
    QCOMPARE(first->property("level").toString(), u"warning"_s);
    QCOMPARE(firstLevel.count(), 1);
    QCOMPARE(enabledLevels(lcOther()), levelsFrom(LogLevel::Warning));

    // Driving one drives them all.
    controlOf(*first).setLevel(LogLevel::Critical);
    QCOMPARE(second->property("level").toString(), u"critical"_s);
    QCOMPARE(secondLevel.count(), 1);
    controlOf(*first).stopLogging();
    QCOMPARE(second->property("logging").toBool(), false);
    QCOMPARE(secondLogging.count(), 1);
    QCOMPARE(enabledLevels(lcOther()), QStringList());

    // The filter stays with the one still running, and goes with it.
    second->stop();
    QCOMPARE(enabledLevels(lcOther()), QStringList());
    first->stop();
    QCOMPARE(enabledLevels(lcOther()), AllLevels);
    m_universe->stop();
    QCOMPARE(first->property("level").toString(), u"debug"_s);
    QCOMPARE(second->property("level").toString(), u"warning"_s);
}

void TestLogComponent::filterInstalledOnTop()
{
    Component* component = instantiate();
    QVERIFY(component->setProperty("level", u"warning"_s));
    QVERIFY(m_universe->start());
    belowDisableInfo = QLoggingCategory::installFilter(&disableInfo);
    QCOMPARE(enabledLevels(lcOther()), levelsFrom(LogLevel::Warning));

    // Stopped, it stays under the other filter, which calls it, and lets everything pass.
    m_universe->stop();
    QCOMPARE(enabledLevels(lcOther()), (QStringList{u"debug"_s, u"warning"_s, u"critical"_s}));
    // Started again, it filters again, from under the other one.
    QVERIFY(component->setProperty("level", u"critical"_s));
    QVERIFY(m_universe->start());
    QCOMPARE(enabledLevels(lcOther()), QStringList{u"critical"_s});
    m_universe->stop();

    // The other one gone, it is on top again, and goes as it stops.
    QLoggingCategory::installFilter(std::exchange(belowDisableInfo, nullptr));
    QCOMPARE(enabledLevels(lcOther()), AllLevels);
    QVERIFY(m_universe->start());
    QCOMPARE(enabledLevels(lcOther()), QStringList{u"critical"_s});
    m_universe->stop();
    QCOMPARE(enabledLevels(lcOther()), AllLevels);
}

void TestLogComponent::settingsFileKeepsTheLevel()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    Component* component = instantiate();
    Nebula::SettingsFile& file = m_universe->addSettingsFile();
    file.setDirectory(directory.path());
    QVERIFY(file.add({.component = component, .object = component, .property = u"level"_s}));

    QVERIFY(m_universe->start());
    controlOf(*component).setLevel(LogLevel::Warning);
    m_universe->stop();
    QCOMPARE(component->property("level").toString(), u"debug"_s);

    // What the user chose, back at the next start.
    QVERIFY(m_universe->start());
    QCOMPARE(component->property("level").toString(), u"warning"_s);
    QCOMPARE(enabledLevels(lcOther()), levelsFrom(LogLevel::Warning));
    m_universe->stop();
}

void TestLogComponent::projectRoundTrip()
{
    Component* component = instantiate();
    QVERIFY(component->setProperty("logging", false));
    QVERIFY(component->setProperty("level", u"critical"_s));
    QVERIFY(component->setProperty("category", u"app"_s));
    const QJsonObject project = m_universe->save();

    // In place of what it saved.
    QCOMPARE(m_universe->load(project), QStringList{});
    component = m_universe->component(u"LogComponent1"_s);
    QVERIFY(component != nullptr);
    QCOMPARE(component->property("logging").toBool(), false);
    QCOMPARE(component->property("level").toString(), u"critical"_s);
    QCOMPARE(component->property("category").toString(), u"app"_s);
}

QTEST_GUILESS_MAIN(TestLogComponent)
#include "TestLogComponent.moc"
