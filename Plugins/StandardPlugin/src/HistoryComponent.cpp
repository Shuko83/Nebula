// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "HistoryComponent.h"

#include <QLoggingCategory>

#include "StandardPlugin.h"

NEBULA_REGISTER_COMPONENT(StandardPlugin, HistoryComponent);

namespace
{

Q_LOGGING_CATEGORY(lcStandard, "nebula.standard")

} // namespace

HistoryComponent::HistoryComponent()
{
    provideInterface<Nebula::IHistory>(QStringLiteral("history"), &m_history);
}

int HistoryComponent::maxSize() const
{
    return m_history.maxSize();
}

void HistoryComponent::setMaxSize(int maxSize)
{
    if(maxSize < 0)
    {
        qCWarning(lcStandard).noquote() << QStringLiteral("%1: a history cannot keep %2 commands, %3 is kept")
                                               .arg(objectName())
                                               .arg(maxSize)
                                               .arg(m_history.maxSize());
        return;
    }
    if(maxSize != m_history.maxSize())
    {
        m_history.setMaxSize(maxSize);
        emit maxSizeChanged(maxSize);
    }
}

bool HistoryComponent::isUndoActive() const
{
    return m_history.isUndoActive();
}

void HistoryComponent::setUndoActive(bool active)
{
    if(active != m_history.isUndoActive())
    {
        m_history.setUndoActive(active);
        emit undoActiveChanged(active);
    }
}

bool HistoryComponent::isRedoActive() const
{
    return m_history.isRedoActive();
}

void HistoryComponent::setRedoActive(bool active)
{
    if(active != m_history.isRedoActive())
    {
        m_history.setRedoActive(active);
        emit redoActiveChanged(active);
    }
}

void HistoryComponent::onStop()
{
    m_history.clear(); // What it did may refer to what goes as it stops
}
