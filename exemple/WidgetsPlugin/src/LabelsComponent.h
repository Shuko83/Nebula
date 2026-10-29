// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <memory>
#include <vector>

#include "Nebula/Component/Component.h"

class Label;

// Labels, as many as its count: each a QLabel it creates and provides as the interface "label<n>", a QWidget, from
// "label1", whose text is edited in the group of that name. Setting its count adds the next ones, or removes the last
// ones with what is bound to them: a project sets it before their texts and their bindings. A TopBarComponent shows
// them in its bar, say. It needs a QApplication.
class LabelsComponent : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Labels, as many as its count, each a widget it provides")
    Q_CLASSINFO("category", "Widgets")
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)

  public:
    // The most it has.
    static constexpr int MaxCount = 10;

    LabelsComponent();
    ~LabelsComponent() override; // Where Label is known

    [[nodiscard]] int count() const;
    // Between 0 and MaxCount.
    void setCount(int count);

  signals:
    void countChanged(int count);

  private:
    // Adds or removes the last ones until it has count of them.
    void resize(int count);
    // Provides one more, after the others.
    void addLabel();
    // Removes the last one, then destroys it, which takes its text away.
    void removeLabel();

    std::vector<std::unique_ptr<Label>> m_labels;
};
