// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QByteArray>
#include <QLoggingCategory>
#include <QString>
#include <memory>

#include "Nebula/Standard/ILog.h"

// What a LogComponent provides as "log": the messages of components, in Qt's journal under its category, which the
// category filter of the process enables or not.
class Log : public Nebula::ILog
{
  public:
    Log();

    void write(Nebula::LogLevel level, const QString& message, const QString& source) override;
    [[nodiscard]] bool isWritten(Nebula::LogLevel level) const override;

    [[nodiscard]] QString category() const;
    // Not empty: the component refuses an empty one.
    void setCategory(const QString& category);

  private:
    // A category of Qt and the name it points to, which must outlive it.
    struct NamedCategory
    {
        explicit NamedCategory(const QString& text);

        QByteArray name;
        QLoggingCategory category;
    };

    std::unique_ptr<NamedCategory> m_category;
};
