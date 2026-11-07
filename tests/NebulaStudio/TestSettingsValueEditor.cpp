// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QSignalSpy>
#include <QStringList>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <cstdint>
#include <memory>

#include "Nebula/Application/Universe.h"
#include "Nebula/Settings/SettingsFile.h"
#include "Options.h"
#include "Panel.h"
#include "PlaceholderStack.h"
#include "Probe.h"
#include "Settings/SettingsValueEditor.h"
#include "TestComponents.h"
#include "WarningLabel.h"

namespace
{

enum Column : std::uint8_t
{
    NameColumn,
    ValueColumn,
};

// The JSON object the file filePath holds; empty when it holds none.
QJsonObject readJson(const QString& filePath)
{
    QFile file(filePath);
    return file.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(file.readAll()).object() : QJsonObject();
}

// Writes content to the file filePath, as another program would.
bool writeFile(const QString& filePath, const QByteArray& content)
{
    QFile file(filePath);
    return file.open(QIODevice::WriteOnly) && file.write(content) == content.size();
}

// The JSON object text holds.
QJsonObject json(const QByteArray& text)
{
    return QJsonDocument::fromJson(text).object();
}

// What the cells of the column column of table show, row by row.
QStringList column(const QTableWidget& table, int column)
{
    QStringList texts;
    for(int row = 0; row < table.rowCount(); ++row)
    {
        const QTableWidgetItem* item = table.item(row, column);
        texts.append(item != nullptr ? item->text() : QString());
    }
    return texts;
}

// The rows of the values of table, those whose value is in italics, the default ones, marked with a star after them.
QStringList values(const QTableWidget& table)
{
    QStringList texts;
    for(int row = 0; row < table.rowCount(); ++row)
    {
        const QTableWidgetItem* item = table.item(row, ValueColumn);
        if(item != nullptr)
        {
            texts.append(table.item(row, NameColumn)->text() + u'=' + item->text() +
                         (item->font().italic() ? QStringLiteral("*") : QString()));
        }
    }
    return texts;
}

} // namespace

// SettingsValueEditor: the values a settings file holds, by instance, edited there and written to the file, which it
// follows.
class TestSettingsValueEditor : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void cleanup();

    void rows();
    void noFileYet();
    void valuesOfTheFile_data();
    void valuesOfTheFile();
    void unreadable();
    void edit();
    void editRefused();
    void followsTheFile();
    void followsTheEntries();
    void whileRunning();
    void noEntry();

  private:
    // The row of the value of the property name of the instance instance.
    [[nodiscard]] int rowOf(const QString& instance, const QString& name) const;
    // The text that tells where the file is.
    [[nodiscard]] QString path() const;
    // The muted text under it: the file does not exist yet, or the application runs; empty when it is hidden.
    [[nodiscard]] QString note() const;
    // Whether the values can be edited.
    [[nodiscard]] bool isEditable() const;

    // An application with a Probe1 and a Panel1, and a settings file in m_directory that holds the count and the text
    // of Probe1, then the level of the second page of Panel1, and the level of its general options, shown by m_editor,
    // whose table is m_table.
    std::unique_ptr<QTemporaryDir> m_directory;
    std::unique_ptr<Nebula::Universe> m_application;
    Probe* m_probe = nullptr;
    Panel* m_panel = nullptr;
    Nebula::SettingsFile* m_file = nullptr;
    std::unique_ptr<SettingsValueEditor> m_editor;
    QTableWidget* m_table = nullptr;
};

void TestSettingsValueEditor::initTestCase()
{
    registerTestComponents();
    // Once, before any test fails on a warning: the offscreen platform warns that it has no fonts the first time a text
    // is laid out.
    QFontDatabase::families();
}

void TestSettingsValueEditor::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
    m_directory = std::make_unique<QTemporaryDir>();
    QVERIFY(m_directory->isValid());
    m_application = std::make_unique<Nebula::Universe>();
    m_probe = qobject_cast<Probe*>(m_application->instantiate(testComponentId(QStringLiteral("Probe"))));
    m_panel = qobject_cast<Panel*>(m_application->instantiate(testComponentId(QStringLiteral("Panel"))));
    QVERIFY(m_probe != nullptr && m_panel != nullptr);
    m_probe->setProperty("count", 3);
    m_probe->setProperty("text", QStringLiteral("project"));
    m_file = &m_application->addSettingsFile();
    m_file->setDirectory(m_directory->path());
    QVERIFY(m_file->add({.component = m_probe, .object = m_probe, .property = QStringLiteral("count")}));
    QVERIFY(m_file->add({.component = m_probe, .object = m_probe, .property = QStringLiteral("text")}));
    QVERIFY(m_file->add({.component = m_panel, .object = m_panel->pages().at(1), .property = QStringLiteral("level")}));
    QVERIFY(m_file->add({.component = m_panel, .object = &m_panel->general(), .property = QStringLiteral("level")}));
    m_editor = std::make_unique<SettingsValueEditor>(*m_application);
    m_editor->setFile(m_file);
    m_table = m_editor->findChild<QTableWidget*>();
    QVERIFY(m_table != nullptr);
}

void TestSettingsValueEditor::cleanup()
{
    m_editor.reset(); // Before its directory, which it watches
    m_application.reset();
    m_directory.reset();
}

int TestSettingsValueEditor::rowOf(const QString& instance, const QString& name) const
{
    const QStringList names = column(*m_table, NameColumn);
    return static_cast<int>(names.indexOf(name, names.indexOf(instance)));
}

QString TestSettingsValueEditor::path() const
{
    const QString shown = QDir::toNativeSeparators(m_file->filePath());
    for(const QLabel* label : m_editor->findChildren<QLabel*>())
    {
        if(label->text() == shown)
        {
            return shown;
        }
    }
    return {};
}

QString TestSettingsValueEditor::note() const
{
    for(const QLabel* label : m_editor->findChildren<QLabel*>())
    {
        if(label->foregroundRole() == QPalette::PlaceholderText && label->isVisibleTo(m_editor.get()) &&
           label->text() != path())
        {
            return label->text();
        }
    }
    return {};
}

bool TestSettingsValueEditor::isEditable() const
{
    return m_table->editTriggers() != QAbstractItemView::NoEditTriggers;
}

// A row per property, under the name of its instance, after the name of its object when it has one.
void TestSettingsValueEditor::rows()
{
    QCOMPARE(column(*m_table, NameColumn), QStringList({"Probe1", "count", "text", "Panel1", "page.level", "level"}));
    // The titles across the table.
    QCOMPARE(m_table->columnSpan(0, NameColumn), 2);
    QCOMPARE(m_table->columnSpan(1, NameColumn), 1);
    QCOMPARE(path(), QDir::toNativeSeparators(m_file->filePath()));
}

// Until the file exists, the values are the default ones, in italics, and it tells that the application writes it.
void TestSettingsValueEditor::noFileYet()
{
    QCOMPARE(values(*m_table), QStringList({"count=3*", "text=project*", "page.level=0*", "level=0*"}));
    QVERIFY(note().startsWith(QStringLiteral("It does not exist yet")));
    QVERIFY(isEditable());
    QVERIFY(!m_editor->findChild<WarningLabel*>()->isVisibleTo(m_editor.get()));
}

void TestSettingsValueEditor::valuesOfTheFile_data()
{
    QTest::addColumn<QByteArray>("content");
    QTest::addColumn<QStringList>("values");

    QTest::newRow("all") << QByteArray(R"({"Probe1": {"properties": {"count": 7, "text": "user"}},
        "Panel1": {"objects": [{"name": "", "properties": {"level": 2}},
                               {"name": "page", "properties": {}}, {"name": "page", "properties": {"level": 4}}]}})")
                         << QStringList({"count=7", "text=user", "page.level=4", "level=2"});
    QTest::newRow("some") << QByteArray(R"({"Other": {}, "Probe1": {"properties": {"count": 7}}})")
                          << QStringList({"count=7", "text=project*", "page.level=0*", "level=0*"});
    QTest::newRow("another type") << QByteArray(R"({"Probe1": {"properties": {"count": {"not": "an int"}}}})")
                                  << QStringList({"count=3*", "text=project*", "page.level=0*", "level=0*"});
}

// The values the file holds; those it holds none for, or one their property cannot have, are the default ones.
void TestSettingsValueEditor::valuesOfTheFile()
{
    QFETCH(QByteArray, content);
    QFETCH(QStringList, values);

    QVERIFY(writeFile(m_file->filePath(), content));
    m_editor->setFile(m_file); // Read again at once

    QCOMPARE(::values(*m_table), values);
    // Of the type of their properties, which gives their cells their editors.
    QCOMPARE(m_table->item(rowOf("Probe1", "count"), ValueColumn)->data(Qt::EditRole).typeId(), QMetaType::Int);
    QCOMPARE(note(), QString());
    QVERIFY(isEditable());
}

// A file that cannot be read is told, and its values are not edited.
void TestSettingsValueEditor::unreadable()
{
    QVERIFY(writeFile(m_file->filePath(), "{ count"));
    m_editor->setFile(m_file);

    const auto* problem = m_editor->findChild<WarningLabel*>();
    QVERIFY(problem->isVisibleTo(m_editor.get()));
    QVERIFY(problem->text().startsWith(QStringLiteral("It cannot be read (")));
    QCOMPARE(values(*m_table), QStringList({"count=3*", "text=project*", "page.level=0*", "level=0*"}));
    QVERIFY(!isEditable());
    // Neither written nor set aside.
    QFile file(m_file->filePath());
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), QByteArray("{ count"));
}

// A value edited is written to the file at once, the project keeping its own.
void TestSettingsValueEditor::edit()
{
    QVERIFY(writeFile(m_file->filePath(), R"({"Other": 1})"));
    m_editor->setFile(m_file);
    const QSignalSpy refused(m_editor.get(), &SettingsValueEditor::refused);

    m_table->item(rowOf("Probe1", "count"), ValueColumn)->setData(Qt::EditRole, 9);
    m_table->item(rowOf("Panel1", "page.level"), ValueColumn)->setData(Qt::EditRole, 5);

    QCOMPARE(readJson(m_file->filePath()), json(R"({"Other": 1, "Probe1": {"properties": {"count": 9}},
        "Panel1": {"objects": [{"name": "page", "properties": {}}, {"name": "page", "properties": {"level": 5}}]}})"));
    QCOMPARE(values(*m_table), QStringList({"count=9", "text=project*", "page.level=5", "level=0*"}));
    QCOMPARE(m_probe->property("count").toInt(), 3);
    QCOMPARE(refused.count(), 0);
}

// A value the property cannot have is not written, which it tells.
void TestSettingsValueEditor::editRefused()
{
    const QSignalSpy refused(m_editor.get(), &SettingsValueEditor::refused);

    m_table->item(rowOf("Probe1", "count"), ValueColumn)->setData(Qt::EditRole, QStringLiteral("nine"));

    QCOMPARE(refused.count(), 1);
    QCOMPARE(refused.first().first().toString(),
             QStringLiteral("count is not written: settings.json cannot hold this value for count"));
    QVERIFY(!QFile::exists(m_file->filePath()));
    QCOMPARE(values(*m_table).first(), QStringLiteral("count=3*"));
}

// What another program writes to the file is shown, from its creation on.
void TestSettingsValueEditor::followsTheFile()
{
    QVERIFY(writeFile(m_file->filePath(), R"({"Probe1": {"properties": {"count": 7}}})"));
    QTRY_COMPARE(values(*m_table).first(), QStringLiteral("count=7"));
    QTRY_VERIFY(note().isEmpty());

    QVERIFY(writeFile(m_file->filePath(), R"({"Probe1": {"properties": {"count": 8, "text": "user"}}})"));
    QTRY_COMPARE(values(*m_table).mid(0, 2), QStringList({"count=8", "text=user"}));

    QVERIFY(QFile::remove(m_file->filePath()));
    QTRY_COMPARE(values(*m_table).first(), QStringLiteral("count=3*"));
}

// The rows follow the entries of the file and the names of their instances, under which the file holds their values.
void TestSettingsValueEditor::followsTheEntries()
{
    QVERIFY(writeFile(m_file->filePath(), R"({"Probe1": {"properties": {"count": 7}}})"));
    m_editor->setFile(m_file);

    m_file->remove({.component = m_probe, .object = m_probe, .property = QStringLiteral("text")});
    QCOMPARE(column(*m_table, NameColumn), QStringList({"Probe1", "count", "Panel1", "page.level", "level"}));
    QVERIFY(m_application->rename(*m_probe, QStringLiteral("Renamed")));
    QCOMPARE(column(*m_table, NameColumn).first(), QStringLiteral("Renamed"));
    QCOMPARE(values(*m_table).first(), QStringLiteral("count=3*"));
    QVERIFY(m_application->remove(*m_panel));
    QCOMPARE(column(*m_table, NameColumn), QStringList({"Renamed", "count"}));

    m_application->removeSettingsFile(*m_file);
    QCOMPARE(m_editor->file(), nullptr);
    QCOMPARE(m_table->rowCount(), 0);
}

// While the application runs, the values it writes are shown, and not edited.
void TestSettingsValueEditor::whileRunning()
{
    QVERIFY(m_application->start());
    QVERIFY(!isEditable());
    QVERIFY(note().startsWith(QStringLiteral("The application runs")));
    // Written with the values of the project as it started, then with those it changes.
    QCOMPARE(values(*m_table), QStringList({"count=3", "text=project", "page.level=0", "level=0"}));
    m_probe->setProperty("count", 6);
    QTRY_COMPARE(values(*m_table).first(), QStringLiteral("count=6"));

    m_application->stop();
    QVERIFY(isEditable());
    QCOMPARE(note(), QString());
    QCOMPARE(values(*m_table).first(), QStringLiteral("count=6"));
    QCOMPARE(m_probe->property("count").toInt(), 3);
}

// A file without entry tells how to choose them, in place of the values.
void TestSettingsValueEditor::noEntry()
{
    Nebula::SettingsFile& empty = m_application->addSettingsFile();
    m_editor->setFile(&empty);

    const auto* stack = m_editor->findChild<PlaceholderStack*>();
    QCOMPARE(stack->currentWidget(), stack->placeholder());
    QCOMPARE(stack->placeholder()->text(),
             QStringLiteral("settings2.json holds no property yet: double-click it to choose those it holds"));
    QVERIFY(empty.add({.component = m_probe, .object = m_probe, .property = QStringLiteral("ratio")}));
    QVERIFY(stack->currentWidget() != stack->placeholder());
}

QTEST_MAIN(TestSettingsValueEditor)
#include "TestSettingsValueEditor.moc"
