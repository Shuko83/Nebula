// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QAbstractItemModelTester>
#include <QBrush>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QMimeData>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <memory>

#include "Galaxy/GalaxyModel.h"
#include "Nebula/Application/Universe.h"
#include "Theme.h"

using namespace Qt::StringLiterals;

namespace
{

const QString FirstPluginId = QStringLiteral("FabienDG.Nebula.FirstPlugin");
const QString SecondPluginId = QStringLiteral("FabienDG.Nebula.SecondPlugin");

// A project of the plugins pluginIds, described as description when it is not empty.
QByteArray project(const QStringList& pluginIds, const QString& description = {})
{
    QByteArrayList plugins;
    for(const QString& pluginId : pluginIds)
    {
        plugins.append(R"({"id": ")" + pluginId.toUtf8() + R"(", "file": "x.nbdl"})");
    }
    const QByteArray describe =
        description.isEmpty() ? QByteArray() : R"("description": ")" + description.toUtf8() + R"(", )";
    return R"({"version": 1, )" + describe + R"("plugins": [)" + plugins.join(", ") + "]}";
}

// Writes content to the file fileName of directory.
bool write(const QTemporaryDir& directory, const QString& fileName, const QByteArray& content)
{
    QFile file(directory.filePath(fileName));
    return file.open(QIODevice::WriteOnly) && file.write(content) == content.size();
}

} // namespace

// GalaxyModel, the list of the galaxies of a directory in NebulaStudio, whose plugins the application knows or not.
class TestGalaxyModel : public QObject
{
    Q_OBJECT

  private slots:
    void init();
    void cleanup();

    void data_data();
    void data();
    void loadable_data();
    void loadable();
    void decoration();
    void drag();
    void headerData_data();
    void headerData();
    void refreshFollowsTheFiles();
    void refreshKnowsNewPlugins();
    void followsTheDirectory();
    void setDirectory();
    void galaxyAndIndexOf();

  private:
    std::unique_ptr<QTemporaryDir> m_directory;
    std::unique_ptr<Nebula::Universe> m_application;
    std::unique_ptr<GalaxyModel> m_model;
    std::unique_ptr<QAbstractItemModelTester> m_tester; // Checks every change of m_model
};

void TestGalaxyModel::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
    m_directory = std::make_unique<QTemporaryDir>();
    // Sorted by name whatever its case: Alpha, beta, Gamma. The text file is not a galaxy.
    QVERIFY(write(*m_directory, u"Alpha.nbp"_s, project({FirstPluginId}, u"Says hello"_s)));
    QVERIFY(write(*m_directory, u"beta.nbp"_s, project({FirstPluginId, SecondPluginId, u"Other.Plugin"_s})));
    QVERIFY(write(*m_directory, u"Gamma.nbp"_s, "{ version"));
    QVERIFY(write(*m_directory, u"notes.txt"_s, project({})));

    m_application = std::make_unique<Nebula::Universe>();
    m_application->plugins().scan(QStringLiteral(NEBULA_TEST_PLUGINS_DIR));
    m_model = std::make_unique<GalaxyModel>(*m_application);
    m_tester = std::make_unique<QAbstractItemModelTester>(m_model.get(),
                                                          QAbstractItemModelTester::FailureReportingMode::QtTest);
    m_model->setDirectory(m_directory->path());
}

void TestGalaxyModel::cleanup()
{
    m_tester.reset();
    m_model.reset();
    m_application.reset();
    m_directory.reset();
}

void TestGalaxyModel::data_data()
{
    QTest::addColumn<int>("row");
    QTest::addColumn<int>("column");
    QTest::addColumn<int>("role");
    QTest::addColumn<QVariant>("value");

    QTest::newRow("name") << 0 << int(GalaxyModel::Name) << int(Qt::DisplayRole) << QVariant(u"Alpha"_s);
    QTest::newRow("name, whatever its case")
        << 1 << int(GalaxyModel::Name) << int(Qt::DisplayRole) << QVariant(u"beta"_s);
    QTest::newRow("name of an unreadable file")
        << 2 << int(GalaxyModel::Name) << int(Qt::DisplayRole) << QVariant(u"Gamma"_s);
    QTest::newRow("description") << 0 << int(GalaxyModel::Name) << int(Qt::ToolTipRole) << QVariant(u"Says hello"_s);
    QTest::newRow("no description") << 1 << int(GalaxyModel::Name) << int(Qt::ToolTipRole) << QVariant();
    QTest::newRow("loadable") << 0 << int(GalaxyModel::State) << int(Qt::DisplayRole) << QVariant(u"Loadable"_s);
    QTest::newRow("missing plugins") << 1 << int(GalaxyModel::State) << int(Qt::DisplayRole)
                                     << QVariant(u"Missing plugins"_s);
    QTest::newRow("unreadable") << 2 << int(GalaxyModel::State) << int(Qt::DisplayRole) << QVariant(u"Unreadable"_s);
    QTest::newRow("nothing missing") << 0 << int(GalaxyModel::State) << int(Qt::ToolTipRole) << QVariant();
    QTest::newRow("what is missing") << 1 << int(GalaxyModel::State) << int(Qt::ToolTipRole)
                                     << QVariant(u"Missing: FabienDG.Nebula.SecondPlugin, Other.Plugin"_s);
    QTest::newRow("other role") << 0 << int(GalaxyModel::Name) << int(Qt::EditRole) << QVariant();
}

void TestGalaxyModel::data()
{
    QFETCH(int, row);
    QFETCH(int, column);
    QFETCH(int, role);
    QFETCH(QVariant, value);

    QCOMPARE(m_model->rowCount(), 3);
    QCOMPARE(m_model->columnCount(), int(GalaxyModel::ColumnCount));
    QCOMPARE(m_model->index(row, column).data(role), value);
}

void TestGalaxyModel::loadable_data()
{
    QTest::addColumn<int>("row");
    QTest::addColumn<bool>("loadable");
    QTest::addColumn<QStringList>("missingPlugins");

    QTest::newRow("known plugins") << 0 << true << QStringList{};
    QTest::newRow("unknown plugins") << 1 << false << QStringList{SecondPluginId, u"Other.Plugin"_s};
    QTest::newRow("unreadable file") << 2 << false << QStringList{};
}

void TestGalaxyModel::loadable()
{
    QFETCH(int, row);
    QFETCH(bool, loadable);
    QFETCH(QStringList, missingPlugins);

    const QString filePath = m_model->index(row, GalaxyModel::State).data(GalaxyModel::FilePathRole).toString();
    QCOMPARE(QFileInfo(filePath).dir(), QDir(m_directory->path()));
    const std::optional<GalaxyModel::Galaxy> galaxy = m_model->galaxy(filePath);
    QVERIFY(galaxy.has_value());
    QCOMPARE(galaxy->isLoadable(), loadable);
    QCOMPARE(galaxy->missingPlugins, missingPlugins);
    QCOMPARE(galaxy->error.isEmpty(), galaxy->info.has_value());
    // Dimmed in every column when it cannot be loaded.
    for(int column = 0; column < GalaxyModel::ColumnCount; ++column)
    {
        const QVariant foreground = m_model->index(row, column).data(Qt::ForegroundRole);
        QCOMPARE(foreground.isValid(), !loadable);
        if(!loadable)
        {
            QCOMPARE(foreground.value<QBrush>().color(), currentThemeColors().mutedText);
        }
    }
}

void TestGalaxyModel::decoration()
{
    for(int row = 0; row < m_model->rowCount(); ++row)
    {
        QVERIFY(!m_model->index(row, GalaxyModel::Name).data(Qt::DecorationRole).value<QIcon>().isNull());
    }
    // The state tells it without color too, with a mark.
    QVERIFY(!m_model->index(0, GalaxyModel::State).data(Qt::DecorationRole).isValid());
    QVERIFY(!m_model->index(1, GalaxyModel::State).data(Qt::DecorationRole).value<QIcon>().isNull());
    QVERIFY(!m_model->index(2, GalaxyModel::State).data(Qt::DecorationRole).value<QIcon>().isNull());
}

void TestGalaxyModel::drag()
{
    // Only a galaxy that can be loaded, to be placed in the edition zone.
    for(int column = 0; column < GalaxyModel::ColumnCount; ++column)
    {
        QVERIFY(m_model->flags(m_model->index(0, column)).testFlag(Qt::ItemIsDragEnabled));
        QVERIFY(!m_model->flags(m_model->index(1, column)).testFlag(Qt::ItemIsDragEnabled));
        QVERIFY(!m_model->flags(m_model->index(2, column)).testFlag(Qt::ItemIsDragEnabled));
    }
    QVERIFY(!m_model->flags({}).testFlag(Qt::ItemIsDragEnabled));

    QCOMPARE(m_model->mimeTypes(), QStringList{GalaxyModel::MimeType});
    const std::unique_ptr<QMimeData> data(
        m_model->mimeData({m_model->index(0, GalaxyModel::Name), m_model->index(0, GalaxyModel::State)}));
    QVERIFY(data != nullptr);
    QCOMPARE(QString::fromUtf8(data->data(GalaxyModel::MimeType)),
             QDir(m_directory->path()).absoluteFilePath(u"Alpha.nbp"_s));
    QVERIFY(m_model->mimeData({}) == nullptr);
}

void TestGalaxyModel::headerData_data()
{
    QTest::addColumn<int>("section");
    QTest::addColumn<QString>("title");

    QTest::newRow("Name") << int(GalaxyModel::Name) << "Name";
    QTest::newRow("State") << int(GalaxyModel::State) << "State";
}

void TestGalaxyModel::headerData()
{
    QFETCH(int, section);
    QFETCH(QString, title);

    QCOMPARE(m_model->headerData(section, Qt::Horizontal).toString(), title);
    QVERIFY(!m_model->headerData(section, Qt::Vertical).isValid());
}

void TestGalaxyModel::refreshFollowsTheFiles()
{
    const QSignalSpy inserted(m_model.get(), &QAbstractItemModel::rowsInserted);
    const QSignalSpy removed(m_model.get(), &QAbstractItemModel::rowsRemoved);

    // Between the others: after Alpha, before beta.
    QVERIFY(write(*m_directory, u"Amber.nbp"_s, project({})));
    m_model->refresh();
    QCOMPARE(inserted.count(), 1);
    QCOMPARE(inserted.last().at(1).toInt(), 1);
    QCOMPARE(m_model->index(1, GalaxyModel::Name).data().toString(), u"Amber"_s);

    QVERIFY(QFile::remove(m_directory->filePath(u"Alpha.nbp"_s)));
    m_model->refresh();
    QCOMPARE(removed.count(), 1);
    QCOMPARE(removed.last().at(1).toInt(), 0);
    QCOMPARE(m_model->rowCount(), 3);

    // Written again: read again, in its row.
    QVERIFY(write(*m_directory, u"Gamma.nbp"_s, project({FirstPluginId}, u"Fixed"_s)));
    m_model->refresh();
    QCOMPARE(inserted.count(), 1);
    QCOMPARE(m_model->index(2, GalaxyModel::Name).data().toString(), u"Gamma"_s);
    QCOMPARE(m_model->index(2, GalaxyModel::State).data().toString(), u"Loadable"_s);
    QCOMPARE(m_model->index(2, GalaxyModel::Name).data(Qt::ToolTipRole).toString(), u"Fixed"_s);
}

void TestGalaxyModel::refreshKnowsNewPlugins()
{
    m_application->plugins().scan(QStringLiteral(NEBULA_OTHER_TEST_PLUGINS_DIR));
    const QSignalSpy dataChanged(m_model.get(), &QAbstractItemModel::dataChanged);
    m_model->refresh();
    QCOMPARE(dataChanged.count(), 1);
    QCOMPARE(m_model->index(1, GalaxyModel::State).data(Qt::ToolTipRole).toString(), u"Missing: Other.Plugin"_s);
}

void TestGalaxyModel::followsTheDirectory()
{
    // Without refresh(): the model follows the files of its directory, written again in place, added and removed.
    QVERIFY(write(*m_directory, u"beta.nbp"_s, project({FirstPluginId})));
    QTRY_COMPARE(m_model->index(1, GalaxyModel::State).data().toString(), u"Loadable"_s);

    QVERIFY(write(*m_directory, u"Delta.nbp"_s, project({})));
    QTRY_COMPARE(m_model->rowCount(), 4);
    QCOMPARE(m_model->index(2, GalaxyModel::Name).data().toString(), u"Delta"_s);

    QVERIFY(QFile::remove(m_directory->filePath(u"Delta.nbp"_s)));
    QTRY_COMPARE(m_model->rowCount(), 3);
}

void TestGalaxyModel::setDirectory()
{
    const QTemporaryDir other;
    const QString later = other.filePath(u"later"_s);
    m_model->setDirectory(later);
    QCOMPARE(m_model->directory(), QDir(later).absolutePath());
    QCOMPARE(m_model->rowCount(), 0);

    // Listed once it exists, by the next refresh().
    QVERIFY(QDir().mkpath(later));
    QFile file(QDir(later).filePath(u"Saved.nbp"_s));
    QVERIFY(file.open(QIODevice::WriteOnly));
    QVERIFY(file.write(project({})) > 0);
    file.close();
    m_model->refresh();
    QCOMPARE(m_model->rowCount(), 1);
    QCOMPARE(m_model->index(0, GalaxyModel::Name).data().toString(), u"Saved"_s);

    m_model->setDirectory(m_directory->path());
    QCOMPARE(m_model->rowCount(), 3);
}

void TestGalaxyModel::galaxyAndIndexOf()
{
    const QString filePath = QDir(m_directory->path()).absoluteFilePath(u"beta.nbp"_s);
    QCOMPARE(m_model->indexOf(filePath), m_model->index(1, 0));
    const std::optional<GalaxyModel::Galaxy> galaxy = m_model->galaxy(filePath);
    QVERIFY(galaxy.has_value() && galaxy->info.has_value());
    QCOMPARE(galaxy->info->name, u"beta"_s);
    QCOMPARE(galaxy->info->plugins, (QStringList{FirstPluginId, SecondPluginId, u"Other.Plugin"_s}));

    QVERIFY(!m_model->indexOf(m_directory->filePath(u"notes.txt"_s)).isValid());
    QVERIFY(!m_model->galaxy(m_directory->filePath(u"Missing.nbp"_s)).has_value());
}

// A QApplication: the mark of the state is an icon of the style.
QTEST_MAIN(TestGalaxyModel)
#include "TestGalaxyModel.moc"
