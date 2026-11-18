// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QApplication>
#include <QFontDatabase>
#include <QMainWindow>
#include <QMessageBox>
#include <QRegularExpression>
#include <QStatusBar>
#include <QTest>
#include <QTimer>
#include <QTranslator>

#include "Language.h"

using namespace Qt::StringLiterals;

Q_DECLARE_METATYPE(LanguageChoice)

// The language NebulaStudio is displayed in, Language.h: what is chosen from given languages, whatever those of the
// system that runs the tests, what tells the user when it is not the one asked for, and the translations embedded.
class TestLanguage : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void cleanup();

    void chosen_data();
    void chosen();
    void frenchIsAvailable();
    void warning_data();
    void warning();
    void warningFollowsTheLanguageShown();
    void unavailableLanguageLogged();
    void translationsApplied();
    void englishHasNoTranslation();
    void preferenceToldInABox();
    void systemToldInTheStatusBar();
    void nothingToldWhenAsked();
};

void TestLanguage::initTestCase()
{
    QFontDatabase::families(); // The platform of the tests warns, once, that it has no fonts: before failOnWarning()
}

void TestLanguage::init()
{
    QTest::failOnWarning();
}

void TestLanguage::cleanup()
{
    qDeleteAll(qApp->findChildren<QTranslator*>(Qt::FindDirectChildrenOnly));
}

void TestLanguage::chosen_data()
{
    QTest::addColumn<QString>("preferred");
    QTest::addColumn<QStringList>("system");
    QTest::addColumn<QString>("shown");
    QTest::addColumn<QString>("unavailablePreference");
    QTest::addColumn<QString>("unavailableSystem");
    QTest::addColumn<bool>("shownBySystem");

    const QStringList french{u"fr-FR"_s, u"fr"_s};
    const QStringList german{u"de-DE"_s};
    QTest::newRow("preference") << u"fr"_s << german << u"fr"_s << QString() << QString() << false;
    QTest::newRow("preference over the system") << u"en"_s << french << u"en"_s << QString() << QString() << false;
    QTest::newRow("region of a preference") << u"fr_CA"_s << german << u"fr"_s << QString() << QString() << false;
    QTest::newRow("system") << u"system"_s << french << u"fr"_s << QString() << QString() << true;
    QTest::newRow("no preference") << QString() << french << u"fr"_s << QString() << QString() << true;
    QTest::newRow("English system") << u"system"_s << QStringList{u"en-GB"_s} << u"en"_s << QString() << QString()
                                    << true;
    QTest::newRow("C locale") << u"system"_s << QStringList{u"C"_s} << u"en"_s << QString() << QString() << true;
    QTest::newRow("system not available") << u"system"_s << german << u"en"_s << QString() << u"de"_s << false;
    QTest::newRow("next language of the system")
        << u"system"_s << QStringList{u"de-DE"_s, u"fr-FR"_s} << u"fr"_s << QString() << u"de"_s << false;
    QTest::newRow("no language of the system")
        << u"system"_s << QStringList() << u"en"_s << QString() << QString() << false;
    QTest::newRow("preference not available, system") << u"de"_s << french << u"fr"_s << u"de"_s << QString() << true;
    QTest::newRow("neither available") << u"de"_s << QStringList{u"it-IT"_s} << u"en"_s << u"de"_s << u"it"_s << false;
    QTest::newRow("unknown preference") << u"klingon!"_s << french << u"fr"_s << u"klingon!"_s << QString() << true;
}

void TestLanguage::chosen()
{
    QFETCH(QString, preferred);
    QFETCH(QStringList, system);
    QFETCH(QString, shown);
    QFETCH(QString, unavailablePreference);
    QFETCH(QString, unavailableSystem);
    QFETCH(bool, shownBySystem);

    const LanguageChoice choice = chooseLanguage(preferred, system, {u"en"_s, u"fr"_s});
    QCOMPARE(choice.shown, shown);
    QCOMPARE(choice.preferred, unavailablePreference);
    QCOMPARE(choice.system, unavailableSystem);
    QCOMPARE(choice.shownBySystem, shownBySystem);
}

// Built with its translation, which this test links, French is available, along with English.
void TestLanguage::frenchIsAvailable()
{
    QCOMPARE(availableLanguages(), QStringList({u"en"_s, u"fr"_s}));
}

void TestLanguage::warning_data()
{
    QTest::addColumn<LanguageChoice>("choice");
    QTest::addColumn<QString>("expected");

    QTest::newRow("as asked") << LanguageChoice{.shown = u"fr"_s} << QString();
    QTest::newRow("system") << LanguageChoice{.shown = u"en"_s, .system = u"de"_s}
                            << u"The language of the system, Deutsch (de), is not available. "
                               "NebulaStudio is displayed in English."_s;
    QTest::newRow("preference") << LanguageChoice{.shown = u"fr"_s, .preferred = u"de"_s, .shownBySystem = true}
                                << u"The language set in the preferences, Deutsch (de), is not available. "
                                   "NebulaStudio is displayed in Français, the language of the system."_s;
    QTest::newRow("preference, no system language")
        << LanguageChoice{.shown = u"en"_s, .preferred = u"de"_s}
        << u"The language set in the preferences, Deutsch (de), is not available. "
           "NebulaStudio is displayed in English."_s;
    QTest::newRow("both") << LanguageChoice{.shown = u"en"_s, .preferred = u"de"_s, .system = u"it"_s}
                          << u"The language set in the preferences, Deutsch (de), is not available, nor is the "
                             "language of the system, italiano (it). NebulaStudio is displayed in English."_s;
    QTest::newRow("no language") << LanguageChoice{.shown = u"en"_s, .preferred = u"klingon!"_s}
                                 << u"The language set in the preferences, klingon!, is not available. "
                                    "NebulaStudio is displayed in English."_s;
}

void TestLanguage::warning()
{
    QFETCH(LanguageChoice, choice);
    QFETCH(QString, expected);
    QCOMPARE(languageWarning(choice), expected);
}

// The warning is written in the language displayed, which the user reads.
void TestLanguage::warningFollowsTheLanguageShown()
{
    QCOMPARE(applyLanguage(u"fr"_s).shown, u"fr"_s);
    const QString warning = languageWarning({.shown = u"fr"_s, .system = u"de"_s});
    QVERIFY2(warning.startsWith(u"La langue du système, Deutsch (de)"_s), qPrintable(warning));
}

// A language that is not available is logged too, whichever one the system gives instead.
void TestLanguage::unavailableLanguageLogged()
{
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(uR"(Deutsch \(de\))"_s));
    QCOMPARE(applyLanguage(u"de"_s).preferred, u"de"_s);
}

// French gives the application the translations of NebulaStudio, and those of Qt, for its standard dialogs.
void TestLanguage::translationsApplied()
{
    const LanguageChoice choice = applyLanguage(u"fr"_s);
    QCOMPARE(choice.shown, u"fr"_s);
    QCOMPARE(QCoreApplication::translate("ProjectComponent", "&File"), u"&Fichier"_s);
    QCOMPARE(QCoreApplication::translate("QPlatformTheme", "Cancel"), u"Annuler"_s);
}

void TestLanguage::englishHasNoTranslation()
{
    QCOMPARE(applyLanguage(u"en"_s).shown, u"en"_s);
    QVERIFY(qApp->findChildren<QTranslator*>(Qt::FindDirectChildrenOnly).isEmpty());
    QCOMPARE(QCoreApplication::translate("ProjectComponent", "&File"), u"&File"_s);
}

// A preference that is not followed waits for the user to read it.
void TestLanguage::preferenceToldInABox()
{
    QMainWindow window;
    QTest::ignoreMessage(QtWarningMsg, "This plugin does not support propagateSizeHints()"); // The box shown
    QString told;
    QTimer::singleShot(0, this,
                       [&told]
                       {
                           auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
                           QVERIFY(box != nullptr);
                           told = box->text();
                           box->accept();
                       });
    const LanguageChoice choice{.shown = u"en"_s, .preferred = u"de"_s};
    tellLanguage(window, choice);
    QCOMPARE(told, languageWarning(choice));
    QVERIFY(window.statusBar()->currentMessage().isEmpty());
}

// The language of the system is not the choice of the user: told in passing, at each launch.
void TestLanguage::systemToldInTheStatusBar()
{
    QMainWindow window;
    const LanguageChoice choice{.shown = u"en"_s, .system = u"de"_s};
    tellLanguage(window, choice);
    QCOMPARE(window.statusBar()->currentMessage(), languageWarning(choice));
    QVERIFY(QApplication::activeModalWidget() == nullptr);
}

void TestLanguage::nothingToldWhenAsked()
{
    QMainWindow window;
    tellLanguage(window, {.shown = u"fr"_s, .shownBySystem = true});
    QVERIFY(window.statusBar()->currentMessage().isEmpty());
    QVERIFY(QApplication::activeModalWidget() == nullptr);
}

QTEST_MAIN(TestLanguage)
#include "TestLanguage.moc"
