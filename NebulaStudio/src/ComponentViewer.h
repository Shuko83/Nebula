// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QMetaProperty>
#include <QString>

#include "Inspector.h"

class QLabel;
class QTreeWidget;

namespace Nebula
{
class ComponentFactory;
struct ComponentInfo;
} // namespace Nebula

// Shows a component registered in a ComponentFactory: what it is and its properties.
class ComponentViewer : public Inspector
{
    Q_OBJECT

  public:
    explicit ComponentViewer(const Nebula::ComponentFactory& factory, QWidget* parent = nullptr);

    // Shows componentId; nothing when it is empty or unknown.
    void setComponentId(const QString& componentId);

  private:
    void showInfo(const Nebula::ComponentInfo& component);
    void showProperties(const QList<QMetaProperty>& properties);

    const Nebula::ComponentFactory& m_factory;

    QLabel* m_name = nullptr;
    QLabel* m_id = nullptr;
    QLabel* m_plugin = nullptr;
    QLabel* m_category = nullptr;
    QLabel* m_description = nullptr;
    QTreeWidget* m_properties = nullptr;
};
