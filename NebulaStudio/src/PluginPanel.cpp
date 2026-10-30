// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "PluginPanel.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QStringList>
#include <QTreeView>
#include <QVBoxLayout>
#include <optional>

#include "Messages.h"
#include "Nebula/Component/Component.h"
#include "PanelWidgets.h"
#include "PluginModel.h"
#include "PluginViewer.h"
#include "Studio.h"

namespace
{

QStringList namesOf(const QList<Nebula::Component*>& components)
{
    QStringList names;
    for(const Nebula::Component* component : components)
    {
        names.append(component->objectName());
    }
    return names;
}

} // namespace

PluginPanel::PluginPanel(Studio& studio, QWidget* parent)
    : QWidget(parent), m_studio(studio), m_openDirectory(Nebula::PluginManager::defaultDirectory()),
      m_model(new PluginModel(studio.application().plugins(), this)),
      m_viewer(new PluginViewer(studio.application().plugins()))
{
    setWindowTitle(tr("Plugins")); // The title of its tab
    refresh();
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->addWidget(createSplitter(createList(), m_viewer));
    followStudio();
}

QWidget* PluginPanel::createList()
{
    m_view = createView(*m_model);
    showCurrentIn(*m_view, PluginModel::IdRole, m_viewer, &PluginViewer::setPluginId);
    m_view->setCurrentIndex(m_model->index(0, 0));
    connect(m_viewer, &PluginViewer::loadRequested, this, &PluginPanel::load);
    connect(m_viewer, &PluginViewer::unloadRequested, this, &PluginPanel::unload);
    QPushButton* open = createButton(tr("Open plugin..."), tr("Load a plugin from anywhere on the disk"));
    connect(open, &QPushButton::clicked, this, &PluginPanel::openPlugin);
    return createColumn(open, m_view, Qt::AlignLeft);
}

void PluginPanel::followStudio()
{
    const Nebula::Universe& application = m_studio.application();
    connect(&m_studio, &Studio::pluginsChanged, this, &PluginPanel::refresh);
    // What runs keeps its plugin.
    m_viewer->setUnloadAllowed(!application.isRunning());
    connect(&application, &Nebula::Universe::runningChanged, m_viewer,
            [viewer = m_viewer](bool running) { viewer->setUnloadAllowed(!running); });
}

void PluginPanel::refresh()
{
    m_model->refresh();
    m_viewer->refresh();
}

void PluginPanel::load(const QString& pluginId)
{
    if(!m_studio.application().plugins().load(pluginId))
    {
        warn(this, tr("Load plugin"),
             tr("%1 cannot be loaded:\n%2").arg(pluginId, m_studio.application().plugins().errorString()));
        return;
    }
    m_studio.refreshPlugins();
}

void PluginPanel::unload(const QString& pluginId)
{
    const QList<Nebula::Component*> used = m_studio.application().componentsOf(pluginId);
    if(used.isEmpty() || confirmUnload(pluginId, used))
    {
        emit removing(used); // With what shows them, before the application would remove them itself
        unloadPlugin(pluginId);
    }
}

bool PluginPanel::confirmUnload(const QString& pluginId, const QList<Nebula::Component*>& used)
{
    const QMessageBox::StandardButton answer =
        QMessageBox::warning(this, tr("Unload plugin"),
                             tr("The application has components of %1:\n%2\n\n"
                                "Unloading the plugin removes them.\nDo you want to unload it?")
                                 .arg(pluginId, namesOf(used).join(QStringLiteral(", "))),
                             QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    return answer == QMessageBox::Yes;
}

void PluginPanel::unloadPlugin(const QString& pluginId)
{
    if(!m_studio.application().unloadPlugin(pluginId))
    {
        warn(this, tr("Unload plugin"),
             tr("%1 cannot be unloaded:\n%2").arg(pluginId, m_studio.application().errorString()));
    }
    m_studio.refreshPlugins();
}

void PluginPanel::openPlugin()
{
    const QString filePath = QFileDialog::getOpenFileName(this, tr("Open plugin"), m_openDirectory,
                                                          tr("Nebula plugins (*.nbdl);;All files (*)"));
    if(!filePath.isEmpty())
    {
        m_openDirectory = QFileInfo(filePath).absolutePath();
        addPlugin(filePath);
    }
}

void PluginPanel::addPlugin(const QString& filePath)
{
    Nebula::PluginManager& plugins = m_studio.application().plugins();
    const std::optional<Nebula::PluginInfo> info = plugins.addFile(filePath);
    if(!info)
    {
        warn(this, tr("Open plugin"),
             tr("%1 cannot be opened:\n%2").arg(QDir::toNativeSeparators(filePath), plugins.errorString()));
        return;
    }
    m_studio.refreshGalaxies(); // Those that missed it are loadable now
    selectAndLoad(info->id);
}

void PluginPanel::selectAndLoad(const QString& pluginId)
{
    m_model->refresh();
    m_view->setCurrentIndex(m_model->indexOf(pluginId));
    if(!m_studio.application().plugins().isLoaded(pluginId))
    {
        load(pluginId);
    }
}
