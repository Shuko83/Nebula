// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "GalaxyPanel.h"

#include <QDir>
#include <QFileDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTreeView>
#include <QVBoxLayout>
#include <optional>

#include "Galaxy/GalaxyModel.h"
#include "Galaxy/GalaxyViewer.h"
#include "PanelWidgets.h"
#include "ProjectFiles.h"
#include "Studio.h"
#include "Theme.h"

GalaxyPanel::GalaxyPanel(Studio& studio, QWidget* parent)
    : QWidget(parent), m_model(new GalaxyModel(studio.application(), this)), m_viewer(new GalaxyViewer(*m_model))
{
    setWindowTitle(tr("Galaxies")); // The title and the icon of its tab
    setWindowIcon(icon(GlossaryTerm::Galaxy));
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->addWidget(createSplitter(createList(), m_viewer));
    followStudio(studio);
    setDirectory({});
}

QWidget* GalaxyPanel::createList()
{
    m_view = createDraggableView(*m_model);
    showCurrentIn(*m_view, GalaxyModel::FilePathRole, m_viewer, &GalaxyViewer::setFilePath);
    connect(m_view, &QAbstractItemView::activated, this,
            [this](const QModelIndex& index) { open(index.data(GalaxyModel::FilePathRole).toString()); });
    connect(m_viewer, &GalaxyViewer::openRequested, this, &GalaxyPanel::open);
    return createColumn(createDirectoryBar(), m_view);
}

QWidget* GalaxyPanel::createDirectoryBar()
{
    m_directory = new QLineEdit;
    m_directory->setReadOnly(true);
    m_directory->setToolTip(tr("The directory whose galaxies are listed"));
    QPushButton* choose = createButton(tr("Choose directory..."), tr("List the galaxies of another directory"));
    connect(choose, &QPushButton::clicked, this, &GalaxyPanel::chooseDirectory);
    return createBar(m_directory, choose);
}

void GalaxyPanel::followStudio(const Studio& studio)
{
    const Nebula::Universe& application = studio.application();
    connect(&studio, &Studio::galaxiesChanged, m_model, &GalaxyModel::refresh);
    m_viewer->setOpenAllowed(!application.isRunning());
    connect(&application, &Nebula::Universe::runningChanged, m_viewer,
            [viewer = m_viewer](bool running) { viewer->setOpenAllowed(!running); });
}

QString GalaxyPanel::directory() const
{
    return m_model->directory();
}

void GalaxyPanel::setDirectory(const QString& directory)
{
    // Where the projects are saved unless told otherwise.
    m_model->setDirectory(directory.isEmpty() ? defaultProjectDirectory() : directory);
    m_directory->setText(QDir::toNativeSeparators(m_model->directory()));
    m_view->setCurrentIndex(m_model->index(0, 0));
    emit directoryChanged(m_model->directory());
}

void GalaxyPanel::chooseDirectory()
{
    const QString directory = QFileDialog::getExistingDirectory(this, tr("Galaxies directory"), m_model->directory());
    if(!directory.isEmpty())
    {
        setDirectory(directory);
    }
}

void GalaxyPanel::open(const QString& filePath)
{
    const std::optional<GalaxyModel::Galaxy> galaxy = m_model->galaxy(filePath);
    if(galaxy && galaxy->isLoadable())
    {
        emit openRequested(filePath);
    }
}
