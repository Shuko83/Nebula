// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Log.h"

#include <QDebug>
#include <QMessageLogger>

using Nebula::LogLevel;

namespace
{

// The stream of a message at level under category, written as it is destroyed.
QDebug streamOf(LogLevel level, const QLoggingCategory& category)
{
    const QMessageLogger logger;
    switch(level)
    {
    case LogLevel::Debug:
        return logger.debug(category);
    case LogLevel::Info:
        return logger.info(category);
    case LogLevel::Warning:
        return logger.warning(category);
    case LogLevel::Critical:
        break;
    }
    return logger.critical(category);
}

} // namespace

Log::NamedCategory::NamedCategory(const QString& text) : name(text.toUtf8()), category(name.constData()) {}

Log::Log() : m_category(std::make_unique<NamedCategory>(QStringLiteral("log"))) {}

void Log::write(LogLevel level, const QString& message, const QString& source)
{
    if(!isWritten(level))
    {
        return;
    }
    streamOf(level, m_category->category).noquote() << (source.isEmpty() ? message : source + u": " + message);
}

bool Log::isWritten(LogLevel level) const
{
    return m_category->category.isEnabled(Nebula::qtMsgType(level));
}

QString Log::category() const
{
    return QString::fromUtf8(m_category->name);
}

void Log::setCategory(const QString& category)
{
    // The new one first: the former goes with its name, which it points to.
    m_category = std::make_unique<NamedCategory>(category);
}
