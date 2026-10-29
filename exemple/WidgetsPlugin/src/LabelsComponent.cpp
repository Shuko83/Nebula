// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "LabelsComponent.h"

#include <QWidget>
#include <algorithm>

#include "Label.h"
#include "WidgetsPlugin.h"

NEBULA_REGISTER_COMPONENT(WidgetsPlugin, LabelsComponent);

namespace
{

// The number of labels it has as it is dropped.
constexpr int DefaultCount = 1;

// The name of the interface of the label number, from 1, and of the group of its text.
QString labelName(qsizetype number)
{
    return QStringLiteral("label%1").arg(number);
}

} // namespace

LabelsComponent::LabelsComponent()
{
    resize(DefaultCount);
}

LabelsComponent::~LabelsComponent() = default;

int LabelsComponent::count() const
{
    return static_cast<int>(m_labels.size());
}

void LabelsComponent::setCount(int count)
{
    count = std::clamp(count, 0, MaxCount);
    if(count == this->count())
    {
        return;
    }
    resize(count);
    emit countChanged(count);
}

void LabelsComponent::resize(int count)
{
    while(this->count() < count)
    {
        addLabel();
    }
    while(this->count() > count)
    {
        removeLabel();
    }
}

void LabelsComponent::addLabel()
{
    const QString name = labelName(count() + 1);
    Label& label = *m_labels.emplace_back(std::make_unique<Label>(QStringLiteral("Label %1").arg(count() + 1)));
    addPropertiesForQObject(&label, name);
    provideInterface<QWidget>(name, &label.widget());
}

void LabelsComponent::removeLabel()
{
    // First: what is bound to it gives the widget back while it still exists.
    removeProvidedInterface(labelName(count()));
    m_labels.pop_back();
}
