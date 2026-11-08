// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QFile>
#include <QFontDatabase>
#include <QGraphicsItem>
#include <QGraphicsScene>
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
#include <QMimeData>
#include <QPalette>
#include <QPointer>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTreeView>
#include <QWindow>
#include <algorithm>
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

// The list in widget whose rows are dragged; nullptr when it has none.
QTreeView* draggableListIn(const QWidget& widget)
{
    for(QTreeView* list : widget.findChildren<QTreeView*>())
    {
        if(list->dragEnabled())
        {
            return list;
        }
    }
    return nullptr;
}

// Has the window of widget tell that what is dragged moved to at, in the window, as the window system would.
void dragTo(const QWidget& widget, QPoint at)
{
    const QMimeData data;
    QDragMoveEvent event(at, Qt::CopyAction, &data, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(widget.windowHandle(), &event);
}

// Has the window of widget tell that what is dragged left it.
void leave(const QWidget& widget)
{
    QDragLeaveEvent event;
    QApplication::sendEvent(widget.windowHandle(), &event);
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

// The instances scene shows: its graphics objects that are in no other item.
qsizetype instancesIn(const QGraphicsScene& scene)
{
    const QList<QGraphicsItem*> items = scene.items();
    return std::ranges::count_if(items, [](const QGraphicsItem* item)
                                 { return item->parentItem() == nullptr && item->toGraphicsObject() != nullptr; });
}

// Puts in the clipboard a group of StudioPlugin, as NebulaStudio copies an instance: one of its components, which the
// application that runs the galaxy registered.
void copyGroup()
{
    auto* data = new QMimeData;
    data->setData(u"application/x-nebula-instances"_s,
                  R"({"components": [{"name": "Group1", "type": "FabienDG.Nebula.StudioPlugin.GroupComponent",
                                       "x": 1000, "y": 800}]})");
    QGuiApplication::clipboard()->setMimeData(data);
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
    void editMenuFollowsTheHistory();
    void editMenuCopiesAndPastes();
    void partsInTheWindow();
    void listsAsideWhileDragging();
    void partsGoWithTheStudio();
    void stoppingStopsTheApplicationAssembled();
    void projectFileOpensAsItStarts();
    void themeChosenInTheViewMenu();
    void overviewShownFromTheViewMenu();
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
    // Starts the galaxy of NebulaStudio, then returns the View menu of its window; nullptr when it has none.
    QMenu* startedViewMenu();
    // Starts the galaxy of NebulaStudio, then returns the Theme menu of its window; nullptr when it has none.
    QMenu* startedThemeMenu();
    // The overview of the edition zone, as its component provides it; nullptr when there is none.
    QWidget* overview() const;

    std::unique_ptr<Universe> m_universe;
    const QTemporaryDir m_settings; // Of the user, those of NebulaStudio among them: not the ones of who runs the tests
};

void TestStudioPlugin::initTestCase()
{
    QFontDatabase::families(); // The platform of the tests warns, once, that it has no fonts: before failOnWarning()
    QVERIFY(m_settings.isValid());
    // The directory of the user settings in any format, that of the preferences of NebulaStudio included.
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
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

QMenu* TestStudioPlugin::startedViewMenu()
{
    openGalaxy();
    ignoreSizeHints();
    if(!m_universe->start() || shownWindow() == nullptr)
    {
        return nullptr;
    }
    return menuOf(*shownWindow(), u"&View"_s);
}

QMenu* TestStudioPlugin::startedThemeMenu()
{
    const QMenu* view = startedViewMenu();
    const QAction* theme = view != nullptr ? actionOf(*view, u"&Theme"_s) : nullptr;
    return theme != nullptr ? theme->menu() : nullptr;
}

QWidget* TestStudioPlugin::overview() const
{
    const Component* overview = m_universe->component(u"Overview"_s);
    return overview != nullptr ? widgetOf(*overview) : nullptr;
}

void TestStudioPlugin::galaxyOpens()
{
    openGalaxy();
    QCOMPARE(m_universe->components().size(), 15);
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
    QCOMPARE(menuTitles(*window), QStringList({u"&File"_s, u"&Edit"_s, u"&View"_s, u"&Run"_s}));
    QCOMPARE(window->statusBar()->findChildren<QLabel*>().size(), 2); // Where the plugins are, and the zoom

    m_universe->stop();
    QVERIFY(!window->isVisible());
}

void TestStudioPlugin::editMenuFollowsTheHistory()
{
    openGalaxy();
    Component* history = m_universe->component(u"History"_s);
    QVERIFY(history != nullptr);
    QCOMPARE(history->componentId(), u"FabienDG.Nebula.StandardPlugin.HistoryComponent"_s);
    ignoreSizeHints();
    QVERIFY(m_universe->start());
    QMenu* edit = menuOf(*shownWindow(), u"&Edit"_s);
    QVERIFY(edit != nullptr);
    QAction* undo = actionOf(*edit, u"&Undo"_s);
    QAction* redo = actionOf(*edit, u"&Redo"_s);
    QVERIFY(undo != nullptr && redo != nullptr);
    QCOMPARE(undo->shortcut(), QKeySequence(QKeySequence::Undo));
    QCOMPARE(redo->shortcut(), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z));
    QVERIFY(undo->isEnabled());
    QVERIFY(redo->isEnabled());
    undo->trigger(); // Nothing to undo: nothing done, nothing told
    redo->trigger();

    // As the history does them, once the menu shows.
    history->setProperty("undoActive", false);
    emit edit->aboutToShow();
    QVERIFY(!undo->isEnabled());
    QVERIFY(redo->isEnabled());
    history->setProperty("redoActive", false);
    emit edit->aboutToShow();
    QVERIFY(!redo->isEnabled());
    m_universe->stop();
}

void TestStudioPlugin::editMenuCopiesAndPastes()
{
    QGuiApplication::clipboard()->clear();
    openGalaxy();
    ignoreSizeHints();
    QVERIFY(m_universe->start());
    QMainWindow* window = shownWindow();
    QVERIFY(window != nullptr);
    const QMenu* edit = menuOf(*window, u"&Edit"_s);
    QVERIFY(edit != nullptr);
    QAction* cut = actionOf(*edit, u"Cu&t"_s);
    QAction* copy = actionOf(*edit, u"&Copy"_s);
    QAction* paste = actionOf(*edit, u"&Paste"_s);
    QAction* duplicate = actionOf(*edit, u"&Duplicate"_s);
    QVERIFY(cut != nullptr && copy != nullptr && paste != nullptr && duplicate != nullptr);
    QCOMPARE(cut->shortcut(), QKeySequence(QKeySequence::Cut));
    QCOMPARE(copy->shortcut(), QKeySequence(QKeySequence::Copy));
    QCOMPARE(paste->shortcut(), QKeySequence(QKeySequence::Paste));
    QCOMPARE(duplicate->shortcut(), QKeySequence(Qt::CTRL | Qt::Key_D));
    // Nothing selected, nothing copied.
    for(const QAction* action : {cut, copy, paste, duplicate})
    {
        QVERIFY2(!action->isEnabled(), qPrintable(action->text()));
    }

    // Instances copied, by another window of NebulaStudio say: pasted in the edition zone, selected.
    copyGroup();
    QVERIFY(paste->isEnabled());
    const QGraphicsScene& scene = *widgetOf(*m_universe->component(u"Edition"_s))->findChild<QGraphicsView*>()->scene();
    paste->trigger();
    QCOMPARE(instancesIn(scene), 1);
    QVERIFY(cut->isEnabled() && copy->isEnabled() && duplicate->isEnabled());

    // Not while a field takes the keys: its own Ctrl+V pastes text, and Ctrl+D is none of its own.
    window->activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(window));
    auto* field = widgetOf(*m_universe->component(u"Edition"_s))->findChild<QLineEdit*>();
    QVERIFY(field != nullptr);
    field->setFocus();
    QCOMPARE(QApplication::focusWidget(), field);
    paste->trigger();
    duplicate->trigger();
    QCOMPARE(instancesIn(scene), 1);
    field->clearFocus();
    duplicate->trigger();
    QCOMPARE(instancesIn(scene), 2);
    m_universe->stop();
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
    QCOMPARE(columns->count(), 2);
    // The edition zone above the problems.
    const auto* edition = qobject_cast<QSplitter*>(columns->widget(0));
    QVERIFY(edition != nullptr);
    QCOMPARE(edition->orientation(), Qt::Vertical);
    QCOMPARE(edition->count(), 2);
    QCOMPARE(edition->widget(0)->findChildren<QGraphicsView*>().size(), 1);
    QCOMPARE(window->findChildren<QGraphicsView*>().size(), 2); // With its overview
    QVERIFY(window->findChildren<QGroupBox*>().isEmpty());
    // Most of the window, and the room it gains.
    QTRY_VERIFY(columns->sizes().at(0) > columns->width() / 2);
    QCOMPARE(edition->sizePolicy().horizontalStretch(), 1);
    // On its right, the lists in tabs, one shown at a time, the components first, above the property editor, then the
    // overview of the scene the edition zone shows.
    const auto* side = qobject_cast<QSplitter*>(columns->widget(1));
    QVERIFY(side != nullptr);
    QCOMPARE(side->orientation(), Qt::Vertical);
    QCOMPARE(side->count(), 3);
    const auto* sources = qobject_cast<QTabWidget*>(side->widget(0));
    QVERIFY(sources != nullptr);
    QCOMPARE(tabTitles(*sources), QStringList({u"Components"_s, u"Galaxies"_s, u"Plugins"_s}));
    QCOMPARE(sources->currentIndex(), 0);
    QCOMPARE(side->widget(1), widgetOf(*m_universe->component(u"Properties"_s)));
    const auto* overview = qobject_cast<QGraphicsView*>(side->widget(2));
    QVERIFY(overview != nullptr && overview->isVisible());
    QCOMPARE(overview->scene(), edition->widget(0)->findChild<QGraphicsView*>()->scene());
}

void TestStudioPlugin::listsAsideWhileDragging()
{
    openGalaxy();
    // The side of the lists and of the property editor, not the lists alone.
    QVERIFY(m_universe->component(u"SideColumn"_s)->property("hiddenWhileDragging").toBool());
    QVERIFY(!m_universe->component(u"Sources"_s)->property("hiddenWhileDragging").toBool());
    ignoreSizeHints();
    QVERIFY(m_universe->start());
    QMainWindow* window = shownWindow();
    QVERIFY(window != nullptr);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QWidget* side = widgetOf(*m_universe->component(u"SideColumn"_s));
    const auto* edition = widgetOf(*m_universe->component(u"Edition"_s))->findChild<QGraphicsView*>();
    QTRY_VERIFY(side->isVisible() && edition->width() > 0);
    const int width = edition->width();
    // The list of the components, whose rows are dragged into the edition zone.
    QTreeView* list = draggableListIn(*widgetOf(*m_universe->component(u"Components"_s)));
    QVERIFY(list != nullptr);

    QVERIFY(QMetaObject::invokeMethod(list, "dragStarted"));
    ignoreSizeHints(); // The window can be smaller without it
    dragTo(*window, edition->mapTo(window, edition->rect().center()));
    QVERIFY(side->isHidden());
    QTRY_VERIFY(edition->width() > width); // Taking its room
    ignoreSizeHints();
    QVERIFY(QMetaObject::invokeMethod(list, "dragFinished"));
    QVERIFY(side->isVisible());
    QTRY_COMPARE(edition->width(), width);
    leave(*window); // What it was dragged over forgets it
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
    QCOMPARE(columns->findChildren<QGraphicsView*>().size(), 2); // The edition zone, and its overview again
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

void TestStudioPlugin::projectFileOpensAsItStarts()
{
    const QTemporaryDir directory;
    const QString project = directory.filePath(u"Clock.nbp"_s);
    QFile file(project);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QJsonDocument(QJsonObject{{u"version"_s, 1}}).toJson());
    file.close();

    openGalaxy();
    QVERIFY(m_universe->component(u"Project"_s)->setProperty("file", project));
    ignoreSizeHints();
    QVERIFY(m_universe->start());
    // Once the window shows, which tells its file.
    QMainWindow* window = shownWindow();
    QVERIFY(window != nullptr);
    QTRY_COMPARE(window->windowTitle(), u"Clock.nbp - NebulaStudio"_s);
}

// The color scheme chosen in View > Theme gives the application its palette at once, and the preferences of the user
// of NebulaStudio keep it for its next launch, in their JSON file.
void TestStudioPlugin::themeChosenInTheViewMenu()
{
    QMenu* theme = startedThemeMenu();
    QVERIFY(theme != nullptr);
    QCOMPARE(theme->actions().size(), 3);
    QVERIFY(actionOf(*theme, u"&System"_s)->isChecked()); // Until another one is chosen
    actionOf(*theme, u"&Light"_s)->trigger();
    QVERIFY(QApplication::palette().color(QPalette::Window).lightness() > 128);
    actionOf(*theme, u"&Dark"_s)->trigger();
    QVERIFY(QApplication::palette().color(QPalette::Window).lightness() < 128);
    QVERIFY(!actionOf(*theme, u"&Light"_s)->isChecked()); // One at a time

    cleanup(); // Its preferences written as it goes
    QFile file(m_settings.filePath(u"FabienDG/NebulaStudio.json"_s));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(QJsonDocument::fromJson(file.readAll()).object().value(u"appearance"_s),
             QJsonObject({{u"colorScheme"_s, u"dark"_s}}));

    init(); // Its next launch
    theme = startedThemeMenu();
    QVERIFY(theme != nullptr);
    QVERIFY(actionOf(*theme, u"&Dark"_s)->isChecked());
    actionOf(*theme, u"&System"_s)->trigger(); // Of the system again, for the tests after this one
}

// Overview, at the end of the View menu, shows or hides the overview of the edition zone, under the property editor,
// what is above it taking its room; the preferences of the user of NebulaStudio keep it for its next launch.
void TestStudioPlugin::overviewShownFromTheViewMenu()
{
    QMenu* view = startedViewMenu();
    QVERIFY(view != nullptr);
    QAction* entry = view->actions().constLast();
    QCOMPARE(entry->text(), u"O&verview"_s);
    QVERIFY(view->actions().at(view->actions().size() - 2)->isSeparator());
    QVERIFY(entry->isChecked()); // Until it is hidden
    QVERIFY(overview()->isVisible());
    const QWidget* sources = widgetOf(*m_universe->component(u"Sources"_s));
    int height = sources->height();
    ignoreSizeHints(); // The window can be smaller without it
    entry->trigger();
    QVERIFY(overview()->isHidden());
    QTRY_VERIFY(sources->height() > height);

    cleanup(); // Its preferences written as it goes
    QFile file(m_settings.filePath(u"FabienDG/NebulaStudio.json"_s));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(QJsonDocument::fromJson(file.readAll()).object().value(u"edition"_s).toObject().value(u"overview"_s),
             QJsonValue(false));

    init(); // Its next launch
    view = startedViewMenu();
    QVERIFY(view != nullptr);
    entry = view->actions().constLast();
    QVERIFY(!entry->isChecked());
    QVERIFY(overview()->isHidden());
    sources = widgetOf(*m_universe->component(u"Sources"_s));
    QTRY_VERIFY(sources->height() > 0);
    height = sources->height();
    ignoreSizeHints();
    entry->trigger(); // Shown again, for the tests after this one
    QVERIFY(overview()->isVisible());
    QTRY_VERIFY(sources->height() < height);
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
