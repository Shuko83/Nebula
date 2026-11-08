// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "PropertyValues.h"

#include <QByteArrayView>
#include <QColor>
#include <QDir>
#include <QMetaObject>
#include <algorithm>
#include <ranges>

namespace
{

// What the name of the type of flags starts with, its enumeration following.
constexpr QByteArrayView FlagsPrefix{"QFlags<"};
// The alpha of an opaque color.
constexpr int Opaque = 255;

// The enumeration a type is named after, without its scope, and whether the type is flags of it.
struct EnumerationName
{
    QByteArrayView name;
    bool flags = false;
};

// What typeName names: Mode for Probe::Mode, flags of Style for QFlags<Probe::Style>.
EnumerationName enumerationName(QByteArrayView typeName)
{
    const bool flags = typeName.startsWith(FlagsPrefix) && typeName.endsWith('>');
    if(flags)
    {
        typeName = typeName.sliced(FlagsPrefix.size()).chopped(1);
    }
    return {.name = typeName.sliced(typeName.lastIndexOf(':') + 1), .flags = flags};
}

// Whether candidate is what wanted names: an enumeration is named after it, flags after the enumeration they are of.
bool isNamed(const QMetaEnum& candidate, const EnumerationName& wanted)
{
    return candidate.isFlag() == wanted.flags &&
           wanted.name == QByteArrayView(wanted.flags ? candidate.enumName() : candidate.name());
}

} // namespace

QMetaEnum metaEnumOf(QMetaType type)
{
    const QMetaObject* scope = type.flags().testFlag(QMetaType::IsEnumeration) ? type.metaObject() : nullptr;
    const EnumerationName wanted = enumerationName(type.name());
    const auto indexes = std::views::iota(0, scope != nullptr ? scope->enumeratorCount() : 0);
    const auto found = std::ranges::find_if(indexes, [scope, &wanted](int index)
                                            { return isNamed(scope->enumerator(index), wanted); });
    return found != indexes.end() ? scope->enumerator(*found) : QMetaEnum();
}

QVariant enumValue(QMetaType type, int value)
{
    QVariant converted(value);
    return converted.convert(type) ? converted : QVariant();
}

QString colorName(const QColor& color)
{
    if(!color.isValid())
    {
        return {};
    }
    return color.name(color.alpha() == Opaque ? QColor::HexRgb : QColor::HexArgb);
}

QUrl urlOf(const QString& text)
{
    return QDir::isAbsolutePath(text) ? QUrl::fromLocalFile(text) : QUrl(text);
}
