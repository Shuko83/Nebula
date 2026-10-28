// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "PluginViewer.h"

#include <QDir>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <optional>

#include "Nebula/Plugin/PluginManager.h"

namespace
{

QString buildDateText(const Nebula::PluginInfo& info, bool loaded)
{
    if(!loaded)
    {
        return PluginViewer::tr("Known once loaded");
    }
    if(!info.buildDate.isValid())
    {
        return PluginViewer::tr("Unknown");
    }
    return QLocale().toString(info.buildDate, QLocale::ShortFormat);
}

} // namespace

PluginViewer::PluginViewer(const Nebula::PluginManager& plugins, QWidget* parent)
    : Inspector(parent), m_plugins(plugins)
{
    addFields();
    addLoadButtons();
    addStretch();
    showPlaceholder(tr("Select a plugin"));
}

void PluginViewer::setPluginId(const QString& pluginId)
{
    m_pluginId = pluginId;
    refresh();
}

void PluginViewer::refresh()
{
    const std::optional<Nebula::PluginInfo> info = m_plugins.plugin(m_pluginId);
    if(!info)
    {
        showPlaceholder(tr("Select a plugin"));
        return;
    }
    const bool loaded = m_plugins.isLoaded(info->id);
    showInfo(*info, loaded);
    showButtons(loaded);
    showPage();
}

void PluginViewer::setUnloadAllowed(bool allowed)
{
    m_unloadAllowed = allowed;
    refresh();
}

void PluginViewer::addFields()
{
    m_name = addField(tr("Name"));
    m_id = addField(tr("Id"));
    m_version = addField(tr("Version"));
    m_description = addField(tr("Description"));
    m_coreVersion = addField(tr("Core version"));
    m_buildDate = addField(tr("Build date"));
    m_state = addField(tr("State"));
    m_file = addField(tr("File"));
}

void PluginViewer::addLoadButtons()
{
    m_loadButton = newButton(tr("Load"), [this] { emit loadRequested(m_pluginId); });
    m_unloadButton = newButton(tr("Unload"), [this] { emit unloadRequested(m_pluginId); });
    addButtons({m_loadButton, m_unloadButton});
}

void PluginViewer::showInfo(const Nebula::PluginInfo& info, bool loaded)
{
    m_name->setText(info.name);
    m_id->setText(info.id);
    m_version->setText(info.version.toString());
    m_description->setText(info.description);
    m_coreVersion->setText(info.coreVersion.toString());
    m_buildDate->setText(buildDateText(info, loaded));
    m_state->setText(loaded ? tr("Loaded") : tr("Not loaded"));
    m_file->setText(QDir::toNativeSeparators(info.filePath));
}

void PluginViewer::showButtons(bool loaded)
{
    m_loadButton->setEnabled(!loaded);
    m_unloadButton->setEnabled(loaded && m_unloadAllowed);
    m_unloadButton->setToolTip(m_unloadAllowed ? QString() : tr("Stop the application to unload a plugin"));
}
