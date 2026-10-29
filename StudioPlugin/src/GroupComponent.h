// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QGroupBox>
#include <QString>

#include "Nebula/Component/Component.h"

class QVBoxLayout;
class QWidget;

// A widget under a title, in a frame: a QGroupBox it creates and provides as "widget", showing the widget it consumes
// as "content", which it cannot do without, for as long as it has it. It needs a QApplication.
class GroupComponent : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "A widget under a title, in a frame")
    Q_CLASSINFO("category", "Layout")
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)

  public:
    GroupComponent();

    [[nodiscard]] QString title() const;
    void setTitle(const QString& title);

  signals:
    void titleChanged(const QString& title);

  private:
    // Shows the widget it consumes in the frame.
    void showContent();
    // Takes the widget it consumes out of the frame, without a parent, as it was provided.
    void giveBackContent();

    QGroupBox m_group;
    QVBoxLayout* m_layout = nullptr; // Of m_group
    QWidget* m_content = nullptr;    // Consumed
};
