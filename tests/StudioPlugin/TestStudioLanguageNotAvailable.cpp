// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QMessageBox>
#include <QRegularExpression>
#include <QTimer>

#include "StudioLanguageTest.h"

using namespace Qt::StringLiterals;

// NebulaStudio with a language it does not have in its preferences: a box tells it over its window, once.
class TestStudioLanguageNotAvailable : public QObject
{
    Q_OBJECT

  private slots:
    void toldOnceInABox();
};

void TestStudioLanguageNotAvailable::toldOnceInABox()
{
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(uR"(Deutsch \(de\))"_s)); // In the log as well
    StudioLanguageRun run;
    QVERIFY(run.prepare("de"));
    QVERIFY2(run.open(), qPrintable(run.universe().errorString()));

    QStringList told;
    QTimer closer; // The box waits for the user, in an event loop of its own
    connect(&closer, &QTimer::timeout, this,
            [&told]
            {
                if(auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget()))
                {
                    told.append(box->text());
                    box->accept();
                }
            });
    closer.start(10);

    QVERIFY2(run.universe().start(), qPrintable(run.universe().errorString()));
    QTRY_COMPARE(told.size(), 1);
    QVERIFY2(told.first().contains(u"Deutsch (de)"_s), qPrintable(told.first()));

    run.universe().stop();
    QVERIFY(run.universe().start()); // Started again: it is told already
    QTest::qWait(50);
    QCOMPARE(told.size(), 1);
}

QTEST_MAIN(TestStudioLanguageNotAvailable)
#include "TestStudioLanguageNotAvailable.moc"
