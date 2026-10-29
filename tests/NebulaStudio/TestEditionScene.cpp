// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QFontDatabase>
#include <QGraphicsPathItem>
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

// The middle of the line of link, in the scene.
QPointF middle(const LinkItem& link)
{
    return link.path().pointAtPercent(0.5);
}

// The points path, made of lines only, goes through, from its start to its end.
QList<QPointF> points(const QPainterPath& path)
{
    QList<QPointF> points;
    for(int index = 0; index < path.elementCount(); ++index)
    {
        const QPainterPath::Element element = path.elementAt(index);
        if(element.isCurveTo())
        {
            return {}; // Not made of lines only
        }
        points.append(QPointF(element));
    }
    return points;
}

// The link being drawn in scene, the only path it shows that is not a link; nullptr when none is.
QGraphicsPathItem* pendingLink(const QGraphicsScene& scene)
{
    const QList<QGraphicsItem*> items = scene.items();
    for(QGraphicsItem* item : items)
    {
        if(auto* path = qgraphicsitem_cast<QGraphicsPathItem*>(item))
        {
            return path;
        }
    }
    return nullptr;
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

// The edition zone, EditionScene: the link between two of its components, how it is drawn, selected and removed.
class TestEditionScene : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void cleanup();

    void route_data();
    void route();
    void curve_data();
    void curve();
    void linkStyle_data();
    void linkStyle();
    void pendingLinkStyle_data();
    void pendingLinkStyle();
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
    void interfacesThatFollowAProperty();

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

void TestEditionScene::route_data()
{
    QTest::addColumn<LinkStyle>("style");
    QTest::addColumn<QPointF>("provided");
    QTest::addColumn<QPointF>("consumed");
    QTest::addColumn<QList<QPointF>>("expected"); // The points it goes through

    const QPointF left(0, 0);
    const QPointF right(200, 100);
    QTest::newRow("straight line") << LinkStyle::Straight << left << right << QList<QPointF>{left, right};
    QTest::newRow("straight line, backwards")
        << LinkStyle::Straight << QPointF(200, 0) << QPointF(0, 100) << QList<QPointF>{{200, 0}, {0, 100}};
    // Across halfway between them.
    QTest::newRow("right angles") << LinkStyle::RightAngled << left << right
                                  << QList<QPointF>{left, {100, 0}, {100, 100}, right};
    // Out of the provider and into the consumer, 40 at least each, and across halfway between their heights.
    QTest::newRow("right angles, close") << LinkStyle::RightAngled << left << QPointF(60, 100)
                                         << QList<QPointF>{left, {40, 0}, {40, 50}, {20, 50}, {20, 100}, {60, 100}};
    QTest::newRow("right angles, backwards")
        << LinkStyle::RightAngled << QPointF(200, 0) << QPointF(0, 100)
        << QList<QPointF>{{200, 0}, {240, 0}, {240, 50}, {-40, 50}, {-40, 100}, {0, 100}};
}

// A link in straight lines goes from one dot to the other; in right angles, out of the right edge of the provider and
// into the left edge of the consumer, in horizontal and vertical lines, wherever they are.
void TestEditionScene::route()
{
    QFETCH(LinkStyle, style);
    QFETCH(QPointF, provided);
    QFETCH(QPointF, consumed);
    QFETCH(QList<QPointF>, expected);

    QCOMPARE(points(LinkItem::route(style, provided, consumed)), expected);
}

void TestEditionScene::curve_data()
{
    QTest::addColumn<QPointF>("provided");
    QTest::addColumn<QPointF>("consumed");
    QTest::addColumn<QList<QPointF>>("controls"); // The control points of the curve

    QTest::newRow("far apart") << QPointF(0, 0) << QPointF(200, 100) << QList<QPointF>{{100, 0}, {100, 100}};
    QTest::newRow("close") << QPointF(0, 0) << QPointF(20, 100) << QList<QPointF>{{40, 0}, {-20, 100}};
    QTest::newRow("backwards") << QPointF(200, 0) << QPointF(0, 100) << QList<QPointF>{{300, 0}, {-100, 100}};
}

// A curved link, as links were drawn before they had a style: out of the right edge of the provider, into the left
// edge of the consumer, as far as half the width between them, 40 at least.
void TestEditionScene::curve()
{
    QFETCH(QPointF, provided);
    QFETCH(QPointF, consumed);
    QFETCH(QList<QPointF>, controls);

    const QPainterPath path = LinkItem::route(LinkStyle::Curved, provided, consumed);
    QCOMPARE(path.elementCount(), 4);
    QCOMPARE(QPointF(path.elementAt(0)), provided);
    QVERIFY(path.elementAt(1).isCurveTo());
    QCOMPARE((QList<QPointF>{path.elementAt(1), path.elementAt(2)}), controls);
    QCOMPARE(QPointF(path.elementAt(3)), consumed);
}

void TestEditionScene::linkStyle_data()
{
    QTest::addColumn<LinkStyle>("style");

    QTest::newRow("curves") << LinkStyle::Curved;
    QTest::newRow("straight lines") << LinkStyle::Straight;
    QTest::newRow("right angles") << LinkStyle::RightAngled;
}

// The links of a scene are drawn in the style it is given, those it shows already as those it shows from then on; they
// follow their components and are selected by a click in that style.
void TestEditionScene::linkStyle()
{
    QFETCH(LinkStyle, style);

    QCOMPARE(m_scene->linkStyle(), LinkStyle::Curved); // Until another one is chosen
    QCOMPARE(m_link->style(), LinkStyle::Curved);
    m_scene->setLinkStyle(style);
    QCOMPARE(m_scene->linkStyle(), style);
    QCOMPARE(m_link->style(), style);
    QCOMPARE(m_link->path(), LinkItem::route(style, m_link->provided().scenePos(), m_link->consumed().scenePos()));

    componentItem(*m_scene, QStringLiteral("Sink1"))->moveBy(0, 40);
    QCOMPARE(m_link->path(), LinkItem::route(style, m_link->provided().scenePos(), m_link->consumed().scenePos()));
    QCOMPARE(m_scene->itemAt(middle(*m_link), QTransform()), m_link); // Nothing is shown over it there
    send(*m_view, QEvent::MouseButtonPress, middle(*m_link), Qt::LeftButton, Qt::LeftButton);
    send(*m_view, QEvent::MouseButtonRelease, middle(*m_link), Qt::LeftButton, Qt::NoButton);
    QVERIFY(m_link->isSelected());

    QCOMPARE(m_scene->load(m_application->save()), QStringList());
    const QList<LinkItem*> shown = links(*m_scene);
    QCOMPARE(shown.size(), 1);
    m_link = shown.constFirst();
    QCOMPARE(m_link->style(), style);
    QCOMPARE(m_scene->linkStyle(), style); // Not saved with the project, nor loaded from it
}

void TestEditionScene::pendingLinkStyle_data()
{
    QTest::addColumn<LinkStyle>("style");
    QTest::addColumn<bool>("fromProvided"); // Drawn from the dot of the provided interface, or of the consumed one

    QTest::newRow("curve from the provided one") << LinkStyle::Curved << true;
    QTest::newRow("straight line from the provided one") << LinkStyle::Straight << true;
    QTest::newRow("right angles from the provided one") << LinkStyle::RightAngled << true;
    QTest::newRow("right angles from the consumed one") << LinkStyle::RightAngled << false;
}

// The link being drawn is in the style of the scene, from the provided interface to the consumed one whichever it is
// drawn from: it is drawn as the link will be.
void TestEditionScene::pendingLinkStyle()
{
    QFETCH(LinkStyle, style);
    QFETCH(bool, fromProvided);

    m_scene->setLinkStyle(style);
    const QPointF start = (fromProvided ? m_link->provided() : m_link->consumed()).scenePos();
    const QPointF cursor(300, 400); // Where nothing is
    QVERIFY(m_scene->items(cursor).isEmpty());
    send(*m_view, QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
    send(*m_view, QEvent::MouseMove, cursor, Qt::NoButton, Qt::LeftButton);
    const QGraphicsPathItem* pending = pendingLink(*m_scene);
    QVERIFY(pending != nullptr);
    QCOMPARE(pending->path(),
             fromProvided ? LinkItem::route(style, start, cursor) : LinkItem::route(style, cursor, start));

    send(*m_view, QEvent::MouseButtonRelease, cursor, Qt::LeftButton, Qt::NoButton);
    QCOMPARE(pendingLink(*m_scene), nullptr);
    QCOMPARE(links(*m_scene).size(), 1); // Released away from any interface: nothing more is bound
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

// A component whose count tells the interfaces it provides: its item shows those it has, and the link of one it removes
// leaves with it, the others stay.
void TestEditionScene::interfacesThatFollowAProperty()
{
    Nebula::Component* outlets = m_application->instantiate(testComponentId(QStringLiteral("Outlets")));
    Nebula::Component* sink = m_application->component(QStringLiteral("Sink1"));
    QVERIFY(outlets->setProperty("count", 2));
    QVERIFY(sink->bindInterface(QStringLiteral("objects"), *outlets, QStringLiteral("object1")));
    QVERIFY(sink->bindInterface(QStringLiteral("objects"), *outlets, QStringLiteral("object2")));
    QCOMPARE(m_scene->load(m_application->save()), QStringList());
    outlets = m_application->component(QStringLiteral("Outlets1"));
    const ComponentItem* item = componentItem(*m_scene, QStringLiteral("Outlets1"));
    QCOMPARE(item->providedInterfaces().size(), 2);
    QCOMPARE(links(*m_scene).size(), 3);

    QVERIFY(outlets->setProperty("count", 1));
    QCOMPARE(item->providedInterfaces().size(), 1);
    QCOMPARE(links(*m_scene).size(), 2);
    QVERIFY(outlets->setProperty("count", 3));
    QCOMPARE(item->providedInterfaces().size(), 3);
    QCOMPARE(links(*m_scene).size(), 2);
}

QTEST_MAIN(TestEditionScene)
#include "TestEditionScene.moc"
