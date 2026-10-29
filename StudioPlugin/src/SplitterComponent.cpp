// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "SplitterComponent.h"

#include <QStringList>
#include <QWidget>

#include "StudioPlugin.h"

NEBULA_REGISTER_COMPONENT(StudioPlugin, SplitterComponent);

namespace
{

// The sizes of sizes, "320, 760, 360"; 0 for one that is not a number.
QList<int> readSizes(const QString& sizes)
{
    QList<int> read;
    for(const QString& size : sizes.split(u',', Qt::SkipEmptyParts))
    {
        read.append(size.trimmed().toInt());
    }
    return read;
}

} // namespace

SplitterComponent::SplitterComponent()
{
    provideInterface<QWidget>(QStringLiteral("widget"), &m_splitter);
    consumeInterfaceList<QWidget>(
        QStringLiteral("widgets"), m_widgets, [this](QWidget* widget) { showWidget(widget); },
        [this](QWidget* widget) { hideWidget(widget); });
}

bool SplitterComponent::isVertical() const
{
    return m_splitter.orientation() == Qt::Vertical;
}

void SplitterComponent::setVertical(bool vertical)
{
    if(isVertical() == vertical)
    {
        return;
    }
    m_splitter.setOrientation(vertical ? Qt::Vertical : Qt::Horizontal);
    emit verticalChanged(vertical);
}

QString SplitterComponent::sizes() const
{
    return m_sizes;
}

void SplitterComponent::setSizes(const QString& sizes)
{
    if(m_sizes == sizes)
    {
        return;
    }
    m_sizes = sizes;
    divide();
    emit sizesChanged(sizes);
}

int SplitterComponent::stretched() const
{
    return m_stretched;
}

void SplitterComponent::setStretched(int stretched)
{
    if(m_stretched == stretched)
    {
        return;
    }
    m_stretched = stretched;
    divide();
    emit stretchedChanged(stretched);
}

void SplitterComponent::showWidget(QWidget* widget)
{
    if(m_splitter.indexOf(widget) >= 0)
    {
        return;
    }
    // Those before it in the list are shown already.
    m_splitter.insertWidget(static_cast<int>(m_widgets.indexOf(widget)), widget);
    widget->show(); // Hidden when it was taken from where it was shown
    divide();
}

void SplitterComponent::hideWidget(QWidget* widget)
{
    if(m_widgets.count(widget) == 1)
    {
        widget->setParent(nullptr);
    }
}

void SplitterComponent::divide()
{
    for(int index = 0; index < m_splitter.count(); ++index)
    {
        m_splitter.setStretchFactor(index, index == m_stretched ? 1 : 0);
    }
    if(!m_sizes.isEmpty())
    {
        m_splitter.setSizes(readSizes(m_sizes));
    }
}
