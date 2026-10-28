// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QGraphicsItem>
#include <QLineF>
#include <QList>
#include <QObject>
#include <QPointer>
#include <functional>

class ComponentItem;
class QGraphicsObject;

namespace Nebula
{
class Component;
}

// An item joined to a component of the edition zone, by what it holds of it: a dashed line between their centers, under
// them, which only shows between them. A settings file is joined so to the components whose properties it holds, and a
// stargate to those whose properties it exposes.
class DashedLinkItem : public QGraphicsItem
{
  public:
    // Deleted with from, which follow() is called by; removed is called once it is deleted, by whoever deletes it.
    DashedLinkItem(QGraphicsItem& from, ComponentItem& component, std::function<void(DashedLinkItem&)> removed);
    ~DashedLinkItem() override;

    // The component it joins to its item; nullptr once it is deleted.
    [[nodiscard]] ComponentItem* component() const;
    // Draws it between its items again, where they are now.
    void follow();

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  private:
    QGraphicsItem& m_from;
    QPointer<ComponentItem> m_component;
    std::function<void(DashedLinkItem&)> m_removed;
    QLineF m_line; // In the scene: the item stays at its origin
};

// The dashed links of an item to components of the edition zone, as SettingsItem and StargateItem draw them: made
// again by refresh(), and drawn again as the item or the components move. A member of the item, deleted with it.
class DashedLinks : public QObject
{
    Q_OBJECT

  public:
    explicit DashedLinks(QGraphicsObject& from);
    ~DashedLinks() override;

    DashedLinks(const DashedLinks&) = delete;
    DashedLinks& operator=(const DashedLinks&) = delete;

    // Joins its item to the items of components in its scene, in place of those it joined; to none while it is in no
    // scene.
    void refresh(const QList<const Nebula::Component*>& components);

  private:
    // Draws them again between their items, where they are now.
    void follow();
    void clear();
    // Joins its item to component, following it as it moves.
    void join(ComponentItem& component);

    QGraphicsObject& m_from;
    QList<DashedLinkItem*> m_links; // In the scene rather than children of m_from: under the components
};
