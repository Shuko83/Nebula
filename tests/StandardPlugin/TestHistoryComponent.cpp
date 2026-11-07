// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>
#include <functional>
#include <memory>
#include <utility>

#include "Nebula/Application/Universe.h"
#include "Nebula/Component/Component.h"
#include "Nebula/Standard/Command.h"
#include "Nebula/Standard/IHistory.h"

using namespace Qt::StringLiterals;
using Nebula::Component;
using Nebula::Universe;

namespace
{

const QString StandardPluginId = u"FabienDG.Nebula.StandardPlugin"_s;

// What the commands of a test did, in order: "do A", "undo A"...
QStringList steps;

// A command that writes what it does to steps, does more when it is given what, and is obsolete once obsolete is true.
class Recorded : public Nebula::Command
{
  public:
    explicit Recorded(const QString& label, std::shared_ptr<bool> obsolete = {}, std::function<void()> also = {})
        : Command(label), m_obsolete(std::move(obsolete)), m_also(std::move(also))
    {
    }

    void execute() override
    {
        steps.append(u"do "_s + label());
        if(m_also)
        {
            m_also();
        }
    }
    void undo() override
    {
        steps.append(u"undo "_s + label());
        if(m_also)
        {
            m_also();
        }
    }
    [[nodiscard]] bool isObsolete() const override
    {
        return m_obsolete != nullptr && *m_obsolete;
    }

  private:
    std::shared_ptr<bool> m_obsolete;
    std::function<void()> m_also;
};

std::unique_ptr<Nebula::Command> recorded(const QString& label)
{
    return std::make_unique<Recorded>(label);
}

Nebula::IHistory& historyOf(const Component& component)
{
    return *component.providedInterface<Nebula::IHistory>(u"history"_s);
}

// Pushes A, B, C... count commands, to history.
void pushCommands(Nebula::IHistory& history, int count)
{
    for(int index = 0; index < count; ++index)
    {
        history.push(recorded(QString(QChar(u'A' + index))));
    }
}

// Undoes history until nothing is left to undo; returns the labels of what was undone, the last one done first.
QStringList undoAll(Nebula::IHistory& history)
{
    steps.clear();
    while(history.undo())
    {
    }
    return steps;
}

} // namespace

// The HistoryComponent of StandardPlugin, loaded from the plugin as an application loads it, and the commands of
// Nebula::Standard it keeps: done, undone and done again, one step at a time, as many as its size.
class TestHistoryComponent : public QObject
{
    Q_OBJECT

  private slots:
    void init();
    void cleanup();

    void interfaceFromConstruction();
    void pushExecutes();
    void undoAndRedoStepByStep();
    void nothingToUndo();
    void pushForgetsWhatWasUndone();
    void maxSize_data();
    void maxSize();
    void smallerMaxSizeForgetsTheOldest();
    void negativeMaxSizeRefused();
    void undoTurnedOff();
    void redoTurnedOff();
    void notifies();
    void obsoleteDropped();
    void clearForgetsAll();
    void clearWhileACommandRuns();
    void pushWhileACommandRuns();
    void undoWhileACommandRuns();
    void compositeIsOneStep();
    void compositeObsoleteWithOne();
    void stopForgets();
    void projectRoundTrip();

  private:
    // An instance of HistoryComponent.
    Component* instantiate();

    std::unique_ptr<Universe> m_universe;
};

void TestHistoryComponent::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
    steps.clear();
    m_universe = std::make_unique<Universe>();
    m_universe->plugins().scan(QStringLiteral(NEBULA_STANDARD_PLUGINS_DIR));
    QVERIFY2(m_universe->plugins().load(StandardPluginId), qPrintable(m_universe->plugins().errorString()));
}

void TestHistoryComponent::cleanup()
{
    m_universe.reset();
}

Component* TestHistoryComponent::instantiate()
{
    return m_universe->instantiate(StandardPluginId + u".HistoryComponent"_s);
}

void TestHistoryComponent::interfaceFromConstruction()
{
    const Component* component = instantiate();
    QVERIFY(component != nullptr);
    QCOMPARE(component->property("maxSize").toInt(), 100);
    QCOMPARE(component->property("undoActive").toBool(), true);
    QCOMPARE(component->property("redoActive").toBool(), true);

    // Of the type this binary knows it by.
    const QList<Nebula::InterfaceInfo> provided = component->providedInterfaces();
    QCOMPARE(provided.size(), 1);
    QCOMPARE(provided.at(0).name, u"history"_s);
    QCOMPARE(provided.at(0).type, QMetaType::fromType<Nebula::IHistory*>());
    QVERIFY(provided.at(0).available);
    QVERIFY(historyOf(*component).isUndoActive());
    QVERIFY(historyOf(*component).isRedoActive());
}

void TestHistoryComponent::pushExecutes()
{
    Nebula::IHistory& history = historyOf(*instantiate());
    history.push(recorded(u"A"_s));
    QCOMPARE(steps, QStringList{u"do A"_s});
    history.push(nullptr); // Nothing
    QCOMPARE(steps, QStringList{u"do A"_s});
}

void TestHistoryComponent::undoAndRedoStepByStep()
{
    Nebula::IHistory& history = historyOf(*instantiate());
    pushCommands(history, 3);
    steps.clear();

    QVERIFY(history.undo());
    QCOMPARE(steps, QStringList{u"undo C"_s});
    QVERIFY(history.undo());
    QCOMPARE(steps, (QStringList{u"undo C"_s, u"undo B"_s}));
    QVERIFY(history.redo());
    QCOMPARE(steps.constLast(), u"do B"_s);
    QVERIFY(history.redo());
    QCOMPARE(steps.constLast(), u"do C"_s);
    QVERIFY(!history.redo()); // Each one done again
    QCOMPARE(steps.size(), 4);

    // The same, back and forth.
    QCOMPARE(undoAll(history), (QStringList{u"undo C"_s, u"undo B"_s, u"undo A"_s}));
    steps.clear();
    while(history.redo())
    {
    }
    QCOMPARE(steps, (QStringList{u"do A"_s, u"do B"_s, u"do C"_s}));
}

void TestHistoryComponent::nothingToUndo()
{
    Nebula::IHistory& history = historyOf(*instantiate());
    QVERIFY(!history.undo());
    QVERIFY(!history.redo());
    history.push(recorded(u"A"_s));
    QVERIFY(!history.redo()); // Nothing was undone
    QVERIFY(history.undo());
    QVERIFY(!history.undo());
    QCOMPARE(steps, (QStringList{u"do A"_s, u"undo A"_s}));
}

void TestHistoryComponent::pushForgetsWhatWasUndone()
{
    Nebula::IHistory& history = historyOf(*instantiate());
    pushCommands(history, 2);
    QVERIFY(history.undo()); // B
    history.push(recorded(u"C"_s));
    steps.clear();
    QVERIFY(!history.redo()); // B can no longer be redone
    QCOMPARE(undoAll(history), (QStringList{u"undo C"_s, u"undo A"_s}));
}

void TestHistoryComponent::maxSize_data()
{
    QTest::addColumn<int>("maxSize");
    QTest::addColumn<int>("pushed");
    QTest::addColumn<QStringList>("undone");

    QTest::newRow("under") << 3 << 2 << QStringList{u"undo B"_s, u"undo A"_s};
    QTest::newRow("as many") << 3 << 3 << QStringList{u"undo C"_s, u"undo B"_s, u"undo A"_s};
    QTest::newRow("beyond") << 3 << 5 << QStringList{u"undo E"_s, u"undo D"_s, u"undo C"_s};
    QTest::newRow("one") << 1 << 3 << QStringList{u"undo C"_s};
    QTest::newRow("none") << 0 << 2 << QStringList{};
}

void TestHistoryComponent::maxSize()
{
    QFETCH(int, maxSize);
    QFETCH(int, pushed);
    QFETCH(QStringList, undone);
    Component* component = instantiate();
    QVERIFY(component->setProperty("maxSize", maxSize));
    Nebula::IHistory& history = historyOf(*component);

    pushCommands(history, pushed);
    QCOMPARE(steps.size(), pushed); // Each one done, even those it does not keep
    QCOMPARE(undoAll(history), undone);
}

void TestHistoryComponent::smallerMaxSizeForgetsTheOldest()
{
    Component* component = instantiate();
    Nebula::IHistory& history = historyOf(*component);
    pushCommands(history, 5);
    QVERIFY(history.undo()); // E
    QVERIFY(history.undo()); // D

    // The oldest done go first: A and B, then C.
    component->setProperty("maxSize", 3);
    steps.clear();
    QVERIFY(history.undo());
    QCOMPARE(steps, QStringList{u"undo C"_s});
    QVERIFY(!history.undo());

    // Then the undone furthest from being redone: E.
    component->setProperty("maxSize", 1);
    steps.clear();
    QVERIFY(history.redo());
    QVERIFY(!history.redo());
    QCOMPARE(steps, QStringList{u"do C"_s});
}

void TestHistoryComponent::negativeMaxSizeRefused()
{
    Component* component = instantiate();
    component->setObjectName(u"History1"_s);
    QTest::ignoreMessage(QtWarningMsg, "History1: a history cannot keep -1 commands, 100 is kept");
    component->setProperty("maxSize", -1);
    QCOMPARE(component->property("maxSize").toInt(), 100);
}

void TestHistoryComponent::undoTurnedOff()
{
    Component* component = instantiate();
    Nebula::IHistory& history = historyOf(*component);
    pushCommands(history, 2);
    QVERIFY(history.undo()); // B, which can be redone

    QVERIFY(component->setProperty("undoActive", false));
    QVERIFY(!history.isUndoActive());
    steps.clear();
    QVERIFY(!history.undo());
    QVERIFY(steps.isEmpty());
    QVERIFY(history.redo()); // Still
    QCOMPARE(steps, QStringList{u"do B"_s});

    component->setProperty("undoActive", true);
    QVERIFY(history.undo());
}

void TestHistoryComponent::redoTurnedOff()
{
    Component* component = instantiate();
    Nebula::IHistory& history = historyOf(*component);
    pushCommands(history, 1);
    QVERIFY(component->setProperty("redoActive", false));
    QVERIFY(!history.isRedoActive());
    QVERIFY(history.undo()); // Still
    steps.clear();
    QVERIFY(!history.redo());
    QVERIFY(steps.isEmpty());

    component->setProperty("redoActive", true);
    QVERIFY(history.redo());
    QCOMPARE(steps, QStringList{u"do A"_s});
}

void TestHistoryComponent::notifies()
{
    Component* component = instantiate();
    QSignalSpy maxSize(component, SIGNAL(maxSizeChanged(int)));
    QSignalSpy undoActive(component, SIGNAL(undoActiveChanged(bool)));
    QSignalSpy redoActive(component, SIGNAL(redoActiveChanged(bool)));

    component->setProperty("maxSize", 10);
    component->setProperty("maxSize", 10); // The same
    component->setProperty("undoActive", false);
    component->setProperty("redoActive", false);
    component->setProperty("redoActive", false);
    QCOMPARE(maxSize.size(), 1);
    QCOMPARE(maxSize.at(0).at(0).toInt(), 10);
    QCOMPARE(undoActive.size(), 1);
    QCOMPARE(undoActive.at(0).at(0).toBool(), false);
    QCOMPARE(redoActive.size(), 1);
}

void TestHistoryComponent::obsoleteDropped()
{
    Nebula::IHistory& history = historyOf(*instantiate());
    const auto obsolete = std::make_shared<bool>(false);
    history.push(recorded(u"A"_s));
    history.push(std::make_unique<Recorded>(u"B"_s, obsolete));
    history.push(recorded(u"C"_s));
    *obsolete = true; // What B acts on is gone

    QCOMPARE(undoAll(history), (QStringList{u"undo C"_s, u"undo A"_s}));
    steps.clear();
    while(history.redo())
    {
    }
    QCOMPARE(steps, (QStringList{u"do A"_s, u"do C"_s}));

    // One undone, then obsolete, is not done again.
    const auto gone = std::make_shared<bool>(false);
    history.push(std::make_unique<Recorded>(u"D"_s, gone));
    QVERIFY(history.undo());
    *gone = true;
    steps.clear();
    QVERIFY(!history.redo());
    QVERIFY(steps.isEmpty());
    QCOMPARE(undoAll(history), (QStringList{u"undo C"_s, u"undo A"_s}));
}

void TestHistoryComponent::clearForgetsAll()
{
    Nebula::IHistory& history = historyOf(*instantiate());
    pushCommands(history, 3);
    QVERIFY(history.undo());
    history.clear();
    steps.clear();
    QVERIFY(!history.undo());
    QVERIFY(!history.redo());
    QVERIFY(steps.isEmpty());
}

void TestHistoryComponent::clearWhileACommandRuns()
{
    Nebula::IHistory& history = historyOf(*instantiate());
    history.push(recorded(u"A"_s));
    // Undoing it clears the history, which it is in: once it is done.
    bool clearing = false;
    history.push(std::make_unique<Recorded>(u"B"_s, nullptr,
                                            [&history, &clearing]
                                            {
                                                if(clearing)
                                                {
                                                    history.clear();
                                                }
                                            }));
    clearing = true;
    QVERIFY(history.undo());
    steps.clear();
    QVERIFY(!history.undo());
    QVERIFY(!history.redo());
    QVERIFY(steps.isEmpty());
}

void TestHistoryComponent::pushWhileACommandRuns()
{
    Nebula::IHistory& history = historyOf(*instantiate());
    QTest::ignoreMessage(QtWarningMsg,
                         "\"inner\" is pushed to the history while another command runs: it is done, but not kept");
    history.push(std::make_unique<Recorded>(u"outer"_s, nullptr, [&history] { history.push(recorded(u"inner"_s)); }));
    QCOMPARE(steps, (QStringList{u"do outer"_s, u"do inner"_s}));

    steps.clear();
    QTest::ignoreMessage(QtWarningMsg,
                         "\"inner\" is pushed to the history while another command runs: it is done, but not kept");
    QVERIFY(history.undo());
    QVERIFY(!history.undo()); // Only the outer one was kept
    QCOMPARE(steps, (QStringList{u"undo outer"_s, u"do inner"_s}));
}

void TestHistoryComponent::undoWhileACommandRuns()
{
    Nebula::IHistory& history = historyOf(*instantiate());
    history.push(recorded(u"A"_s));
    bool undone = true;
    QTest::ignoreMessage(QtWarningMsg, "Nothing is undone while a command of the history runs");
    history.push(std::make_unique<Recorded>(u"B"_s, nullptr, [&history, &undone] { undone = history.undo(); }));
    QVERIFY(!undone); // Refused, as B was pushed

    QTest::ignoreMessage(QtWarningMsg, "Nothing is undone while a command of the history runs");
    QVERIFY(history.undo()); // B, which tries to undo A
    QVERIFY(!undone);
    steps.clear();
    QVERIFY(history.undo());
    QCOMPARE(steps, QStringList{u"undo A"_s});
}

void TestHistoryComponent::compositeIsOneStep()
{
    auto composite = std::make_unique<Nebula::CompositeCommand>(u"Remove Sensor1"_s);
    QVERIFY(composite->isEmpty());
    composite->add(recorded(u"links"_s));
    composite->add(nullptr); // Nothing
    composite->add(recorded(u"component"_s));
    QCOMPARE(composite->count(), std::size_t(2));
    QCOMPARE(composite->label(), u"Remove Sensor1"_s);

    Nebula::IHistory& history = historyOf(*instantiate());
    history.push(std::move(composite));
    QCOMPARE(steps, (QStringList{u"do links"_s, u"do component"_s}));
    steps.clear();
    QVERIFY(history.undo());
    QCOMPARE(steps, (QStringList{u"undo component"_s, u"undo links"_s})); // The other way round
    QVERIFY(!history.undo());                                             // One step
    steps.clear();
    QVERIFY(history.redo());
    QCOMPARE(steps, (QStringList{u"do links"_s, u"do component"_s}));
}

void TestHistoryComponent::compositeObsoleteWithOne()
{
    const auto obsolete = std::make_shared<bool>(false);
    Nebula::CompositeCommand composite(u"Both"_s);
    composite.add(recorded(u"A"_s));
    composite.add(std::make_unique<Recorded>(u"B"_s, obsolete));
    QVERIFY(!composite.isObsolete());
    *obsolete = true;
    QVERIFY(composite.isObsolete());
}

void TestHistoryComponent::stopForgets()
{
    Component* component = instantiate();
    Nebula::IHistory& history = historyOf(*component);
    QVERIFY(m_universe->start());
    pushCommands(history, 2);
    QVERIFY(history.undo()); // While it runs
    m_universe->stop();
    steps.clear();
    QVERIFY(!history.undo());
    QVERIFY(!history.redo());
    QVERIFY(steps.isEmpty());
}

void TestHistoryComponent::projectRoundTrip()
{
    Component* component = instantiate();
    component->setProperty("maxSize", 20);
    component->setProperty("redoActive", false);
    const QJsonObject project = m_universe->save();

    Universe other;
    QVERIFY(other.load(project).isEmpty());
    const Component* read = other.component(component->objectName());
    QVERIFY(read != nullptr);
    QCOMPARE(read->property("maxSize").toInt(), 20);
    QCOMPARE(read->property("undoActive").toBool(), true);
    QCOMPARE(read->property("redoActive").toBool(), false);
    QVERIFY(!historyOf(*read).isRedoActive());
}

QTEST_GUILESS_MAIN(TestHistoryComponent)
#include "TestHistoryComponent.moc"
