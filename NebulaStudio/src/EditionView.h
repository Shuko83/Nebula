// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QCursor>
#include <QGraphicsView>
#include <optional>

// Shows the edition zone, at a zoom level that Ctrl and the mouse wheel change around the point under the cursor, and
// the + and - keys around the middle of the view, unless the name of an item is edited there: what is typed is then
// for it. Dragging with the middle button held moves the view, what is under the cursor following it. The zoom and
// the move only change what is seen, not the scene: they are not saved with the project. As the view grows or shrinks
// on its left or top side, a panel beside it hidden or shown, what it shows stays where it is on the screen: the panel
// uncovers or covers it. The item selected alone stays in view then, when it was: one just dropped where the panel
// comes back, say. It tells what follows it, a SceneOverview say, the scene it shows and what it shows of it.
class EditionView : public QGraphicsView
{
    Q_OBJECT

  public:
    explicit EditionView(QGraphicsScene* scene, QWidget* parent = nullptr);

    // How much the scene is enlarged: 1 shows it at its own size.
    [[nodiscard]] qreal zoom() const;
    // Shows the scene enlarged by zoom, kept between the smallest and the largest zoom levels, around the middle of the
    // view.
    void setZoom(qreal zoom);
    // One step closer, or further, around the middle of the view.
    void zoomIn();
    void zoomOut();
    // Shows the scene at its own size.
    void resetZoom();
    // Shows scene in place of the one it shows, rather than setScene(), which does not tell it: see sceneChanged().
    void showScene(QGraphicsScene* scene);
    // What it shows of its scene, in the coordinates of the scene, beyond its edges too when the view is larger than
    // the scene; empty without a scene.
    [[nodiscard]] QRectF shownArea() const;

  signals:
    void zoomChanged(qreal zoom);
    // It shows scene from now on, see showScene().
    void sceneChanged(QGraphicsScene* scene);
    // What it shows of its scene moved, grew or shrank, see shownArea(): it scrolled, zoomed, or was resized.
    void shownAreaChanged();

  protected:
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void scrollContentsBy(int horizontal, int vertical) override;

  private:
    // Scrolls the view by delta, in pixels of the viewport.
    void scrollBy(QPoint delta);
    // Zooms to zoom, what is under cursor staying there.
    void zoomAround(QPoint cursor, qreal zoom);
    // Zooms in or out when event is + or -, which it accepts then.
    void zoomByKey(QKeyEvent& event);
    // Ends the pan once the middle button is released, see endPanIfReleased(); while it goes on, accepts event, which
    // is not for the scene then. Returns whether it goes on.
    bool isPanning(QMouseEvent& event);
    // Moves the view for what was under the cursor to stay under it, now at at.
    void pan(QPoint at);
    void startPan(QPoint at);
    // Ends the pan once the middle button is no longer held with event, its release or not: the release is lost when
    // another window takes the mouse meanwhile.
    void endPanIfReleased(const QMouseEvent& event);
    // Scrolls the view by as far as the viewport moved in its window since it was last resized: what it shows stays
    // where it is on the screen.
    void keepInPlace();
    // Keeps the item selected alone in view, when it was entirely in shown, what the view showed before.
    void keepSelectionShown(const QRectF& shown);

    std::optional<QPoint> m_panFrom; // Where the cursor was when the view last moved, while the middle button pans it
    QCursor m_cursor;                // The one of the viewport before the pan
    std::optional<QPoint> m_origin;  // Of the viewport in its window, when it was last resized
};
