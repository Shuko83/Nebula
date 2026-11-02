// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QApplication>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QImage>
#include <QLineF>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTest>
#include <memory>

#include "EditionView.h"
#include "SceneOverview.h"

namespace
{

// The scene of NebulaStudio, and a smaller one, a galaxy opened say.
const QRectF SceneRect(0, 0, 4000, 3000);
const QRectF GalaxyRect(0, 0, 1000, 500);
// The view, far from the edges of the scene, and the overview beside it, in the proportions of the scene.
constexpr QSize ViewSize(400, 300);
constexpr QPoint Scrolled(1000, 1000);
constexpr QSize OverviewSize(240, 180);
// The margin fitInView() leaves around what it fits, on each side.
constexpr qreal FitMargin = 2;

// The values of the scroll bars of view.
QPoint scrolled(const QGraphicsView& view)
{
    return {view.horizontalScrollBar()->value(), view.verticalScrollBar()->value()};
}

// Sends overview the mouse event type at at, a position in its viewport, as the window system would.
void send(SceneOverview& overview, QEvent::Type type, QPoint at, Qt::MouseButton button, Qt::MouseButtons buttons)
{
    QMouseEvent event(type, at, overview.viewport()->mapToGlobal(at), button, buttons, Qt::NoModifier);
    QApplication::sendEvent(overview.viewport(), &event);
}

// Clicks overview at at with button.
void click(SceneOverview& overview, QPoint at, Qt::MouseButton button)
{
    send(overview, QEvent::MouseButtonPress, at, button, button);
    send(overview, QEvent::MouseButtonRelease, at, button, Qt::NoButton);
}

// Where rect of the scene of overview is in its viewport.
QRectF shownIn(const SceneOverview& overview, const QRectF& rect)
{
    return overview.mapFromScene(rect).boundingRect();
}

// The color overview paints at at, in its viewport.
QColor colorAt(SceneOverview& overview, QPoint at)
{
    return overview.viewport()->grab().toImage().pixelColor(at);
}

} // namespace

// The overview of the edition zone, SceneOverview: the whole scene of its view, what the view shows framed, and the
// view moved from it.
class TestSceneOverview : public QObject
{
    Q_OBJECT

  private slots:
    void init();
    void cleanup();

    void wholeScene_data();
    void wholeScene();
    void followsTheSceneShown();
    void framesWhatTheViewShows();
    void showsInTheView_data();
    void showsInTheView();
    void dragShowsInTheView();
    void otherButtonsDoNotMoveTheView_data();
    void otherButtonsDoNotMoveTheView();
    void sceneLeftAlone();

  private:
    // An overview of m_view, of size, shown.
    std::unique_ptr<SceneOverview> showOverview(QSize size);

    std::unique_ptr<QGraphicsScene> m_scene;
    std::unique_ptr<EditionView> m_view;
    std::unique_ptr<SceneOverview> m_overview; // Of OverviewSize
};

void TestSceneOverview::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
    m_scene = std::make_unique<QGraphicsScene>(SceneRect);
    m_view = std::make_unique<EditionView>(m_scene.get());
    m_view->setAlignment(Qt::AlignLeft | Qt::AlignTop); // As the edition zone shows it
    m_view->resize(ViewSize);
    m_view->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_view.get()));
    m_view->horizontalScrollBar()->setValue(Scrolled.x());
    m_view->verticalScrollBar()->setValue(Scrolled.y());
    m_overview = showOverview(OverviewSize);
    QVERIFY(m_overview != nullptr);
}

void TestSceneOverview::cleanup()
{
    m_overview.reset(); // Before the view it follows
    m_view.reset();
    m_scene.reset();
}

std::unique_ptr<SceneOverview> TestSceneOverview::showOverview(QSize size)
{
    auto overview = std::make_unique<SceneOverview>(*m_view);
    overview->resize(size);
    overview->show();
    return QTest::qWaitForWindowExposed(overview.get()) ? std::move(overview) : nullptr;
}

void TestSceneOverview::wholeScene_data()
{
    QTest::addColumn<QSize>("size");

    QTest::newRow("in the proportions of the scene") << OverviewSize;
    QTest::newRow("wider") << QSize(400, 180);
    QTest::newRow("taller") << QSize(240, 400);
}

// The whole scene fits in the overview, in its middle, as large as it can be.
void TestSceneOverview::wholeScene()
{
    QFETCH(QSize, size);

    const std::unique_ptr<SceneOverview> overview = showOverview(size);
    QVERIFY(overview != nullptr);
    const QRectF viewport = overview->viewport()->rect();
    const QRectF shown = shownIn(*overview, SceneRect);
    QVERIFY(viewport.adjusted(FitMargin, FitMargin, -FitMargin, -FitMargin).contains(shown.adjusted(1, 1, -1, -1)));
    QVERIFY(qAbs(shown.center().x() - viewport.center().x()) <= 1);
    QVERIFY(qAbs(shown.center().y() - viewport.center().y()) <= 1);
    QVERIFY(shown.width() >= viewport.width() - 2 * FitMargin - 1 ||
            shown.height() >= viewport.height() - 2 * FitMargin - 1);
    QCOMPARE(scrolled(*overview), QPoint());
}

// The view shows a galaxy opened, then the application again: the overview shows each one whole.
void TestSceneOverview::followsTheSceneShown()
{
    QGraphicsScene galaxy(GalaxyRect);
    m_view->showScene(&galaxy);
    QCOMPARE(m_overview->scene(), &galaxy);
    const QRectF shown = shownIn(*m_overview, GalaxyRect);
    QVERIFY(QRectF(m_overview->viewport()->rect()).contains(shown.adjusted(1, 1, -1, -1)));
    QVERIFY(shown.width() >= m_overview->viewport()->width() - 2 * FitMargin - 1);

    m_view->showScene(m_scene.get());
    QCOMPARE(m_overview->scene(), m_scene.get());
    QVERIFY(shownIn(*m_overview, SceneRect).width() >= m_overview->viewport()->width() - 2 * FitMargin - 1);
}

// What the view shows is filled with the color of the selection, translucent, and follows the view as it scrolls.
void TestSceneOverview::framesWhatTheViewShows()
{
    const QPoint inFrame = m_overview->mapFromScene(m_view->shownArea().center());
    const QPoint outside = m_overview->mapFromScene(QPointF(200, 2500)); // Before as after
    const QColor scene = colorAt(*m_overview, outside);
    QVERIFY(colorAt(*m_overview, inFrame) != scene);

    m_view->horizontalScrollBar()->setValue(3000);
    m_view->verticalScrollBar()->setValue(2200);
    QCOMPARE(colorAt(*m_overview, inFrame), scene);
    QVERIFY(colorAt(*m_overview, m_overview->mapFromScene(m_view->shownArea().center())) != scene);
}

void TestSceneOverview::showsInTheView_data()
{
    // Where the overview is clicked, in the scene, and where the view then shows it in its middle, or, at the edges of
    // the scene, what it shows from its top left corner.
    QTest::addColumn<QPointF>("at");
    QTest::addColumn<bool>("inTheMiddle");

    QTest::newRow("the middle of the scene") << QPointF(2000, 1500) << true;
    QTest::newRow("lower left") << QPointF(800, 2400) << true;
    QTest::newRow("upper right") << QPointF(3600, 400) << true;
    QTest::newRow("the top left corner") << QPointF(50, 50) << false;
}

void TestSceneOverview::showsInTheView()
{
    QFETCH(QPointF, at);
    QFETCH(bool, inTheMiddle);

    const QPoint clicked = m_overview->mapFromScene(at);
    const QPointF under = m_overview->mapToScene(clicked);
    click(*m_overview, clicked, Qt::LeftButton);

    if(inTheMiddle)
    {
        QVERIFY(QLineF(m_view->shownArea().center(), under).length() <= 1);
    }
    else
    {
        QCOMPARE(scrolled(*m_view), QPoint());
    }
}

// Dragging in the overview has the view follow the cursor.
void TestSceneOverview::dragShowsInTheView()
{
    const QPoint from = m_overview->mapFromScene(QPointF(1000, 1000));
    const QPoint to = m_overview->mapFromScene(QPointF(3000, 2000));
    send(*m_overview, QEvent::MouseButtonPress, from, Qt::LeftButton, Qt::LeftButton);
    send(*m_overview, QEvent::MouseMove, to, Qt::NoButton, Qt::LeftButton);
    QVERIFY(QLineF(m_view->shownArea().center(), m_overview->mapToScene(to)).length() <= 1);
    send(*m_overview, QEvent::MouseButtonRelease, to, Qt::LeftButton, Qt::NoButton);

    // No longer once the button is released.
    const QPoint moved = scrolled(*m_view);
    send(*m_overview, QEvent::MouseMove, from, Qt::NoButton, Qt::NoButton);
    QCOMPARE(scrolled(*m_view), moved);
}

void TestSceneOverview::otherButtonsDoNotMoveTheView_data()
{
    QTest::addColumn<Qt::MouseButton>("button");

    QTest::newRow("right") << Qt::RightButton;
    QTest::newRow("middle") << Qt::MiddleButton;
}

void TestSceneOverview::otherButtonsDoNotMoveTheView()
{
    QFETCH(Qt::MouseButton, button);

    click(*m_overview, m_overview->mapFromScene(QPointF(3000, 2000)), button);
    QCOMPARE(scrolled(*m_view), Scrolled);
}

// Nothing in the scene is selected, moved or given the keys from the overview.
void TestSceneOverview::sceneLeftAlone()
{
    QGraphicsRectItem* pressed = m_scene->addRect(-100, -100, 200, 200);
    pressed->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsFocusable);
    pressed->setPos(2000, 1500);
    QGraphicsRectItem* selected = m_scene->addRect(-100, -100, 200, 200);
    selected->setFlags(QGraphicsItem::ItemIsSelectable);
    selected->setPos(500, 500);
    selected->setSelected(true);

    const QPoint at = m_overview->mapFromScene(pressed->pos());
    send(*m_overview, QEvent::MouseButtonPress, at, Qt::LeftButton, Qt::LeftButton);
    send(*m_overview, QEvent::MouseMove, at + QPoint(10, 10), Qt::NoButton, Qt::LeftButton);
    send(*m_overview, QEvent::MouseButtonRelease, at + QPoint(10, 10), Qt::LeftButton, Qt::NoButton);

    QVERIFY(!pressed->isSelected());
    QCOMPARE(pressed->pos(), QPointF(2000, 1500));
    QVERIFY(selected->isSelected());
    QCOMPARE(m_scene->focusItem(), nullptr);
    QCOMPARE(m_overview->focusPolicy(), Qt::NoFocus);
}

QTEST_MAIN(TestSceneOverview)
#include "TestSceneOverview.moc"
