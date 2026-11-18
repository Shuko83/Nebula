// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QLatin1StringView>
#include <QString>
#include <QStringList>
#include <array>

class QMainWindow;

// The language NebulaStudio is displayed in: the one of its preferences, else the one of the system, else English,
// its source language, which it always has. Chosen once, as it starts: what is already made is not translated again.

// A language NebulaStudio knows, under its code, and its name, written in that language.
struct Language
{
    QLatin1StringView code;
    const char* name; // In UTF-8
};

inline constexpr QLatin1StringView DefaultLanguage("en");
// What the preferences hold to follow the language of the system.
inline constexpr QLatin1StringView SystemLanguage("system");
inline constexpr std::array Languages{
    Language{.code = DefaultLanguage, .name = "English"},
    Language{.code = QLatin1StringView("fr"),
             .name = "Fran\xC3\xA7"
                     "ais"},
};

// The language chosen, and those that could not be.
struct LanguageChoice
{
    QString shown;              // The code of the language displayed
    QString preferred = {};     // What the preferences ask for, when it is not available
    QString system = {};        // The code of the first language of the system, when it is not available
    bool shownBySystem = false; // Whether the language displayed is the first one of the system
};

// The languages NebulaStudio can be displayed in: English, and those whose translation it was built with.
[[nodiscard]] QStringList availableLanguages();
// Chooses among available, by their code: preferred, what the preferences hold, unless it is SystemLanguage or empty,
// else the first of the languages of the system that is available (fr-CA gives fr), else English.
[[nodiscard]] LanguageChoice chooseLanguage(const QString& preferred, const QStringList& system,
                                            const QStringList& available);
// Why choice is not what was asked for, in the language displayed: empty when it is.
[[nodiscard]] QString languageWarning(const LanguageChoice& choice);
// Chooses the language from preferred and the system, gives the application its translations, those of Qt included,
// and logs languageWarning(). Needs a QCoreApplication; to be called once, before what it translates is made.
LanguageChoice applyLanguage(const QString& preferred);
// Tells the user languageWarning(), if any, over window: in a box when their preference is not followed, in the status
// bar when only the system is not, which would otherwise ask them for it at each launch.
void tellLanguage(QMainWindow& window, const LanguageChoice& choice);
