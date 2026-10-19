// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QApplication>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QScrollBar>
#include <QSignalSpy>
#include <QTest>
#include <memory>

#include "EditionView.h"

namespace
{

// The scene of NebulaStudio, larger than the view both ways.
const QRectF SceneRect(0, 0, 4000, 3000);
// Where the view starts, far from the edges of the scene.
constexpr QPoint Scrolled(1000, 1000);
// In the viewport: where the pans start, and where the item is.
constexpr QPoint From(200, 150);
constexpr QPoint ItemAt(100, 100);

// The values of the scroll bars of view.
QPoint scrolled(const EditionView& view)
{
    return {view.horizontalScrollBar()->value(), view.verticalScrollBar()->value()};
}

// Sends view the mouse event type at at, a position in its viewport, as the window system would.
void send(EditionView& view, QEvent::Type type, QPoint at, Qt::MouseButton button, Qt::MouseButtons buttons)
{
    QMouseEvent event(type, at, view.viewport()->mapToGlobal(at), button, buttons, Qt::NoModifier);
    QApplication::sendEvent(view.viewport(), &event);
}

} // namespace

// The view of the edition zone, EditionView: its zoom, and the middle button that pans it.
class TestEditionView : public QObject
{
    Q_OBJECT

  private slots:
    void init();
    void cleanup();

    void setZoom_data();
    void setZoom();
    void zoomSteps();
    void pan_data();
    void pan();
    void otherButtonsDoNotPan_data();
    void otherButtonsDoNotPan();
    void panLeavesTheScene_data();
    void panLeavesTheScene();
    void lostRelease_data();
    void lostRelease();
    void panKeepsItsCursor();

  private:
    std::unique_ptr<QGraphicsScene> m_scene;
    std::unique_ptr<EditionView> m_view;
    QGraphicsRectItem* m_item = nullptr; // Selectable and movable, at ItemAt in the viewport
};

void TestEditionView::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
    m_scene = std::make_unique<QGraphicsScene>(SceneRect);
    m_view = std::make_unique<EditionView>(m_scene.get());
    // As MainWindow shows it.
    m_view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_view->setDragMode(QGraphicsView::RubberBandDrag);
    m_view->resize(400, 300);
    m_view->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_view.get()));
    m_view->horizontalScrollBar()->setValue(Scrolled.x());
    m_view->verticalScrollBar()->setValue(Scrolled.y());

    m_item = m_scene->addRect(-50, -50, 100, 100);
    m_item->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
    // As the items of NebulaStudio: the viewport then gets the moves of the mouse without a button held.
    m_item->setAcceptHoverEvents(true);
    m_item->setPos(m_view->mapToScene(ItemAt));
}

void TestEditionView::cleanup()
{
    m_view.reset();
    m_scene.reset();
}

void TestEditionView::setZoom_data()
{
    QTest::addColumn<qreal>("zoom");
    QTest::addColumn<qreal>("shown");

    QTest::newRow("closer") << qreal(2) << qreal(2);
    QTest::newRow("further") << qreal(0.5) << qreal(0.5);
    QTest::newRow("largest") << qreal(4) << qreal(4);
    QTest::newRow("beyond the largest") << qreal(10) << qreal(4);
    QTest::newRow("smallest") << qreal(0.25) << qreal(0.25);
    QTest::newRow("below the smallest") << qreal(0.1) << qreal(0.25);
    QTest::newRow("unchanged") << qreal(1) << qreal(1);
}

void TestEditionView::setZoom()
{
    QFETCH(qreal, zoom);
    QFETCH(qreal, shown);

    QSignalSpy changed(m_view.get(), &EditionView::zoomChanged);
    m_view->setZoom(zoom);

    QCOMPARE(m_view->zoom(), shown);
    if(qFuzzyCompare(shown, qreal(1)))
    {
        QCOMPARE(changed.count(), 0);
    }
    else
    {
        QCOMPARE(changed.count(), 1);
        QCOMPARE(changed.first().first().toReal(), shown);
    }
}

void TestEditionView::zoomSteps()
{
    m_view->zoomIn();
    QCOMPARE(m_view->zoom(), qreal(1.25));
    m_view->zoomIn();
    QCOMPARE(m_view->zoom(), qreal(1.5625));
    m_view->zoomOut();
    QCOMPARE(m_view->zoom(), qreal(1.25));
    m_view->resetZoom();
    QCOMPARE(m_view->zoom(), qreal(1));
}

void TestEditionView::pan_data()
{
    // Where the cursor goes, the middle button held since From, and where the view ends.
    QTest::addColumn<QList<QPoint>>("moves");
    QTest::addColumn<QPoint>("scroll");

    QTest::newRow("right") << QList<QPoint>{From + QPoint(60, 0)} << Scrolled - QPoint(60, 0);
    QTest::newRow("up") << QList<QPoint>{From + QPoint(0, -50)} << Scrolled + QPoint(0, 50);
    QTest::newRow("several moves") << QList<QPoint>{From + QPoint(10, 10), From + QPoint(30, 40),
                                                    From + QPoint(-20, -10)}
                                   << Scrolled + QPoint(20, 10);
    QTest::newRow("back where it started") << QList<QPoint>{From + QPoint(100, 100), From} << Scrolled;
    QTest::newRow("out of the view") << QList<QPoint>{From + QPoint(600, 500)} << Scrolled - QPoint(600, 500);
    QTest::newRow("beyond the edges of the scene") << QList<QPoint>{From + QPoint(1500, 1500)} << QPoint(0, 0);
}

void TestEditionView::pan()
{
    QFETCH(QList<QPoint>, moves);
    QFETCH(QPoint, scroll);

    send(*m_view, QEvent::MouseButtonPress, From, Qt::MiddleButton, Qt::MiddleButton);
    QCOMPARE(m_view->viewport()->cursor().shape(), Qt::ClosedHandCursor);
    for(const QPoint at : moves)
    {
        send(*m_view, QEvent::MouseMove, at, Qt::NoButton, Qt::MiddleButton);
    }
    send(*m_view, QEvent::MouseButtonRelease, moves.last(), Qt::MiddleButton, Qt::NoButton);

    QCOMPARE(scrolled(*m_view), scroll);
    QCOMPARE(m_view->viewport()->cursor().shape(), Qt::ArrowCursor);
    QVERIFY(m_view->isInteractive());
}

void TestEditionView::otherButtonsDoNotPan_data()
{
    // Pressed in this order at From, then held while the cursor moves.
    QTest::addColumn<QList<Qt::MouseButton>>("buttons");

    QTest::newRow("left") << QList<Qt::MouseButton>{Qt::LeftButton};
    QTest::newRow("right") << QList<Qt::MouseButton>{Qt::RightButton};
    QTest::newRow("middle after the left one") << QList<Qt::MouseButton>{Qt::LeftButton, Qt::MiddleButton};
}

void TestEditionView::otherButtonsDoNotPan()
{
    QFETCH(QList<Qt::MouseButton>, buttons);

    Qt::MouseButtons held;
    for(const Qt::MouseButton button : buttons)
    {
        held |= button;
        send(*m_view, QEvent::MouseButtonPress, From, button, held);
    }
    QCOMPARE(m_view->viewport()->cursor().shape(), Qt::ArrowCursor);
    send(*m_view, QEvent::MouseMove, From + QPoint(60, 40), Qt::NoButton, held);
    for(auto button = buttons.crbegin(); button != buttons.crend(); ++button)
    {
        held &= ~*button;
        send(*m_view, QEvent::MouseButtonRelease, From + QPoint(60, 40), *button, held);
    }

    QCOMPARE(scrolled(*m_view), Scrolled);
}

void TestEditionView::panLeavesTheScene_data()
{
    QTest::addColumn<QPoint>("at");

    QTest::newRow("on the background") << From;
    QTest::newRow("on an item") << ItemAt;
}

void TestEditionView::panLeavesTheScene()
{
    QFETCH(QPoint, at);

    m_item->setSelected(true);
    const QPointF position = m_item->pos();
    // A press on the background would clear the selection, were it for the scene.
    send(*m_view, QEvent::MouseButtonPress, at, Qt::MiddleButton, Qt::MiddleButton);
    QCOMPARE(m_scene->mouseGrabberItem(), nullptr);
    send(*m_view, QEvent::MouseMove, at + QPoint(60, 40), Qt::NoButton, Qt::MiddleButton);
    send(*m_view, QEvent::MouseButtonRelease, at + QPoint(60, 40), Qt::MiddleButton, Qt::NoButton);

    QVERIFY(m_item->isSelected());
    QCOMPARE(m_item->pos(), position);
}

void TestEditionView::lostRelease_data()
{
    // What comes once the release of the middle button went to another window, and whether the view pans then.
    QTest::addColumn<QEvent::Type>("type");
    QTest::addColumn<Qt::MouseButton>("button");
    QTest::addColumn<bool>("panning");

    QTest::newRow("a move") << QEvent::MouseMove << Qt::NoButton << false;
    QTest::newRow("a press of another button") << QEvent::MouseButtonPress << Qt::LeftButton << false;
    QTest::newRow("a press of the middle button") << QEvent::MouseButtonPress << Qt::MiddleButton << true;
}

void TestEditionView::lostRelease()
{
    QFETCH(QEvent::Type, type);
    QFETCH(Qt::MouseButton, button);
    QFETCH(bool, panning);

    const QPoint at = From + QPoint(60, 40);
    send(*m_view, QEvent::MouseButtonPress, From, Qt::MiddleButton, Qt::MiddleButton);
    send(*m_view, type, at, button, button);
    QCOMPARE(m_view->viewport()->cursor().shape() == Qt::ClosedHandCursor, panning);
    if(type == QEvent::MouseButtonPress)
    {
        send(*m_view, QEvent::MouseButtonRelease, at, button, Qt::NoButton);
    }

    QCOMPARE(scrolled(*m_view), Scrolled);
    QCOMPARE(m_view->viewport()->cursor().shape(), Qt::ArrowCursor);
    QVERIFY(m_view->isInteractive());
    // The scene has the mouse again: a click on the background clears the selection.
    m_item->setSelected(true);
    send(*m_view, QEvent::MouseButtonPress, From, Qt::LeftButton, Qt::LeftButton);
    send(*m_view, QEvent::MouseButtonRelease, From, Qt::LeftButton, Qt::NoButton);
    QVERIFY(!m_item->isSelected());
}

void TestEditionView::panKeepsItsCursor()
{
    m_item->setCursor(Qt::CrossCursor);
    // The view keeps the last move of the mouse, which it replays to the scene at each scroll.
    send(*m_view, QEvent::MouseMove, From, Qt::NoButton, Qt::NoButton);
    QCOMPARE(m_view->viewport()->cursor().shape(), Qt::ArrowCursor);

    send(*m_view, QEvent::MouseButtonPress, From, Qt::MiddleButton, Qt::MiddleButton);
    // The item passes where the cursor was before the pan.
    send(*m_view, QEvent::MouseMove, From + (From - ItemAt), Qt::NoButton, Qt::MiddleButton);
    QCOMPARE(m_view->viewport()->cursor().shape(), Qt::ClosedHandCursor);
    send(*m_view, QEvent::MouseButtonRelease, From + (From - ItemAt), Qt::MiddleButton, Qt::NoButton);
    QCOMPARE(m_view->viewport()->cursor().shape(), Qt::ArrowCursor);
}

QTEST_MAIN(TestEditionView)
#include "TestEditionView.moc"
