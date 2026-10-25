// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QAction>
#include <QJsonObject>
#include <QKeySequence>
#include <QLabel>
#include <QMainWindow>
#include <QPointer>
#include <QTest>
#include <QToolBar>
#include <QWidgetAction>
#include <memory>

#include "Nebula/Application/Universe.h"
#include "Nebula/Component/Component.h"

using namespace Qt::StringLiterals;
using Nebula::Component;
using Nebula::Universe;

namespace
{

const QString WidgetsPluginId = u"FabienDG.Nebula.WidgetsPlugin"_s;

// The window a MainWindowComponent provides.
QMainWindow* windowOf(const Component& mainWindow)
{
    return mainWindow.providedInterface<QMainWindow>(u"mainWindow"_s);
}

// The bar of the TopBarComponent in window; nullptr when there is none.
QToolBar* barIn(const QMainWindow& window)
{
    const QList<QToolBar*> bars = window.findChildren<QToolBar*>();
    return bars.size() == 1 ? bars.constFirst() : nullptr;
}

// The texts of the actions bar shows, in their order, but those holding its widgets.
QStringList actionTexts(const QToolBar& bar)
{
    QStringList texts;
    for(const QAction* action : bar.actions())
    {
        if(qobject_cast<const QWidgetAction*>(action) == nullptr)
        {
            texts.append(action->text());
        }
    }
    return texts;
}

// The place, among the actions of bar, of the one holding widget; -1 when it does not hold it, or it is hidden.
qsizetype placeIn(const QToolBar& bar, const QWidget& widget)
{
    const QList<QAction*> actions = bar.actions();
    for(qsizetype place = 0; place < actions.size(); ++place)
    {
        const auto* holder = qobject_cast<const QWidgetAction*>(actions.at(place));
        if(holder != nullptr && holder->isVisible() && holder->defaultWidget() == widget.parentWidget())
        {
            return place;
        }
    }
    return -1;
}

// The warning of the platform of the tests, offscreen, as a window with a bar shows: it does not follow the size the
// window asks for, which a platform with a screen does.
void ignoreSizeHints()
{
    QTest::ignoreMessage(QtWarningMsg, "This plugin does not support propagateSizeHints()");
}

} // namespace

// The components of WidgetsPlugin, loaded from the plugin as an application loads them, and assembled by their
// interfaces: a main window, its top bar, actions, and counters of these actions.
class TestWidgetsPlugin : public QObject
{
    Q_OBJECT

  private slots:
    void init();
    void cleanup();

    void mainWindowShowsWhileItRuns();
    void centralWidget();
    void topBarNeedsTheMainWindow();
    void actionsInTheOrderOfTheirBindings();
    void widgetsInTheOrderOfTheirBindings();
    void actionShortcut_data();
    void actionShortcut();
    void counterCountsWhileItRuns();
    void counterText_data();
    void counterText();
    void projectRoundTrip();

  private:
    // An instance of the component name of WidgetsPlugin.
    Component* instantiate(const QString& name);

    std::unique_ptr<Universe> m_universe;
};

void TestWidgetsPlugin::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
    m_universe = std::make_unique<Universe>();
    m_universe->plugins().scan(QStringLiteral(NEBULA_EXAMPLE_PLUGINS_DIR));
    QVERIFY2(m_universe->plugins().load(WidgetsPluginId), qPrintable(m_universe->plugins().errorString()));
}

void TestWidgetsPlugin::cleanup()
{
    m_universe.reset();
}

Component* TestWidgetsPlugin::instantiate(const QString& name)
{
    return m_universe->instantiate(WidgetsPluginId + u'.' + name);
}

void TestWidgetsPlugin::mainWindowShowsWhileItRuns()
{
    Component* mainWindow = instantiate(u"MainWindowComponent"_s);
    QVERIFY(mainWindow != nullptr);
    // Available while it is edited, for others to fill it.
    QMainWindow* window = windowOf(*mainWindow);
    QVERIFY(window != nullptr);
    QVERIFY(mainWindow->setProperty("title", u"Clock"_s));
    QVERIFY(mainWindow->setProperty("width", 640));
    QVERIFY(mainWindow->setProperty("height", 480));
    QCOMPARE(window->windowTitle(), u"Clock"_s);
    QVERIFY(!window->isVisible());

    QVERIFY(m_universe->start());
    QVERIFY(window->isVisible());
    QCOMPARE(window->size(), QSize(640, 480));
    m_universe->stop();
    QVERIFY(!window->isVisible());
}

void TestWidgetsPlugin::centralWidget()
{
    Component* mainWindow = instantiate(u"MainWindowComponent"_s);
    Component* counter = instantiate(u"CounterComponent"_s);
    QMainWindow* window = windowOf(*mainWindow);
    const QPointer<QWidget> display = counter->providedInterface<QWidget>(u"display"_s);
    QVERIFY(display != nullptr);
    QVERIFY(mainWindow->unboundMandatoryInterfaces().isEmpty()); // Optional

    QVERIFY(mainWindow->bindInterface(u"centralWidget"_s, *counter, u"display"_s));
    QCOMPARE(window->centralWidget(), display.data());
    // Given back as it was provided, again and again.
    mainWindow->unbindInterface(u"centralWidget"_s);
    QCOMPARE(window->centralWidget(), nullptr);
    QCOMPARE(display->parent(), nullptr);
    QVERIFY(mainWindow->bindInterface(u"centralWidget"_s, *counter, u"display"_s));
    QCOMPARE(window->centralWidget(), display.data());

    // Neither deletes what the other one owns as it goes.
    QVERIFY(m_universe->remove(*mainWindow));
    QVERIFY(display != nullptr);
    QCOMPARE(display->parent(), nullptr);
    mainWindow = instantiate(u"MainWindowComponent"_s);
    QVERIFY(mainWindow->bindInterface(u"centralWidget"_s, *counter, u"display"_s));
    QVERIFY(m_universe->remove(*counter));
    QVERIFY(display == nullptr);
    QCOMPARE(windowOf(*mainWindow)->centralWidget(), nullptr);
}

void TestWidgetsPlugin::topBarNeedsTheMainWindow()
{
    Component* mainWindow = instantiate(u"MainWindowComponent"_s);
    Component* topBar = instantiate(u"TopBarComponent"_s);
    QMainWindow* window = windowOf(*mainWindow);
    QCOMPARE(topBar->unboundMandatoryInterfaces(), QStringList{u"mainWindow"_s});
    QCOMPARE(barIn(*window), nullptr);

    QVERIFY(topBar->bindInterface(u"mainWindow"_s, *mainWindow, u"mainWindow"_s));
    QVERIFY(topBar->unboundMandatoryInterfaces().isEmpty());
    const QPointer<QToolBar> bar = barIn(*window);
    QVERIFY(bar != nullptr);
    QCOMPARE(window->toolBarArea(bar), Qt::TopToolBarArea);
    QVERIFY(topBar->setProperty("title", u"Tools"_s));
    QCOMPARE(bar->windowTitle(), u"Tools"_s);
    QVERIFY(!bar->isMovable());
    QVERIFY(topBar->setProperty("movable", true));
    QVERIFY(bar->isMovable());
    ignoreSizeHints();
    QVERIFY(m_universe->start());
    QVERIFY(bar->isVisible());
    m_universe->stop();

    topBar->unbindInterface(u"mainWindow"_s);
    QCOMPARE(barIn(*window), nullptr);
    QVERIFY(bar != nullptr);
    QCOMPARE(bar->parent(), nullptr);

    // Neither deletes what the other one owns as it goes.
    QVERIFY(topBar->bindInterface(u"mainWindow"_s, *mainWindow, u"mainWindow"_s));
    QVERIFY(m_universe->remove(*mainWindow));
    QVERIFY(bar != nullptr);
    QCOMPARE(topBar->unboundMandatoryInterfaces(), QStringList{u"mainWindow"_s});
    mainWindow = instantiate(u"MainWindowComponent"_s);
    QVERIFY(topBar->bindInterface(u"mainWindow"_s, *mainWindow, u"mainWindow"_s));
    QVERIFY(m_universe->remove(*topBar));
    QVERIFY(bar == nullptr);
    QCOMPARE(barIn(*windowOf(*mainWindow)), nullptr);
}

void TestWidgetsPlugin::actionsInTheOrderOfTheirBindings()
{
    Component* mainWindow = instantiate(u"MainWindowComponent"_s);
    Component* topBar = instantiate(u"TopBarComponent"_s);
    QVERIFY(topBar->bindInterface(u"mainWindow"_s, *mainWindow, u"mainWindow"_s));
    QToolBar* bar = barIn(*windowOf(*mainWindow));
    QList<Component*> actions;
    for(const QString& text : {u"A"_s, u"B"_s, u"C"_s})
    {
        actions.append(instantiate(u"ActionComponent"_s));
        QVERIFY(actions.last()->setProperty("text", text));
    }
    QCOMPARE(actionTexts(*bar), QStringList{});
    QVERIFY(topBar->consumedInterfaces().at(1).list);

    QVERIFY(topBar->bindInterface(u"actions"_s, *actions.at(1), u"action"_s));
    QVERIFY(topBar->bindInterface(u"actions"_s, *actions.at(0), u"action"_s));
    QCOMPARE(actionTexts(*bar), (QStringList{u"B"_s, u"A"_s}));
    QVERIFY(topBar->bindInterface(u"actions"_s, *actions.at(2), u"action"_s));
    QCOMPARE(actionTexts(*bar), (QStringList{u"B"_s, u"A"_s, u"C"_s}));
    topBar->unbindInterface(u"actions"_s, *actions.at(1), u"action"_s);
    QCOMPARE(actionTexts(*bar), (QStringList{u"A"_s, u"C"_s}));
    // Bound again, it comes last.
    QVERIFY(topBar->bindInterface(u"actions"_s, *actions.at(1), u"action"_s));
    QCOMPARE(actionTexts(*bar), (QStringList{u"A"_s, u"C"_s, u"B"_s}));
    // Not twice to the same one.
    QVERIFY(!topBar->bindInterface(u"actions"_s, *actions.at(1), u"action"_s));
    QCOMPARE(actionTexts(*bar), (QStringList{u"A"_s, u"C"_s, u"B"_s}));

    // Before the widgets, those bound before them as those bound after.
    Component* counter = instantiate(u"CounterComponent"_s);
    QVERIFY(topBar->bindInterface(u"widgets"_s, *counter, u"display"_s));
    QCOMPARE(placeIn(*bar, *counter->providedInterface<QWidget>(u"display"_s)), 3);
    Component* late = instantiate(u"ActionComponent"_s);
    QVERIFY(late->setProperty("text", u"D"_s));
    QVERIFY(topBar->bindInterface(u"actions"_s, *late, u"action"_s));
    QCOMPARE(actionTexts(*bar), (QStringList{u"A"_s, u"C"_s, u"B"_s, u"D"_s}));
    QCOMPARE(placeIn(*bar, *counter->providedInterface<QWidget>(u"display"_s)), 4);

    QVERIFY(m_universe->remove(*late));
    QVERIFY(m_universe->remove(*actions.at(0)));
    QCOMPARE(actionTexts(*bar), (QStringList{u"C"_s, u"B"_s}));
    topBar->unbindInterface(u"actions"_s);
    QCOMPARE(actionTexts(*bar), QStringList{});
    QCOMPARE(placeIn(*bar, *counter->providedInterface<QWidget>(u"display"_s)), 0);
}

void TestWidgetsPlugin::widgetsInTheOrderOfTheirBindings()
{
    Component* mainWindow = instantiate(u"MainWindowComponent"_s);
    Component* topBar = instantiate(u"TopBarComponent"_s);
    QVERIFY(topBar->bindInterface(u"mainWindow"_s, *mainWindow, u"mainWindow"_s));
    QToolBar* bar = barIn(*windowOf(*mainWindow));
    Component* first = instantiate(u"CounterComponent"_s);
    Component* second = instantiate(u"CounterComponent"_s);
    const QPointer<QWidget> firstDisplay = first->providedInterface<QWidget>(u"display"_s);
    const QPointer<QWidget> secondDisplay = second->providedInterface<QWidget>(u"display"_s);

    QVERIFY(topBar->bindInterface(u"widgets"_s, *second, u"display"_s));
    QVERIFY(topBar->bindInterface(u"widgets"_s, *first, u"display"_s));
    QCOMPARE(placeIn(*bar, *secondDisplay), 0);
    QCOMPARE(placeIn(*bar, *firstDisplay), 1);
    // Shown with the bar, its place being visible: not shown here, as the platform of the tests has no font to show it.
    QVERIFY(firstDisplay->isVisibleTo(firstDisplay->parentWidget()));

    // Given back as it was provided, its place gone.
    topBar->unbindInterface(u"widgets"_s, *second, u"display"_s);
    QCOMPARE(secondDisplay->parent(), nullptr);
    QCOMPARE(placeIn(*bar, *secondDisplay), -1);
    QCOMPARE(placeIn(*bar, *firstDisplay), 0);
    // Bound again, in its place among the others: after them.
    QVERIFY(topBar->bindInterface(u"widgets"_s, *second, u"display"_s));
    QCOMPARE(placeIn(*bar, *firstDisplay), 0);
    QCOMPARE(placeIn(*bar, *secondDisplay), 1);
    QVERIFY(m_universe->remove(*topBar));
    QVERIFY(firstDisplay != nullptr);
    QCOMPARE(firstDisplay->parent(), nullptr);
    QVERIFY(secondDisplay != nullptr);
    QCOMPARE(secondDisplay->parent(), nullptr);
}

void TestWidgetsPlugin::actionShortcut_data()
{
    QTest::addColumn<QString>("shortcut");
    QTest::addColumn<QKeySequence>("keys");
    QTest::addColumn<QString>("read");

    QTest::newRow("one") << "Ctrl+N" << QKeySequence(Qt::CTRL | Qt::Key_N) << "Ctrl+N";
    QTest::newRow("none") << "" << QKeySequence() << "";
}

void TestWidgetsPlugin::actionShortcut()
{
    QFETCH(QString, shortcut);
    QFETCH(QKeySequence, keys);
    QFETCH(QString, read);

    Component* action = instantiate(u"ActionComponent"_s);
    QVERIFY(action->setProperty("shortcut", u"Ctrl+Q"_s));
    QVERIFY(action->setProperty("shortcut", shortcut));
    QCOMPARE(action->providedInterface<QAction>(u"action"_s)->shortcut(), keys);
    QCOMPARE(action->property("shortcut").toString(), read);
}

void TestWidgetsPlugin::counterCountsWhileItRuns()
{
    Component* action = instantiate(u"ActionComponent"_s);
    Component* counter = instantiate(u"CounterComponent"_s);
    auto* display = qobject_cast<QLabel*>(counter->providedInterface<QWidget>(u"display"_s));
    QVERIFY(display != nullptr);
    QCOMPARE(counter->unboundMandatoryInterfaces(), QStringList{u"action"_s});
    QVERIFY(counter->bindInterface(u"action"_s, *action, u"action"_s));
    QAction* triggered = action->providedInterface<QAction>(u"action"_s);

    triggered->trigger();
    QCOMPARE(counter->property("count").toInt(), 0);
    QVERIFY(m_universe->start());
    triggered->trigger();
    triggered->trigger();
    QCOMPARE(counter->property("count").toInt(), 2);
    QCOMPARE(display->text(), u"Triggered 2 times"_s);
    // Back to the count of the edition.
    m_universe->stop();
    QCOMPARE(counter->property("count").toInt(), 0);
    QCOMPARE(display->text(), u"Triggered 0 times"_s);

    counter->unbindInterface(u"action"_s);
    QVERIFY(m_universe->start());
    triggered->trigger();
    QCOMPARE(counter->property("count").toInt(), 0);
}

void TestWidgetsPlugin::counterText_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("shown");

    QTest::newRow("count in it") << "Clicks: %1" << "Clicks: 3";
    QTest::newRow("count twice") << "%1 / %1" << "3 / 3";
    QTest::newRow("no count") << "Clicks" << "Clicks";
}

void TestWidgetsPlugin::counterText()
{
    QFETCH(QString, text);
    QFETCH(QString, shown);

    Component* counter = instantiate(u"CounterComponent"_s);
    QVERIFY(counter->setProperty("count", 3));
    QVERIFY(counter->setProperty("text", text));
    QCOMPARE(qobject_cast<QLabel*>(counter->providedInterface<QWidget>(u"display"_s))->text(), shown);
}

void TestWidgetsPlugin::projectRoundTrip()
{
    Component* mainWindow = instantiate(u"MainWindowComponent"_s);
    Component* topBar = instantiate(u"TopBarComponent"_s);
    Component* action = instantiate(u"ActionComponent"_s);
    Component* counter = instantiate(u"CounterComponent"_s);
    QVERIFY(mainWindow->setProperty("title", u"Clicks"_s));
    QVERIFY(action->setProperty("text", u"Add one"_s));
    QVERIFY(action->setProperty("shortcut", u"Ctrl+N"_s));
    QVERIFY(topBar->bindInterface(u"mainWindow"_s, *mainWindow, u"mainWindow"_s));
    QVERIFY(topBar->bindInterface(u"actions"_s, *action, u"action"_s));
    QVERIFY(counter->bindInterface(u"action"_s, *action, u"action"_s));
    QVERIFY(mainWindow->bindInterface(u"centralWidget"_s, *counter, u"display"_s));
    const QJsonObject project = m_universe->save();

    // In place of what it saved: none of these components is left.
    QCOMPARE(m_universe->load(project), QStringList{});
    QMainWindow* window = windowOf(*m_universe->component(u"MainWindowComponent1"_s));
    QCOMPARE(window->windowTitle(), u"Clicks"_s);
    QCOMPARE(window->centralWidget(),
             m_universe->component(u"CounterComponent1"_s)->providedInterface<QWidget>(u"display"_s));
    QToolBar* bar = barIn(*window);
    QVERIFY(bar != nullptr);
    QCOMPARE(actionTexts(*bar), QStringList{u"Add one"_s});
    QCOMPARE(bar->actions().constFirst()->shortcut(), QKeySequence(Qt::CTRL | Qt::Key_N));
}

QTEST_MAIN(TestWidgetsPlugin)
#include "TestWidgetsPlugin.moc"
