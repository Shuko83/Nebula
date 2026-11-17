// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QFile>
#include <QFontDatabase>
#include <QPushButton>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTreeView>
#include <algorithm>
#include <memory>

#include "Galaxy/GalaxyModel.h"
#include "Galaxy/GalaxyPanel.h"
#include "Studio.h"
#include "WarningLabel.h"

using namespace Qt::StringLiterals;

namespace
{

// The galaxies of the directory listed, in the order of their rows: one the application knows the plugins of, one of a
// plugin it does not know, and one whose file cannot be read.
constexpr int Whole = 0;
constexpr int MissingPlugins = 1;
constexpr int Unreadable = 2;

// Writes content to the file fileName of directory.
bool write(const QTemporaryDir& directory, const QString& fileName, const QByteArray& content)
{
    QFile file(directory.filePath(fileName));
    return file.open(QIODevice::WriteOnly) && file.write(content) == content.size();
}

} // namespace

// GalaxyPanel, the Galaxies tab of NebulaStudio, with the viewer of the galaxy selected, GalaxyViewer: a galaxy whose
// file can be read is opened, without the components of the plugins it misses, which the viewer tells; one whose file
// cannot be read is not.
class TestGalaxyPanel : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void cleanup();

    void viewer_data();
    void viewer();
    void open_data();
    void open();
    void notWhileRunning();

  private:
    // Selects the galaxy of row, which the viewer shows then.
    void select(int row);
    // The Open button of the viewer.
    [[nodiscard]] QPushButton* openButton() const;

    QTemporaryDir m_settings; // The preferences of the studio
    // The galaxies listed, the same for every test: removed once the last panel that follows it is gone, as the thread
    // of its watcher may still look at it a while.
    QTemporaryDir m_directory;
    std::unique_ptr<Studio> m_studio;
    std::unique_ptr<GalaxyPanel> m_panel;
};

void TestGalaxyPanel::initTestCase()
{
    // Its own preferences, rather than those of the user, which nothing writes anyway.
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
    QVERIFY(write(m_directory, u"Alpha.nbp"_s, R"({"version": 1, "description": "Says hello"})"));
    QVERIFY(write(m_directory, u"Beta.nbp"_s,
                  R"({"version": 1, "plugins": [{"id": "Other.Plugin", "file": "other.nbdl"}]})"));
    QVERIFY(write(m_directory, u"Gamma.nbp"_s, "{ version"));
    // Once, before any test fails on a warning: the offscreen platform warns that it has no fonts the first time a text
    // is shown.
    QFontDatabase::families();
}

void TestGalaxyPanel::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
    m_studio = std::make_unique<Studio>(u"FabienDG"_s, u"TestGalaxyPanel"_s);
    m_panel = std::make_unique<GalaxyPanel>(*m_studio);
    m_panel->setDirectory(m_directory.path());
}

void TestGalaxyPanel::cleanup()
{
    m_panel.reset();
    m_studio.reset();
}

void TestGalaxyPanel::select(int row)
{
    auto* view = m_panel->findChild<QTreeView*>();
    view->setCurrentIndex(view->model()->index(row, GalaxyModel::Name));
}

QPushButton* TestGalaxyPanel::openButton() const
{
    const QList<QPushButton*> buttons = m_panel->findChildren<QPushButton*>();
    const auto found = std::ranges::find(buttons, u"Open"_s, &QPushButton::text);
    return found != buttons.end() ? *found : nullptr;
}

void TestGalaxyPanel::viewer_data()
{
    QTest::addColumn<int>("row");
    QTest::addColumn<QString>("warning"); // How it starts; empty when there is none
    QTest::addColumn<bool>("openable");

    QTest::newRow("whole") << Whole << "" << true;
    // Opened all the same, without what the plugin would give it.
    QTest::newRow("missing plugins") << MissingPlugins
                                     << "NebulaStudio does not know these plugins of this galaxy: their components are "
                                        "left out when it is opened or placed.\nOther.Plugin\n"
                                     << true;
    QTest::newRow("unreadable") << Unreadable << "This galaxy cannot be loaded: its file cannot be read." << false;
}

void TestGalaxyPanel::viewer()
{
    QFETCH(int, row);
    QFETCH(QString, warning);
    QFETCH(bool, openable);

    select(row);
    const auto* label = m_panel->findChild<WarningLabel*>();
    QVERIFY(label != nullptr);
    QCOMPARE(label->text().isEmpty(), warning.isEmpty());
    QVERIFY2(warning.isEmpty() || label->text().startsWith(warning), qPrintable(label->text()));
    QVERIFY(openButton() != nullptr);
    QCOMPARE(openButton()->isEnabled(), openable);
}

void TestGalaxyPanel::open_data()
{
    QTest::addColumn<int>("row");
    QTest::addColumn<bool>("opened");

    QTest::newRow("whole") << Whole << true;
    QTest::newRow("missing plugins") << MissingPlugins << true;
    QTest::newRow("unreadable") << Unreadable << false;
}

// Activating a galaxy, or clicking Open, asks for it to be opened, when its file can be read.
void TestGalaxyPanel::open()
{
    QFETCH(int, row);
    QFETCH(bool, opened);

    const QSignalSpy openRequested(m_panel.get(), &GalaxyPanel::openRequested);
    select(row);
    auto* view = m_panel->findChild<QTreeView*>();
    emit view->activated(view->currentIndex());
    openButton()->click(); // Does nothing while it is disabled
    QCOMPARE(openRequested.count(), opened ? 2 : 0);
    if(opened)
    {
        QCOMPARE(openRequested.first().first().toString(),
                 view->currentIndex().data(GalaxyModel::FilePathRole).toString());
    }
}

// While the application runs, none is opened from its viewer, which tells why.
void TestGalaxyPanel::notWhileRunning()
{
    QVERIFY(m_studio->application().start());
    select(MissingPlugins);
    QVERIFY(!openButton()->isEnabled());
    QCOMPARE(openButton()->toolTip(), u"Stop the application to open a galaxy"_s);
    m_studio->application().stop();
    QVERIFY(openButton()->isEnabled());
}

QTEST_MAIN(TestGalaxyPanel)
#include "TestGalaxyPanel.moc"
