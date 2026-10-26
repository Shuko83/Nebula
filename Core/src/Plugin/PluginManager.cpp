// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Nebula/Plugin/PluginManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonObject>
#include <QLocale>
#include <QLoggingCategory>
#include <QPluginLoader>
#include <algorithm>
#include <ranges>
#include <utility>

#include "Core_info.h"
#include "Nebula/Component/ComponentFactory.h"
#include "Nebula/Plugin/IPlugin.h"

namespace Nebula
{

namespace
{

Q_LOGGING_CATEGORY(lcPlugins, "nebula.plugins")

QVersionNumber readVersion(const QJsonObject& metaData, QStringView key)
{
    return QVersionNumber::fromString(metaData.value(key).toString());
}

// The info of the plugin filePath, from the object nebula_add_plugin passes to Q_PLUGIN_METADATA.
PluginInfo toInfo(const QJsonObject& metaData, const QString& filePath)
{
    return {.id = metaData.value(u"id").toString(),
            .name = metaData.value(u"name").toString(),
            .description = metaData.value(u"description").toString(),
            .version = readVersion(metaData, u"version"),
            .coreVersion = readVersion(metaData, u"coreVersion"),
            .filePath = filePath};
}

// Reads the object nebula_add_plugin passes to Q_PLUGIN_METADATA; nullopt when its id or name is missing.
std::optional<PluginInfo> readInfo(const QJsonObject& metaData, const QString& filePath)
{
    PluginInfo info = toInfo(metaData, filePath);
    if(info.id.isEmpty() || info.name.isEmpty())
    {
        return std::nullopt;
    }
    return info;
}

// __DATE__ " " __TIME__, e.g. "Oct  6 2026 21:14:03", in the local time of the build machine.
QDateTime fromCompilerDate(std::string_view dateTime)
{
    const QString text = QString::fromLatin1(dateTime.data(), static_cast<qsizetype>(dateTime.size())).simplified();
    return QLocale::c().toDateTime(text, QStringLiteral("MMM d yyyy HH:mm:ss"));
}

// A plugin runs on the Core it was built against, or on a newer one with the same major version.
bool isCompatibleCore(const QVersionNumber& pluginCoreVersion)
{
    const QVersionNumber coreVersion = QVersionNumber::fromString(CoreInfo::version).normalized();
    return pluginCoreVersion.majorVersion() == coreVersion.majorVersion() &&
           pluginCoreVersion.normalized() <= coreVersion;
}

// Why the library of loader, whose metadata is metaData, is not a Nebula plugin; empty when it is one.
QString metaDataError(const QPluginLoader& loader, const QJsonObject& metaData)
{
    if(metaData.isEmpty())
    {
        return loader.errorString();
    }
    const QString iid = metaData.value(u"IID").toString();
    if(iid != QLatin1StringView(NEBULA_PLUGIN_IID))
    {
        return QStringLiteral("Built against another plugin interface: %1").arg(iid);
    }
    return {};
}

// Why the plugin of info cannot run on this Core; empty when it can.
QString coreError(const PluginInfo& info)
{
    if(isCompatibleCore(info.coreVersion))
    {
        return {};
    }
    return QStringLiteral("Built against Core %1 instead of %2")
        .arg(info.coreVersion.toString(), QLatin1StringView(CoreInfo::version));
}

// Reads the metadata of filePath without loading it; nullopt, with error set, when the plugin cannot be used.
std::optional<PluginInfo> readPlugin(const QPluginLoader& loader, const QString& filePath, QString& error)
{
    const QJsonObject metaData = loader.metaData();
    error = metaDataError(loader, metaData);
    if(!error.isEmpty())
    {
        return std::nullopt;
    }
    std::optional<PluginInfo> info = readInfo(metaData.value(u"MetaData").toObject(), filePath);
    error = info ? coreError(*info) : QStringLiteral("No id or name in its metadata");
    return error.isEmpty() ? info : std::nullopt;
}

// The canonical paths of the plugins in directory, by the extension nebula_add_plugin gives them
// (Core/cmake/NebulaPlugin.cmake).
QStringList pluginFiles(const QString& directory)
{
    QStringList filePaths;
    for(const QFileInfo& file : QDir(directory).entryInfoList({QStringLiteral("*.nbdl")}, QDir::Files, QDir::Name))
    {
        filePaths.append(file.canonicalFilePath());
    }
    return filePaths;
}

} // namespace

PluginManager::PluginManager()
{
    // Created before any manager is: it is still there when the last one unloads its plugins.
    ComponentFactory::instance();
}

// What it loaded goes with it: destroying the loaders alone would leave the plugins loaded, with no one to unload them.
PluginManager::~PluginManager()
{
    for(const auto& [pluginId, entry] : m_entries)
    {
        if(entry.loaded)
        {
            unload(pluginId);
        }
    }
}

QString PluginManager::defaultDirectory()
{
    // The directory nebula_add_plugin writes the plugins to (Core/cmake/NebulaPlugin.cmake).
    return QCoreApplication::applicationDirPath() + QStringLiteral("/plugins");
}

QList<PluginInfo> PluginManager::scan(const QString& directory)
{
    const auto isNew = [this](const QString& filePath) { return findFile(filePath) == nullptr; };
    QList<PluginInfo> found;
    for(const QString& filePath : pluginFiles(directory) | std::views::filter(isNew))
    {
        if(std::optional<PluginInfo> info = scanFile(filePath))
        {
            found.append(*info);
        }
    }
    return found;
}

std::optional<PluginInfo> PluginManager::addFile(const QString& filePath)
{
    const QFileInfo file(filePath);
    if(!file.isFile())
    {
        m_errorString = QStringLiteral("%1 is not a file").arg(QDir::toNativeSeparators(filePath));
        return std::nullopt;
    }
    m_errorString.clear();
    const QString canonicalPath = file.canonicalFilePath();
    const Entry* known = findFile(canonicalPath);
    return known != nullptr ? known->info : addNewFile(canonicalPath, m_errorString);
}

QList<PluginInfo> PluginManager::plugins() const
{
    QList<PluginInfo> plugins;
    plugins.reserve(static_cast<qsizetype>(m_entries.size()));
    for(const Entry& entry : m_entries | std::views::values)
    {
        plugins.append(entry.info);
    }
    return plugins;
}

std::optional<PluginInfo> PluginManager::plugin(const QString& pluginId) const
{
    const auto entry = m_entries.find(pluginId);
    if(entry == m_entries.end())
    {
        return std::nullopt;
    }
    return entry->second.info;
}

bool PluginManager::load(const QString& pluginId)
{
    Entry* entry = knownEntry(pluginId);
    IPlugin* root = entry != nullptr ? loadRoot(*entry->loader) : nullptr;
    if(root == nullptr)
    {
        return false;
    }
    entry->loaded = true;
    entry->info.buildDate = fromCompilerDate(root->buildDate());
    ComponentFactory::instance().add(pluginId, *root);
    return true;
}

bool PluginManager::unload(const QString& pluginId)
{
    Entry* entry = knownEntry(pluginId);
    if(entry == nullptr)
    {
        return false;
    }
    return !entry->loader->isLoaded() || unloadLoaded(pluginId, *entry);
}

bool PluginManager::isLoaded(const QString& pluginId) const
{
    const auto entry = m_entries.find(pluginId);
    return entry != m_entries.end() && entry->second.loader->isLoaded();
}

QString PluginManager::errorString() const
{
    return m_errorString;
}

bool PluginManager::refuse(const QString& error)
{
    m_errorString = error;
    return false;
}

const PluginManager::Entry* PluginManager::findFile(const QString& filePath) const
{
    const auto entries = m_entries | std::views::values;
    const auto entry = std::ranges::find(entries, filePath, [](const Entry& known) { return known.info.filePath; });
    return entry == entries.end() ? nullptr : &*entry;
}

PluginManager::Entry* PluginManager::knownEntry(const QString& pluginId)
{
    const auto entry = m_entries.find(pluginId);
    const bool known = entry != m_entries.end();
    m_errorString = known ? QString() : QStringLiteral("Unknown plugin %1").arg(pluginId);
    return known ? &entry->second : nullptr;
}

std::optional<PluginInfo> PluginManager::scanFile(const QString& filePath)
{
    QString error;
    std::optional<PluginInfo> info = addNewFile(filePath, error);
    if(!info)
    {
        qCWarning(lcPlugins).noquote() << "Skipping" << filePath << "-" << error;
    }
    return info;
}

std::optional<PluginInfo> PluginManager::addNewFile(const QString& filePath, QString& error)
{
    auto loader = std::make_unique<QPluginLoader>(filePath);
    std::optional<PluginInfo> info = readNewPlugin(*loader, filePath, error);
    if(info)
    {
        m_entries.emplace(info->id, Entry{.info = *info, .loader = std::move(loader)});
    }
    return info;
}

std::optional<PluginInfo> PluginManager::readNewPlugin(const QPluginLoader& loader, const QString& filePath,
                                                       QString& error) const
{
    std::optional<PluginInfo> info = readPlugin(loader, filePath, error);
    const auto known = info ? m_entries.find(info->id) : m_entries.end();
    if(known == m_entries.end())
    {
        return info;
    }
    error = QStringLiteral("%1 is already provided by %2")
                .arg(info->id, QDir::toNativeSeparators(known->second.info.filePath));
    return std::nullopt;
}

IPlugin* PluginManager::loadRoot(QPluginLoader& loader)
{
    // load() takes the share of the library that unload() gives back, even when another manager loaded it first;
    // instance() is its root object, generated by nebula_add_plugin.
    IPlugin* root = loader.load() ? qobject_cast<IPlugin*>(loader.instance()) : nullptr;
    m_errorString = root != nullptr ? QString() : loader.errorString();
    if(root == nullptr)
    {
        loader.unload(); // Not kept when its root object is not the one of a plugin
    }
    return root;
}

bool PluginManager::unloadLoaded(const QString& pluginId, Entry& entry)
{
    if(!entry.loaded)
    {
        return refuse(QStringLiteral("%1 was loaded by another plugin manager").arg(pluginId));
    }
    entry.loaded = false;
    // Before its root object, which creates them, is destroyed.
    ComponentFactory::instance().remove(pluginId);
    return entry.loader->unload() || keptByAnother(pluginId, *entry.loader);
}

bool PluginManager::keptByAnother(const QString& pluginId, QPluginLoader& loader)
{
    if(IPlugin* root = qobject_cast<IPlugin*>(loader.instance()))
    {
        ComponentFactory::instance().add(pluginId, *root);
    }
    return refuse(QStringLiteral("%1 is still loaded by another plugin manager").arg(pluginId));
}

} // namespace Nebula
