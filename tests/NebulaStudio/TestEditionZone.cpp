// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QAction>
#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <memory>

#include "EditionScene.h"
#include "EditionZone.h"
#include "Nebula/Application/Universe.h"
#include "Nebula/Galaxy/Galaxy.h"
#include "Studio.h"
#include "TestComponents.h"

using Nebula::Galaxy;
using namespace Qt::StringLiterals;

namespace
{

// A galaxy of a Source, and of a component no plugin provides: it is placed without it.
const QByteArray Part = R"({"version": 1, "components": [{"name": "Source", "type": "Test.Source"},
                                                          {"name": "X", "type": "Test.Unknown"}]})";

// How the question asked before a galaxy placed without what could not be restored of its file is saved starts.
const QString LosingPart = u"Part1 was placed without what could not be restored of its file:\n"
                           u"X is missing: no loaded plugin provides Test.Unknown\nSaving it writes "_s;

// Writes content to the file fileName of directory; returns its path, empty when it could not be written.
QString write(const QTemporaryDir& directory, const QString& fileName, const QByteArray& content)
{
    QFile file(directory.filePath(fileName));
    if(!file.open(QIODevice::WriteOnly) || file.write(content) != content.size())
    {
        return {};
    }
    return file.fileName();
}

// What the file filePath holds.
QByteArray readFile(const QString& filePath)
{
    QFile file(filePath);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

// The names of components, in their order.
QStringList namesOf(const std::vector<std::unique_ptr<Nebula::Component>>& components)
{
    QStringList names;
    for(const std::unique_ptr<Nebula::Component>& component : components)
    {
        names.append(component->objectName());
    }
    return names;
}

// What the offscreen platform warns of as each of count boxes is shown, which the test expects.
void expectBoxes(int count)
{
    for(int box = 0; box < count; ++box)
    {
        QTest::ignoreMessage(QtWarningMsg, "This plugin does not support propagateSizeHints()");
    }
}

// Answers the boxes shown from now on, one after the other, each with the next of answers, once the events are
// handled, and keeps in told what each told; nothing is answered when no box is shown by then.
void answerBoxes(QObject& context, const QList<QMessageBox::StandardButton>& answers, QStringList& told)
{
    if(answers.isEmpty())
    {
        return;
    }
    QTimer::singleShot(0, &context,
                       [&context, answers, &told]
                       {
                           auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
                           if(box == nullptr)
                           {
                               return;
                           }
                           told.append(box->text());
                           answerBoxes(context, answers.sliced(1), told); // The next one, once this one is answered
                           box->button(answers.first())->click();
                       });
}

} // namespace

// EditionZone, the edition zone of NebulaStudio: a galaxy placed without what could not be restored of its file, a
// component no plugin provides say, is saved only once its user confirms it, as saving it leaves that out of its file.
class TestEditionZone : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void cleanup();

    void saveGalaxyPlacedInPart();
    void saveWholeGalaxy();
    void leaveGalaxyPlacedInPart();

  private:
    // Places the galaxy of the file filePath in the application, and opens it in the zone.
    Galaxy* openGalaxy(const QString& filePath);

    QTemporaryDir m_settings; // The preferences of the studio
    std::unique_ptr<QTemporaryDir> m_directory;
    std::unique_ptr<Studio> m_studio;
    std::unique_ptr<EditionZone> m_zone;
};

void TestEditionZone::initTestCase()
{
    registerTestComponents();
    // Its own preferences, rather than those of the user, which nothing writes anyway.
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
    // Once, before any test fails on a warning: the offscreen platform warns that it has no fonts the first time a text
    // is shown.
    QFontDatabase::families();
}

void TestEditionZone::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
    m_directory = std::make_unique<QTemporaryDir>();
    m_studio = std::make_unique<Studio>(u"FabienDG"_s, u"TestEditionZone"_s);
    m_zone = std::make_unique<EditionZone>(*m_studio);
}

void TestEditionZone::cleanup()
{
    m_zone.reset();
    m_studio.reset();
    m_directory.reset();
}

Galaxy* TestEditionZone::openGalaxy(const QString& filePath)
{
    Galaxy* galaxy = m_studio->application().instantiateGalaxy(filePath, u"Part1"_s);
    if(galaxy != nullptr)
    {
        emit m_zone->applicationScene().galaxyOpened(*galaxy); // As double-clicking its item asks
    }
    return galaxy;
}

// Save Galaxy asks first, and writes nothing when the answer is Cancel; once the answer is Save, the galaxy is written
// without what it missed, and read again from its file, missing nothing: nothing is asked then.
void TestEditionZone::saveGalaxyPlacedInPart()
{
    const QString filePath = write(*m_directory, u"Part.nbp"_s, Part);
    Galaxy* galaxy = openGalaxy(filePath);
    QVERIFY(galaxy != nullptr);
    QVERIFY(m_zone->saveGalaxyAction()->isEnabled());

    QStringList told;
    expectBoxes(1);
    answerBoxes(*this, {QMessageBox::Cancel}, told);
    m_zone->saveGalaxyAction()->trigger();
    QCOMPARE(told.size(), 1);
    QVERIFY2(told.first().startsWith(LosingPart), qPrintable(told.first()));
    QCOMPARE(readFile(filePath), Part);
    QCOMPARE(galaxy->ownProblems().size(), 1);

    told.clear();
    expectBoxes(1);
    answerBoxes(*this, {QMessageBox::Save}, told);
    m_zone->saveGalaxyAction()->trigger();
    QCOMPARE(told.size(), 1);
    QVERIFY(!readFile(filePath).contains("Test.Unknown"));
    QCOMPARE(namesOf(galaxy->components()), QStringList{"Source"});
    QCOMPARE(galaxy->problems(), QStringList());

    told.clear();
    answerBoxes(*this, {QMessageBox::Cancel}, told);
    m_zone->saveGalaxyAction()->trigger();
    QCoreApplication::processEvents(); // Where a box would be answered
    QCOMPARE(told, QStringList());
}

// A galaxy that misses nothing is saved at once.
void TestEditionZone::saveWholeGalaxy()
{
    const QByteArray whole = R"({"version": 1, "components": [{"name": "Source", "type": "Test.Source"}]})";
    const QString filePath = write(*m_directory, u"Part.nbp"_s, whole);
    QVERIFY(openGalaxy(filePath) != nullptr);
    QStringList told;
    answerBoxes(*this, {QMessageBox::Cancel}, told);
    m_zone->saveGalaxyAction()->trigger();
    QCoreApplication::processEvents(); // Where a box would be answered
    QCOMPARE(told, QStringList());
    QVERIFY(readFile(filePath) != whole); // Written as the zone shows it, with where its components are
}

// Leaving a galaxy placed in part that changed asks whether to save it; Save asks again, as Save Galaxy does: answered
// Cancel, the galaxy is not left, and its file is not written.
void TestEditionZone::leaveGalaxyPlacedInPart()
{
    const QString filePath = write(*m_directory, u"Part.nbp"_s, Part);
    Galaxy* galaxy = openGalaxy(filePath);
    QVERIFY(galaxy != nullptr);
    QVERIFY(m_studio->application().instantiate(testComponentId(u"Probe"_s), {}, galaxy) != nullptr); // A change

    QStringList told;
    expectBoxes(2);
    answerBoxes(*this, {QMessageBox::Save, QMessageBox::Cancel}, told);
    QVERIFY(!m_zone->leaveGalaxies(0));
    QCOMPARE(told.size(), 2);
    QVERIFY2(told.first().startsWith(u"The galaxy Part1 has been modified."_s), qPrintable(told.first()));
    QVERIFY2(told.last().startsWith(LosingPart), qPrintable(told.last()));
    QCOMPARE(readFile(filePath), Part);
    QVERIFY(m_zone->saveGalaxyAction()->isEnabled()); // Still shown
}

QTEST_MAIN(TestEditionZone)
#include "TestEditionZone.moc"
