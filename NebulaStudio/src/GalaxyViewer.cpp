// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "GalaxyViewer.h"

#include <QDir>
#include <QEvent>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QStackedWidget>
#include <QStyle>
#include <QVBoxLayout>
#include <optional>

#include "GalaxyModel.h"
#include "Theme.h"

namespace
{

// A row of form whose value can be selected and wraps.
QLabel* addField(QFormLayout& form, const QString& name)
{
    auto* value = new QLabel;
    value->setTextInteractionFlags(Qt::TextSelectableByMouse);
    value->setWordWrap(true);
    form.addRow(name, value);
    return value;
}

// Why galaxy cannot be loaded, and what to do about it; empty when it can be.
QString errorText(const GalaxyModel::Galaxy& galaxy)
{
    if(!galaxy.info)
    {
        return GalaxyViewer::tr("This galaxy cannot be loaded: its file cannot be read.\n%1").arg(galaxy.error);
    }
    if(galaxy.missingPlugins.isEmpty())
    {
        return {};
    }
    return GalaxyViewer::tr("This galaxy cannot be loaded: NebulaStudio does not know these plugins of it.\n%1\n"
                            "Open them with Open plugin... in the Plugins tab, or give their directory to "
                            "NebulaStudio with --plugin-dir.")
        .arg(galaxy.missingPlugins.join(u'\n'));
}

} // namespace

GalaxyViewer::GalaxyViewer(const GalaxyModel& model, QWidget* parent) : QWidget(parent), m_model(model)
{
    // The galaxy shown changes with its file, and with the plugins the application knows: the model reads them again.
    connect(&m_model, &QAbstractItemModel::dataChanged, this, &GalaxyViewer::refresh);
    connect(&m_model, &QAbstractItemModel::rowsInserted, this, &GalaxyViewer::refresh);
    connect(&m_model, &QAbstractItemModel::rowsRemoved, this, &GalaxyViewer::refresh);

    m_placeholder = new QLabel;
    m_placeholder->setAlignment(Qt::AlignCenter);
    m_placeholder->setWordWrap(true);

    m_page = new QWidget;
    auto* form = new QFormLayout;
    m_name = addField(*form, tr("Name"));
    m_description = addField(*form, tr("Description"));
    m_file = addField(*form, tr("File"));
    m_plugins = addField(*form, tr("Plugins"));
    m_state = addField(*form, tr("State"));

    auto* errorIcon = new QLabel;
    errorIcon->setPixmap(
        style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(style()->pixelMetric(QStyle::PM_SmallIconSize)));
    m_errorText = new QLabel;
    m_errorText->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_errorText->setWordWrap(true);
    m_error = new QWidget;
    auto* errorLayout = new QHBoxLayout(m_error);
    errorLayout->setContentsMargins({});
    errorLayout->addWidget(errorIcon, 0, Qt::AlignTop);
    errorLayout->addWidget(m_errorText, 1);
    colorError();

    m_openButton = new QPushButton(tr("Open"));
    connect(m_openButton, &QPushButton::clicked, this, [this] { emit openRequested(m_filePath); });
    auto* buttons = new QHBoxLayout;
    buttons->addWidget(m_openButton);
    buttons->addStretch();

    auto* pageLayout = new QVBoxLayout(m_page);
    pageLayout->addLayout(form);
    pageLayout->addWidget(m_error);
    pageLayout->addLayout(buttons);
    pageLayout->addStretch();

    m_stack = new QStackedWidget;
    m_stack->addWidget(m_placeholder);
    m_stack->addWidget(m_page);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->addWidget(m_stack);

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
    QWidget::changeEvent(event);
    if(event->type() == QEvent::PaletteChange)
    {
        colorError();
    }
}

void GalaxyViewer::refresh()
{
    const std::optional<GalaxyModel::Galaxy> galaxy = m_model.galaxy(m_filePath);
    if(!galaxy)
    {
        m_placeholder->setText(m_model.rowCount() == 0
                                   ? tr("No galaxy in this directory: the projects saved there are listed here")
                                   : tr("Select a galaxy"));
        m_stack->setCurrentWidget(m_placeholder);
        return;
    }
    const QModelIndex index = m_model.indexOf(m_filePath);
    m_name->setText(index.data().toString());
    m_description->setText(galaxy->info ? galaxy->info->description : QString());
    m_file->setText(QDir::toNativeSeparators(galaxy->filePath));
    if(!galaxy->info)
    {
        m_plugins->setText(tr("Unknown"));
    }
    else
    {
        m_plugins->setText(galaxy->info->plugins.isEmpty() ? tr("None") : galaxy->info->plugins.join(u'\n'));
    }
    m_state->setText(index.siblingAtColumn(GalaxyModel::State).data().toString());
    const QString error = errorText(*galaxy);
    m_errorText->setText(error);
    m_error->setVisible(!error.isEmpty());
    m_openButton->setEnabled(galaxy->isLoadable() && m_openAllowed);
    m_openButton->setToolTip(m_openAllowed ? QString() : tr("Stop the application to open a galaxy"));
    m_stack->setCurrentWidget(m_page);
}

void GalaxyViewer::colorError()
{
    QPalette palette = m_errorText->palette();
    palette.setColor(QPalette::WindowText, currentThemeColors().warning);
    m_errorText->setPalette(palette);
}
