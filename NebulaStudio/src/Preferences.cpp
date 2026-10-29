// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Preferences.h"

#include <QLatin1StringView>
#include <QSettings>
#include <QString>
#include <algorithm>
#include <array>

namespace
{

using namespace Qt::Literals::StringLiterals;

constexpr QLatin1StringView LinkStyleKey = "edition/linkStyle"_L1;

// A style of the links, under the name the settings hold it by.
struct NamedLinkStyle
{
    LinkStyle style;
    QLatin1StringView name;
};

constexpr std::array LinkStyleNames{
    NamedLinkStyle{.style = LinkStyle::Curved, .name = "curves"_L1},
    NamedLinkStyle{.style = LinkStyle::Straight, .name = "straightLines"_L1},
    NamedLinkStyle{.style = LinkStyle::RightAngled, .name = "rightAngles"_L1},
};

} // namespace

LinkStyle readLinkStyle(const QSettings& settings)
{
    const QString name = settings.value(LinkStyleKey).toString();
    const auto found = std::ranges::find(LinkStyleNames, name, &NamedLinkStyle::name);
    return found != LinkStyleNames.end() ? found->style : LinkStyle::Curved;
}

void writeLinkStyle(QSettings& settings, LinkStyle style)
{
    const auto found = std::ranges::find(LinkStyleNames, style, &NamedLinkStyle::style);
    if(found != LinkStyleNames.end())
    {
        settings.setValue(LinkStyleKey, QString(found->name));
    }
}
