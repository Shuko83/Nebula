// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QFile>
#include <QFontDatabase>
#include <QLabel>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTreeWidget>

#include "Nebula/Application/Universe.h"
#include "Nebula/Galaxy/Galaxy.h"
#include "PlaceholderStack.h"
#include "ProblemList.h"
#include "TestComponents.h"

using Nebula::Galaxy;
using namespace Qt::StringLiterals;

namespace
{

// The columns of the list.
constexpr int ComponentColumn = 0;
constexpr int ProblemColumn = 1;
constexpr int FixColumn = 2;

// A galaxy of a plugin the application does not know, and of a component no plugin provides.
const QByteArray Partial = R"({"version": 1, "plugins": [{"id": "Other.Plugin", "file": "other.nbdl"}],
    "components": [{"name": "X", "type": "Test.Unknown"}]})";

// What a galaxy of the file Partial misses, in that order.
const QStringList PartialProblems{u"The plugin Other.Plugin is not loaded: the application does not know it"_s,
                                  u"X is missing: no loaded plugin provides Test.Unknown"_s};

// Writes content to the file fileName of directory, in place of what it holds; returns its path, empty when it could
// not be written.
QString write(const QTemporaryDir& directory, const QString& fileName, const QByteArray& content)
{
    QFile file(directory.filePath(fileName));
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate) || file.write(content) != content.size())
    {
        return {};
    }
    return file.fileName();
}

// The tree of list.
QTreeWidget* treeOf(const ProblemList& list)
{
    return list.findChild<QTreeWidget*>();
}

// The rows of list, each its component and its problem joined by " | ", in their order.
QStringList rowsOf(const ProblemList& list)
{
    const QTreeWidget* tree = treeOf(list);
    QStringList rows;
    for(int index = 0; index < tree->topLevelItemCount(); ++index)
    {
        const QTreeWidgetItem* item = tree->topLevelItem(index);
        rows.append(item->text(ComponentColumn) + u" | "_s + item->text(ProblemColumn));
    }
    return rows;
}

// problems, each the one of component, as rowsOf() gives them.
QStringList rows(const QString& component, const QStringList& problems)
{
    QStringList rows;
    for(const QString& problem : problems)
    {
        rows.append(component + u" | "_s + problem);
    }
    return rows;
}

// Whether list shows its tree, rather than the sentence that tells nothing keeps the components from working.
bool showsTree(const ProblemList& list)
{
    const auto* stack = list.findChild<PlaceholderStack*>();
    return stack->currentWidget() == treeOf(list);
}

} // namespace

// ProblemList, what keeps the components of the application from working: the interfaces they cannot do without bound
// to nothing, and what its galaxies miss of their files, as they miss it now.
class TestProblemList : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();

    void nothingKeepsThemFromWorking();
    void unboundInterfaces();
    void whatGalaxiesMiss();
    void followsWhatGalaxiesMiss();
    void activated();
};

void TestProblemList::initTestCase()
{
    registerTestComponents();
    // Once, before any test fails on a warning: the offscreen platform warns that it has no fonts the first time a text
    // is shown.
    QFontDatabase::families();
}

void TestProblemList::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
}

void TestProblemList::nothingKeepsThemFromWorking()
{
    Nebula::Universe application;
    QVERIFY(application.instantiate(testComponentId(u"Probe"_s)) != nullptr);
    const ProblemList list(application);
    QVERIFY(!showsTree(list));
    QCOMPARE(list.findChild<PlaceholderStack*>()->placeholder()->text(),
             u"Every component of the application has the interfaces it needs, and every galaxy what its file holds"_s);
}

// A row for each interface a component cannot do without, bound to nothing, with the interfaces of its type the others
// provide.
void TestProblemList::unboundInterfaces()
{
    Nebula::Universe application;
    QVERIFY(application.instantiate(testComponentId(u"Source"_s)) != nullptr);
    QVERIFY(application.instantiate(testComponentId(u"Sink"_s)) != nullptr);
    const ProblemList list(application);
    QCOMPARE(rowsOf(list), QStringList{u"Sink1 | object (QObject) is bound to nothing"_s});
    const QTreeWidgetItem* row = treeOf(list)->topLevelItem(0);
    QCOMPARE(row->text(FixColumn), u"Drag from the dot of object to the dot of Source1.object"_s);
    QCOMPARE(row->toolTip(ProblemColumn),
             u"object (QObject) is bound to nothing: the component does not work without it"_s);
    QCOMPARE(row->toolTip(FixColumn), row->text(FixColumn));
    QVERIFY(showsTree(list));
}

// A row for each problem of a galaxy: what could not be restored of its file, and how to fix it.
void TestProblemList::whatGalaxiesMiss()
{
    const QTemporaryDir directory;
    Nebula::Universe application;
    QVERIFY(application.instantiateGalaxy(write(directory, u"Partial.nbp"_s, Partial)) != nullptr);
    const ProblemList list(application);
    QCOMPARE(rowsOf(list), rows(u"Partial1"_s, PartialProblems));
    const QTreeWidgetItem* row = treeOf(list)->topLevelItem(1);
    QCOMPARE(row->toolTip(ProblemColumn), PartialProblems.at(1) + u": the galaxy is placed without it"_s);
    QVERIFY(row->text(FixColumn).contains(u"open the project again"_s));
    QCOMPARE(row->toolTip(FixColumn), row->text(FixColumn));
}

// What a galaxy misses is listed as it misses it now: placed later, or once a galaxy it holds is read again from its
// file mended.
void TestProblemList::followsWhatGalaxiesMiss()
{
    const QTemporaryDir directory;
    const QString partialPath = write(directory, u"Partial.nbp"_s, Partial);
    const QString homePath = write(directory, u"Home.nbp"_s, R"({"version": 1})");
    Nebula::Universe application;
    Galaxy* home = application.instantiateGalaxy(homePath);
    QVERIFY(home != nullptr);
    const ProblemList list(application);
    QCOMPARE(rowsOf(list), QStringList());

    // Placed in a galaxy: the galaxy of the application misses it.
    Galaxy* inner = application.instantiateGalaxy(partialPath, u"Part1"_s, home);
    QVERIFY(inner != nullptr);
    QTRY_COMPARE(rowsOf(list),
                 rows(u"Home1"_s, {u"In Part1: "_s + PartialProblems.at(0), u"In Part1: "_s + PartialProblems.at(1)}));
    QVERIFY(showsTree(list));

    // Mended, and read again.
    QVERIFY(!write(directory, u"Partial.nbp"_s, R"({"version": 1})").isEmpty());
    QCOMPARE(application.reloadGalaxy(*inner), QStringList());
    QTRY_COMPARE(rowsOf(list), QStringList());
    QVERIFY(!showsTree(list));
}

// Activating the row of what a galaxy misses asks for the galaxy.
void TestProblemList::activated()
{
    const QTemporaryDir directory;
    Nebula::Universe application;
    QVERIFY(application.instantiateGalaxy(write(directory, u"Partial.nbp"_s, Partial)) != nullptr);
    const ProblemList list(application);
    const QSignalSpy activated(&list, &ProblemList::componentActivated);
    QTreeWidget* tree = treeOf(list);
    emit tree->itemActivated(tree->topLevelItem(0), ProblemColumn);
    QCOMPARE(activated.count(), 1);
    QCOMPARE(activated.first().first().toString(), u"Partial1"_s);
}

QTEST_MAIN(TestProblemList)
#include "TestProblemList.moc"
