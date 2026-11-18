// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Language.h"

#include <QCoreApplication>
#include <QFile>
#include <QLocale>
#include <QLoggingCategory>
#include <QMainWindow>
#include <QRegularExpression>
#include <QStatusBar>
#include <QTranslator>
#include <algorithm>

#include "Messages.h"

namespace
{

using namespace Qt::Literals::StringLiterals;

Q_LOGGING_CATEGORY(languageLog, "nebula.studio.language")

// The file of a translation among the resources of StudioPlugin; prefix is nebulastudio, or qtbase for the one of Qt.
QString translationPath(QLatin1StringView prefix, const QString& code)
{
    return u":/translations/%1_%2.qm"_s.arg(prefix, code);
}

// The code of the language of name, a code or the name of a locale: fr for fr-CA or fr_CA; en for the C locale.
QString languageCode(const QString& name)
{
    const QString code = name.section(QRegularExpression(u"[-_]"_s), 0, 0).toLower();
    return code == "c"_L1 || code == "posix"_L1 ? QString(DefaultLanguage) : code;
}

// The first of languages that is available, by its code: English when none is.
QString firstAvailable(const QStringList& languages, const QStringList& available)
{
    for(const QString& language : languages)
    {
        if(available.contains(languageCode(language)))
        {
            return languageCode(language);
        }
    }
    return DefaultLanguage;
}

// The language system gives among available, and the first one of system when it is not available.
LanguageChoice chooseFromSystem(const QStringList& system, const QStringList& available)
{
    const QString shown = firstAvailable(system, available);
    const QString first = system.isEmpty() ? QString() : languageCode(system.first());
    const bool shownBySystem = first == shown;
    return {.shown = shown, .system = shownBySystem ? QString() : first, .shownBySystem = shownBySystem};
}

// The name of the language of code, as NebulaStudio writes it, when it knows it.
QString knownName(const QString& code)
{
    const auto found = std::ranges::find(Languages, code, &Language::code);
    return found != Languages.end() ? QString::fromUtf8(found->name) : QString();
}

// What tells the user which language text asks for: its name in that language and its code, or text alone when it is
// no language.
QString describeLanguage(const QString& text)
{
    const QString known = knownName(languageCode(text));
    const QLocale locale(text);
    const QString name = known.isEmpty() && locale.language() != QLocale::C ? locale.nativeLanguageName() : known;
    return name.isEmpty() ? text : u"%1 (%2)"_s.arg(name, languageCode(text));
}

// The sentence that tells which of the languages asked for is not available.
QString unavailableSentence(const LanguageChoice& choice)
{
    if(choice.preferred.isEmpty())
    {
        return QCoreApplication::translate("Language", "The language of the system, %1, is not available.")
            .arg(describeLanguage(choice.system));
    }
    if(choice.system.isEmpty())
    {
        return QCoreApplication::translate("Language", "The language set in the preferences, %1, is not available.")
            .arg(describeLanguage(choice.preferred));
    }
    return QCoreApplication::translate("Language",
                                       "The language set in the preferences, %1, is not available, nor is the "
                                       "language of the system, %2.")
        .arg(describeLanguage(choice.preferred), describeLanguage(choice.system));
}

// The sentence that tells the language displayed, and why when it is the one of the system.
QString shownSentence(const LanguageChoice& choice)
{
    const QString name = knownName(choice.shown);
    return choice.shownBySystem
               ? QCoreApplication::translate("Language", "NebulaStudio is displayed in %1, the language of the system.")
                     .arg(name)
               : QCoreApplication::translate("Language", "NebulaStudio is displayed in %1.").arg(name);
}

// Gives the application the translation in path, which it keeps until it goes.
void installTranslation(const QString& path)
{
    auto* translator = new QTranslator(QCoreApplication::instance());
    if(translator->load(path))
    {
        QCoreApplication::installTranslator(translator);
        return;
    }
    qCWarning(languageLog, "Cannot read the translation %ls", qUtf16Printable(path));
    delete translator;
}

// Gives the application the translations of code, those of NebulaStudio and of Qt: none for English, their language.
void installLanguage(const QString& code)
{
    if(code != DefaultLanguage)
    {
        installTranslation(translationPath("nebulastudio"_L1, code));
        installTranslation(translationPath("qtbase"_L1, code));
    }
}

} // namespace

QStringList availableLanguages()
{
    QStringList available{DefaultLanguage};
    for(const Language& language : Languages)
    {
        if(QFile::exists(translationPath("nebulastudio"_L1, language.code)))
        {
            available.append(language.code);
        }
    }
    return available;
}

LanguageChoice chooseLanguage(const QString& preferred, const QStringList& system, const QStringList& available)
{
    const bool followsSystem = preferred.isEmpty() || preferred == SystemLanguage;
    if(!followsSystem && available.contains(languageCode(preferred)))
    {
        return {.shown = languageCode(preferred)};
    }
    LanguageChoice choice = chooseFromSystem(system, available);
    choice.preferred = followsSystem ? QString() : preferred;
    return choice;
}

QString languageWarning(const LanguageChoice& choice)
{
    if(choice.preferred.isEmpty() && choice.system.isEmpty())
    {
        return {};
    }
    return unavailableSentence(choice) + u' ' + shownSentence(choice);
}

LanguageChoice applyLanguage(const QString& preferred)
{
    const LanguageChoice choice = chooseLanguage(preferred, QLocale::system().uiLanguages(), availableLanguages());
    installLanguage(choice.shown);
    const QString warning = languageWarning(choice);
    if(!warning.isEmpty())
    {
        qCWarning(languageLog, "%ls", qUtf16Printable(warning));
    }
    return choice;
}

void tellLanguage(QMainWindow& window, const LanguageChoice& choice)
{
    const QString warning = languageWarning(choice);
    if(!choice.preferred.isEmpty())
    {
        warn(&window, QCoreApplication::translate("Language", "Language not available"), warning);
    }
    else if(!warning.isEmpty())
    {
        tell(*window.statusBar(), warning);
    }
}
