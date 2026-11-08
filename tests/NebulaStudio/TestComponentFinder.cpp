// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QAbstractItemView>
#include <QCompleter>
#include <QFile>
#include <QFontDatabase>
#include <QGraphicsView>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QVector2D>
#include <map>
#include <memory>

#include "ComponentFinder.h"
#include "ComponentItem.h"
#include "EditionScene.h"
#include "Nebula/Application/Universe.h"
#include "Nebula/Component/ComponentFactory.h"
#include "Nebula/Galaxy/Galaxy.h"
#include "TestComponents.h"

namespace
{

// Where the instances are shown: two in the top left corner, which the view shows, and the third one far from it,
// where the view has to move to show it in its middle.
constexpr QPointF SourceAt(40, 40);
constexpr QPointF SinkAt(460, 120);
constexpr QPointF FeederAt(2400, 1800);
constexpr QPointF PairAt(40, 280);

// The name of the instance of Source renamed, to be found by the name of its component only.
const QString Feeder = QStringLiteral("Feeder");

// A galaxy of a Sink, Sink1.
const QByteArray Lone = R"({"version": 1, "components": [
    {"name": "Sink1", "type": "Test.Sink", "x": 40, "y": 40}]})";
// A galaxy of a Source, Source1, and of Lone, Lone1, far from it.
const QByteArray Pair = R"({"version": 1, "components": [
    {"name": "Source1", "type": "Test.Source", "x": 40, "y": 40},
    {"name": "Lone1", "type": "Nebula.Galaxy", "galaxy": "Lone.nbp", "x": 2400, "y": 1800}]})";

// Writes content to the file named fileName in directory.
bool write(const QTemporaryDir& directory, const QString& fileName, const QByteArray& content)
{
    QFile file(directory.filePath(fileName));
    return file.open(QIODevice::WriteOnly) && file.write(content) == content.size();
}

// Has project, as a project of NebulaStudio, show the instance named name at at.
void place(QJsonObject& project, const QString& name, QPointF at)
{
    QJsonArray components = project.value(u"components").toArray();
    for(QJsonValueRef value : components)
    {
        QJsonObject component = value.toObject();
        if(component.value(u"name").toString() == name)
        {
            component.insert(u"x", at.x());
            component.insert(u"y", at.y());
            value = component;
        }
    }
    project.insert(u"components", components);
}

// The names of the instances selected in scene.
QStringList selectedNames(const QGraphicsScene& scene)
{
    QStringList names;
    const QList<QGraphicsItem*> selected = scene.selectedItems();
    for(QGraphicsItem* item : selected)
    {
        if(const auto* component = qobject_cast<ComponentItem*>(item->toGraphicsObject()))
        {
            names.append(component->component().objectName());
        }
    }
    return names;
}

// The names of the galaxies from application down to the one scene, an EditionScene, shows; none for the application.
QStringList pathTo(const Nebula::Universe& application, const QGraphicsScene& scene)
{
    QStringList path;
    for(const Nebula::Galaxy* galaxy = static_cast<const EditionScene&>(scene).galaxy(); galaxy != nullptr;
        galaxy = application.galaxyOf(*galaxy))
    {
        path.prepend(galaxy->objectName());
    }
    return path;
}

// What the completer of finder offers, in its order: the name of each instance, then the name of its component.
QStringList offered(ComponentFinder& finder)
{
    QStringList rows;
    const QAbstractItemModel* completions = finder.completer()->completionModel();
    for(int row = 0; row < completions->rowCount(); ++row)
    {
        rows.append(QStringLiteral("%1: %2").arg(completions->index(row, 0).data().toString(),
                                                 completions->index(row, 1).data().toString()));
    }
    return rows;
}

// What the offscreen platform warns about once the popup of a completer shows, which it neither raises nor gives the
// keyboard to, as for a menu: expected, as it tells nothing of the field. Once each time the popup shows, from hidden.
void ignorePopupWarnings()
{
    QTest::ignoreMessage(QtWarningMsg, "This plugin does not support raise()");
    QTest::ignoreMessage(QtWarningMsg, "This plugin does not support grabbing the keyboard");
}

} // namespace

// The field that finds the instances of the edition zone, ComponentFinder: by their names and the names of their
// components, in the application and in its galaxies, one after the other, the galaxy of the one found shown.
class TestComponentFinder : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void cleanup();

    void find_data();
    void find();
    void centersTheView();
    void enterKey();
    void completions_data();
    void completions();
    void completionsFollowNames();
    void chooseACompletion();
    void inGalaxies_data();
    void inGalaxies();
    void galaxyNotShown();
    void galaxyCompletions_data();
    void galaxyCompletions();

  private:
    // Places Pair1, an instance of Pair, in the application, at PairAt: Pair1.Source1, Pair1.Lone1 and
    // Pair1.Lone1.Sink1 in it. The view shows the scene of the galaxy at the end of a path the finder asks for.
    void placePair();
    // Has the view show the galaxy at the end of path, the names of the galaxies from the application down to it, in a
    // scene of its own, as the edition zone does: the application when path is empty.
    void show(const QStringList& path);

    // Source1, Sink1 and Feeder, an instance of Source, shown at SourceAt, SinkAt and FeederAt.
    std::unique_ptr<Nebula::Universe> m_application;
    std::unique_ptr<EditionScene> m_scene;
    std::map<QString, std::unique_ptr<EditionScene>> m_galaxyScenes; // By the names of their path, joined by dots
    std::unique_ptr<QGraphicsView> m_view;
    std::unique_ptr<ComponentFinder> m_finder;  // Of m_application, in m_view
    std::unique_ptr<QTemporaryDir> m_directory; // Of the galaxy files, once placePair() wrote them
};

void TestComponentFinder::initTestCase()
{
    registerTestComponents();
    // Once, before any test fails on a warning: the offscreen platform warns that it has no fonts the first time a text
    // is shown, and the names of the instances are texts.
    QFontDatabase::families();
}

void TestComponentFinder::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
    m_application = std::make_unique<Nebula::Universe>();
    Nebula::Component* source = m_application->instantiate(testComponentId(QStringLiteral("Source")));
    Nebula::Component* sink = m_application->instantiate(testComponentId(QStringLiteral("Sink")));
    Nebula::Component* feeder = m_application->instantiate(testComponentId(QStringLiteral("Source")));
    QVERIFY(source != nullptr && sink != nullptr && feeder != nullptr);
    QVERIFY(m_application->rename(*feeder, Feeder));

    QJsonObject project = m_application->save();
    place(project, source->objectName(), SourceAt);
    place(project, sink->objectName(), SinkAt);
    place(project, Feeder, FeederAt);
    m_scene = std::make_unique<EditionScene>(*m_application, Nebula::ComponentFactory::instance());
    QCOMPARE(m_scene->load(project), QStringList());

    // As the edition zone shows them.
    m_view = std::make_unique<QGraphicsView>(m_scene.get());
    m_view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_view->resize(780, 560);
    m_view->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_view.get()));
    m_finder = std::make_unique<ComponentFinder>(*m_application, *m_view);
    m_finder->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_finder.get()));
}

void TestComponentFinder::cleanup()
{
    m_finder.reset();
    m_view.reset();
    m_galaxyScenes.clear(); // Before the galaxies they show
    m_scene.reset();
    m_application.reset();
    m_directory.reset();
}

void TestComponentFinder::placePair()
{
    m_directory = std::make_unique<QTemporaryDir>();
    QVERIFY(write(*m_directory, QStringLiteral("Lone.nbp"), Lone));
    QVERIFY(write(*m_directory, QStringLiteral("Pair.nbp"), Pair));
    QVERIFY(m_application->instantiateGalaxy(m_directory->filePath(QStringLiteral("Pair.nbp")),
                                             QStringLiteral("Pair1")) != nullptr);
    QJsonObject project = m_scene->save();
    place(project, QStringLiteral("Pair1"), PairAt);
    QCOMPARE(m_scene->load(project), QStringList());
    connect(m_finder.get(), &ComponentFinder::galaxyRequested, this, &TestComponentFinder::show);
}

void TestComponentFinder::show(const QStringList& path)
{
    Nebula::Galaxy* galaxy = nullptr;
    for(const QString& name : path)
    {
        galaxy =
            qobject_cast<Nebula::Galaxy*>(galaxy != nullptr ? galaxy->component(name) : m_application->component(name));
    }
    std::unique_ptr<EditionScene>& scene = m_galaxyScenes[path.join(u'.')];
    if(galaxy != nullptr && scene == nullptr)
    {
        scene = std::make_unique<EditionScene>(*m_application, Nebula::ComponentFactory::instance(), *galaxy);
    }
    m_view->setScene(path.isEmpty() ? m_scene.get() : scene.get());
}

void TestComponentFinder::find_data()
{
    QTest::addColumn<QString>("selected"); // Before it looks; none when empty
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("times");     // It looks for text
    QTest::addColumn<QString>("found"); // The last time; none when empty
    QTest::addColumn<QString>("told");  // The last time; nothing when empty

    QTest::newRow("its name") << QString() << "Sink1" << 1 << "Sink1" << "Sink1 found";
    QTest::newRow("whatever its case") << QString() << "sink1" << 1 << "Sink1" << "Sink1 found";
    QTest::newRow("part of its name") << QString() << "ink" << 1 << "Sink1" << "Sink1 found";
    // Feeder, then Source1, in the order of their names: Feeder is an instance of Source.
    const QString first = QStringLiteral("Feeder found, 1 of 2: Enter shows the next one");
    const QString second = QStringLiteral("Source1 found, 2 of 2: Enter shows the next one");
    QTest::newRow("its component") << QString() << "source" << 1 << Feeder << first;
    QTest::newRow("the next one") << QString() << "source" << 2 << "Source1" << second;
    QTest::newRow("the first one after the last") << QString() << "source" << 3 << Feeder << first;
    QTest::newRow("after the one selected") << Feeder << "source" << 1 << "Source1" << second;
    QTest::newRow("another one selected") << "Sink1" << "source" << 1 << Feeder << first;
    // Not Source10, which would hold it, were there one: the one named so, each time.
    QTest::newRow("named so") << QString() << "source1" << 2 << "Source1" << "Source1 found";
    // The selection stays.
    QTest::newRow("nothing found")
        << "Sink1" << "drain" << 1 << QString()
        << "No instance of the application, nor of its galaxies, has \"drain\" in its name or in the name of its "
           "component";
    QTest::newRow("nothing typed") << "Sink1" << QString() << 1 << QString() << QString();
}

void TestComponentFinder::find()
{
    QFETCH(QString, selected);
    QFETCH(QString, text);
    QFETCH(int, times);
    QFETCH(QString, found);
    QFETCH(QString, told);

    if(!selected.isEmpty())
    {
        QVERIFY(m_scene->select(*m_application->component(selected)) != nullptr);
    }
    const QSignalSpy spy(m_finder.get(), &ComponentFinder::told);
    const ComponentItem* item = nullptr;
    for(int time = 0; time < times; ++time)
    {
        item = m_finder->find(text);
    }

    QCOMPARE(item != nullptr ? item->component().objectName() : QString(), found);
    QCOMPARE(selectedNames(*m_scene), found.isEmpty() ? QStringList(selected) : QStringList(found));
    QCOMPARE(spy.isEmpty() ? QString() : spy.constLast().constFirst().toString(), told);
    QCOMPARE(spy.size(), told.isEmpty() ? 0 : times);
}

void TestComponentFinder::centersTheView()
{
    const ComponentItem* item = m_finder->find(QStringLiteral("feed"));
    QVERIFY(item != nullptr);

    // In the middle of the view, to the rounding of the scroll bars.
    const QPointF middle = m_view->mapToScene(m_view->viewport()->rect().center());
    QCOMPARE_LE(QVector2D(middle - item->sceneBoundingRect().center()).length(), 1.5F);
}

void TestComponentFinder::enterKey()
{
    m_finder->setText(QStringLiteral("source"));
    QTest::keyClick(m_finder.get(), Qt::Key_Return);
    QCOMPARE(selectedNames(*m_scene), QStringList(Feeder));
    QTest::keyClick(m_finder.get(), Qt::Key_Return);
    QCOMPARE(selectedNames(*m_scene), QStringList(QStringLiteral("Source1")));
}

void TestComponentFinder::completions_data()
{
    QTest::addColumn<QString>("typed");
    QTest::addColumn<QStringList>("offered");

    // In the order of their names, each by the name of its component.
    QTest::newRow("all of them") << "s" << QStringList{"Feeder: Source", "Sink1: Sink", "Source1: Source"};
    QTest::newRow("by its component") << "sou" << QStringList{"Feeder: Source", "Source1: Source"};
    QTest::newRow("by its name") << "fee" << QStringList{"Feeder: Source"};
    QTest::newRow("whatever its case") << "SINK" << QStringList{"Sink1: Sink"};
    // Not across the name of an instance and the name of its component.
    QTest::newRow("not across both") << "1si" << QStringList();
    QTest::newRow("none") << "drain" << QStringList();
}

void TestComponentFinder::completions()
{
    QFETCH(QString, typed);
    QFETCH(QStringList, offered);

    ignorePopupWarnings(); // Even when nothing is offered in the end: something is, once its first letter is typed
    QTest::keyClicks(m_finder.get(), typed);
    QCOMPARE(::offered(*m_finder), offered);
    QCOMPARE(m_finder->completer()->popup()->isVisible(), !offered.isEmpty());
}

void TestComponentFinder::completionsFollowNames()
{
    // Named so once the field was made: what is typed next is looked for among the names they have then.
    QVERIFY(m_application->rename(*m_application->component(QStringLiteral("Sink1")), QStringLiteral("Drain")));
    ignorePopupWarnings();
    QTest::keyClicks(m_finder.get(), QStringLiteral("dra"));
    QCOMPARE(offered(*m_finder), QStringList(QStringLiteral("Drain: Sink")));
}

void TestComponentFinder::chooseACompletion()
{
    m_finder->activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(m_finder.get()));
    m_finder->setFocus();
    QTRY_VERIFY(m_finder->hasFocus());
    ignorePopupWarnings();
    QTest::keyClicks(m_finder.get(), QStringLiteral("sou"));
    QAbstractItemView* popup = m_finder->completer()->popup();
    QVERIFY(popup->isVisible());

    // The second one offered, and it alone: Enter does not go on to the next one then.
    QTest::keyClick(popup, Qt::Key_Down);
    QTest::keyClick(popup, Qt::Key_Down);
    QTest::keyClick(popup, Qt::Key_Return);
    QCOMPARE(m_finder->text(), QStringLiteral("Source1"));
    QCOMPARE(selectedNames(*m_scene), QStringList(QStringLiteral("Source1")));
}

void TestComponentFinder::inGalaxies_data()
{
    QTest::addColumn<QStringList>("shown"); // The path to the galaxy the view shows before it looks
    QTest::addColumn<QString>("selected");  // There, before it looks; none when empty
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("times");     // It looks for text
    QTest::addColumn<QString>("found"); // The last time, by its name after those of its galaxies
    QTest::addColumn<int>("requests");  // The galaxies it asked the view to show, all those times
    QTest::addColumn<QString>("told");  // The last time

    const QStringList application;
    const QStringList pair{QStringLiteral("Pair1")};
    QTest::newRow("in a galaxy") << application << QString() << "pair1.source1" << 1 << "Pair1.Source1" << 1
                                 << "Pair1.Source1 found";
    QTest::newRow("two galaxies down") << application << QString() << "pair1.lone1.sink1" << 1 << "Pair1.Lone1.Sink1"
                                       << 1 << "Pair1.Lone1.Sink1 found";
    QTest::newRow("by its own name") << application << QString() << "lone1" << 1 << "Pair1.Lone1" << 1
                                     << "Pair1.Lone1 found";
    // Not Pair1.Lone1.Sink1, whose own name it is too.
    QTest::newRow("the one named so") << application << QString() << "sink1" << 1 << "Sink1" << 0 << "Sink1 found";
    // Placed in the application, where it is selected: not opened.
    QTest::newRow("a galaxy") << application << QString() << "pair" << 1 << "Pair1" << 0
                              << "Pair1 found, 1 of 4: Enter shows the next one";
    // Feeder, Source1, then Pair1.Source1: the application first.
    QTest::newRow("the application first")
        << application << QString() << "source" << 1 << Feeder << 0 << "Feeder found, 1 of 3: Enter shows the next one";
    QTest::newRow("then its galaxies") << application << QString() << "source" << 3 << "Pair1.Source1" << 1
                                       << "Pair1.Source1 found, 3 of 3: Enter shows the next one";
    QTest::newRow("the application again")
        << application << QString() << "source" << 4 << Feeder << 2 << "Feeder found, 1 of 3: Enter shows the next one";
    QTest::newRow("from the galaxy shown") << pair << QString() << "source" << 1 << "Pair1.Source1" << 0
                                           << "Pair1.Source1 found, 3 of 3: Enter shows the next one";
    // Sink1 first, of the application.
    QTest::newRow("in a galaxy it holds") << pair << QString() << "sink" << 1 << "Pair1.Lone1.Sink1" << 1
                                          << "Pair1.Lone1.Sink1 found, 2 of 2: Enter shows the next one";
    QTest::newRow("after the one selected there")
        << pair << "Source1" << "source" << 1 << Feeder << 1 << "Feeder found, 1 of 3: Enter shows the next one";
}

void TestComponentFinder::inGalaxies()
{
    QFETCH(QStringList, shown);
    QFETCH(QString, selected);
    QFETCH(QString, text);
    QFETCH(int, times);
    QFETCH(QString, found);
    QFETCH(int, requests);
    QFETCH(QString, told);

    placePair();
    show(shown);
    auto* scene = static_cast<EditionScene*>(m_view->scene());
    if(!selected.isEmpty())
    {
        const Nebula::Galaxy* galaxy = scene->galaxy();
        QVERIFY(scene->select(galaxy != nullptr ? *galaxy->component(selected) : *m_application->component(selected)) !=
                nullptr);
    }
    const QSignalSpy requested(m_finder.get(), &ComponentFinder::galaxyRequested);
    const QSignalSpy spy(m_finder.get(), &ComponentFinder::told);
    const ComponentItem* item = nullptr;
    for(int time = 0; time < times; ++time)
    {
        item = m_finder->find(text);
    }

    // Selected alone in the galaxy it is in, which the view shows.
    QStringList path = found.split(u'.');
    const QString name = path.takeLast();
    QVERIFY(item != nullptr);
    QCOMPARE(item->scene(), m_view->scene());
    QCOMPARE(pathTo(*m_application, *m_view->scene()), path);
    QCOMPARE(selectedNames(*m_view->scene()), QStringList(name));
    QCOMPARE(requested.size(), requests);
    QCOMPARE(spy.constLast().constFirst().toString(), told);
}

void TestComponentFinder::galaxyNotShown()
{
    placePair();
    disconnect(m_finder.get(), &ComponentFinder::galaxyRequested, this, &TestComponentFinder::show);
    QVERIFY(m_scene->select(*m_application->component(QStringLiteral("Sink1"))) != nullptr);
    const QSignalSpy requested(m_finder.get(), &ComponentFinder::galaxyRequested);
    const QSignalSpy told(m_finder.get(), &ComponentFinder::told);

    // Asked for, but not shown: nothing is, the selection stays and nothing is told.
    QCOMPARE(m_finder->find(QStringLiteral("pair1.source1")), nullptr);
    QCOMPARE(requested.size(), 1);
    QCOMPARE(requested.constFirst().constFirst().toStringList(), QStringList(QStringLiteral("Pair1")));
    QCOMPARE(m_view->scene(), m_scene.get());
    QCOMPARE(selectedNames(*m_scene), QStringList(QStringLiteral("Sink1")));
    QVERIFY(told.isEmpty());
}

void TestComponentFinder::galaxyCompletions_data()
{
    QTest::addColumn<QString>("typed");
    QTest::addColumn<QStringList>("offered");

    // Those of the application first, then those one galaxy down, and so on, each named after its galaxies.
    QTest::newRow("at any depth") << "sink" << QStringList{"Sink1: Sink", "Pair1.Lone1.Sink1: Sink"};
    QTest::newRow("by the name of its galaxy")
        << "pair1." << QStringList{"Pair1.Lone1: Lone", "Pair1.Source1: Source", "Pair1.Lone1.Sink1: Sink"};
    // A galaxy by the name of its file.
    QTest::newRow("a galaxy") << "pai"
                              << QStringList{"Pair1: Pair", "Pair1.Lone1: Lone", "Pair1.Source1: Source",
                                             "Pair1.Lone1.Sink1: Sink"};
}

void TestComponentFinder::galaxyCompletions()
{
    QFETCH(QString, typed);
    QFETCH(QStringList, offered);

    placePair();
    ignorePopupWarnings();
    QTest::keyClicks(m_finder.get(), typed);
    QCOMPARE(::offered(*m_finder), offered);
}

QTEST_MAIN(TestComponentFinder)

#include "TestComponentFinder.moc"
