// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QGraphicsView>

class EditionView;

// The whole scene an EditionView shows, small: at the zoom that fits it in the overview, without its grid, which would
// only blur it there, its area on the background of the window. What the view shows of it is framed in the color of
// the selection. Pressing the overview, and dragging in it, has the view show what is under the cursor in its middle:
// nothing in the scene is selected, moved or edited from it, and it does not take the keys. It follows the view: the
// scene it shows, a galaxy opened say, and what it shows of it as it scrolls, zooms or is resized. It is deleted
// before the view.
class SceneOverview : public QGraphicsView
{
    Q_OBJECT

  public:
    explicit SceneOverview(EditionView& view, QWidget* parent = nullptr);

    // As small as a panel beside the view, in the proportions of its scene.
    [[nodiscard]] QSize sizeHint() const override;

  protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void drawForeground(QPainter* painter, const QRectF& rect) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

  private:
    // Shows the scene the view shows, and what it shows of it, from now on.
    void followView();
    // Shows scene, the one the view shows from now on, whole.
    void showScene(QGraphicsScene* scene);
    // Fits the whole scene in the overview.
    void fitScene();
    // Has the view show in its middle the point of the scene at at, in the viewport of the overview.
    void showInView(QPoint at);

    EditionView& m_view;
};
