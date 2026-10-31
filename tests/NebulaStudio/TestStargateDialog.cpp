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
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <functional>
#include <memory>

#include "DialogHeader.h"
#include "Galaxy/StargateDialog.h"
#include "Nebula/Application/Universe.h"
#include "Nebula/Component/ComponentFactory.h"
#include "Nebula/Galaxy/Stargate.h"
#include "PlaceholderStack.h"
#include "Probe.h"
#include "Sink.h"
#include "Source.h"
#include "TestComponents.h"
#include "WarningLabel.h"

using Nebula::Exposure;

namespace
{

// The column of what can be exposed, where it is checked, of the name it is exposed as, and of the stargate that
// exposes it when it is another one.
constexpr int ExposesColumn = 0;
constexpr int NameColumn = 2;
constexpr int ByColumn = 3;

// The group of what the stargate exposes, bound to none of the instances.
const QString Kept = QStringLiteral("Bound to none of these instances");

// The path of row: the texts of the rows above it, then its own, joined by slashes.
QString pathOf(const QTreeWidgetItem& row)
{
    QStringList texts{row.text(ExposesColumn)};
    for(const QTreeWidgetItem* parent = row.parent(); parent != nullptr; parent = parent->parent())
    {
        texts.prepend(parent->text(ExposesColumn));
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

bool isChecked(const QTreeWidgetItem& row)
{
    return row.checkState(ExposesColumn) == Qt::Checked;
}

// The paths of the rows of tree that group none, and that accepts accepts.
QStringList choices(QTreeWidget& tree, const std::function<bool(const QTreeWidgetItem&)>& accepts)
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

// What the rows of tree checked are exposed as: their path, then their name after an equal sign.
QStringList exposedRows(QTreeWidget& tree)
{
    QStringList exposed;
    for(QTreeWidgetItemIterator row(&tree); *row != nullptr; ++row)
    {
        if((*row)->childCount() == 0 && isChecked(**row))
        {
            exposed.append(pathOf(**row) + u'=' + (*row)->text(NameColumn));
        }
    }
    return exposed;
}

// Checks row when it is not, unchecks it otherwise, as a click does.
void click(QTreeWidgetItem& row)
{
    row.setCheckState(ExposesColumn, isChecked(row) ? Qt::Unchecked : Qt::Checked);
}

// The state of the row of path, as the data of the tests tell it after its path.
QString stateOf(QTreeWidget& tree, const QString& path)
{
    switch(rowAt(tree, path)->checkState(ExposesColumn))
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

// StargateDialog: what a stargate exposes, checked one by one or by their component or group, named, filtered,
// counted, and given to the stargate, which is bound accordingly.
class TestStargateDialog : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void cleanup();

    void header();
    void exposedByAnother();
    void check_data();
    void check();
    void filter_data();
    void filter();
    void name_data();
    void name();
    void apply();
    void noneExposed();
    void nothingToExpose();

  private:
    // The tree of what can be exposed.
    [[nodiscard]] QTreeWidget& tree() const;
    // The placeholder of the tree, when it is shown in place of the tree; nullptr otherwise.
    [[nodiscard]] QLabel* shownPlaceholder() const;
    // The text that tells how many are exposed.
    [[nodiscard]] QString count() const;
    [[nodiscard]] bool isAcceptable() const;

    // An application with a Source1, a Sink1, a Probe1 and two stargates: m_stargate, Stargate1, which exposes the
    // count of Probe1, and a timer the galaxy consumes, clock, bound to nothing, and is edited by m_dialog; and
    // Stargate2, which exposes the ratio of Probe1, and an object the galaxy provides, object, bound to nothing.
    std::unique_ptr<Nebula::Universe> m_application;
    Source* m_source = nullptr;
    Sink* m_sink = nullptr;
    Probe* m_probe = nullptr;
    Nebula::Stargate* m_stargate = nullptr;
    std::unique_ptr<StargateDialog> m_dialog;
};

void TestStargateDialog::initTestCase()
{
    registerTestComponents();
    // Once, before any test fails on a warning: the offscreen platform warns that it has no fonts the first time a text
    // is laid out.
    QFontDatabase::families();
}

void TestStargateDialog::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
    m_application = std::make_unique<Nebula::Universe>();
    m_source = qobject_cast<Source*>(m_application->instantiate(testComponentId(QStringLiteral("Source"))));
    m_sink = qobject_cast<Sink*>(m_application->instantiate(testComponentId(QStringLiteral("Sink"))));
    m_probe = qobject_cast<Probe*>(m_application->instantiate(testComponentId(QStringLiteral("Probe"))));
    QVERIFY(m_source != nullptr && m_sink != nullptr && m_probe != nullptr);
    m_stargate = m_application->addStargate();
    QVERIFY(m_stargate->exposeProperty({.name = QStringLiteral("count"),
                                        .component = m_probe,
                                        .object = m_probe,
                                        .property = QStringLiteral("count")}));
    QVERIFY(m_stargate->exposeInterface(Exposure::Consumed, QStringLiteral("clock"), QMetaType::fromType<QTimer*>()));
    Nebula::Stargate* other = m_application->addStargate();
    QVERIFY(other->exposeProperty({.name = QStringLiteral("ratio"),
                                   .component = m_probe,
                                   .object = m_probe,
                                   .property = QStringLiteral("ratio")}));
    QVERIFY(other->exposeInterface(Exposure::Provided, QStringLiteral("object"), QMetaType::fromType<QObject*>()));
    m_dialog = std::make_unique<StargateDialog>(*m_stargate, *m_application, Nebula::ComponentFactory::instance());
}

void TestStargateDialog::cleanup()
{
    m_dialog.reset();
    m_application.reset();
}

QTreeWidget& TestStargateDialog::tree() const
{
    return *m_dialog->findChild<QTreeWidget*>();
}

QLabel* TestStargateDialog::shownPlaceholder() const
{
    const auto* stack = m_dialog->findChild<PlaceholderStack*>();
    return stack->currentWidget() == stack->placeholder() ? stack->placeholder() : nullptr;
}

QString TestStargateDialog::count() const
{
    for(const QLabel* label : m_dialog->findChildren<QLabel*>())
    {
        if(label->text().endsWith(QStringLiteral(" exposed")))
        {
            return label->text();
        }
    }
    return {};
}

bool TestStargateDialog::isAcceptable() const
{
    return m_dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->isEnabled();
}

// The header tells the name of the stargate, and what it is for.
void TestStargateDialog::header()
{
    const auto* header = m_dialog->findChild<DialogHeader*>();
    QCOMPARE(header->title(), QStringLiteral("Stargate1"));
    QVERIFY2(header->text().startsWith(QStringLiteral("The galaxy of this project has what the stargate exposes")),
             qPrintable(header->text()));
}

// What another stargate exposes is listed with its name, and cannot be checked.
void TestStargateDialog::exposedByAnother()
{
    QTreeWidgetItem* ratio = rowAt(tree(), QStringLiteral("Probe1/ratio"));
    QVERIFY(ratio != nullptr);
    QCOMPARE(ratio->flags(), Qt::NoItemFlags);
    QCOMPARE(ratio->text(ByColumn), QStringLiteral("Stargate2"));
    QVERIFY(!ratio->icon(ByColumn).isNull());
    QVERIFY(!ratio->data(ExposesColumn, Qt::CheckStateRole).isValid());
}

void TestStargateDialog::check_data()
{
    QTest::addColumn<QStringList>("clicked");
    QTest::addColumn<QStringList>("exposed");
    QTest::addColumn<QStringList>("groups");
    QTest::addColumn<QString>("count");

    const QString kept = Kept + QStringLiteral("/clock=clock");
    const QStringList probe{"Probe1/refusesToStart=refusesToStart",
                            "Probe1/count=count",
                            "Probe1/text=text",
                            "Probe1/date=date",
                            "Probe1/time=time",
                            "Probe1/moment=moment"};
    const QStringList sink{"Sink1/object=object", "Sink1/timer=timer", "Sink1/objects=objects"};
    QTest::newRow("as it is") << QStringList() << QStringList{"Probe1/count=count", kept}
                              << QStringList{"Source1 unchecked", "Sink1 unchecked", "Probe1 partially",
                                             Kept + " checked"}
                              << "2 of 12 exposed";
    QTest::newRow("a property") << QStringList{"Source1/label"}
                                << QStringList{"Source1/label=label", "Probe1/count=count", kept}
                                << QStringList{"Source1 partially", "Sink1 unchecked", "Probe1 partially",
                                               Kept + " checked"}
                                << "3 of 12 exposed";
    // Another stargate exposes an object the galaxy provides as object.
    QTest::newRow("an interface, its name taken")
        << QStringList{"Source1/object"} << QStringList{"Source1/object=object2", "Probe1/count=count", kept}
        << QStringList{"Source1 partially", "Sink1 unchecked", "Probe1 partially", Kept + " checked"}
        << "3 of 12 exposed";
    QTest::newRow("its last row unchecked")
        << QStringList{"Probe1/count"} << QStringList{kept}
        << QStringList{"Source1 unchecked", "Sink1 unchecked", "Probe1 unchecked", Kept + " checked"}
        << "1 of 12 exposed";
    // Each is named after what it exposes, the galaxy consuming what Sink1 consumes, the other way.
    QTest::newRow("a component") << QStringList{"Sink1"} << sink + QStringList{"Probe1/count=count", kept}
                                 << QStringList{"Source1 unchecked", "Sink1 checked", "Probe1 partially",
                                                Kept + " checked"}
                                 << "5 of 12 exposed";
    // Partially checked, it is checked, with all it has but what another stargate exposes; its name kept.
    QTest::newRow("a component partially checked")
        << QStringList{"Probe1"} << probe + QStringList{kept}
        << QStringList{"Source1 unchecked", "Sink1 unchecked", "Probe1 checked", Kept + " checked"}
        << "7 of 12 exposed";
    QTest::newRow("a component, twice") << QStringList{"Probe1", "Probe1"} << QStringList{kept}
                                        << QStringList{"Source1 unchecked", "Sink1 unchecked", "Probe1 unchecked",
                                                       Kept + " checked"}
                                        << "1 of 12 exposed";
    QTest::newRow("each row of a component")
        << QStringList{"Source1/object", "Source1/label"}
        << QStringList{"Source1/object=object2", "Source1/label=label", "Probe1/count=count", kept}
        << QStringList{"Source1 checked", "Sink1 unchecked", "Probe1 partially", Kept + " checked"}
        << "4 of 12 exposed";
    QTest::newRow("what is bound to none of them")
        << QStringList{Kept} << QStringList{"Probe1/count=count"}
        << QStringList{"Source1 unchecked", "Sink1 unchecked", "Probe1 partially", Kept + " unchecked"}
        << "1 of 12 exposed";
    QTest::newRow("all") << QStringList{"Source1", "Sink1", "Probe1"}
                         << QStringList{"Source1/object=object2", "Source1/label=label"} + sink + probe +
                                QStringList{kept}
                         << QStringList{"Source1 checked", "Sink1 checked", "Probe1 checked", Kept + " checked"}
                         << "12 of 12 exposed";
}

// Checking a component, or what is bound to none of them, checks what it groups, each named after what it exposes
// unless the name is taken, and each tells whether all, some or none of what it groups is checked. Unchecked, a row
// loses its name.
void TestStargateDialog::check()
{
    QFETCH(QStringList, clicked);
    QFETCH(QStringList, exposed);
    QFETCH(QStringList, groups);
    QFETCH(QString, count);

    for(const QString& path : clicked)
    {
        QTreeWidgetItem* row = rowAt(tree(), path);
        QVERIFY2(row != nullptr, qPrintable(path));
        click(*row);
    }
    QCOMPARE(exposedRows(tree()), exposed);
    QVERIFY(
        choices(tree(), [](const QTreeWidgetItem& row) { return !isChecked(row) && !row.text(NameColumn).isEmpty(); })
            .isEmpty()); // Unchecked, a row has no name
    QStringList states;
    for(const QString& group : {QStringLiteral("Source1"), QStringLiteral("Sink1"), QStringLiteral("Probe1"), Kept})
    {
        states.append(stateOf(tree(), group));
    }
    QCOMPARE(states, groups);
    QCOMPARE(this->count(), count);
    QVERIFY(isAcceptable());
}

void TestStargateDialog::filter_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("exposedOnly");
    QTest::addColumn<QStringList>("shown");
    QTest::addColumn<QString>("placeholder");

    const QStringList probe{"Probe1/refusesToStart", "Probe1/count", "Probe1/ratio", "Probe1/text",
                            "Probe1/date",           "Probe1/time",  "Probe1/moment"};
    const QStringList sink{"Sink1/object", "Sink1/timer", "Sink1/objects"};
    QTest::newRow("all") << QString() << false
                         << QStringList{"Source1/object", "Source1/label"} + sink + probe + QStringList{Kept + "/clock"}
                         << QString();
    QTest::newRow("an interface") << "obj" << false << QStringList{"Source1/object", "Sink1/object", "Sink1/objects"}
                                  << QString();
    QTest::newRow("whatever its case") << "TIME" << false << QStringList{"Sink1/timer", "Probe1/time"} << QString();
    QTest::newRow("what another stargate exposes") << "ratio" << false << QStringList{"Probe1/ratio"} << QString();
    QTest::newRow("a component") << "sink" << false << sink << QString();
    QTest::newRow("what is bound to none of them") << "bound" << false << QStringList{Kept + "/clock"} << QString();
    QTest::newRow("nothing found") << "nothing" << false << QStringList() << "Nothing found";
    QTest::newRow("exposed only") << QString() << true << QStringList{"Probe1/count", Kept + "/clock"} << QString();
    QTest::newRow("exposed only, found") << "cl" << true << QStringList{Kept + "/clock"} << QString();
    QTest::newRow("exposed only, not found") << "label" << true << QStringList() << "Nothing found";
    // Another stargate exposes it: not this one.
    QTest::newRow("exposed only by another stargate") << "ratio" << true << QStringList() << "Nothing found";
}

// What is shown is what the text of the filter is found in, in its name or in the one of its component or group, what
// the stargate exposes only when asked to; the rows above are shown, open, the others hidden.
void TestStargateDialog::filter()
{
    QFETCH(QString, text);
    QFETCH(bool, exposedOnly);
    QFETCH(QStringList, shown);
    QFETCH(QString, placeholder);

    m_dialog->findChild<QCheckBox*>()->setChecked(exposedOnly);
    field(*m_dialog, QStringLiteral("Filter interfaces and properties"))->setText(text);
    QCOMPARE(choices(tree(), isShown), shown);
    for(const QString& path : text.isEmpty() ? QStringList() : shown) // Found by a text, they are shown open
    {
        QVERIFY2(rowAt(tree(), path)->parent()->isExpanded(), qPrintable(path));
    }
    const QLabel* shownPlaceholder = this->shownPlaceholder();
    QCOMPARE(shownPlaceholder != nullptr ? shownPlaceholder->text() : QString(), placeholder);

    field(*m_dialog, QStringLiteral("Filter interfaces and properties"))->clear();
    m_dialog->findChild<QCheckBox*>()->setChecked(false);
    QCOMPARE(choices(tree(), isShown).size(), 13); // All of them again
}

void TestStargateDialog::name_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("problem");

    const QString object = QStringLiteral("Source1/object");
    QTest::newRow("its own") << object << "object2" << QString();
    QTest::newRow("on one line") << object << "  source \n object " << QString();
    QTest::newRow("none") << object << " " << "object needs a name to be exposed as: double-click it to type one";
    QTest::newRow("taken by another stargate")
        << object << "object" << "Another stargate exposes object the same way: object needs another name";
    QTest::newRow("a property taken by another stargate")
        << "Source1/label" << "ratio" << "Another stargate exposes ratio the same way: label needs another name";
    // An interface and a property do not share a name.
    QTest::newRow("taken by a property") << object << "count" << QString();
    QTest::newRow("taken by a property, the same way")
        << "Source1/label" << "count"
        << "label and count are both exposed as count: only interfaces the galaxy consumes, of the same type, share a "
           "name";
    QTest::newRow("taken by an interface of another type, the same way")
        << "Sink1/object" << "clock"
        << "object and clock are both exposed as clock: only interfaces the galaxy consumes, of the same type, share a "
           "name";
    // The galaxy gives both what it consumes through clock.
    QTest::newRow("shared") << "Sink1/timer" << "clock" << QString();
}

// A row checked is given the name typed, on one line; the dialog is not accepted without a name, nor with one another
// stargate, or another row exposed the same way, has, which it tells.
void TestStargateDialog::name()
{
    QFETCH(QString, path);
    QFETCH(QString, name);
    QFETCH(QString, problem);

    QTreeWidgetItem* row = rowAt(tree(), path);
    click(*row);
    row->setText(NameColumn, name);
    QCOMPARE(row->text(NameColumn), name.simplified());
    const auto* warning = m_dialog->findChild<WarningLabel*>();
    QCOMPARE(warning->text(), problem);
    QCOMPARE(isAcceptable(), problem.isEmpty());
}

// Once accepted, the stargate exposes what is checked, in the order of the tree, under the names given, and the
// interfaces are bound to the stargate; what is no longer checked is no longer exposed.
void TestStargateDialog::apply()
{
    click(*rowAt(tree(), QStringLiteral("Sink1")));
    click(*rowAt(tree(), Kept));
    rowAt(tree(), QStringLiteral("Probe1/count"))->setText(NameColumn, QStringLiteral("total"));
    m_dialog->accept();

    const QList<Nebula::ExposedInterface> interfaces{
        {.name = QStringLiteral("object"), .type = QMetaType::fromType<QObject*>(), .exposure = Exposure::Consumed},
        {.name = QStringLiteral("timer"), .type = QMetaType::fromType<QTimer*>(), .exposure = Exposure::Consumed},
        {.name = QStringLiteral("objects"), .type = QMetaType::fromType<QObject*>(), .exposure = Exposure::Consumed},
    };
    QCOMPARE(m_stargate->interfaces(), interfaces);
    const QList<Nebula::ExposedProperty> properties{
        {.name = QStringLiteral("total"), .component = m_probe, .object = m_probe, .property = QStringLiteral("count")},
    };
    QCOMPARE(m_stargate->properties(), properties);
    for(const QString& name : {QStringLiteral("object"), QStringLiteral("timer"), QStringLiteral("objects")})
    {
        const QList<Nebula::InterfaceBinding> bindings = m_sink->bindings(name);
        QCOMPARE(bindings.size(), 1);
        QCOMPARE(bindings.constFirst().provider, static_cast<Nebula::Component*>(m_stargate));
        QCOMPARE(bindings.constFirst().providedName, name);
    }
}

// Showing what is exposed only, it tells that the stargate exposes nothing, in place of the tree.
void TestStargateDialog::noneExposed()
{
    click(*rowAt(tree(), QStringLiteral("Probe1/count")));
    click(*rowAt(tree(), Kept));
    m_dialog->findChild<QCheckBox*>()->setChecked(true);
    QVERIFY(choices(tree(), isShown).isEmpty());
    QVERIFY(shownPlaceholder() != nullptr);
    QCOMPARE(shownPlaceholder()->text(), QStringLiteral("The stargate exposes nothing yet"));
    QCOMPARE(count(), QStringLiteral("0 of 12 exposed"));
}

// Without an instance that has something to expose, it tells so in place of the tree.
void TestStargateDialog::nothingToExpose()
{
    m_dialog.reset();
    m_application = std::make_unique<Nebula::Universe>();
    m_stargate = m_application->addStargate();
    m_dialog = std::make_unique<StargateDialog>(*m_stargate, *m_application, Nebula::ComponentFactory::instance());
    QVERIFY(shownPlaceholder() != nullptr);
    QCOMPARE(shownPlaceholder()->text(),
             QStringLiteral("None of the instances has an interface or a property to expose"));
    QCOMPARE(count(), QStringLiteral("0 of 0 exposed"));
}

QTEST_MAIN(TestStargateDialog)
#include "TestStargateDialog.moc"
