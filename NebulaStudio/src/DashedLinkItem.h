// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QGraphicsItem>
#include <QLineF>
#include <QPointer>
#include <functional>

class ComponentItem;

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
