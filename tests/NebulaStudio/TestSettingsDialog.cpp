// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStringList>
#include <QTest>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <memory>

#include "DialogHeader.h"
#include "Nebula/Application/Universe.h"
#include "Nebula/Component/ComponentFactory.h"
#include "Nebula/Settings/SettingsFile.h"
#include "Options.h"
#include "Panel.h"
#include "PlaceholderStack.h"
#include "Probe.h"
#include "Settings/SettingsDialog.h"
#include "TestComponents.h"
#include "WarningLabel.h"

namespace
{

// The column of the properties, where they are checked.
constexpr int PropertyColumn = 0;
constexpr int FileColumn = 2;

// The path of row: the texts of the rows above it, then its own, joined by slashes.
QString pathOf(const QTreeWidgetItem& row)
{
    QStringList texts{row.text(PropertyColumn)};
    for(const QTreeWidgetItem* parent = row.parent(); parent != nullptr; parent = parent->parent())
    {
        texts.prepend(parent->text(PropertyColumn));
    }
    return texts.join(u'/');
}

// The first row of tree whose path is path; nullptr when none is.
QTreeWidgetItem* rowAt(QTreeWidget& tree, const QString& path)
{
    for(QTreeWidgetItemIterator row(&tree); *row != nullptr; ++row)
    {
        if(pathOf(**row) == path)
        {
            return *row;
        }
    }
    return nullptr;
}

// Whether row is shown: neither it nor a row above it is hidden.
bool isShown(const QTreeWidgetItem& row)
{
    for(const QTreeWidgetItem* current = &row; current != nullptr; current = current->parent())
    {
        if(current->isHidden())
        {
            return false;
        }
    }
    return true;
}

// The paths of the properties of tree, the rows that group none, that accepts accepts.
QStringList properties(QTreeWidget& tree, const std::function<bool(const QTreeWidgetItem&)>& accepts)
{
    QStringList paths;
    for(QTreeWidgetItemIterator row(&tree); *row != nullptr; ++row)
    {
        if((*row)->childCount() == 0 && accepts(**row))
        {
            paths.append(pathOf(**row));
        }
    }
    return paths;
}

// Checks row when it is not, unchecks it otherwise, as a click does.
void click(QTreeWidgetItem& row)
{
    row.setCheckState(PropertyColumn, row.checkState(PropertyColumn) == Qt::Checked ? Qt::Unchecked : Qt::Checked);
}

// The state of row, as the data of the tests tell it after its path.
QString stateOf(QTreeWidget& tree, const QString& path)
{
    switch(rowAt(tree, path)->checkState(PropertyColumn))
    {
    case Qt::Checked:
        return path + QStringLiteral(" checked");
    case Qt::PartiallyChecked:
        return path + QStringLiteral(" partially");
    default:
        return path + QStringLiteral(" unchecked");
    }
}

// The line edit of dialog showing placeholder when empty.
QLineEdit* field(const QDialog& dialog, const QString& placeholder)
{
    for(QLineEdit* edit : dialog.findChildren<QLineEdit*>())
    {
        if(edit->placeholderText() == placeholder)
        {
            return edit;
        }
    }
    return nullptr;
}

} // namespace

// SettingsDialog: the properties a settings file holds, checked one by one or by their component or group, filtered,
// counted, and given to the file; the name of the file, refused when another file has it.
class TestSettingsDialog : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void cleanup();

    void heldByAnotherFile();
    void check_data();
    void check();
    void filter_data();
    void filter();
    void name_data();
    void name();
    void apply();
    void noneHeld();
    void noProperty();

  private:
    // The tree of the properties of the dialog.
    [[nodiscard]] QTreeWidget& tree() const;
    // The field of the name of the file, as long as it shows the name of m_file.
    [[nodiscard]] QLineEdit* nameField() const;
    // The placeholder of the properties, when it is shown in place of the tree; nullptr otherwise.
    [[nodiscard]] QLabel* shownPlaceholder() const;
    // The text that tells how many properties are held.
    [[nodiscard]] QString count() const;

    // An application with a Probe1, then a Panel1, and two settings files: m_file, settings.json, which holds the count
    // of Probe1 and is edited by m_dialog, and other.json, which holds its ratio.
    std::unique_ptr<Nebula::Universe> m_application;
    Probe* m_probe = nullptr;
    Panel* m_panel = nullptr;
    Nebula::SettingsFile* m_file = nullptr;
    std::unique_ptr<SettingsDialog> m_dialog;
};

void TestSettingsDialog::initTestCase()
{
    registerTestComponents();
    // Once, before any test fails on a warning: the offscreen platform warns that it has no fonts the first time a text
    // is laid out.
    QFontDatabase::families();
}

void TestSettingsDialog::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
    m_application = std::make_unique<Nebula::Universe>();
    m_probe = qobject_cast<Probe*>(m_application->instantiate(testComponentId(QStringLiteral("Probe"))));
    m_panel = qobject_cast<Panel*>(m_application->instantiate(testComponentId(QStringLiteral("Panel"))));
    QVERIFY(m_probe != nullptr && m_panel != nullptr);
    m_file = &m_application->addSettingsFile();
    QVERIFY(m_file->add({.component = m_probe, .object = m_probe, .property = QStringLiteral("count")}));
    Nebula::SettingsFile& other = m_application->addSettingsFile();
    QVERIFY(other.setFileName(QStringLiteral("other.json")));
    QVERIFY(other.add({.component = m_probe, .object = m_probe, .property = QStringLiteral("ratio")}));
    m_dialog = std::make_unique<SettingsDialog>(*m_file, *m_application, Nebula::ComponentFactory::instance());
}

void TestSettingsDialog::cleanup()
{
    m_dialog.reset();
    m_application.reset();
}

QTreeWidget& TestSettingsDialog::tree() const
{
    return *m_dialog->findChild<QTreeWidget*>();
}

QLineEdit* TestSettingsDialog::nameField() const
{
    for(QLineEdit* edit : m_dialog->findChildren<QLineEdit*>())
    {
        if(edit->text() == m_file->name())
        {
            return edit;
        }
    }
    return nullptr;
}

QLabel* TestSettingsDialog::shownPlaceholder() const
{
    const auto* stack = m_dialog->findChild<PlaceholderStack*>();
    return stack->currentWidget() == stack->placeholder() ? stack->placeholder() : nullptr;
}

QString TestSettingsDialog::count() const
{
    for(const QLabel* label : m_dialog->findChildren<QLabel*>())
    {
        if(label->text().endsWith(QStringLiteral(" held")))
        {
            return label->text();
        }
    }
    return {};
}

// A property another file holds is listed with the name of that file, and cannot be checked.
void TestSettingsDialog::heldByAnotherFile()
{
    QTreeWidgetItem* ratio = rowAt(tree(), QStringLiteral("Probe1/ratio"));
    QVERIFY(ratio != nullptr);
    QCOMPARE(ratio->flags(), Qt::NoItemFlags);
    QCOMPARE(ratio->text(FileColumn), QStringLiteral("other.json"));
    QVERIFY(!ratio->data(PropertyColumn, Qt::CheckStateRole).isValid());
}

void TestSettingsDialog::check_data()
{
    QTest::addColumn<QStringList>("clicked");
    QTest::addColumn<QStringList>("checked");
    QTest::addColumn<QStringList>("groups");
    QTest::addColumn<QString>("count");

    const QStringList probe{"Probe1/refusesToStart", "Probe1/count", "Probe1/text",
                            "Probe1/date",           "Probe1/time",  "Probe1/moment"};
    const QStringList pages{"Panel1/page/level", "Panel1/page/enabled", "Panel1/page/level", "Panel1/page/enabled"};
    QTest::newRow("as it is") << QStringList() << QStringList{"Probe1/count"}
                              << QStringList{"Probe1 partially", "Panel1 unchecked", "Panel1/page unchecked"}
                              << "1 of 13 held";
    QTest::newRow("a property") << QStringList{"Panel1/title"} << QStringList{"Probe1/count", "Panel1/title"}
                                << QStringList{"Probe1 partially", "Panel1 partially", "Panel1/page unchecked"}
                                << "2 of 13 held";
    QTest::newRow("its last property unchecked")
        << QStringList{"Probe1/count"} << QStringList()
        << QStringList{"Probe1 unchecked", "Panel1 unchecked", "Panel1/page unchecked"} << "0 of 13 held";
    // Partially checked, it is checked, with all its properties but the one another file holds.
    QTest::newRow("a component") << QStringList{"Probe1"} << probe
                                 << QStringList{"Probe1 checked", "Panel1 unchecked", "Panel1/page unchecked"}
                                 << "6 of 13 held";
    QTest::newRow("a component, twice") << QStringList{"Probe1", "Probe1"} << QStringList()
                                        << QStringList{"Probe1 unchecked", "Panel1 unchecked", "Panel1/page unchecked"}
                                        << "0 of 13 held";
    QTest::newRow("a group of objects") << QStringList{"Panel1/page"} << QStringList{"Probe1/count"} + pages
                                        << QStringList{"Probe1 partially", "Panel1 partially", "Panel1/page checked"}
                                        << "5 of 13 held";
    QTest::newRow("each property of a component")
        << QStringList{"Panel1/title", "Panel1/level", "Panel1/enabled", "Panel1/page"}
        << QStringList{"Probe1/count", "Panel1/title", "Panel1/level", "Panel1/enabled"} + pages
        << QStringList{"Probe1 partially", "Panel1 checked", "Panel1/page checked"} << "8 of 13 held";
    QTest::newRow("a component checked") << QStringList{"Panel1"}
                                         << QStringList{"Probe1/count", "Panel1/title", "Panel1/level",
                                                        "Panel1/enabled"} +
                                                pages
                                         << QStringList{"Probe1 partially", "Panel1 checked", "Panel1/page checked"}
                                         << "8 of 13 held";
    QTest::newRow("a property of a group of a component checked")
        << QStringList{"Panel1", "Panel1/page/level"}
        << QStringList{"Probe1/count",        "Panel1/title",      "Panel1/level",       "Panel1/enabled",
                       "Panel1/page/enabled", "Panel1/page/level", "Panel1/page/enabled"}
        << QStringList{"Probe1 partially", "Panel1 partially", "Panel1/page partially"} << "7 of 13 held";
}

// Checking a component, or a group of objects, checks what it groups, and each tells whether all, some or none of what
// it groups is checked.
void TestSettingsDialog::check()
{
    QFETCH(QStringList, clicked);
    QFETCH(QStringList, checked);
    QFETCH(QStringList, groups);
    QFETCH(QString, count);

    for(const QString& path : clicked)
    {
        QTreeWidgetItem* row = rowAt(tree(), path);
        QVERIFY2(row != nullptr, qPrintable(path));
        click(*row);
    }
    QCOMPARE(
        properties(tree(), [](const QTreeWidgetItem& row) { return row.checkState(PropertyColumn) == Qt::Checked; }),
        checked);
    QStringList states;
    for(const QString& group : {"Probe1", "Panel1", "Panel1/page"})
    {
        states.append(stateOf(tree(), group));
    }
    QCOMPARE(states, groups);
    QCOMPARE(this->count(), count);
}

void TestSettingsDialog::filter_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("heldOnly");
    QTest::addColumn<QStringList>("shown");
    QTest::addColumn<QString>("placeholder");

    const QStringList panel{"Panel1/title",        "Panel1/level",      "Panel1/enabled",     "Panel1/page/level",
                            "Panel1/page/enabled", "Panel1/page/level", "Panel1/page/enabled"};
    QTest::newRow("all") << QString() << false
                         << QStringList{"Probe1/refusesToStart", "Probe1/count", "Probe1/ratio", "Probe1/text",
                                        "Probe1/date",           "Probe1/time",  "Probe1/moment"} +
                                panel
                         << QString();
    QTest::newRow("a property") << "ti" << false << QStringList{"Probe1/ratio", "Probe1/time", "Panel1/title"}
                                << QString();
    QTest::newRow("whatever its case") << "TIME" << false << QStringList{"Probe1/time"} << QString();
    QTest::newRow("a property another file holds") << "ratio" << false << QStringList{"Probe1/ratio"} << QString();
    QTest::newRow("a component") << "panel" << false << panel << QString();
    QTest::newRow("a group of objects") << "page" << false
                                        << QStringList{"Panel1/page/level", "Panel1/page/enabled", "Panel1/page/level",
                                                       "Panel1/page/enabled"}
                                        << QString();
    QTest::newRow("nothing found") << "nothing" << false << QStringList() << "No property found";
    QTest::newRow("held only") << QString() << true << QStringList{"Probe1/count"} << QString();
    QTest::newRow("held only, found") << "co" << true << QStringList{"Probe1/count"} << QString();
    QTest::newRow("held only, not found") << "level" << true << QStringList() << "No property found";
    // Another file holds it: not this one.
    QTest::newRow("held only by another file") << "ratio" << true << QStringList() << "No property found";
}

// The properties shown are those whose name, or the one of their component or group, contains the text of the filter,
// those the file holds only when asked to; the rows above them are shown, open, the others hidden.
void TestSettingsDialog::filter()
{
    QFETCH(QString, text);
    QFETCH(bool, heldOnly);
    QFETCH(QStringList, shown);
    QFETCH(QString, placeholder);

    m_dialog->findChild<QCheckBox*>()->setChecked(heldOnly);
    field(*m_dialog, QStringLiteral("Filter properties"))->setText(text);
    QCOMPARE(properties(tree(), isShown), shown);
    for(const QString& path : text.isEmpty() ? QStringList() : shown) // Found by a text, they are shown open
    {
        QVERIFY2(rowAt(tree(), path)->parent()->isExpanded(), qPrintable(path));
    }
    const QLabel* shownPlaceholder = this->shownPlaceholder();
    QCOMPARE(shownPlaceholder != nullptr ? shownPlaceholder->text() : QString(), placeholder);

    field(*m_dialog, QStringLiteral("Filter properties"))->clear();
    m_dialog->findChild<QCheckBox*>()->setChecked(false);
    QCOMPARE(properties(tree(), isShown).size(), 14); // All of them again
}

void TestSettingsDialog::name_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("title");
    QTest::addColumn<QString>("refusal");

    QTest::newRow("its own") << "settings.json" << "settings.json" << QString();
    QTest::newRow("its own, whatever its case") << "Settings.JSON" << "Settings.JSON" << QString();
    QTest::newRow("another one") << "profile.json" << "profile.json" << QString();
    QTest::newRow("trimmed") << "  profile.json " << "profile.json" << QString();
    QTest::newRow("the one of another file") << "other.json" << "other.json"
                                             << "Another settings file is named other.json";
    QTest::newRow("the one of another file, whatever its case")
        << "Other.json" << "Other.json" << "Another settings file is named other.json";
    QTest::newRow("none") << " " << "Settings file" << "A file name is needed";
}

// The header shows the name typed; the dialog is not accepted without a name, nor with the one of another file, which
// it tells.
void TestSettingsDialog::name()
{
    QFETCH(QString, name);
    QFETCH(QString, title);
    QFETCH(QString, refusal);

    nameField()->setText(name);
    QCOMPARE(m_dialog->findChild<DialogHeader*>()->title(), title);
    const auto* warning = m_dialog->findChild<WarningLabel*>();
    QCOMPARE(warning->text(), refusal);
    QCOMPARE(warning->isVisibleTo(m_dialog.get()), !refusal.isEmpty());
    QCOMPARE(m_dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->isEnabled(), refusal.isEmpty());
}

// Once accepted, the file holds what is checked, in the order of the tree, under the name typed.
void TestSettingsDialog::apply()
{
    click(*rowAt(tree(), QStringLiteral("Panel1/page")));
    click(*rowAt(tree(), QStringLiteral("Probe1/count")));
    nameField()->setText(QStringLiteral("pages.json"));
    m_dialog->accept();

    QCOMPARE(m_file->fileName(), QStringLiteral("pages.json"));
    const QList<Options*> pages = m_panel->pages();
    const QList<Nebula::SettingsEntry> expected{
        {.component = m_panel, .object = pages.at(0), .property = QStringLiteral("level")},
        {.component = m_panel, .object = pages.at(0), .property = QStringLiteral("enabled")},
        {.component = m_panel, .object = pages.at(1), .property = QStringLiteral("level")},
        {.component = m_panel, .object = pages.at(1), .property = QStringLiteral("enabled")},
    };
    QCOMPARE(m_file->entries(), expected);
}

// Showing those held only, it tells that the file holds none, in place of the tree.
void TestSettingsDialog::noneHeld()
{
    click(*rowAt(tree(), QStringLiteral("Probe1/count")));
    m_dialog->findChild<QCheckBox*>()->setChecked(true);
    QVERIFY(properties(tree(), isShown).isEmpty());
    QVERIFY(shownPlaceholder() != nullptr);
    QCOMPARE(shownPlaceholder()->text(), QStringLiteral("The file holds no property yet"));

    click(*rowAt(tree(), QStringLiteral("Probe1/text"))); // Shown once the filter is changed
    field(*m_dialog, QStringLiteral("Filter properties"))->setText(QStringLiteral("t"));
    QCOMPARE(properties(tree(), isShown), QStringList{"Probe1/text"});
    QVERIFY(shownPlaceholder() == nullptr);
}

// Without a property a file can hold, it tells so in place of the tree.
void TestSettingsDialog::noProperty()
{
    m_dialog.reset();
    m_application = std::make_unique<Nebula::Universe>();
    m_dialog = std::make_unique<SettingsDialog>(m_application->addSettingsFile(), *m_application,
                                                Nebula::ComponentFactory::instance());
    QVERIFY(shownPlaceholder() != nullptr);
    QCOMPARE(shownPlaceholder()->text(),
             QStringLiteral("None of the instances has a property a settings file can hold"));
    QCOMPARE(count(), QStringLiteral("0 of 0 held"));
}

QTEST_MAIN(TestSettingsDialog)
#include "TestSettingsDialog.moc"
