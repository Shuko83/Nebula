// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QFontDatabase>
#include <QGraphicsView>
#include <QJsonArray>
#include <QJsonObject>
#include <QMenu>
#include <QMouseEvent>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>
#include <algorithm>
#include <memory>

#include "ComponentItem.h"
#include "EditionScene.h"
#include "InterfaceItem.h"
#include "LinkItem.h"
#include "Nebula/Application/Universe.h"
#include "Nebula/Component/ComponentFactory.h"
#include "TestComponents.h"

namespace
{

// Where the components are shown, in the top left corner of the scene, which the view shows: far enough from each
// other for their link to be clicked between them.
constexpr QPointF SourceAt(40, 40);
constexpr QPointF SinkAt(460, 120);
// What the rubber band is dragged around to select both components, in the scene.
constexpr QPointF AroundFrom(10, 10);
constexpr QPointF AroundTo(760, 400);
// Selects the link in the data of deleteKey(), in place of the name of a component.
const QString Link = QStringLiteral("link");

// The links scene shows.
QList<LinkItem*> links(const QGraphicsScene& scene)
{
    QList<LinkItem*> links;
    const QList<QGraphicsItem*> items = scene.items();
    for(QGraphicsItem* item : items)
    {
        if(auto* link = qgraphicsitem_cast<LinkItem*>(item))
        {
            links.append(link);
        }
    }
    return links;
}

// What scene shows of the component named name; nullptr when it does not show it.
ComponentItem* componentItem(const QGraphicsScene& scene, const QString& name)
{
    const QList<QGraphicsItem*> items = scene.items();
    for(QGraphicsItem* item : items)
    {
        auto* component = qobject_cast<ComponentItem*>(item->toGraphicsObject());
        if(component != nullptr && component->component().objectName() == name)
        {
            return component;
        }
    }
    return nullptr;
}

// The names of the components of application, in their order.
QStringList names(const Nebula::Universe& application)
{
    QStringList names;
    for(const std::unique_ptr<Nebula::Component>& component : application.components())
    {
        names.append(component->objectName());
    }
    return names;
}

// The middle of the curve of link, in the scene.
QPointF middle(const LinkItem& link)
{
    return LinkItem::curve(link.provided().scenePos(), link.consumed().scenePos()).pointAtPercent(0.5);
}

// Sends the viewport of view the mouse event type at at, a position in the scene, as the window system would.
void send(QGraphicsView& view, QEvent::Type type, QPointF at, Qt::MouseButton button, Qt::MouseButtons buttons)
{
    const QPoint position = view.mapFromScene(at);
    QMouseEvent event(type, position, view.viewport()->mapToGlobal(position), button, buttons, Qt::NoModifier);
    QApplication::sendEvent(view.viewport(), &event);
}

// Drags the rubber band of view from from to to, positions in the scene.
void dragRubberBand(QGraphicsView& view, QPointF from, QPointF to)
{
    send(view, QEvent::MouseButtonPress, from, Qt::LeftButton, Qt::LeftButton);
    send(view, QEvent::MouseMove, (from + to) / 2, Qt::NoButton, Qt::LeftButton);
    send(view, QEvent::MouseMove, to, Qt::NoButton, Qt::LeftButton);
    send(view, QEvent::MouseButtonRelease, to, Qt::LeftButton, Qt::NoButton);
}

} // namespace

// The edition zone, EditionScene: the link between two of its components, how it is selected and removed.
class TestEditionScene : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void cleanup();

    void click_data();
    void click();
    void rubberBandAcrossLink();
    void rubberBandAroundComponents();
    void deleteKey_data();
    void deleteKey();
    void deleteKeyWhileRunning();
    void contextMenu_data();
    void contextMenu();
    void linksOfAList();

  private:
    // Source1, which provides "object", and Sink1, whose "object" is bound to it.
    std::unique_ptr<Nebula::Universe> m_application;
    std::unique_ptr<EditionScene> m_scene;
    std::unique_ptr<QGraphicsView> m_view;
    LinkItem* m_link = nullptr; // Between them
};

void TestEditionScene::initTestCase()
{
    registerTestComponents();
    // Once, before any test fails on a warning: the offscreen platform warns that it has no fonts the first time a text
    // is shown, wherever it does, and the names of the components and the menus are texts.
    QFontDatabase::families();
}

void TestEditionScene::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
    m_application = std::make_unique<Nebula::Universe>();
    Nebula::Component* source = m_application->instantiate(testComponentId(QStringLiteral("Source")));
    Nebula::Component* sink = m_application->instantiate(testComponentId(QStringLiteral("Sink")));
    QVERIFY(source != nullptr && sink != nullptr);
    QVERIFY(sink->bindInterface(QStringLiteral("object"), *source, QStringLiteral("object")));

    // Shown where they are told to be, as a project of NebulaStudio holds them.
    QJsonObject project = m_application->save();
    QJsonArray components = project.value(u"components").toArray();
    for(QJsonValueRef value : components)
    {
        QJsonObject component = value.toObject();
        const QPointF at = component.value(u"name").toString() == source->objectName() ? SourceAt : SinkAt;
        component.insert(u"x", at.x());
        component.insert(u"y", at.y());
        value = component;
    }
    project.insert(u"components", components);
    m_scene = std::make_unique<EditionScene>(*m_application, Nebula::ComponentFactory::instance());
    QCOMPARE(m_scene->load(project), QStringList());

    // As MainWindow shows it.
    m_view = std::make_unique<QGraphicsView>(m_scene.get());
    m_view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_view->setDragMode(QGraphicsView::RubberBandDrag);
    m_view->resize(780, 560);
    m_view->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_view.get()));

    const QList<LinkItem*> shown = links(*m_scene);
    QCOMPARE(shown.size(), 1);
    m_link = shown.constFirst();
    QCOMPARE(m_link->provided().component().objectName(), QStringLiteral("Source1"));
    QCOMPARE(m_link->consumed().component().objectName(), QStringLiteral("Sink1"));
    QCOMPARE(m_link->consumed().name(), QStringLiteral("object"));
    // Nothing is shown over the middle of the link, where it is clicked.
    QCOMPARE(m_scene->itemAt(middle(*m_link), QTransform()), m_link);
}

void TestEditionScene::cleanup()
{
    m_link = nullptr;
    m_view.reset();
    m_scene.reset();
    m_application.reset();
}

void TestEditionScene::click_data()
{
    QTest::addColumn<QPointF>("offset"); // From the middle of the link
    QTest::addColumn<bool>("selected");

    QTest::newRow("on it") << QPointF(0, 0) << true;
    QTest::newRow("next to it") << QPointF(0, 3) << true;
    QTest::newRow("away from it") << QPointF(0, 20) << false;
}

// Clicking a link selects it alone, as clicking a component does, though not exactly on its curve.
void TestEditionScene::click()
{
    QFETCH(QPointF, offset);
    QFETCH(bool, selected);

    componentItem(*m_scene, QStringLiteral("Source1"))->setSelected(true);
    const QPointF at = middle(*m_link) + offset;
    send(*m_view, QEvent::MouseButtonPress, at, Qt::LeftButton, Qt::LeftButton);
    send(*m_view, QEvent::MouseButtonRelease, at, Qt::LeftButton, Qt::NoButton);
    QCOMPARE(m_link->isSelected(), selected);
    QCOMPARE(m_scene->selectedItems().size(), selected ? 1 : 0);
    QCOMPARE(m_scene->selectedComponent(), nullptr); // A link is not a component
}

// A rubber band dragged across a link does not select it.
void TestEditionScene::rubberBandAcrossLink()
{
    const QPointF from = middle(*m_link) + QPointF(-15, -40);
    const QPointF to = middle(*m_link) + QPointF(15, 40);
    QVERIFY(m_scene->items(from).isEmpty());
    QVERIFY(m_scene->items(to).isEmpty());
    dragRubberBand(*m_view, from, to);
    QVERIFY(!m_link->isSelected());
    QVERIFY(m_scene->selectedItems().isEmpty());
}

// A rubber band dragged around components selects them, but not the link between them, which the Delete key would
// remove from what is left otherwise.
void TestEditionScene::rubberBandAroundComponents()
{
    QVERIFY(m_scene->items(AroundFrom).isEmpty());
    QVERIFY(m_scene->items(AroundTo).isEmpty());
    dragRubberBand(*m_view, AroundFrom, AroundTo);
    QVERIFY(componentItem(*m_scene, QStringLiteral("Source1"))->isSelected());
    QVERIFY(componentItem(*m_scene, QStringLiteral("Sink1"))->isSelected());
    QVERIFY(!m_link->isSelected());
    QCOMPARE(m_scene->selectedItems().size(), 2);
}

void TestEditionScene::deleteKey_data()
{
    QTest::addColumn<QStringList>("selected"); // Link, or the name of a component
    QTest::addColumn<QStringList>("left");     // The components left
    QTest::addColumn<bool>("bound");           // Whether Sink1 is still bound to Source1

    const QString source = QStringLiteral("Source1");
    const QString sink = QStringLiteral("Sink1");
    QTest::newRow("nothing") << QStringList() << QStringList{source, sink} << true;
    QTest::newRow("the link") << QStringList{Link} << QStringList{source, sink} << false;
    QTest::newRow("the provider") << QStringList{source} << QStringList{sink} << false;
    QTest::newRow("the link and its consumer") << QStringList{Link, sink} << QStringList{source} << false;
    QTest::newRow("everything") << QStringList{Link, source, sink} << QStringList() << false;
}

// The Delete key removes what is selected, the link between two components included, which unbinds them.
void TestEditionScene::deleteKey()
{
    QFETCH(QStringList, selected);
    QFETCH(QStringList, left);
    QFETCH(bool, bound);

    for(const QString& name : std::as_const(selected))
    {
        if(name == Link)
        {
            m_link->setSelected(true);
        }
        else
        {
            componentItem(*m_scene, name)->setSelected(true);
        }
    }
    QSignalSpy refused(m_scene.get(), &EditionScene::refused);
    QTest::keyClick(m_view.get(), Qt::Key_Delete);
    QCOMPARE(refused.size(), 0);
    QCOMPARE(names(*m_application), left);
    QCOMPARE(links(*m_scene).size(), bound ? 1 : 0);
    if(Nebula::Component* sink = m_application->component(QStringLiteral("Sink1")))
    {
        QCOMPARE(sink->binding(QStringLiteral("object")).provider != nullptr, bound);
        // It does not work without it, which NebulaStudio tells.
        QCOMPARE(sink->unboundMandatoryInterfaces(), bound ? QStringList() : QStringList{QStringLiteral("object")});
    }
}

// While the application runs, the Delete key removes nothing, a link no more than a component.
void TestEditionScene::deleteKeyWhileRunning()
{
    QVERIFY(m_application->start());
    QCOMPARE(links(*m_scene).size(), 1);
    m_link = links(*m_scene).constFirst();
    m_link->setSelected(true);
    QSignalSpy refused(m_scene.get(), &EditionScene::refused);
    QTest::keyClick(m_view.get(), Qt::Key_Delete);
    QCOMPARE(refused.size(), 1);
    QCOMPARE(links(*m_scene).size(), 1);
    QVERIFY(m_application->component(QStringLiteral("Sink1"))->binding(QStringLiteral("object")).provider != nullptr);
    m_application->stop();
}

void TestEditionScene::contextMenu_data()
{
    QTest::addColumn<QPointF>("offset"); // From the middle of the link
    QTest::addColumn<bool>("removable");

    QTest::newRow("on the link") << QPointF(0, 0) << true;
    QTest::newRow("away from it") << QPointF(0, 40) << false;
}

// The menu opened on a link removes it; opened elsewhere, it does not offer to.
void TestEditionScene::contextMenu()
{
    QFETCH(QPointF, offset);
    QFETCH(bool, removable);

    // What the offscreen platform cannot do for a menu.
    QTest::ignoreMessage(QtWarningMsg, "This plugin does not support raise()");
    QTest::ignoreMessage(QtWarningMsg, "This plugin does not support grabbing the keyboard");
    // What the menu offers, read once it is open; then Remove link is chosen, when it is offered, or the menu closed.
    QStringList offered;
    QTimer::singleShot(0, this,
                       [&offered]
                       {
                           auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
                           QVERIFY(menu != nullptr);
                           const QList<QAction*> actions = menu->actions();
                           for(const QAction* action : actions)
                           {
                               offered.append(action->text());
                           }
                           const auto remove =
                               std::ranges::find(actions, QStringLiteral("Remove link"), &QAction::text);
                           if(remove != actions.end())
                           {
                               menu->setActiveAction(*remove);
                               QTest::keyClick(menu, Qt::Key_Return);
                           }
                           else
                           {
                               QTest::keyClick(menu, Qt::Key_Escape);
                           }
                       });
    const QPoint at = m_view->mapFromScene(middle(*m_link) + offset);
    QContextMenuEvent event(QContextMenuEvent::Mouse, at, m_view->viewport()->mapToGlobal(at));
    QApplication::sendEvent(m_view->viewport(), &event);

    QCOMPARE(offered.contains(QStringLiteral("Remove link")), removable);
    QCOMPARE(links(*m_scene).size(), removable ? 0 : 1);
    QCOMPARE(m_application->component(QStringLiteral("Sink1"))->binding(QStringLiteral("object")).provider == nullptr,
             removable);
    QCOMPARE(names(*m_application), (QStringList{QStringLiteral("Source1"), QStringLiteral("Sink1")}));
}

// A list a component consumes has a link for each of what it is bound to; removing one leaves the others.
void TestEditionScene::linksOfAList()
{
    Nebula::Component* other = m_application->instantiate(testComponentId(QStringLiteral("Source")));
    Nebula::Component* source = m_application->component(QStringLiteral("Source1"));
    Nebula::Component* sink = m_application->component(QStringLiteral("Sink1"));
    QVERIFY(sink->bindInterface(QStringLiteral("objects"), *source, QStringLiteral("object")));
    QVERIFY(sink->bindInterface(QStringLiteral("objects"), *other, QStringLiteral("object")));
    QCOMPARE(m_scene->load(m_application->save()), QStringList());
    sink = m_application->component(QStringLiteral("Sink1"));

    QList<LinkItem*> toList;
    const QList<LinkItem*> shown = links(*m_scene);
    for(LinkItem* link : shown)
    {
        if(link->consumed().name() == QStringLiteral("objects"))
        {
            QVERIFY(link->consumed().isList());
            toList.append(link);
        }
    }
    QCOMPARE(shown.size(), 3);
    QCOMPARE(toList.size(), 2);

    const auto toFirst = std::ranges::find(toList, QStringLiteral("Source1"), [](const LinkItem* link)
                                           { return link->provided().component().objectName(); });
    QVERIFY(toFirst != toList.end());
    (*toFirst)->setSelected(true);
    QTest::keyClick(m_view.get(), Qt::Key_Delete);
    QCOMPARE(links(*m_scene).size(), 2);
    const QList<Nebula::InterfaceBinding> bound = sink->bindings(QStringLiteral("objects"));
    QCOMPARE(bound.size(), 1);
    QCOMPARE(bound.constFirst().provider, m_application->component(QStringLiteral("Source2")));
    QVERIFY(sink->binding(QStringLiteral("object")).provider != nullptr); // The other interface keeps its own
}

QTEST_MAIN(TestEditionScene)
#include "TestEditionScene.moc"
