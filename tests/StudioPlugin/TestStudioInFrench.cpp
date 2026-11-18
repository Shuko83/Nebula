// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>

#include "StudioLanguageTest.h"

using namespace Qt::StringLiterals;

// NebulaStudio with French in its preferences: its window is in French, and nothing is told about the language.
class TestStudioInFrench : public QObject
{
    Q_OBJECT

  private slots:
    void windowInFrench();
};

void TestStudioInFrench::windowInFrench()
{
    StudioLanguageRun run;
    QVERIFY(run.prepare("fr"));
    QVERIFY2(run.open(), qPrintable(run.universe().errorString()));
    QVERIFY2(run.universe().start(), qPrintable(run.universe().errorString()));
    QMainWindow* window = StudioLanguageRun::window();
    QVERIFY(window != nullptr);
    QStringList titles;
    for(const QAction* menu : window->menuBar()->actions())
    {
        titles.append(menu->text());
    }
    QCOMPARE(titles, QStringList({u"&Fichier"_s, u"\u00C9&dition"_s, u"&Affichage"_s, u"E&x\u00E9cution"_s}));
    QTest::qWait(0); // What the window tells once it is shown
    QVERIFY(QApplication::activeModalWidget() == nullptr);
    QVERIFY(window->statusBar()->currentMessage().isEmpty());
}

QTEST_MAIN(TestStudioInFrench)
#include "TestStudioInFrench.moc"
