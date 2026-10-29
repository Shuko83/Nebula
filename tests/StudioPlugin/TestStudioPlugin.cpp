// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QGraphicsView>
#include <QGroupBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPointer>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <memory>

#include "Nebula/Application/Universe.h"
#include "Nebula/Component/Component.h"

using namespace Qt::StringLiterals;
using Nebula::Component;
using Nebula::Universe;

namespace
{

const QString StudioPluginId = u"FabienDG.Nebula.StudioPlugin"_s;

// The main window of NebulaStudio, as a StudioWindowComponent shows it; nullptr when none shows.
QMainWindow* shownWindow()
{
    for(QWidget* widget : QApplication::topLevelWidgets())
    {
        auto* window = qobject_cast<QMainWindow*>(widget);
        if(window != nullptr && window->isVisible())
        {
            return window;
        }
    }
    return nullptr;
}

// The texts of the menus of window, in their order.
QStringList menuTitles(const QMainWindow& window)
{
    QStringList titles;
    for(const QAction* menu : window.menuBar()->actions())
    {
        titles.append(menu->text());
    }
    return titles;
}

// The menu of window titled title; nullptr when it has none.
QMenu* menuOf(const QMainWindow& window, const QString& title)
{
    for(const QAction* menu : window.menuBar()->actions())
    {
        if(menu->text() == title)
        {
            return menu->menu();
        }
    }
    return nullptr;
}

// The action of menu whose text is text; nullptr when it has none.
QAction* actionOf(const QMenu& menu, const QString& text)
{
    for(QAction* action : menu.actions())
    {
        if(action->text() == text)
        {
            return action;
        }
    }
    return nullptr;
}

// The titles of the tabs of tabs, in their order.
QStringList tabTitles(const QTabWidget& tabs)
{
    QStringList titles;
    for(int index = 0; index < tabs.count(); ++index)
    {
        titles.append(tabs.tabText(index));
    }
    return titles;
}

// The title of group, a QGroupBox; none when it is not one.
QString groupTitle(const QWidget& group)
{
    const auto* box = qobject_cast<const QGroupBox*>(&group);
    return box != nullptr ? box->title() : QString();
}

// The field of the Galaxies tab in widget, which tells the directory listed; nullptr when there is none.
QLineEdit* galaxyDirectoryIn(const QWidget& widget)
{
    for(QLineEdit* field : widget.findChildren<QLineEdit*>())
    {
        if(field->isReadOnly())
        {
            return field;
        }
    }
    return nullptr;
}

// The galaxy of NebulaStudio, as its file holds it.
QJsonObject galaxyFile()
{
    QFile file(QStringLiteral(NEBULA_STUDIO_GALAXY));
    if(!file.open(QIODevice::ReadOnly))
    {
        return {};
    }
    return QJsonDocument::fromJson(file.readAll()).object();
}

// What json, the components of a project, holds of each: its name, its type and its properties, by name.
QHash<QString, QJsonObject> componentsOf(const QJsonArray& json)
{
    QHash<QString, QJsonObject> components;
    for(const auto& value : json)
    {
        const QJsonObject component = value.toObject();
        components.insert(component.value(u"name").toString(),
                          {{u"type"_s, component.value(u"type")}, {u"properties"_s, component.value(u"properties")}});
    }
    return components;
}

// What json, the bindings of a project, binds, in its order, without the anchors of NebulaStudio.
QStringList bindingsOf(const QJsonArray& json)
{
    QStringList bindings;
    for(const auto& value : json)
    {
        const QJsonObject binding = value.toObject();
        bindings.append(
            u"%1.%2 <- %3.%4"_s.arg(binding.value(u"consumer").toString(), binding.value(u"consumed").toString(),
                                    binding.value(u"provider").toString(), binding.value(u"provided").toString()));
    }
    return bindings;
}

// The warning of the platform of the tests, offscreen, as a window with a menu bar shows: it does not follow the size
// the window asks for, which a platform with a screen does.
void ignoreSizeHints()
{
    QTest::ignoreMessage(QtWarningMsg, "This plugin does not support propagateSizeHints()");
}

} // namespace

// The components of StudioPlugin, loaded from the plugin as an application loads them, and the galaxy of NebulaStudio,
// which assembles them into its window.
class TestStudioPlugin : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void cleanup();

    void galaxyOpens();
    void windowShowsWhileItRuns();
    void partsInTheWindow();
    void partsGoWithTheStudio();
    void stoppingStopsTheApplicationAssembled();
    void galaxyDirectory_data();
    void galaxyDirectory();
    void splitterInTheOrderOfItsBindings();
    void splitterSizes();
    void tabsUnderTheTitlesOfTheirPages();
    void groupGivesItsContentBack();
    void galaxyRoundTrip();

  private:
    // Opens the galaxy of NebulaStudio in place of the application.
    void openGalaxy();
    // An instance of the component name of StudioPlugin.
    Component* instantiate(const QString& name);
    // The widget component provides as "widget".
    static QWidget* widgetOf(const Component& component);

    std::unique_ptr<Universe> m_universe;
};

void TestStudioPlugin::initTestCase()
{
    QFontDatabase::families(); // The platform of the tests warns, once, that it has no fonts: before failOnWarning()
}

void TestStudioPlugin::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
    m_universe = std::make_unique<Universe>();
    m_universe->plugins().scan(QStringLiteral(NEBULA_STUDIO_PLUGINS_DIR));
    QVERIFY2(m_universe->plugins().load(StudioPluginId), qPrintable(m_universe->plugins().errorString()));
}

void TestStudioPlugin::cleanup()
{
    m_universe.reset();
}

void TestStudioPlugin::openGalaxy()
{
    QVERIFY2(m_universe->open(QStringLiteral(NEBULA_STUDIO_GALAXY)), qPrintable(m_universe->errorString()));
}

Component* TestStudioPlugin::instantiate(const QString& name)
{
    return m_universe->instantiate(StudioPluginId + u'.' + name);
}

QWidget* TestStudioPlugin::widgetOf(const Component& component)
{
    return component.providedInterface<QWidget>(u"widget"_s);
}

void TestStudioPlugin::galaxyOpens()
{
    openGalaxy();
    QCOMPARE(m_universe->components().size(), 14);
    // Each of its parts has what it needs.
    for(const std::unique_ptr<Component>& component : m_universe->components())
    {
        QVERIFY2(component->unboundMandatoryInterfaces().isEmpty(), qPrintable(component->objectName()));
    }
    QVERIFY(shownWindow() == nullptr); // Not until it runs
}

void TestStudioPlugin::windowShowsWhileItRuns()
{
    openGalaxy();
    ignoreSizeHints();
    QVERIFY(m_universe->start());
    QMainWindow* window = shownWindow();
    QVERIFY(window != nullptr);
    QCOMPARE(window->size(), QSize(1440, 800));
    QCOMPARE(window->windowTitle(), u"NebulaStudio"_s); // Its project has no file
    // In the order of their bindings.
    QCOMPARE(menuTitles(*window), QStringList({u"&File"_s, u"&View"_s, u"&Run"_s}));
    QCOMPARE(window->statusBar()->findChildren<QLabel*>().size(), 2); // Where the plugins are, and the zoom

    m_universe->stop();
    QVERIFY(!window->isVisible());
}

void TestStudioPlugin::partsInTheWindow()
{
    openGalaxy();
    ignoreSizeHints();
    QVERIFY(m_universe->start());
    const QMainWindow* window = shownWindow();
    QVERIFY(window != nullptr);
    const auto* columns = qobject_cast<QSplitter*>(window->centralWidget());
    QVERIFY(columns != nullptr);
    QCOMPARE(columns->count(), 3);
    const auto* sources = qobject_cast<QTabWidget*>(columns->widget(0));
    QVERIFY(sources != nullptr);
    QCOMPARE(tabTitles(*sources), QStringList({u"Plugins"_s, u"Galaxies"_s}));
    QCOMPARE(groupTitle(*columns->widget(1)), u"Application"_s);
    QCOMPARE(groupTitle(*columns->widget(2)), u"Components"_s);
    // The edition zone, in the Application group, above the property editor and the problems.
    QCOMPARE(window->findChildren<QGraphicsView*>().size(), 1);
    const auto* edition = window->findChild<QSplitter*>();
    QVERIFY(edition != nullptr);
}

void TestStudioPlugin::partsGoWithTheStudio()
{
    openGalaxy();
    Component* window = m_universe->component(u"Window"_s);
    const QPointer<QWidget> columns = widgetOf(*m_universe->component(u"Columns"_s));
    const QPointer<QWidget> sources = widgetOf(*m_universe->component(u"Sources"_s));
    const QPointer<QWidget> edition = widgetOf(*m_universe->component(u"Edition"_s));
    QVERIFY(edition != nullptr);

    // What it was given goes, what holds it stays, and none of its parts is left in it.
    QVERIFY(m_universe->remove(*m_universe->component(u"Studio"_s)));
    QVERIFY(edition == nullptr);
    QVERIFY(columns != nullptr && sources != nullptr);
    QCOMPARE(qobject_cast<QTabWidget*>(sources)->count(), 0);
    QCOMPARE(columns->findChildren<QGraphicsView*>().size(), 0);
    for(const QString& part : {u"Edition"_s, u"Project"_s, u"Plugins"_s, u"Properties"_s})
    {
        QVERIFY2(!m_universe->component(part)->unboundMandatoryInterfaces().isEmpty(), qPrintable(part));
    }
    QVERIFY(window->unboundMandatoryInterfaces().isEmpty()); // Its studio is optional

    // Another one, bound to them, has them built again.
    Component* studio = instantiate(u"StudioComponent"_s);
    for(const QString& part : {u"Edition"_s, u"Project"_s, u"Plugins"_s, u"Galaxies"_s})
    {
        QVERIFY(m_universe->component(part)->bindInterface(u"studio"_s, *studio, u"studio"_s));
    }
    QCOMPARE(qobject_cast<QTabWidget*>(sources)->count(), 2);
    QCOMPARE(columns->findChildren<QGraphicsView*>().size(), 1);
}

void TestStudioPlugin::stoppingStopsTheApplicationAssembled()
{
    openGalaxy();
    ignoreSizeHints();
    QVERIFY(m_universe->start());
    QMenu* run = menuOf(*shownWindow(), u"&Run"_s);
    QVERIFY(run != nullptr);
    QAction* start = actionOf(*run, u"&Start"_s);
    QAction* stop = actionOf(*run, u"S&top"_s);
    QVERIFY(start != nullptr && stop != nullptr);
    QVERIFY(start->isEnabled() && !stop->isEnabled());

    start->trigger(); // The application it assembles, empty
    QVERIFY(!start->isEnabled() && stop->isEnabled());
    m_universe->stop();
    QVERIFY(start->isEnabled() && !stop->isEnabled());
}

void TestStudioPlugin::galaxyDirectory_data()
{
    QTest::addColumn<QString>("directory");
    QTest::addColumn<QString>("listed");

    const QString application = QCoreApplication::applicationDirPath();
    QTest::newRow("none: scene/ next to the application") << QString() << application + u"/scene"_s;
    QTest::newRow("relative: from the application") << u"galaxies"_s << application + u"/galaxies"_s;
    QTest::newRow("absolute") << QDir::tempPath() << QDir::tempPath();
}

void TestStudioPlugin::galaxyDirectory()
{
    QFETCH(QString, directory);
    QFETCH(QString, listed);

    openGalaxy();
    Component* galaxies = m_universe->component(u"Galaxies"_s);
    QVERIFY(galaxies->setProperty("directory", directory));
    const QLineEdit* field = galaxyDirectoryIn(*widgetOf(*galaxies));
    QVERIFY(field != nullptr);
    QCOMPARE(field->text(), QDir::toNativeSeparators(listed));
}

void TestStudioPlugin::splitterInTheOrderOfItsBindings()
{
    Component* splitter = instantiate(u"SplitterComponent"_s);
    Component* first = instantiate(u"GroupComponent"_s);
    Component* second = instantiate(u"GroupComponent"_s);
    auto* widget = qobject_cast<QSplitter*>(widgetOf(*splitter));
    QVERIFY(widget != nullptr);
    QCOMPARE(widget->orientation(), Qt::Horizontal);
    QVERIFY(splitter->setProperty("vertical", true));
    QCOMPARE(widget->orientation(), Qt::Vertical);

    QVERIFY(splitter->bindInterface(u"widgets"_s, *second, u"widget"_s));
    QVERIFY(splitter->bindInterface(u"widgets"_s, *first, u"widget"_s));
    QCOMPARE(widget->widget(0), widgetOf(*second));
    QCOMPARE(widget->widget(1), widgetOf(*first));
    // Given back as it was provided, the other one kept.
    splitter->unbindInterface(u"widgets"_s, *second, u"widget"_s);
    QCOMPARE(widget->count(), 1);
    QCOMPARE(widgetOf(*second)->parent(), nullptr);
    QCOMPARE(widget->widget(0), widgetOf(*first));
}

void TestStudioPlugin::splitterSizes()
{
    Component* splitter = instantiate(u"SplitterComponent"_s);
    auto* widget = qobject_cast<QSplitter*>(widgetOf(*splitter));
    for(int count = 0; count < 3; ++count)
    {
        QVERIFY(splitter->bindInterface(u"widgets"_s, *instantiate(u"GroupComponent"_s), u"widget"_s));
    }
    // The one stretched takes what it gains.
    QVERIFY(splitter->setProperty("stretched", 1));
    QCOMPARE(widget->widget(0)->sizePolicy().horizontalStretch(), 0);
    QCOMPARE(widget->widget(1)->sizePolicy().horizontalStretch(), 1);
    widget->resize(600 + 2 * widget->handleWidth(), 100); // The size they make up together
    QVERIFY(splitter->setProperty("sizes", u"100, 300, 200"_s));
    QCOMPARE(widget->sizes(), QList<int>({100, 300, 200}));
}

void TestStudioPlugin::tabsUnderTheTitlesOfTheirPages()
{
    Component* tabs = instantiate(u"TabsComponent"_s);
    Component* first = instantiate(u"GroupComponent"_s);
    Component* second = instantiate(u"GroupComponent"_s);
    widgetOf(*first)->setWindowTitle(u"First"_s);
    widgetOf(*second)->setWindowTitle(u"Second"_s);
    auto* widget = qobject_cast<QTabWidget*>(widgetOf(*tabs));
    QVERIFY(widget != nullptr);
    QVERIFY(widget->documentMode());

    QVERIFY(tabs->bindInterface(u"pages"_s, *second, u"widget"_s));
    QVERIFY(tabs->bindInterface(u"pages"_s, *first, u"widget"_s));
    QCOMPARE(tabTitles(*widget), QStringList({u"Second"_s, u"First"_s}));
    tabs->unbindInterface(u"pages"_s, *second, u"widget"_s);
    QCOMPARE(tabTitles(*widget), QStringList{u"First"_s});
    QCOMPARE(widgetOf(*second)->parent(), nullptr);
}

void TestStudioPlugin::groupGivesItsContentBack()
{
    Component* group = instantiate(u"GroupComponent"_s);
    Component* content = instantiate(u"GroupComponent"_s);
    QCOMPARE(group->unboundMandatoryInterfaces(), QStringList{u"content"_s});
    QVERIFY(group->setProperty("title", u"Application"_s));
    QCOMPARE(qobject_cast<QGroupBox*>(widgetOf(*group))->title(), u"Application"_s);

    QVERIFY(group->bindInterface(u"content"_s, *content, u"widget"_s));
    QCOMPARE(widgetOf(*content)->parentWidget(), widgetOf(*group));
    // Neither deletes what the other one owns as it goes.
    const QPointer<QWidget> given = widgetOf(*content);
    QVERIFY(m_universe->remove(*group));
    QVERIFY(given != nullptr);
    QCOMPARE(given->parent(), nullptr);
}

void TestStudioPlugin::galaxyRoundTrip()
{
    openGalaxy();
    const QJsonObject saved = m_universe->save();
    const QJsonObject file = galaxyFile();
    QCOMPARE(saved.value(u"description"), file.value(u"description"));
    // Its file holds what Core writes of it, but where NebulaStudio shows it.
    QCOMPARE(componentsOf(saved.value(u"components").toArray()), componentsOf(file.value(u"components").toArray()));
    QCOMPARE(bindingsOf(saved.value(u"bindings").toArray()), bindingsOf(file.value(u"bindings").toArray()));
}

QTEST_MAIN(TestStudioPlugin)
#include "TestStudioPlugin.moc"
