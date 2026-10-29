// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <memory>

#include "Preferences.h"

// What NebulaStudio keeps from one launch to the next, Preferences.h: in a file of its own here, not in the
// preferences of the user who runs the tests.
class TestPreferences : public QObject
{
    Q_OBJECT

  private slots:
    void init();
    void cleanup();

    void linkStyleByDefault();
    void linkStyle_data();
    void linkStyle();
    void unknownLinkStyle();

  private:
    // A new file of settings, written to a directory removed after each test, and read again from it.
    [[nodiscard]] std::unique_ptr<QSettings> settings() const;

    std::unique_ptr<QTemporaryDir> m_directory;
};

void TestPreferences::init()
{
    QTest::failOnWarning();
    m_directory = std::make_unique<QTemporaryDir>();
    QVERIFY(m_directory->isValid());
}

void TestPreferences::cleanup()
{
    m_directory.reset();
}

std::unique_ptr<QSettings> TestPreferences::settings() const
{
    return std::make_unique<QSettings>(m_directory->filePath(QStringLiteral("preferences.ini")), QSettings::IniFormat);
}

// The links are drawn as curves until another style is chosen.
void TestPreferences::linkStyleByDefault()
{
    QCOMPARE(readLinkStyle(*settings()), LinkStyle::Curved);
}

void TestPreferences::linkStyle_data()
{
    QTest::addColumn<LinkStyle>("style");
    QTest::addColumn<QString>("name"); // As the file holds it

    QTest::newRow("curves") << LinkStyle::Curved << QStringLiteral("curves");
    QTest::newRow("straight lines") << LinkStyle::Straight << QStringLiteral("straightLines");
    QTest::newRow("right angles") << LinkStyle::RightAngled << QStringLiteral("rightAngles");
}

// The style written is read again at the next launch, by its name.
void TestPreferences::linkStyle()
{
    QFETCH(LinkStyle, style);
    QFETCH(QString, name);

    writeLinkStyle(*settings(), style); // Written to the file as the settings go
    const std::unique_ptr<QSettings> again = settings();
    QCOMPARE(readLinkStyle(*again), style);
    QCOMPARE(again->value(QStringLiteral("edition/linkStyle")).toString(), name);
}

// A style the file holds but NebulaStudio does not know, from another version or written by hand, draws curves.
void TestPreferences::unknownLinkStyle()
{
    settings()->setValue(QStringLiteral("edition/linkStyle"), QStringLiteral("zigzag"));
    QCOMPARE(readLinkStyle(*settings()), LinkStyle::Curved);
}

QTEST_GUILESS_MAIN(TestPreferences)
#include "TestPreferences.moc"
