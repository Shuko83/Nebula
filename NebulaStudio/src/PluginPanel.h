// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QString>
#include <QWidget>

class PluginModel;
class PluginViewer;
class QTreeView;
class Studio;

namespace Nebula
{
class Component;
}

// The plugins of the application of a Studio, those of plugins/ next to it and of the directories of its command
// line, under the button that opens one from anywhere on the disk, above the viewer of the one selected, which loads
// and unloads it; the first one selected at first. Unloading a plugin whose components are in the application asks
// first, then removes them with it, see removing(); nothing is done otherwise. A plugin is not unloaded while the
// application runs. It tells through its studio the plugins it loads and unloads, and is listed again when another
// part does. Named Plugins, the title of its tab.
class PluginPanel : public QWidget
{
    Q_OBJECT

  public:
    explicit PluginPanel(Studio& studio, QWidget* parent = nullptr);

  signals:
    // components, those of the application that are of a plugin about to be unloaded, are about to be removed with it:
    // what shows them lets them go now, as the edition zone does, see EditionZone::remove().
    void removing(const QList<Nebula::Component*>& components);

  private:
    // The plugins, under the button that opens one from anywhere on the disk; the first one selected.
    QWidget* createList();
    // Follows what the studio tells: the plugins to list again, and whether the application runs.
    void followStudio();
    // Lists the plugins again, as they are loaded or not, and shows the one selected again.
    void refresh();

    // Loads pluginId; tells why when it cannot be loaded.
    void load(const QString& pluginId);
    // Unloads pluginId. Those of the components of the application that are its own are removed with it, once that is
    // confirmed: nothing is done otherwise. Tells why when it cannot be unloaded.
    void unload(const QString& pluginId);
    // Whether to unload pluginId, which removes used, the components of the application that are its own.
    bool confirmUnload(const QString& pluginId, const QList<Nebula::Component*>& used);
    // Unloads pluginId, whose components are no longer in the application; tells why when it cannot be.
    void unloadPlugin(const QString& pluginId);
    // Asks for a plugin file, then selects and loads it.
    void openPlugin();
    // Lists the plugin of the file filePath, then selects and loads it; tells why when it cannot be read.
    void addPlugin(const QString& filePath);
    // Selects pluginId, just listed, and loads it unless it is.
    void selectAndLoad(const QString& pluginId);

    Studio& m_studio;
    QString m_openDirectory; // Where openPlugin() starts browsing: the directory of the last plugin opened
    PluginModel* m_model = nullptr;
    PluginViewer* m_viewer = nullptr;
    QTreeView* m_view = nullptr;
};
