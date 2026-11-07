// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QFontDatabase>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QStringList>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <cstdint>
#include <functional>
#include <memory>

#include "Nebula/Application/Universe.h"
#include "Nebula/Component/ComponentFactory.h"
#include "Nebula/Settings/SettingsFile.h"
#include "Outlets.h"
#include "PropertyEditor.h"
#include "Settings/SettingsValueEditor.h"
#include "TestComponents.h"

namespace
{

enum Column : std::uint8_t
{
    NameColumn,
    ValueColumn,
};

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

// The table of the properties of a component in editor, rather than the one of the values of a settings file: the page
// shown is the last child of their stack, as it is raised.
QTableWidget* componentTable(const PropertyEditor& editor)
{
    const auto* settings = editor.findChild<SettingsValueEditor*>();
    for(QTableWidget* table : editor.findChildren<QTableWidget*>())
    {
        if(!settings->isAncestorOf(table))
        {
            return table;
        }
    }
    return nullptr;
}

} // namespace

// PropertyEditor: the properties it shows of a component and of its objects, as they come and go, and the values of a
// settings file in their place.
class TestPropertyEditor : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void cleanup();

    void objectsThatFollowAProperty();
    void writtenThroughItsEdit();
    void settingsFile();

  private:
    // An application with an Outlets1, shown by m_editor, whose table is m_table.
    std::unique_ptr<Nebula::Universe> m_application;
    std::unique_ptr<PropertyEditor> m_editor;
    Outlets* m_outlets = nullptr;
    QTableWidget* m_table = nullptr;
};

void TestPropertyEditor::initTestCase()
{
    registerTestComponents();
    // Once, before any test fails on a warning: the offscreen platform warns that it has no fonts the first time a text
    // is laid out, and the table lays out the names and values of the properties.
    QFontDatabase::families();
}

void TestPropertyEditor::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
    m_application = std::make_unique<Nebula::Universe>();
    m_outlets = qobject_cast<Outlets*>(m_application->instantiate(testComponentId(QStringLiteral("Outlets"))));
    QVERIFY(m_outlets != nullptr);
    m_editor = std::make_unique<PropertyEditor>(*m_application, Nebula::ComponentFactory::instance());
    m_editor->setComponent(m_outlets);
    m_table = componentTable(*m_editor);
    QVERIFY(m_table != nullptr);
}

void TestPropertyEditor::cleanup()
{
    m_table = nullptr;
    m_editor.reset();
    m_application.reset();
}

// Its count, edited in the table, adds or destroys objects: their groups follow once the events are processed, the
// cell edited living until then.
void TestPropertyEditor::objectsThatFollowAProperty()
{
    QCOMPARE(column(*m_table, NameColumn), (QStringList{"count", "active"}));

    QTableWidgetItem* count = m_table->item(0, ValueColumn);
    count->setData(Qt::EditRole, 2);
    QCOMPARE(m_outlets->count(), 2);
    QCOMPARE(m_table->item(0, ValueColumn), count);
    QTRY_COMPARE(column(*m_table, NameColumn),
                 (QStringList{"count", "active", "object1", "level", "enabled", "object2", "level", "enabled"}));

    m_table->item(0, ValueColumn)->setData(Qt::EditRole, 1);
    QCOMPARE(m_outlets->count(), 1);
    QTRY_COMPARE(column(*m_table, NameColumn), (QStringList{"count", "active", "object1", "level", "enabled"}));
    QCOMPARE(column(*m_table, ValueColumn).at(0), QStringLiteral("1"));
}

// A value edited is written through the edit it is given, a step of the history of NebulaStudio, named after the
// property and the component; one the edit does not write is not written.
void TestPropertyEditor::writtenThroughItsEdit()
{
    m_outlets->setObjectName(QStringLiteral("Outlets1"));
    QStringList steps;
    bool writes = true;
    m_editor->setEdit(
        [&steps, &writes](const QString& what, const std::function<void()>& change)
        {
            steps.append(what);
            if(writes)
            {
                change();
            }
        });
    m_table->item(0, ValueColumn)->setData(Qt::EditRole, 2);
    QCOMPARE(steps, QStringList{QStringLiteral("Change count of Outlets1")});
    QCOMPARE(m_outlets->count(), 2);

    writes = false;
    m_table->item(0, ValueColumn)->setData(Qt::EditRole, 5);
    QCOMPARE(steps.size(), 2);
    QCOMPARE(m_outlets->count(), 2);
    QCOMPARE(column(*m_table, ValueColumn).at(0), QStringLiteral("2")); // What it holds, shown again
}

// A settings file selected is shown in place of a component, what it refuses to write told by the editor.
void TestPropertyEditor::settingsFile()
{
    const QTemporaryDir directory;
    Nebula::SettingsFile& file = m_application->addSettingsFile();
    file.setDirectory(directory.path());
    QVERIFY(file.add({.component = m_outlets, .object = m_outlets, .property = QStringLiteral("count")}));
    const auto* pages = m_editor->findChild<QStackedWidget*>();
    auto* settings = m_editor->findChild<SettingsValueEditor*>();
    const QSignalSpy refused(m_editor.get(), &PropertyEditor::refused);

    m_editor->showSelection(m_outlets, &file);
    QCOMPARE(pages->currentWidget(), settings);
    QCOMPARE(settings->file(), &file);
    auto* values = settings->findChild<QTableWidget*>();
    values->item(1, ValueColumn)->setData(Qt::EditRole, QStringLiteral("two"));
    QCOMPARE(refused.count(), 1);

    m_editor->showSelection(m_outlets, nullptr);
    QVERIFY(pages->currentWidget()->isAncestorOf(m_table));
    QCOMPARE(settings->file(), nullptr);
    QCOMPARE(column(*m_table, NameColumn), (QStringList{"count", "active"}));

    // Nothing selected.
    m_editor->setSettingsFile(&file);
    m_editor->setSettingsFile(nullptr);
    QVERIFY(pages->currentWidget()->isAncestorOf(m_table));
    QCOMPARE(m_table->rowCount(), 0);
}

QTEST_MAIN(TestPropertyEditor)
#include "TestPropertyEditor.moc"
