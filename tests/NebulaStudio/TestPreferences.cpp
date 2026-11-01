// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <memory>

#include "Preferences.h"

using namespace Qt::StringLiterals;

// What NebulaStudio keeps from one launch to the next, Preferences.h: in a JSON file of its own here, not in the
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
    void colorSchemeByDefault();
    void colorScheme_data();
    void colorScheme();
    void unknownColorScheme();
    void jsonFile();
    void othersKept();
    void unreadableFile();

  private:
    // The path of the file of the preferences, in a directory removed after each test.
    [[nodiscard]] QString filePath() const;
    // New settings on that file, read again from it.
    [[nodiscard]] std::unique_ptr<QSettings> settings() const;
    // What that file holds, as JSON: an empty object when it holds no object.
    [[nodiscard]] QJsonObject written() const;
    // Writes json to that file.
    void write(const QByteArray& json) const;

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

QString TestPreferences::filePath() const
{
    return m_directory->filePath(u"preferences.json"_s);
}

std::unique_ptr<QSettings> TestPreferences::settings() const
{
    return std::make_unique<QSettings>(filePath(), preferencesFormat());
}

QJsonObject TestPreferences::written() const
{
    QFile file(filePath());
    return file.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(file.readAll()).object() : QJsonObject();
}

void TestPreferences::write(const QByteArray& json) const
{
    QFile file(filePath());
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(json);
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

    QTest::newRow("curves") << LinkStyle::Curved << u"curves"_s;
    QTest::newRow("straight lines") << LinkStyle::Straight << u"straightLines"_s;
    QTest::newRow("right angles") << LinkStyle::RightAngled << u"rightAngles"_s;
}

// The style written is read again at the next launch, by its name.
void TestPreferences::linkStyle()
{
    QFETCH(LinkStyle, style);
    QFETCH(QString, name);

    writeLinkStyle(*settings(), style); // Written to the file as the settings go
    const std::unique_ptr<QSettings> again = settings();
    QCOMPARE(readLinkStyle(*again), style);
    QCOMPARE(again->value(u"edition/linkStyle"_s).toString(), name);
}

// A style the file holds but NebulaStudio does not know, from another version or written by hand, draws curves.
void TestPreferences::unknownLinkStyle()
{
    settings()->setValue(u"edition/linkStyle"_s, u"zigzag"_s);
    QCOMPARE(readLinkStyle(*settings()), LinkStyle::Curved);
}

// NebulaStudio is in the color scheme of the system until another one is chosen.
void TestPreferences::colorSchemeByDefault()
{
    QCOMPARE(readColorScheme(*settings()), Qt::ColorScheme::Unknown);
}

void TestPreferences::colorScheme_data()
{
    QTest::addColumn<Qt::ColorScheme>("scheme");
    QTest::addColumn<QString>("name"); // As the file holds it

    QTest::newRow("system") << Qt::ColorScheme::Unknown << u"system"_s;
    QTest::newRow("light") << Qt::ColorScheme::Light << u"light"_s;
    QTest::newRow("dark") << Qt::ColorScheme::Dark << u"dark"_s;
}

// The scheme written is read again at the next launch, by its name.
void TestPreferences::colorScheme()
{
    QFETCH(Qt::ColorScheme, scheme);
    QFETCH(QString, name);

    writeColorScheme(*settings(), scheme);
    const std::unique_ptr<QSettings> again = settings();
    QCOMPARE(readColorScheme(*again), scheme);
    QCOMPARE(again->value(u"appearance/colorScheme"_s).toString(), name);
}

// A scheme the file holds but NebulaStudio does not know is the one of the system.
void TestPreferences::unknownColorScheme()
{
    settings()->setValue(u"appearance/colorScheme"_s, u"sepia"_s);
    QCOMPARE(readColorScheme(*settings()), Qt::ColorScheme::Unknown);
}

// The file is a JSON object, each group of the keys an object of its own.
void TestPreferences::jsonFile()
{
    {
        const std::unique_ptr<QSettings> preferences = settings();
        writeLinkStyle(*preferences, LinkStyle::RightAngled);
        writeColorScheme(*preferences, Qt::ColorScheme::Dark);
    }
    QCOMPARE(written(), (QJsonObject{{u"edition"_s, QJsonObject{{u"linkStyle"_s, u"rightAngles"_s}}},
                                     {u"appearance"_s, QJsonObject{{u"colorScheme"_s, u"dark"_s}}}}));
}

// What the file holds that NebulaStudio does not know, from another version say, is kept when it is written again.
void TestPreferences::othersKept()
{
    write(R"({"edition": {"linkStyle": "straightLines", "grid": true}, "other": {"size": 3}})");
    {
        const std::unique_ptr<QSettings> preferences = settings();
        QCOMPARE(readLinkStyle(*preferences), LinkStyle::Straight);
        writeColorScheme(*preferences, Qt::ColorScheme::Light);
    }
    QCOMPARE(written(),
             (QJsonObject{{u"edition"_s, QJsonObject{{u"linkStyle"_s, u"straightLines"_s}, {u"grid"_s, true}}},
                          {u"other"_s, QJsonObject{{u"size"_s, 3}}},
                          {u"appearance"_s, QJsonObject{{u"colorScheme"_s, u"light"_s}}}}));
}

// A file that is not a JSON object, written by hand say, is not read: the preferences are those by default, and the file
// is written again with the one chosen then.
void TestPreferences::unreadableFile()
{
    write("{ not json");
    {
        const std::unique_ptr<QSettings> preferences = settings();
        QCOMPARE(readLinkStyle(*preferences), LinkStyle::Curved);
        QCOMPARE(readColorScheme(*preferences), Qt::ColorScheme::Unknown);
        QCOMPARE(preferences->status(), QSettings::FormatError);
        writeColorScheme(*preferences, Qt::ColorScheme::Dark);
    }
    QCOMPARE(written(), (QJsonObject{{u"appearance"_s, QJsonObject{{u"colorScheme"_s, u"dark"_s}}}}));
}

QTEST_GUILESS_MAIN(TestPreferences)
#include "TestPreferences.moc"
