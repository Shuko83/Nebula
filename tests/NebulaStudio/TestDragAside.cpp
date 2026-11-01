// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QApplication>
#include <QCursor>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QHBoxLayout>
#include <QMimeData>
#include <QTest>
#include <QVBoxLayout>
#include <QWindow>
#include <memory>

#include "DragAside.h"
#include "DraggableView.h"

namespace
{

// The window: the panel on its left, what it is beside on the rest.
constexpr QSize WindowSize(600, 300);
constexpr int PanelWidth = 200;
// In the window: in the panel, and beside it.
constexpr QPoint InPanel(100, 150);
constexpr QPoint BesidePanel(400, 150);

// Has the window of widget tell that what is dragged moved to at, in the window, as the window system would.
void dragTo(const QWidget& widget, QPoint at)
{
    const QMimeData data;
    QDragMoveEvent event(at, Qt::CopyAction, &data, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(widget.windowHandle(), &event);
}

// Has the window of widget tell that what is dragged left it, or that the drag was given up over it.
void leave(const QWidget& widget)
{
    QDragLeaveEvent event;
    QApplication::sendEvent(widget.windowHandle(), &event);
}

} // namespace

// What sets a widget aside while rows dragged from its lists are out of it, DragAside, and the lists that tell when
// they are dragged, DraggableView.
class TestDragAside : public QObject
{
    Q_OBJECT

  private slots:
    void init();
    void cleanup();

    void listDragsOnly();
    void asideOnceOut_data();
    void asideOnceOut();
    void leavingTheWindow_data();
    void leavingTheWindow();
    void notWhileInactive();
    void notForAnotherList();
    void notForAListForgotten();
    void notShownOnceTakenAway();

  private:
    std::unique_ptr<QWidget> m_window;
    QWidget* m_panel = nullptr;         // On the left of m_window
    DraggableView* m_list = nullptr;    // In m_panel
    QWidget* m_beside = nullptr;        // On the right of m_panel, which takes its room
    std::unique_ptr<DragAside> m_aside; // Of m_panel, following m_list
};

void TestDragAside::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
    m_window = std::make_unique<QWidget>();
    auto* layout = new QHBoxLayout(m_window.get());
    layout->setContentsMargins({});
    layout->setSpacing(0);
    // The platform of the tests, offscreen, warns as the size of a window is constrained.
    layout->setSizeConstraint(QLayout::SetNoConstraint);
    m_panel = new QWidget;
    m_panel->setFixedWidth(PanelWidth);
    m_list = new DraggableView;
    (new QVBoxLayout(m_panel))->addWidget(m_list);
    layout->addWidget(m_panel);
    m_beside = new QWidget;
    layout->addWidget(m_beside, 1);
    m_window->resize(WindowSize);
    m_window->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_window.get()));

    m_aside = std::make_unique<DragAside>(*m_panel);
    m_aside->setActive(true);
    m_aside->follow(*m_panel);
}

void TestDragAside::cleanup()
{
    m_aside.reset();
    m_window.reset();
}

void TestDragAside::listDragsOnly()
{
    QVERIFY(m_list->dragEnabled());
    QCOMPARE(m_list->dragDropMode(), QAbstractItemView::DragOnly);
    QVERIFY(!m_list->acceptDrops());
}

void TestDragAside::asideOnceOut_data()
{
    // Where what is dragged goes, in the window, and whether the panel is set aside then.
    QTest::addColumn<QList<QPoint>>("moves");
    QTest::addColumn<bool>("aside");

    QTest::newRow("in it") << QList<QPoint>{InPanel} << false;
    QTest::newRow("on its right edge") << QList<QPoint>{{PanelWidth - 1, 150}} << false;
    QTest::newRow("beside it") << QList<QPoint>{InPanel, BesidePanel} << true;
    QTest::newRow("back where it was") << QList<QPoint>{BesidePanel, InPanel} << true;
}

void TestDragAside::asideOnceOut()
{
    QFETCH(QList<QPoint>, moves);
    QFETCH(bool, aside);

    emit m_list->dragStarted();
    for(const QPoint at : moves)
    {
        dragTo(*m_window, at);
    }
    QCOMPARE(m_panel->isHidden(), aside);
    // What it is beside has its room.
    QTRY_COMPARE(m_beside->width(), aside ? WindowSize.width() : WindowSize.width() - PanelWidth);

    emit m_list->dragFinished();
    QVERIFY(m_panel->isVisible());
    QTRY_COMPARE(m_beside->width(), WindowSize.width() - PanelWidth);
    // The window is no longer watched.
    dragTo(*m_window, BesidePanel);
    QVERIFY(m_panel->isVisible());
}

void TestDragAside::leavingTheWindow_data()
{
    // Where the cursor is, in the window, as the window tells that what is dragged left it.
    QTest::addColumn<QPoint>("cursor");
    QTest::addColumn<bool>("aside");

    QTest::newRow("out of the window") << QPoint(WindowSize.width() + 100, 150) << true;
    QTest::newRow("in it: the drag is given up") << InPanel << false;
}

void TestDragAside::leavingTheWindow()
{
    QFETCH(QPoint, cursor);
    QFETCH(bool, aside);

    emit m_list->dragStarted();
    QCursor::setPos(m_window->mapToGlobal(cursor));
    leave(*m_window);
    QCOMPARE(m_panel->isHidden(), aside);
    emit m_list->dragFinished();
    QVERIFY(m_panel->isVisible());
}

void TestDragAside::notWhileInactive()
{
    m_aside->setActive(false);
    QVERIFY(!m_aside->isActive());
    emit m_list->dragStarted();
    dragTo(*m_window, BesidePanel);
    QVERIFY(m_panel->isVisible());
    emit m_list->dragFinished();
    QVERIFY(m_panel->isVisible());
}

void TestDragAside::notForAnotherList()
{
    auto* other = new DraggableView(m_beside);
    emit other->dragStarted();
    dragTo(*m_window, BesidePanel);
    QVERIFY(m_panel->isVisible());
    emit other->dragFinished();
}

void TestDragAside::notForAListForgotten()
{
    m_aside->forget(*m_panel);
    emit m_list->dragStarted();
    dragTo(*m_window, BesidePanel);
    QVERIFY(m_panel->isVisible());
    emit m_list->dragFinished();
}

void TestDragAside::notShownOnceTakenAway()
{
    emit m_list->dragStarted();
    dragTo(*m_window, BesidePanel);
    QVERIFY(m_panel->isHidden());
    // Taken from the window while it is set aside: shown, it would be a window of its own.
    const std::unique_ptr<QWidget> taken(m_panel);
    m_panel->setParent(nullptr);
    emit m_list->dragFinished();
    QVERIFY(m_panel->isHidden());
}

QTEST_MAIN(TestDragAside)
#include "TestDragAside.moc"
