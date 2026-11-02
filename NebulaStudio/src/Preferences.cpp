// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Preferences.h"

#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLatin1StringView>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <algorithm>
#include <array>
#include <cstddef>

namespace
{

using namespace Qt::Literals::StringLiterals;

constexpr QLatin1StringView LinkStyleKey = "edition/linkStyle"_L1;
constexpr QLatin1StringView ColorSchemeKey = "appearance/colorScheme"_L1;
constexpr QLatin1StringView OverviewKey = "edition/overview"_L1;

// A value of a preference, under the name the settings hold it by.
template <typename Value> struct Named
{
    Value value;
    QLatin1StringView name;
};

constexpr std::array LinkStyleNames{
    Named<LinkStyle>{.value = LinkStyle::Curved, .name = "curves"_L1},
    Named<LinkStyle>{.value = LinkStyle::Straight, .name = "straightLines"_L1},
    Named<LinkStyle>{.value = LinkStyle::RightAngled, .name = "rightAngles"_L1},
};

constexpr std::array ColorSchemeNames{
    Named<Qt::ColorScheme>{.value = Qt::ColorScheme::Unknown, .name = "system"_L1},
    Named<Qt::ColorScheme>{.value = Qt::ColorScheme::Light, .name = "light"_L1},
    Named<Qt::ColorScheme>{.value = Qt::ColorScheme::Dark, .name = "dark"_L1},
};

// The value settings hold under key, by its name among names: the first of names when it holds none of them.
template <typename Value, std::size_t Count>
Value readNamed(const QSettings& settings, QLatin1StringView key, const std::array<Named<Value>, Count>& names)
{
    const QString name = settings.value(key).toString();
    const auto found = std::ranges::find(names, name, &Named<Value>::name);
    return found != names.end() ? found->value : names.front().value;
}

// Writes value under key, by its name among names.
template <typename Value, std::size_t Count>
void writeNamed(QSettings& settings, QLatin1StringView key, const std::array<Named<Value>, Count>& names, Value value)
{
    const auto found = std::ranges::find(names, value, &Named<Value>::value);
    if(found != names.end())
    {
        settings.setValue(key, QString(found->name));
    }
}

// Adds to settings value under key, or, when it is an object, each of its members under key, '/' and its name: under
// its name alone when key is empty, for the object of the file.
void addSetting(const QString& key, const QJsonValue& value, QSettings::SettingsMap& settings)
{
    if(!value.isObject())
    {
        settings.insert(key, value.toVariant());
        return;
    }
    for(const QString& name : value.toObject().keys())
    {
        addSetting(key.isEmpty() ? name : key + u'/' + name, value[name], settings);
    }
}

// Reads into settings the JSON object of device, see preferencesFormat(); false when it is not one.
bool readJson(QIODevice& device, QSettings::SettingsMap& settings)
{
    const QJsonDocument document = QJsonDocument::fromJson(device.readAll());
    if(!document.isObject())
    {
        return false;
    }
    addSetting(QString(), document.object(), settings);
    return true;
}

// object, with value under the path of its groups, each an object made when it has none.
QJsonObject withMember(QJsonObject object, QStringList path, const QJsonValue& value)
{
    const QString name = path.takeFirst();
    object.insert(name, path.isEmpty() ? value : QJsonValue(withMember(object.value(name).toObject(), path, value)));
    return object;
}

// Writes settings to device as a JSON object, see preferencesFormat().
bool writeJson(QIODevice& device, const QSettings::SettingsMap& settings)
{
    QJsonObject object;
    for(auto setting = settings.cbegin(); setting != settings.cend(); ++setting)
    {
        object = withMember(object, setting.key().split(u'/'), QJsonValue::fromVariant(setting.value()));
    }
    return device.write(QJsonDocument(object).toJson()) != -1;
}

} // namespace

QSettings::Format preferencesFormat()
{
    // Registered once by each copy of this code: the one of NebulaStudio, and the one of StudioPlugin.
    static const QSettings::Format format = QSettings::registerFormat(u"json"_s, &readJson, &writeJson);
    return format;
}

LinkStyle readLinkStyle(const QSettings& settings)
{
    return readNamed(settings, LinkStyleKey, LinkStyleNames);
}

void writeLinkStyle(QSettings& settings, LinkStyle style)
{
    writeNamed(settings, LinkStyleKey, LinkStyleNames, style);
}

Qt::ColorScheme readColorScheme(const QSettings& settings)
{
    return readNamed(settings, ColorSchemeKey, ColorSchemeNames);
}

void writeColorScheme(QSettings& settings, Qt::ColorScheme scheme)
{
    writeNamed(settings, ColorSchemeKey, ColorSchemeNames, scheme);
}

bool readOverviewShown(const QSettings& settings)
{
    const QVariant shown = settings.value(OverviewKey);
    return shown.typeId() != QMetaType::Bool || shown.toBool();
}

void writeOverviewShown(QSettings& settings, bool shown)
{
    settings.setValue(OverviewKey, shown);
}
