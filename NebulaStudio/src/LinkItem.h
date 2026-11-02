// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QCoreApplication>
#include <QGraphicsItem>
#include <QList>
#include <QPainterPath>
#include <QPointF>
#include <QString>
#include <functional>

#include "LinkStyle.h"

class AnchorItem;
class InterfaceItem;

namespace Nebula
{
class Component;
} // namespace Nebula

// An interface a component consumes bound to the one another component provides, in the edition zone: a line between
// their dots, drawn in the style it is given, which follows them, out of the edge of its component each dot is on.
// Clicking it selects it, which removing it needs, see EditionScene; the rubber band of a view does not: a link that
// crosses it would go with the components it selects. Hovered, it is drawn thicker, and its tooltip tells what it
// binds; selected, in the color of the selection, as a component is, over the other links.
// It can be made to go through points of the edition zone, its anchors, in their order, wherever its components are:
// see AnchorItem. Its line goes from each of them to the next one in its style, as it goes from one dot to the other,
// through each of them the way from the point before it to the one after it.
// The links of a same provided interface can go through the same points, each through the same ones before them:
// their line is one up to where they part, a junction, through which each goes the way from the point before it. That
// line is drawn once, by the first link through its last point; clicking or hovering it is clicking or hovering each
// link that shares it.
class LinkItem : public QGraphicsItem
{
    Q_DECLARE_TR_FUNCTIONS(LinkItem)

  public:
    // For qgraphicsitem_cast.
    enum
    {
        Type = UserType + 3
    };

    // What a link binds: consumed, an interface of consumer, to provided, one of provider. It tells the link a link
    // drawn again replaces, as the interfaces of their components are shown again.
    struct Ends
    {
        const Nebula::Component* consumer = nullptr;
        QString consumed;
        const Nebula::Component* provider = nullptr;
        QString provided;

        // For a std::map.
        [[nodiscard]] bool operator<(const Ends& other) const;
    };

    // A point the line of a link goes through, in the scene, and the way it goes there, horizontally: rightwards, as
    // out of the right edge of a provider or into the left edge of a consumer, or leftwards.
    struct Stop
    {
        QPointF point;
        bool leftwards = false;
    };

    // Not a child of them, but deleted with the first one deleted. Drawn in style until it is given another one.
    // anchored is called with it whenever its anchors change, once it is built: given, added, moved or removed.
    LinkItem(InterfaceItem& provided, InterfaceItem& consumed, LinkStyle style,
             std::function<void(const LinkItem&)> anchored = {});
    // Leaves its anchors: one no other link goes through goes with it.
    ~LinkItem() override;

    // The line of a link in style from one stop to the next one: from the dot of a provided interface to the dot of a
    // consumed one, say, see stopAt().
    [[nodiscard]] static QPainterPath route(LinkStyle style, const Stop& from, const Stop& to);
    // Where a link starts or ends at the dot of dot: out of the edge of its component it is on, for a provided
    // interface, into it for a consumed one.
    [[nodiscard]] static Stop stopAt(const InterfaceItem& dot);
    // Has no link go through anchor any longer, which deletes it: each goes from the point before it to the one after
    // it.
    static void removeAnchor(AnchorItem& anchor);
    // Has the links through junction go through neither it nor the points before it any longer, which deletes it: each
    // goes from its provided interface again, through its own points after it.
    static void removeJunction(AnchorItem& junction);

    [[nodiscard]] int type() const override;
    // The one it is drawn from.
    [[nodiscard]] InterfaceItem& provided() const;
    // The one it is drawn to, whose component is bound to the other one.
    [[nodiscard]] InterfaceItem& consumed() const;
    // What it binds, as its interfaces are named now.
    [[nodiscard]] Ends ends() const;

    [[nodiscard]] LinkStyle style() const;
    // Draws it in style from now on.
    void setStyle(LinkStyle style);
    // What it draws, in the scene: see route(), from each of the points it goes through to the next one.
    [[nodiscard]] const QPainterPath& path() const;

    // The points it goes through between its interfaces, in their order, in the scene.
    [[nodiscard]] QList<QPointF> anchors() const;
    // Those points, which other links of its provided interface can go through as well.
    [[nodiscard]] const QList<AnchorItem*>& anchorItems() const;
    // The point it goes through after anchor, one of its own; nullptr when it goes to its consumed interface then.
    [[nodiscard]] AnchorItem* after(const AnchorItem& anchor) const;
    // Its points up to anchor, one of them, anchor included: those a link that branches off it there goes through.
    [[nodiscard]] QList<AnchorItem*> anchorsUpTo(const AnchorItem& anchor) const;
    // Has it go through anchors, in their order, in place of those it went through. Those another link of its provided
    // interface goes through first, in the same order, are the same points: their line is one up to where they part.
    void setAnchors(const QList<QPointF>& anchors);
    // Has it go through anchors, in their order, in place of those it went through, those of other links of its
    // provided interface included: it goes through the same points before them as they do.
    void setAnchorItems(const QList<AnchorItem*>& anchors);
    // Has it go through position as well, between the two points of its line position is closest to: its interfaces,
    // or the anchors next to each other there. The links that share its line there go through it too. Returns it.
    AnchorItem& addAnchor(const QPointF& position);
    // The links whose line goes where its own is the closest to position: those that share it there, itself
    // included, or itself alone.
    [[nodiscard]] QList<LinkItem*> linksAlong(const QPointF& position);
    // Where a link that branches off it at position, a point of its line, starts: gone through the way from the point
    // of its line before position, as a junction is.
    [[nodiscard]] Stop branchStop(const QPointF& position) const;
    // Draws it again through its anchors, with the other links of its provided interface, and tells who it was built
    // with that they changed: one of them was moved by the mouse, say.
    void anchorsChanged();
    // Forgets anchor, deleted while it still went through it: the scene is cleared, and it goes as well.
    void forgetAnchor(const AnchorItem& anchor);

    // Whether the mouse drags its anchors.
    [[nodiscard]] bool isEditable() const;
    void setEditable(bool editable);

    // Draws it between its interfaces again, where they are now.
    void follow();

    [[nodiscard]] QRectF boundingRect() const override;
    // Around its line, wide enough to be clicked.
    [[nodiscard]] QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    // Selects the links that share its line where it was clicked with it, once it is selected alone.
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    // Refuses to be selected by the rubber band of a view; once it is selected or no longer is, comes over the other
    // links or goes back among them, and has its anchors painted again, in its color.
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

  private:
    // Once it is selected or no longer is, or in another scene, see itemChange().
    void followChange(GraphicsItemChange change);
    void setHovered(bool hovered);
    // Shows hovered, links of its provided interface, as hovered, and the others not.
    void hoverAlong(const QList<LinkItem*>& hovered);
    // What links tell in their tooltip, those that share a stretch of its line.
    [[nodiscard]] QString about(const QList<LinkItem*>& links) const;
    // Selects the links that share its line at position, or not, as it is.
    void selectAlong(const QPointF& position);
    // Paints its anchors again, in its color.
    void updateAnchors();
    // Where its line goes through: the dot of its provided interface, its anchors, then the dot of its consumed one.
    [[nodiscard]] QList<Stop> stops() const;
    // Where position is inserted among its anchors: the index of the stretch of its line, between two of its stops,
    // position is the closest to.
    [[nodiscard]] qsizetype nearestStretch(const QPointF& position) const;
    // Those whose line goes along stretch of its own: the links through the point it ends at, which go through the
    // same points before it, or itself alone, when it ends at its consumed interface.
    [[nodiscard]] QList<LinkItem*> linksAlong(qsizetype stretch);
    // The first of anchors that other links of its provided interface go through, in that order.
    [[nodiscard]] QList<AnchorItem*> sharedAnchors(const QList<QPointF>& anchors) const;
    // The point another link of its provided interface goes through after shared, the points it goes through first,
    // where the next one of anchors is, past those shared; nullptr when none does, or anchors has no next one.
    [[nodiscard]] AnchorItem* sharedAfter(const QList<AnchorItem*>& shared, const QList<QPointF>& anchors) const;
    // Goes through anchor, which joins its scene.
    void join(AnchorItem& anchor);
    // No longer goes through anchor, which goes when no other link does.
    void leave(AnchorItem& anchor);
    // Draws the links of its provided interface again: which of their points are junctions, and which of them draws a
    // stretch they share, change with the points each goes through.
    void followSiblings();
    // What it draws when it is neither hovered nor selected: the stretches of its line it draws, see drawsStretch().
    [[nodiscard]] QPainterPath ownPath() const;
    // Whether it draws stretch, one of its line: the first link through the point it ends at draws the stretch they
    // share.
    [[nodiscard]] bool drawsStretch(qsizetype stretch) const;

    InterfaceItem& m_provided;
    InterfaceItem& m_consumed;
    LinkStyle m_style;
    QPainterPath m_path;             // In the scene: the item stays at its origin
    QList<QPainterPath> m_stretches; // Those of m_path, from each of its stops to the next one
    QPainterPath m_shape;            // Around m_path
    bool m_hovered = false;
    QList<AnchorItem*> m_anchors; // In their order along its line
    std::function<void(const LinkItem&)> m_anchored;
    bool m_editable = true;
};
