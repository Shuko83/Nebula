// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "GalaxyViewer.h"

#include <QDir>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QStyle>
#include <optional>

#include "Galaxy/GalaxyModel.h"
#include "Theme.h"

namespace
{

// Why a galaxy cannot be loaded when the application does not know missing, its plugins, and what to do about it.
QString missingPluginsText(const QStringList& missing)
{
    return GalaxyViewer::tr("This galaxy cannot be loaded: NebulaStudio does not know these plugins of it.\n%1\n"
                            "Open them with Open plugin... in the Plugins tab, or give their directory to "
                            "NebulaStudio with --plugin-dir.")
        .arg(missing.join(u'\n'));
}

// Why galaxy cannot be loaded, and what to do about it; empty when it can be.
QString errorText(const GalaxyModel::Galaxy& galaxy)
{
    if(!galaxy.info)
    {
        return GalaxyViewer::tr("This galaxy cannot be loaded: its file cannot be read.\n%1").arg(galaxy.error);
    }
    return galaxy.missingPlugins.isEmpty() ? QString() : missingPluginsText(galaxy.missingPlugins);
}

// The plugins of galaxy, one per line.
QString pluginsText(const GalaxyModel::Galaxy& galaxy)
{
    if(!galaxy.info)
    {
        return GalaxyViewer::tr("Unknown");
    }
    return galaxy.info->plugins.isEmpty() ? GalaxyViewer::tr("None") : galaxy.info->plugins.join(u'\n');
}

// A label showing the warning icon of style, as a small icon.
QLabel* newWarningIcon(const QStyle& style)
{
    auto* icon = new QLabel;
    icon->setPixmap(
        style.standardIcon(QStyle::SP_MessageBoxWarning).pixmap(style.pixelMetric(QStyle::PM_SmallIconSize)));
    return icon;
}

} // namespace

GalaxyViewer::GalaxyViewer(const GalaxyModel& model, QWidget* parent) : Inspector(parent), m_model(model)
{
    followModel();
    addFields();
    addBelow(newError());
    m_openButton = newButton(tr("Open"), [this] { emit openRequested(m_filePath); });
    addButtons({m_openButton});
    addStretch();
    refresh();
}

void GalaxyViewer::setFilePath(const QString& filePath)
{
    m_filePath = filePath;
    refresh();
}

void GalaxyViewer::setOpenAllowed(bool allowed)
{
    m_openAllowed = allowed;
    refresh();
}

void GalaxyViewer::changeEvent(QEvent* event)
{
    Inspector::changeEvent(event);
    if(event->type() == QEvent::PaletteChange)
    {
        colorError();
    }
}

void GalaxyViewer::followModel()
{
    connect(&m_model, &QAbstractItemModel::dataChanged, this, &GalaxyViewer::refresh);
    connect(&m_model, &QAbstractItemModel::rowsInserted, this, &GalaxyViewer::refresh);
    connect(&m_model, &QAbstractItemModel::rowsRemoved, this, &GalaxyViewer::refresh);
}

void GalaxyViewer::addFields()
{
    m_name = addField(tr("Name"));
    m_description = addField(tr("Description"));
    m_file = addField(tr("File"));
    m_plugins = addField(tr("Plugins"));
    m_state = addField(tr("State"));
}

QWidget* GalaxyViewer::newError()
{
    m_errorText = newValueLabel();
    m_error = new QWidget;
    auto* layout = new QHBoxLayout(m_error);
    layout->setContentsMargins({});
    layout->addWidget(newWarningIcon(*style()), 0, Qt::AlignTop);
    layout->addWidget(m_errorText, 1);
    colorError();
    return m_error;
}

void GalaxyViewer::refresh()
{
    const std::optional<GalaxyModel::Galaxy> galaxy = m_model.galaxy(m_filePath);
    if(!galaxy)
    {
        showPlaceholder(placeholderText());
        return;
    }
    showGalaxy(*galaxy);
    showError(errorText(*galaxy));
    showOpenButton(galaxy->isLoadable());
    showPage();
}

QString GalaxyViewer::placeholderText() const
{
    return m_model.rowCount() == 0 ? tr("No galaxy in this directory: the projects saved there are listed here")
                                   : tr("Select a galaxy");
}

void GalaxyViewer::showGalaxy(const GalaxyModel::Galaxy& galaxy)
{
    const QModelIndex index = m_model.indexOf(m_filePath);
    m_name->setText(index.data().toString());
    m_description->setText(galaxy.info ? galaxy.info->description : QString());
    m_file->setText(QDir::toNativeSeparators(galaxy.filePath));
    m_plugins->setText(pluginsText(galaxy));
    m_state->setText(index.siblingAtColumn(GalaxyModel::State).data().toString());
}

void GalaxyViewer::showError(const QString& error)
{
    m_errorText->setText(error);
    m_error->setVisible(!error.isEmpty());
}

void GalaxyViewer::showOpenButton(bool loadable)
{
    m_openButton->setEnabled(loadable && m_openAllowed);
    m_openButton->setToolTip(m_openAllowed ? QString() : tr("Stop the application to open a galaxy"));
}

void GalaxyViewer::colorError()
{
    QPalette palette = m_errorText->palette();
    palette.setColor(QPalette::WindowText, currentThemeColors().warning);
    m_errorText->setPalette(palette);
}
