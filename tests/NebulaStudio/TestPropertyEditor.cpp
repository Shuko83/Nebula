// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QFontDatabase>
#include <QStringList>
#include <QTableWidget>
#include <QTest>
#include <cstdint>
#include <memory>

#include "Nebula/Application/Universe.h"
#include "Nebula/Component/ComponentFactory.h"
#include "Outlets.h"
#include "PropertyEditor.h"
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

} // namespace

// PropertyEditor: the properties it shows of a component and of its objects, as they come and go.
class TestPropertyEditor : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void cleanup();

    void objectsThatFollowAProperty();

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
    m_table = m_editor->findChild<QTableWidget*>();
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

QTEST_MAIN(TestPropertyEditor)
#include "TestPropertyEditor.moc"
