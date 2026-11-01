// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QContextMenuEvent>
#include <QFontDatabase>
#include <QGraphicsPathItem>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QJsonArray>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>
#include <algorithm>
#include <memory>

#include "AnchorItem.h"
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
// Where another source is shown, below the first one, when a test adds one.
constexpr QPointF OtherSourceAt(40, 300);
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

// The interface named name of the component scene shows named component, consumed or provided; nullptr when there is
// none.
InterfaceItem* interfaceItem(const QGraphicsScene& scene, const QString& component, const QString& name)
{
    const ComponentItem* item = componentItem(scene, component);
    if(item == nullptr)
    {
        return nullptr;
    }
    for(InterfaceItem* shown : item->consumedInterfaces() + item->providedInterfaces())
    {
        if(shown->name() == name)
        {
            return shown;
        }
    }
    return nullptr;
}

// Has project, as a project of NebulaStudio, show the component named name at at.
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

// What name, an interface component consumes, is bound to, in their order: Source1.object.
QStringList boundTo(const Nebula::Component& component, const QString& name)
{
    QStringList bound;
    const QList<Nebula::InterfaceBinding> bindings = component.bindings(name);
    for(const Nebula::InterfaceBinding& binding : bindings)
    {
        bound.append(binding.provider->objectName() + u'.' + binding.providedName);
    }
    return bound;
}

// What the context menu of a dot offers under Bind to.
struct BindMenu
{
    bool offered = false;   // Whether the menu has Bind to
    QStringList entries;    // Its entries, in their order
    QStringList checked;    // Those of them checked
    QStringList disabled;   // Those of them that cannot be chosen
    bool exclusive = false; // Whether its entries are radio buttons: one at most can be checked
};

// Reads the entries of bindTo, a submenu.
BindMenu readBindMenu(const QMenu& bindTo)
{
    BindMenu read{.offered = true};
    const QList<QAction*> actions = bindTo.actions();
    for(const QAction* action : actions)
    {
        read.entries.append(action->text());
        if(action->isChecked())
        {
            read.checked.append(action->text());
        }
        if(!action->isEnabled())
        {
            read.disabled.append(action->text());
        }
        read.exclusive = action->actionGroup() != nullptr && action->actionGroup()->isExclusive();
    }
    return read;
}

// Chooses the entry of bindTo, a submenu of menu, telling chosen, as the keyboard would: Bind to opens it, then the
// entry is triggered. Closes menu, choosing nothing, when there is no such entry, or it cannot be chosen.
void chooseIn(QMenu& menu, QAction& bindTo, const QString& chosen)
{
    const QList<QAction*> actions = bindTo.menu()->actions();
    const auto entry = std::ranges::find(actions, chosen, &QAction::text);
    if(entry == actions.end() || !(*entry)->isEnabled())
    {
        QTest::keyClick(&menu, Qt::Key_Escape);
        return;
    }
    // What the offscreen platform cannot do for the submenu either.
    QTest::ignoreMessage(QtWarningMsg, "This plugin does not support raise()");
    QTest::ignoreMessage(QtWarningMsg, "This plugin does not support grabbing the keyboard");
    menu.setActiveAction(&bindTo); // Which opens its submenu
    QVERIFY(bindTo.menu()->isVisible());
    bindTo.menu()->setActiveAction(*entry);
    QTest::keyClick(bindTo.menu(), Qt::Key_Return);
}

// Opens the context menu of view at at, a position in its scene, and reads what it offers under Bind to; then chooses
// chosen there, unless it is empty or not offered, which closes the menu.
BindMenu openBindMenu(QGraphicsView& view, QPointF at, const QString& chosen)
{
    // What the offscreen platform cannot do for a menu.
    QTest::ignoreMessage(QtWarningMsg, "This plugin does not support raise()");
    QTest::ignoreMessage(QtWarningMsg, "This plugin does not support grabbing the keyboard");
    BindMenu read;
    QTimer::singleShot(0, &view,
                       [&read, &chosen]
                       {
                           auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
                           QVERIFY(menu != nullptr);
                           const QList<QAction*> actions = menu->actions();
                           const auto bindTo = std::ranges::find(actions, QStringLiteral("Bind to"), &QAction::text);
                           if(bindTo == actions.end())
                           {
                               QTest::keyClick(menu, Qt::Key_Escape);
                               return;
                           }
                           read = readBindMenu(*(*bindTo)->menu());
                           chooseIn(*menu, **bindTo, chosen);
                       });
    const QPoint position = view.mapFromScene(at);
    QContextMenuEvent event(QContextMenuEvent::Mouse, position, view.viewport()->mapToGlobal(position));
    QApplication::sendEvent(view.viewport(), &event);
    return read;
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

// Drags from from to to, positions in the scene of view, with the left button: the rubber band of view, or what is
// pressed there.
void drag(QGraphicsView& view, QPointF from, QPointF to)
{
    send(view, QEvent::MouseButtonPress, from, Qt::LeftButton, Qt::LeftButton);
    send(view, QEvent::MouseMove, (from + to) / 2, Qt::NoButton, Qt::LeftButton);
    send(view, QEvent::MouseMove, to, Qt::NoButton, Qt::LeftButton);
    send(view, QEvent::MouseButtonRelease, to, Qt::LeftButton, Qt::NoButton);
}

// Double-clicks view at at, a position in its scene, as the window system does: a click, then a double click.
void doubleClick(QGraphicsView& view, QPointF at)
{
    send(view, QEvent::MouseButtonPress, at, Qt::LeftButton, Qt::LeftButton);
    send(view, QEvent::MouseButtonRelease, at, Qt::LeftButton, Qt::NoButton);
    send(view, QEvent::MouseButtonDblClick, at, Qt::LeftButton, Qt::LeftButton);
    send(view, QEvent::MouseButtonRelease, at, Qt::LeftButton, Qt::NoButton);
}

// Where view sends a mouse event at at, a position in its scene, to its scene: on the pixel at falls on.
QPointF onPixel(const QGraphicsView& view, QPointF at)
{
    return view.mapToScene(view.mapFromScene(at));
}

// The anchors scene shows, of all its links.
QList<AnchorItem*> anchorItems(const QGraphicsScene& scene)
{
    QList<AnchorItem*> anchors;
    const QList<QGraphicsItem*> items = scene.items();
    for(QGraphicsItem* item : items)
    {
        if(auto* anchor = qgraphicsitem_cast<AnchorItem*>(item))
        {
            anchors.append(anchor);
        }
    }
    return anchors;
}

// What a context menu offers.
struct Menu
{
    QStringList offered; // Its entries, without its separators
    QStringList checked; // Those of them checked
    bool radio = false;  // Whether those that can be checked are radio buttons: one of them at most is checked
};

// Reads what menu offers.
Menu readMenu(const QMenu& menu)
{
    Menu read;
    const QList<QAction*> actions = menu.actions();
    for(const QAction* action : actions)
    {
        if(!action->isSeparator())
        {
            read.offered.append(action->text());
        }
        if(action->isChecked())
        {
            read.checked.append(action->text());
        }
        if(action->isCheckable())
        {
            read.radio = action->actionGroup() != nullptr && action->actionGroup()->isExclusive();
        }
    }
    return read;
}

// Opens the context menu of view at at, a position in its scene, as the mouse does with modifiers held, and chooses the
// entry telling chosen there, unless it is empty, not offered or disabled, which closes the menu. Returns what it
// offers.
Menu openMenu(QGraphicsView& view, QPointF at, const QString& chosen, Qt::KeyboardModifiers modifiers)
{
    // What the offscreen platform cannot do for a menu.
    QTest::ignoreMessage(QtWarningMsg, "This plugin does not support raise()");
    QTest::ignoreMessage(QtWarningMsg, "This plugin does not support grabbing the keyboard");
    Menu read;
    QTimer::singleShot(0, &view,
                       [&read, &chosen]
                       {
                           auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
                           QVERIFY(menu != nullptr);
                           read = readMenu(*menu);
                           const QList<QAction*> actions = menu->actions();
                           // Not a separator, which tells no text.
                           const auto entry =
                               chosen.isEmpty() ? actions.end() : std::ranges::find(actions, chosen, &QAction::text);
                           if(entry != actions.end() && (*entry)->isEnabled())
                           {
                               menu->setActiveAction(*entry);
                               QTest::keyClick(menu, Qt::Key_Return);
                               return;
                           }
                           QTest::keyClick(menu, Qt::Key_Escape);
                       });
    const QPoint position = view.mapFromScene(at);
    QContextMenuEvent event(QContextMenuEvent::Mouse, position, view.viewport()->mapToGlobal(position), modifiers);
    QApplication::sendEvent(view.viewport(), &event);
    return read;
}

// Opens the context menu of view at at, a position in its scene, and chooses the entry telling chosen there, unless it
// is empty, not offered or disabled, which closes the menu. Returns what it offers, without its separators.
QStringList chooseFromMenu(QGraphicsView& view, QPointF at, const QString& chosen)
{
    return openMenu(view, at, chosen, Qt::NoModifier).offered;
}

// The points a link through stops goes through, from the dot of provided to the dot of consumed, in style: those of
// its route from each of them to the next one, which ends where the next one starts.
QList<QPointF> anchoredPoints(LinkStyle style, const QList<LinkItem::Stop>& stops)
{
    QList<QPointF> found{stops.constFirst().point};
    for(qsizetype index = 1; index < stops.size(); ++index)
    {
        found.append(points(LinkItem::route(style, stops.at(index - 1), stops.at(index))).sliced(1));
    }
    return found;
}

// The stops of a link through anchors, each gone through the way told: leftwards or not.
QList<LinkItem::Stop> stopsThrough(const LinkItem& link, const QList<QPointF>& anchors, bool leftwards)
{
    QList<LinkItem::Stop> stops{LinkItem::stopAt(link.provided())};
    for(const QPointF& anchor : anchors)
    {
        stops.append({.point = anchor, .leftwards = leftwards});
    }
    stops.append(LinkItem::stopAt(link.consumed()));
    return stops;
}

// The x of the dot of an interface on the edge onRight tells of its component, as InterfaceItem::pos() tells it: on its
// edges, one pixel inside its bounds, which are 160 wide around its position.
qreal dotX(bool onRight)
{
    return onRight ? 79 : -79;
}

// The side onRight tells.
InterfaceItem::Side side(bool onRight)
{
    return onRight ? InterfaceItem::Side::Right : InterfaceItem::Side::Left;
}

// What a project holds of the component named name: its entry in components.
QJsonObject componentJson(const QJsonObject& project, const QString& name)
{
    const QJsonArray components = project.value(u"components").toArray();
    for(const auto& value : components)
    {
        if(value.toObject().value(u"name").toString() == name)
        {
            return value.toObject();
        }
    }
    return {};
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
    void bindMenu_data();
    void bindMenu();
    void noBindMenuOnAProvidedInterface();
    void linksOfAList();
    void interfacesThatFollowAProperty();
    void anchoredPath_data();
    void anchoredPath();
    void doubleClickAnchors_data();
    void doubleClickAnchors();
    void anchorMenu_data();
    void anchorMenu();
    void dragAnchor();
    void anchorsWhileRunning();
    void anchorsSaved();
    void anchorsOfALinkDrawnAgain();
    void anchorsOfALinkRemoved();
    void sideMenu_data();
    void sideMenu();
    void noSideMenuWithoutShift();
    void sideMenuWhileRunning();
    void sidesSaved();
    void sidesOfInterfacesShownAgain();
    void exposeNextToTheDot_data();
    void exposeNextToTheDot();

  private:
    // The middle of the link, moved down by offset: where nothing else is shown, below it.
    [[nodiscard]] QPointF belowLink(qreal offset) const;

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
    place(project, source->objectName(), SourceAt);
    place(project, sink->objectName(), SinkAt);
    m_scene = std::make_unique<EditionScene>(*m_application, Nebula::ComponentFactory::instance());
    QCOMPARE(m_scene->load(project), QStringList());

    // As the edition zone shows it.
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
    QTest::addColumn<bool>("providedOnLeft"); // Whether the dot of the provided interface is on the left edge
    QTest::addColumn<QPointF>("consumed");
    QTest::addColumn<bool>("consumedOnRight");    // Whether the dot of the consumed interface is on the right edge
    QTest::addColumn<QList<QPointF>>("expected"); // The points it goes through

    const QPointF left(0, 0);
    const QPointF right(200, 100);
    QTest::newRow("straight line") << LinkStyle::Straight << left << false << right << false
                                   << QList<QPointF>{left, right};
    QTest::newRow("straight line, backwards") << LinkStyle::Straight << QPointF(200, 0) << false << QPointF(0, 100)
                                              << false << QList<QPointF>{{200, 0}, {0, 100}};
    QTest::newRow("straight line, out of the left edge")
        << LinkStyle::Straight << QPointF(200, 0) << true << QPointF(0, 100) << true
        << QList<QPointF>{{200, 0}, {0, 100}};
    // Across halfway between them.
    QTest::newRow("right angles") << LinkStyle::RightAngled << left << false << right << false
                                  << QList<QPointF>{left, {100, 0}, {100, 100}, right};
    // Out of the provider and into the consumer, 40 at least each, and across halfway between their heights.
    QTest::newRow("right angles, close") << LinkStyle::RightAngled << left << false << QPointF(60, 100) << false
                                         << QList<QPointF>{left, {40, 0}, {40, 50}, {20, 50}, {20, 100}, {60, 100}};
    QTest::newRow("right angles, backwards")
        << LinkStyle::RightAngled << QPointF(200, 0) << false << QPointF(0, 100) << false
        << QList<QPointF>{{200, 0}, {240, 0}, {240, 50}, {-40, 50}, {-40, 100}, {0, 100}};
    // Out of the left edge of the provider, into the right edge of the consumer: the other way round.
    QTest::newRow("right angles, leftwards") << LinkStyle::RightAngled << QPointF(200, 0) << true << QPointF(0, 100)
                                             << true << QList<QPointF>{{200, 0}, {100, 0}, {100, 100}, {0, 100}};
    QTest::newRow("right angles, leftwards, close")
        << LinkStyle::RightAngled << QPointF(60, 0) << true << QPointF(0, 100) << true
        << QList<QPointF>{{60, 0}, {20, 0}, {20, 50}, {40, 50}, {40, 100}, {0, 100}};
    // Out of one, past the other one, and back into it.
    QTest::newRow("right angles, both on their right edges")
        << LinkStyle::RightAngled << left << false << QPointF(100, 100) << true
        << QList<QPointF>{left, {140, 0}, {140, 100}, {100, 100}};
    QTest::newRow("right angles, both on their left edges")
        << LinkStyle::RightAngled << QPointF(100, 0) << true << QPointF(0, 100) << false
        << QList<QPointF>{{100, 0}, {-40, 0}, {-40, 100}, {0, 100}};
}

// A link in straight lines goes from one dot to the other; in right angles, out of the edge of the provider its dot is
// on and into the edge of the consumer its dot is on, in horizontal and vertical lines, wherever they are.
void TestEditionScene::route()
{
    QFETCH(LinkStyle, style);
    QFETCH(QPointF, provided);
    QFETCH(bool, providedOnLeft);
    QFETCH(QPointF, consumed);
    QFETCH(bool, consumedOnRight);
    QFETCH(QList<QPointF>, expected);

    QCOMPARE(points(LinkItem::route(style, {.point = provided, .leftwards = providedOnLeft},
                                    {.point = consumed, .leftwards = consumedOnRight})),
             expected);
}

void TestEditionScene::curve_data()
{
    QTest::addColumn<QPointF>("provided");
    QTest::addColumn<bool>("providedOnLeft"); // Whether the dot of the provided interface is on the left edge
    QTest::addColumn<QPointF>("consumed");
    QTest::addColumn<bool>("consumedOnRight");    // Whether the dot of the consumed interface is on the right edge
    QTest::addColumn<QList<QPointF>>("controls"); // The control points of the curve

    QTest::newRow("far apart") << QPointF(0, 0) << false << QPointF(200, 100) << false
                               << QList<QPointF>{{100, 0}, {100, 100}};
    QTest::newRow("close") << QPointF(0, 0) << false << QPointF(20, 100) << false
                           << QList<QPointF>{{40, 0}, {-20, 100}};
    QTest::newRow("backwards") << QPointF(200, 0) << false << QPointF(0, 100) << false
                               << QList<QPointF>{{300, 0}, {-100, 100}};
    QTest::newRow("leftwards") << QPointF(200, 0) << true << QPointF(0, 100) << true
                               << QList<QPointF>{{100, 0}, {100, 100}};
    QTest::newRow("both on their right edges")
        << QPointF(0, 0) << false << QPointF(100, 100) << true << QList<QPointF>{{50, 0}, {150, 100}};
}

// A curved link, as links were drawn before they had a style: out of the edge of the provider its dot is on, into the
// edge of the consumer its dot is on, as far as half the width between them, 40 at least.
void TestEditionScene::curve()
{
    QFETCH(QPointF, provided);
    QFETCH(bool, providedOnLeft);
    QFETCH(QPointF, consumed);
    QFETCH(bool, consumedOnRight);
    QFETCH(QList<QPointF>, controls);

    const QPainterPath path = LinkItem::route(LinkStyle::Curved, {.point = provided, .leftwards = providedOnLeft},
                                              {.point = consumed, .leftwards = consumedOnRight});
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
    QCOMPARE(m_link->path(),
             LinkItem::route(style, LinkItem::stopAt(m_link->provided()), LinkItem::stopAt(m_link->consumed())));

    componentItem(*m_scene, QStringLiteral("Sink1"))->moveBy(0, 40);
    QCOMPARE(m_link->path(),
             LinkItem::route(style, LinkItem::stopAt(m_link->provided()), LinkItem::stopAt(m_link->consumed())));
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
    QTest::addColumn<bool>("otherEdge");    // Whether that dot is on the other edge of its component

    QTest::newRow("curve from the provided one") << LinkStyle::Curved << true << false;
    QTest::newRow("straight line from the provided one") << LinkStyle::Straight << true << false;
    QTest::newRow("right angles from the provided one") << LinkStyle::RightAngled << true << false;
    QTest::newRow("right angles from the consumed one") << LinkStyle::RightAngled << false << false;
    QTest::newRow("curve from the left edge of the provider") << LinkStyle::Curved << true << true;
    QTest::newRow("right angles from the right edge of the consumer") << LinkStyle::RightAngled << false << true;
}

// The link being drawn is in the style of the scene, from the provided interface to the consumed one whichever it is
// drawn from, out of the edge the dot it is drawn from is on, and into the cursor the same way: it is drawn as the link
// will be.
void TestEditionScene::pendingLinkStyle()
{
    QFETCH(LinkStyle, style);
    QFETCH(bool, fromProvided);
    QFETCH(bool, otherEdge);

    m_scene->setLinkStyle(style);
    InterfaceItem& source = fromProvided ? m_link->provided() : m_link->consumed();
    if(otherEdge)
    {
        ComponentItem& component = source.componentItem();
        component.moveBy(100, 0); // Its left edge in the view as well
        component.setInterfaceSide(source.kind(), source.name(), side(source.side() == InterfaceItem::Side::Left));
    }
    const LinkItem::Stop start = LinkItem::stopAt(source);
    QCOMPARE(start.leftwards, otherEdge);
    const QPointF cursor(300, 400); // Where nothing is
    QVERIFY(m_scene->items(cursor).isEmpty());
    send(*m_view, QEvent::MouseButtonPress, start.point, Qt::LeftButton, Qt::LeftButton);
    send(*m_view, QEvent::MouseMove, cursor, Qt::NoButton, Qt::LeftButton);
    const QGraphicsPathItem* pending = pendingLink(*m_scene);
    QVERIFY(pending != nullptr);
    const LinkItem::Stop end{.point = cursor, .leftwards = otherEdge};
    QCOMPARE(pending->path(), fromProvided ? LinkItem::route(style, start, end) : LinkItem::route(style, end, start));

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
    drag(*m_view, from, to);
    QVERIFY(!m_link->isSelected());
    QVERIFY(m_scene->selectedItems().isEmpty());
}

// A rubber band dragged around components selects them, but not the link between them, which the Delete key would
// remove from what is left otherwise.
void TestEditionScene::rubberBandAroundComponents()
{
    QVERIFY(m_scene->items(AroundFrom).isEmpty());
    QVERIFY(m_scene->items(AroundTo).isEmpty());
    drag(*m_view, AroundFrom, AroundTo);
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

void TestEditionScene::bindMenu_data()
{
    QTest::addColumn<QString>("consumed");    // The interface of Sink1 whose dot is right-clicked
    QTest::addColumn<QStringList>("before");  // What it is bound to first, besides object to Source1.object
    QTest::addColumn<QStringList>("offered"); // The entries of Bind to
    QTest::addColumn<QStringList>("checked"); // Those of them checked
    QTest::addColumn<bool>("exclusive");      // Whether they are radio buttons
    QTest::addColumn<QString>("chosen");      // The one chosen; none when the menu is closed
    QTest::addColumn<QStringList>("after");   // What it is bound to then

    const QString first = QStringLiteral("Source1.object");
    const QString second = QStringLiteral("Source2.object");
    const QString object = QStringLiteral("object");
    const QString objects = QStringLiteral("objects");
    const QStringList both{first, second};
    QTest::newRow("bound to another one")
        << object << QStringList() << both << QStringList{first} << true << second << QStringList{second};
    QTest::newRow("unbound from its own")
        << object << QStringList() << both << QStringList{first} << true << first << QStringList();
    QTest::newRow("menu closed") << object << QStringList() << both << QStringList{first} << true << QString()
                                 << QStringList{first};
    QTest::newRow("list bound to a first one")
        << objects << QStringList() << both << QStringList() << false << second << QStringList{second};
    QTest::newRow("list bound to one more")
        << objects << QStringList{first} << both << QStringList{first} << false << second << both;
    QTest::newRow("list unbound from one of them")
        << objects << both << both << both << false << first << QStringList{second};
    QTest::newRow("nothing provides its type") << QStringLiteral("timer") << QStringList()
                                               << QStringList{QStringLiteral("No other component here provides QTimer")}
                                               << QStringList() << false << QString() << QStringList();
}

// The menu of the dot of a consumed interface offers, under Bind to, the interfaces shown it can be bound to, those it
// is bound to checked: choosing one binds it, in place of what it was bound to, or after them for a list, and choosing
// a checked one unbinds it. Its links follow.
void TestEditionScene::bindMenu()
{
    QFETCH(QString, consumed);
    QFETCH(QStringList, before);
    QFETCH(QStringList, offered);
    QFETCH(QStringList, checked);
    QFETCH(bool, exclusive);
    QFETCH(QString, chosen);
    QFETCH(QStringList, after);

    // Another source, which Sink1 can be bound to as well, shown below the first one.
    QVERIFY(m_application->instantiate(testComponentId(QStringLiteral("Source"))) != nullptr);
    for(const QString& provider : std::as_const(before))
    {
        QVERIFY(m_application->component(QStringLiteral("Sink1"))
                    ->bindInterface(consumed, *m_application->component(provider.section(u'.', 0, 0)),
                                    provider.section(u'.', 1)));
    }
    QJsonObject project = m_scene->save();
    place(project, QStringLiteral("Source2"), OtherSourceAt);
    QCOMPARE(m_scene->load(project), QStringList());
    const Nebula::Component* sink = m_application->component(QStringLiteral("Sink1"));

    const InterfaceItem* dot = interfaceItem(*m_scene, QStringLiteral("Sink1"), consumed);
    QVERIFY(dot != nullptr);
    const BindMenu read = openBindMenu(*m_view, dot->scenePos(), chosen);
    QVERIFY(read.offered);
    QCOMPARE(read.entries, offered);
    QCOMPARE(read.checked, checked);
    QCOMPARE(read.exclusive, exclusive);
    QCOMPARE(read.disabled, consumed == QStringLiteral("timer") ? offered : QStringList());

    QCOMPARE(boundTo(*sink, consumed), after);
    QCOMPARE(interfaceItem(*m_scene, QStringLiteral("Sink1"), consumed)->links().size(), after.size());
    if(consumed != QStringLiteral("object")) // Which keeps its own
    {
        QCOMPARE(boundTo(*sink, QStringLiteral("object")), QStringList{QStringLiteral("Source1.object")});
    }
}

// The menu of the dot of a provided interface does not offer Bind to: several interfaces can be bound to it, each from
// its own dot.
void TestEditionScene::noBindMenuOnAProvidedInterface()
{
    const InterfaceItem* dot = interfaceItem(*m_scene, QStringLiteral("Source1"), QStringLiteral("object"));
    QVERIFY(dot != nullptr);
    QVERIFY(!openBindMenu(*m_view, dot->scenePos(), {}).offered);
    QCOMPARE(links(*m_scene).size(), 1);
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

QPointF TestEditionScene::belowLink(qreal offset) const
{
    return middle(*m_link) + QPointF(0, offset);
}

void TestEditionScene::anchoredPath_data()
{
    QTest::addColumn<LinkStyle>("style");
    QTest::addColumn<int>("count");      // Of its anchors, below it, from its provided interface to its consumed one
    QTest::addColumn<bool>("backwards"); // Whether its consumer is on the left of its provider

    QTest::newRow("straight lines, one anchor") << LinkStyle::Straight << 1 << false;
    QTest::newRow("straight lines, two anchors") << LinkStyle::Straight << 2 << false;
    QTest::newRow("right angles, one anchor") << LinkStyle::RightAngled << 1 << false;
    QTest::newRow("right angles, two anchors") << LinkStyle::RightAngled << 2 << false;
    QTest::newRow("curves, one anchor") << LinkStyle::Curved << 1 << false;
    QTest::newRow("curves, two anchors") << LinkStyle::Curved << 2 << false;
    // Each gone through leftwards, from the point before it to the one after it.
    QTest::newRow("right angles, one anchor, backwards") << LinkStyle::RightAngled << 1 << true;
    QTest::newRow("right angles, two anchors, backwards") << LinkStyle::RightAngled << 2 << true;
}

// A link goes through its anchors, in their order, from each point to the next one in its style, as it goes from one
// dot to the other, through each of them the way from the point before it to the one after it; they are shown where
// they are.
void TestEditionScene::anchoredPath()
{
    QFETCH(LinkStyle, style);
    QFETCH(int, count);
    QFETCH(bool, backwards);

    m_scene->setLinkStyle(style);
    if(backwards)
    {
        componentItem(*m_scene, QStringLiteral("Source1"))->setPos(SinkAt + QPointF(260, -80));
    }
    QList<QPointF> anchors;
    for(int index = 0; index < count; ++index)
    {
        anchors.append(belowLink(80) + QPointF((backwards ? -60 : 60) * index, 0));
    }
    m_link->setAnchors(anchors);
    QCOMPARE(m_link->anchors(), anchors);
    QCOMPARE(anchorItems(*m_scene).size(), count);
    for(const AnchorItem* anchor : anchorItems(*m_scene))
    {
        QVERIFY(anchors.contains(anchor->scenePos()));
        QCOMPARE(&anchor->link(), m_link);
    }

    const QPainterPath path = m_link->path();
    if(style == LinkStyle::Curved) // A curve to each of them, then to the consumed interface
    {
        QCOMPARE(path.elementCount(), 1 + 3 * (count + 1));
        for(int index = 0; index < count; ++index)
        {
            QCOMPARE(QPointF(path.elementAt(3 * (index + 1))), anchors.at(index));
        }
        return;
    }
    QCOMPARE(points(path), anchoredPoints(style, stopsThrough(*m_link, anchors, backwards)));
}

void TestEditionScene::doubleClickAnchors_data()
{
    QTest::addColumn<bool>("anchored"); // Whether the link has an anchor first, below its middle
    QTest::addColumn<QString>("where"); // What is double-clicked: the link, before or after that anchor, or the anchor
    QTest::addColumn<int>("added");     // Where the anchor added is among those of the link; -1 when none is

    QTest::newRow("the link") << false << QStringLiteral("link") << 0;
    QTest::newRow("the link, before its anchor") << true << QStringLiteral("before") << 0;
    QTest::newRow("the link, after its anchor") << true << QStringLiteral("after") << 1;
    QTest::newRow("an anchor") << true << QStringLiteral("anchor") << -1;
}

// Double-clicking a link anchors it there, the new anchor among the others where the link went through that point;
// double-clicking an anchor removes it.
void TestEditionScene::doubleClickAnchors()
{
    QFETCH(bool, anchored);
    QFETCH(QString, where);
    QFETCH(int, added);

    m_scene->setLinkStyle(LinkStyle::Straight); // From one point to the next one, through the middle between them
    const QPointF start = m_link->provided().scenePos();
    const QPointF end = m_link->consumed().scenePos();
    const QPointF anchor = belowLink(80);
    m_link->setAnchors(anchored ? QList<QPointF>{anchor} : QList<QPointF>());
    QPointF at = anchor;
    if(where != QStringLiteral("anchor"))
    {
        at = onPixel(*m_view, where == QStringLiteral("link")
                                  ? (start + end) / 2
                                  : (where == QStringLiteral("before") ? (start + anchor) / 2 : (anchor + end) / 2));
        QCOMPARE(m_scene->itemAt(at, QTransform()), m_link); // Nothing is shown over it there
    }

    QSignalSpy refused(m_scene.get(), &EditionScene::refused);
    doubleClick(*m_view, at);
    QCOMPARE(refused.size(), 0);
    QList<QPointF> expected = added >= 0 && anchored ? QList<QPointF>{anchor} : QList<QPointF>();
    if(added >= 0)
    {
        expected.insert(added, at);
    }
    QCOMPARE(m_link->anchors(), expected);
    QCOMPARE(anchorItems(*m_scene).size(), expected.size());
    QCOMPARE(links(*m_scene).size(), 1); // Still bound
}

void TestEditionScene::anchorMenu_data()
{
    QTest::addColumn<bool>("anchored");       // Whether the link has an anchor first, below its middle
    QTest::addColumn<bool>("onAnchor");       // Whether the menu is opened on that anchor, or on the link
    QTest::addColumn<QStringList>("offered"); // What the menu offers
    QTest::addColumn<QString>("chosen");
    QTest::addColumn<int>("left"); // The anchors the link has then

    const QStringList onLink{QStringLiteral("Add anchor point"), QStringLiteral("Remove anchor points"),
                             QStringLiteral("Remove link")};
    QTest::newRow("link anchored") << false << false << onLink << QStringLiteral("Add anchor point") << 1;
    QTest::newRow("link anchored once more") << true << false << onLink << QStringLiteral("Add anchor point") << 2;
    QTest::newRow("anchors of the link removed")
        << true << false << onLink << QStringLiteral("Remove anchor points") << 0;
    QTest::newRow("anchor removed") << true << true << QStringList{QStringLiteral("Remove anchor point")}
                                    << QStringLiteral("Remove anchor point") << 0;
}

// The menu opened on a link anchors it where it was opened, or removes its anchors; the one opened on an anchor removes
// it alone.
void TestEditionScene::anchorMenu()
{
    QFETCH(bool, anchored);
    QFETCH(bool, onAnchor);
    QFETCH(QStringList, offered);
    QFETCH(QString, chosen);
    QFETCH(int, left);

    m_scene->setLinkStyle(LinkStyle::Straight);
    const QPointF anchor = belowLink(80);
    m_link->setAnchors(anchored ? QList<QPointF>{anchor} : QList<QPointF>());
    // On the link, between its provided interface and where it goes next.
    const QPointF at =
        onAnchor ? anchor
                 : onPixel(*m_view, (m_link->provided().scenePos() + (anchored ? anchor : middle(*m_link))) / 2);

    QCOMPARE(chooseFromMenu(*m_view, at, chosen), offered);
    QCOMPARE(m_link->anchors().size(), left);
    QCOMPARE(m_link->anchors().contains(at), !onAnchor && left > 0);
    QCOMPARE(links(*m_scene).size(), 1);
}

// Dragging an anchor moves it, and its link follows; the components selected stay where they are.
void TestEditionScene::dragAnchor()
{
    m_scene->setLinkStyle(LinkStyle::Straight);
    ComponentItem* source = componentItem(*m_scene, QStringLiteral("Source1"));
    source->setSelected(true);
    const QPointF sourceAt = source->pos();
    const QPointF anchor = belowLink(80);
    m_link->setAnchors({anchor});

    const QPointF moved = anchor + QPointF(30, 40);
    drag(*m_view, anchor, moved);
    QCOMPARE(m_link->anchors(), QList<QPointF>{moved});
    QCOMPARE(points(m_link->path()),
             (QList<QPointF>{m_link->provided().scenePos(), moved, m_link->consumed().scenePos()}));
    QCOMPARE(source->pos(), sourceAt);
    QVERIFY(source->isSelected());
    QCOMPARE(m_scene->save().value(u"bindings").toArray().at(0).toObject().value(u"anchors").toArray(),
             (QJsonArray{QJsonObject{{QStringLiteral("x"), moved.x()}, {QStringLiteral("y"), moved.y()}}}));
}

// While the application runs, an anchor is neither moved, added nor removed: pressing it selects its link.
void TestEditionScene::anchorsWhileRunning()
{
    m_scene->setLinkStyle(LinkStyle::Straight);
    const QPointF anchor = belowLink(80);
    const QPointF onLink = (m_link->provided().scenePos() + anchor) / 2;
    m_link->setAnchors({anchor});
    QVERIFY(m_application->start());
    QCOMPARE(links(*m_scene).size(), 1);
    m_link = links(*m_scene).constFirst();
    QCOMPARE(m_link->anchors(), QList<QPointF>{anchor});

    drag(*m_view, anchor, anchor + QPointF(30, 40));
    QVERIFY(m_link->isSelected());
    doubleClick(*m_view, anchor);
    doubleClick(*m_view, onLink);
    QCOMPARE(m_link->anchors(), QList<QPointF>{anchor});
    m_application->stop();
}

// A project holds the anchors of a link in its binding, in their order, and gives them back once opened; a link without
// anchors is held as it was.
void TestEditionScene::anchorsSaved()
{
    QVERIFY(!m_scene->save().value(u"bindings").toArray().at(0).toObject().contains(u"anchors"));
    const QList<QPointF> anchors{belowLink(80), belowLink(80) + QPointF(60, 0)};
    m_link->setAnchors(anchors);
    const QJsonObject project = m_scene->save();
    const QJsonArray bindings = project.value(u"bindings").toArray();
    QCOMPARE(bindings.size(), 1);
    QCOMPARE(
        bindings.at(0).toObject().value(u"anchors").toArray(),
        (QJsonArray{QJsonObject{{QStringLiteral("x"), anchors.at(0).x()}, {QStringLiteral("y"), anchors.at(0).y()}},
                    QJsonObject{{QStringLiteral("x"), anchors.at(1).x()}, {QStringLiteral("y"), anchors.at(1).y()}}}));

    QCOMPARE(m_scene->load(project), QStringList()); // Nothing Core does not understand
    QCOMPARE(links(*m_scene).size(), 1);
    m_link = links(*m_scene).constFirst();
    QCOMPARE(m_link->anchors(), anchors);
    QCOMPARE(m_scene->save(), project);
}

// A link drawn again, as the interfaces of its component are shown again, goes through the same anchors; the anchors of
// a link whose interface goes go with it.
void TestEditionScene::anchorsOfALinkDrawnAgain()
{
    Nebula::Component* outlets = m_application->instantiate(testComponentId(QStringLiteral("Outlets")));
    Nebula::Component* sink = m_application->component(QStringLiteral("Sink1"));
    QVERIFY(outlets->setProperty("count", 2));
    QVERIFY(sink->bindInterface(QStringLiteral("objects"), *outlets, QStringLiteral("object1")));
    QVERIFY(sink->bindInterface(QStringLiteral("objects"), *outlets, QStringLiteral("object2")));
    QCOMPARE(m_scene->load(m_application->save()), QStringList());
    outlets = m_application->component(QStringLiteral("Outlets1"));
    const auto linkTo = [this](const QString& provided)
    {
        const QList<LinkItem*> shown = links(*m_scene);
        const auto found =
            std::ranges::find(shown, provided, [](const LinkItem* link) { return link->provided().name(); });
        return found != shown.end() ? *found : nullptr;
    };
    const QList<QPointF> first{{300, 400}, {340, 420}};
    const QList<QPointF> second{{300, 500}};
    linkTo(QStringLiteral("object1"))->setAnchors(first);
    linkTo(QStringLiteral("object2"))->setAnchors(second);

    QVERIFY(outlets->setProperty("count", 3)); // Which shows each of its interfaces again
    QCOMPARE(linkTo(QStringLiteral("object1"))->anchors(), first);
    QCOMPARE(linkTo(QStringLiteral("object2"))->anchors(), second);
    QCOMPARE(anchorItems(*m_scene).size(), 3);

    QVERIFY(outlets->setProperty("count", 1));
    QCOMPARE(linkTo(QStringLiteral("object2")), nullptr);
    QCOMPARE(linkTo(QStringLiteral("object1"))->anchors(), first);
    QVERIFY(outlets->setProperty("count", 2)); // Not bound again
    QCOMPARE(linkTo(QStringLiteral("object2")), nullptr);
    QCOMPARE(anchorItems(*m_scene).size(), 2);
}

// A link removed takes its anchors with it: bound again, its interfaces are linked without them.
void TestEditionScene::anchorsOfALinkRemoved()
{
    m_link->setAnchors({belowLink(80)});
    m_link->setSelected(true);
    QTest::keyClick(m_view.get(), Qt::Key_Delete);
    QCOMPARE(links(*m_scene).size(), 0);
    QVERIFY(anchorItems(*m_scene).isEmpty());

    const InterfaceItem* dot = interfaceItem(*m_scene, QStringLiteral("Sink1"), QStringLiteral("object"));
    QVERIFY(openBindMenu(*m_view, dot->scenePos(), QStringLiteral("Source1.object")).offered);
    QCOMPARE(links(*m_scene).size(), 1);
    QVERIFY(links(*m_scene).constFirst()->anchors().isEmpty());
    QVERIFY(anchorItems(*m_scene).isEmpty());
    QVERIFY(!m_scene->save().value(u"bindings").toArray().at(0).toObject().contains(u"anchors"));
}

void TestEditionScene::sideMenu_data()
{
    QTest::addColumn<QString>("component"); // Whose dot of object is Shift+right-clicked: Source1 provides it
    QTest::addColumn<QString>("chosen");    // The entry chosen; none when the menu is closed
    QTest::addColumn<bool>("onRight");      // Whether the dot is on the right edge then

    const QString source = QStringLiteral("Source1");
    const QString sink = QStringLiteral("Sink1");
    const QString left = QStringLiteral("Dot on the left edge");
    const QString right = QStringLiteral("Dot on the right edge");
    QTest::newRow("consumed, to the right edge") << sink << right << true;
    QTest::newRow("consumed, left where it is") << sink << left << false;
    QTest::newRow("provided, to the left edge") << source << left << false;
    QTest::newRow("provided, nothing chosen") << source << QString() << true;
}

// Shift+right-clicking the dot of an interface opens a menu of its own, which puts it on the left or the right edge of
// its component, the one it is on checked, as radio buttons: its name follows it, inside the component, and its link
// goes out of that edge.
void TestEditionScene::sideMenu()
{
    QFETCH(QString, component);
    QFETCH(QString, chosen);
    QFETCH(bool, onRight);

    m_scene->setLinkStyle(LinkStyle::RightAngled);
    const ComponentItem* item = componentItem(*m_scene, component);
    InterfaceItem* dot = interfaceItem(*m_scene, component, QStringLiteral("object"));
    const bool wasOnRight = dot->side() == InterfaceItem::Side::Right;
    const Menu menu = openMenu(*m_view, dot->scenePos(), chosen, Qt::ShiftModifier);
    QCOMPARE(menu.offered,
             (QStringList{QStringLiteral("Dot on the left edge"), QStringLiteral("Dot on the right edge")}));
    QCOMPARE(menu.checked, QStringList{menu.offered.at(wasOnRight ? 1 : 0)});
    QVERIFY(menu.radio);

    QCOMPARE(interfaceItem(*m_scene, component, QStringLiteral("object")), dot); // The same, moved
    QCOMPARE(dot->side(), side(onRight));
    QCOMPARE(dot->pos(), QPointF(dotX(onRight), dot->y()));
    QVERIFY(item->boundingRect().contains(dot->mapRectToParent(dot->nameRect())));
    QCOMPARE(links(*m_scene), QList<LinkItem*>{m_link}); // Still bound
    QCOMPARE(m_link->path(), LinkItem::route(LinkStyle::RightAngled, LinkItem::stopAt(m_link->provided()),
                                             LinkItem::stopAt(m_link->consumed())));
}

// Without Shift, the menu of a dot is the one of its component, which does not put it on another edge.
void TestEditionScene::noSideMenuWithoutShift()
{
    const InterfaceItem* dot = interfaceItem(*m_scene, QStringLiteral("Sink1"), QStringLiteral("object"));
    const QStringList offered = chooseFromMenu(*m_view, dot->scenePos(), QString());
    QVERIFY(offered.contains(QStringLiteral("Bind to")));
    QVERIFY(!offered.contains(QStringLiteral("Dot on the left edge")));
    QVERIFY(!offered.contains(QStringLiteral("Dot on the right edge")));
}

// While the application runs, Shift+right-clicking a dot opens no menu, and tells why.
void TestEditionScene::sideMenuWhileRunning()
{
    QVERIFY(m_application->start());
    const InterfaceItem* dot = interfaceItem(*m_scene, QStringLiteral("Sink1"), QStringLiteral("object"));
    QSignalSpy refused(m_scene.get(), &EditionScene::refused);
    // A menu opened is closed, for the test not to wait on it; nothing is told once the test is over.
    const QObject guard;
    bool opened = false;
    QTimer::singleShot(0, &guard,
                       [&opened]
                       {
                           if(QWidget* popup = QApplication::activePopupWidget())
                           {
                               opened = true;
                               popup->close();
                           }
                       });
    const QPoint position = m_view->mapFromScene(dot->scenePos());
    QContextMenuEvent event(QContextMenuEvent::Mouse, position, m_view->viewport()->mapToGlobal(position),
                            Qt::ShiftModifier);
    QApplication::sendEvent(m_view->viewport(), &event);
    QVERIFY(!opened);
    QCOMPARE(refused.size(), 1);
    QCOMPARE(dot->side(), InterfaceItem::Side::Left);
    m_application->stop();
}

// A project holds the edges the dots of the interfaces of a component are on, when one is not on the edge of its kind,
// and puts them there again once opened; an edge it does not know, or an interface the component does not have, is
// left out.
void TestEditionScene::sidesSaved()
{
    const QString source = QStringLiteral("Source1");
    const QString sink = QStringLiteral("Sink1");
    const QString object = QStringLiteral("object");
    QVERIFY(!componentJson(m_scene->save(), sink).contains(u"sides"));
    QVERIFY(!componentJson(m_scene->save(), source).contains(u"sides"));
    componentItem(*m_scene, sink)->setInterfaceSide(InterfaceItem::Kind::Consumed, object, InterfaceItem::Side::Right);
    componentItem(*m_scene, source)->setInterfaceSide(InterfaceItem::Kind::Provided, object, InterfaceItem::Side::Left);
    const QJsonObject project = m_scene->save();
    QCOMPARE(componentJson(project, sink).value(u"sides").toObject(),
             (QJsonObject{{QStringLiteral("consumed"), QJsonObject{{object, QStringLiteral("right")}}}}));
    QCOMPARE(componentJson(project, source).value(u"sides").toObject(),
             (QJsonObject{{QStringLiteral("provided"), QJsonObject{{object, QStringLiteral("left")}}}}));

    QCOMPARE(m_scene->load(project), QStringList()); // Nothing Core does not understand
    QCOMPARE(interfaceItem(*m_scene, sink, object)->side(), InterfaceItem::Side::Right);
    QCOMPARE(interfaceItem(*m_scene, source, object)->side(), InterfaceItem::Side::Left);
    QCOMPARE(links(*m_scene).size(), 1);
    m_link = links(*m_scene).constFirst();
    QCOMPARE(m_link->path(), LinkItem::route(LinkStyle::Curved, LinkItem::stopAt(m_link->provided()),
                                             LinkItem::stopAt(m_link->consumed())));
    QCOMPARE(m_scene->save(), project);

    // Back on the edge of its kind, it is held no longer.
    componentItem(*m_scene, sink)->setInterfaceSide(InterfaceItem::Kind::Consumed, object, InterfaceItem::Side::Left);
    QVERIFY(!componentJson(m_scene->save(), sink).contains(u"sides"));

    QJsonObject odd = project;
    QJsonArray components = odd.value(u"components").toArray();
    for(QJsonValueRef value : components)
    {
        QJsonObject component = value.toObject();
        component.insert(u"sides", QJsonObject{{QStringLiteral("consumed"),
                                                QJsonObject{{object, QStringLiteral("top")},
                                                            {QStringLiteral("missing"), QStringLiteral("right")}}}});
        value = component;
    }
    odd.insert(u"components", components);
    QCOMPARE(m_scene->load(odd), QStringList());
    QCOMPARE(interfaceItem(*m_scene, sink, object)->side(), InterfaceItem::Side::Left);
    QVERIFY(!componentJson(m_scene->save(), sink).contains(u"sides"));
    QVERIFY(!componentJson(m_scene->save(), source).contains(u"sides"));
}

// The dot of an interface stays on its edge while its component shows its interfaces again, as they change, and goes
// with its interface.
void TestEditionScene::sidesOfInterfacesShownAgain()
{
    Nebula::Component* outlets = m_application->instantiate(testComponentId(QStringLiteral("Outlets")));
    QVERIFY(outlets->setProperty("count", 3));
    QCOMPARE(m_scene->load(m_application->save()), QStringList());
    outlets = m_application->component(QStringLiteral("Outlets1"));
    ComponentItem* item = componentItem(*m_scene, QStringLiteral("Outlets1"));
    const auto sideOf = [item](const QString& name)
    { return item->interfaceItem(InterfaceItem::Kind::Provided, name)->side(); };
    item->setInterfaceSide(InterfaceItem::Kind::Provided, QStringLiteral("object1"), InterfaceItem::Side::Left);
    item->setInterfaceSide(InterfaceItem::Kind::Provided, QStringLiteral("object9"), InterfaceItem::Side::Left);
    QCOMPARE(item->interfaceItem(InterfaceItem::Kind::Provided, QStringLiteral("object9")), nullptr);

    QVERIFY(outlets->setProperty("count", 2)); // Which shows each of its interfaces again
    QCOMPARE(sideOf(QStringLiteral("object1")), InterfaceItem::Side::Left);
    QCOMPARE(item->interfaceItem(InterfaceItem::Kind::Provided, QStringLiteral("object1"))->x(), dotX(false));
    QCOMPARE(sideOf(QStringLiteral("object2")), InterfaceItem::Side::Right);
    QCOMPARE(
        componentJson(m_scene->save(), QStringLiteral("Outlets1")).value(u"sides").toObject(),
        (QJsonObject{{QStringLiteral("provided"), QJsonObject{{QStringLiteral("object1"), QStringLiteral("left")}}}}));

    QVERIFY(outlets->setProperty("count", 0));
    QVERIFY(outlets->setProperty("count", 1));
    QCOMPARE(sideOf(QStringLiteral("object1")), InterfaceItem::Side::Right);
    QVERIFY(!componentJson(m_scene->save(), QStringLiteral("Outlets1")).contains(u"sides"));
}

void TestEditionScene::exposeNextToTheDot_data()
{
    QTest::addColumn<QString>("component"); // Whose interface object is exposed: Source1 provides it
    QTest::addColumn<bool>("otherEdge");    // Whether its dot is on the other edge of its component
    QTest::addColumn<bool>("onRight");      // Whether its dot is on the right edge, and the stargate on that side

    QTest::newRow("provided, on the right edge") << QStringLiteral("Source1") << false << true;
    QTest::newRow("provided, on the left edge") << QStringLiteral("Source1") << true << false;
    QTest::newRow("consumed, on the left edge") << QStringLiteral("Sink1") << false << false;
    QTest::newRow("consumed, on the right edge") << QStringLiteral("Sink1") << true << true;
}

// Expose..., in the menu of the dot of an interface, adds the stargate next to its component on the edge the dot is
// on, the dot of the stargate facing it, which it keeps facing once its name, edited at once, is typed.
void TestEditionScene::exposeNextToTheDot()
{
    QFETCH(QString, component);
    QFETCH(bool, otherEdge);
    QFETCH(bool, onRight);

    ComponentItem* item = componentItem(*m_scene, component);
    item->moveBy(200, 0); // Its left edge in the view as well
    InterfaceItem* dot = interfaceItem(*m_scene, component, QStringLiteral("object"));
    if(otherEdge)
    {
        item->setInterfaceSide(dot->kind(), dot->name(), side(dot->side() == InterfaceItem::Side::Left));
    }
    QCOMPARE(dot->side(), side(onRight));
    QVERIFY(
        chooseFromMenu(*m_view, dot->scenePos(), QStringLiteral("Expose...")).contains(QStringLiteral("Expose...")));

    const ComponentItem* stargate = componentItem(*m_scene, QStringLiteral("Stargate1"));
    QVERIFY(stargate != nullptr);
    QCOMPARE(stargate->x() > item->x(), onRight);
    const InterfaceItem::Kind exposing =
        dot->kind() == InterfaceItem::Kind::Provided ? InterfaceItem::Kind::Consumed : InterfaceItem::Kind::Provided;
    const InterfaceItem* facing = stargate->interfaceItem(exposing, QStringLiteral("object"));
    QVERIFY(facing != nullptr);
    QCOMPARE(facing->side(), side(!onRight));

    const QList<QGraphicsItem*> children = stargate->childItems();
    const auto editor =
        std::ranges::find_if(children, [](const QGraphicsItem* child)
                             { return child->type() == QGraphicsTextItem::Type && child->isVisible(); });
    QVERIFY(editor != children.end());
    auto* nameEditor = static_cast<QGraphicsTextItem*>(*editor);
    nameEditor->setPlainText(QStringLiteral("renamed"));
    QKeyEvent enter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QVERIFY(m_scene->sendEvent(nameEditor, &enter));
    QCOMPARE(stargate->interfaceItem(exposing, QStringLiteral("object")), nullptr);
    const InterfaceItem* renamed = stargate->interfaceItem(exposing, QStringLiteral("renamed"));
    QVERIFY(renamed != nullptr);
    QCOMPARE(renamed->side(), side(!onRight));
    QCOMPARE(links(*m_scene).size(), component == QStringLiteral("Source1") ? 2 : 1); // Still bound to it
}

QTEST_MAIN(TestEditionScene)
#include "TestEditionScene.moc"
