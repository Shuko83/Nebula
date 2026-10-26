// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QPluginLoader>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTest>
#include <memory>
#include <optional>

#include "Nebula/Component/ComponentFactory.h"
#include "Nebula/Plugin/IPlugin.h"
#include "Nebula/Plugin/PluginManager.h"

using Nebula::PluginInfo;
using Nebula::PluginManager;

namespace
{

const QString FirstPluginId = QStringLiteral("FabienDG.Nebula.FirstPlugin");
const QString SecondPluginId = QStringLiteral("FabienDG.Nebula.SecondPlugin");
const QString ThirdPluginId = QStringLiteral("FabienDG.Nebula.ThirdPlugin");

// The file of FirstPlugin, as the build wrote it.
QString firstPluginFile()
{
    return QStringLiteral(NEBULA_TEST_PLUGINS_DIR "/FirstPlugin.nbdl");
}

// Whether ComponentFactory::instance() can create componentId.
bool isRegistered(const QString& componentId)
{
    return Nebula::ComponentFactory::instance().component(componentId).has_value();
}

} // namespace

// PluginManager, with the plugins of the tests: how it finds them, loads and unloads them, and what it refuses.
class TestPluginManager : public QObject
{
    Q_OBJECT

  private slots:
    void init();

    void defaultDirectory();
    void scan_data();
    void scan();
    void scanSkipsWhatItKnows();
    void scanReadsOnlyPlugins();
    void addFile_data();
    void addFile();
    void loadAndUnload_data();
    void loadAndUnload();
    void rootObjectCreatesItsComponents_data();
    void rootObjectCreatesItsComponents();
    void unknownPlugin();
    void unloadWhatIsNotLoaded();
    void pluginLoadedByTwoManagers();
    void pluginLoadedByAnotherManager();
    void destructorUnloads();
};

void TestPluginManager::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
}

void TestPluginManager::defaultDirectory()
{
    QCOMPARE(PluginManager::defaultDirectory(), QCoreApplication::applicationDirPath() + QStringLiteral("/plugins"));
}

void TestPluginManager::scan_data()
{
    QTest::addColumn<QString>("directory");
    QTest::addColumn<QString>("id");
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("description");
    QTest::addColumn<QString>("version");
    // How many plugins its directory has.
    QTest::addColumn<int>("count");

    QTest::newRow("FirstPlugin") << NEBULA_TEST_PLUGINS_DIR << FirstPluginId << "FirstPlugin"
                                 << "First plugin of the tests" << "1.0.0" << 2;
    QTest::newRow("ThirdPlugin") << NEBULA_TEST_PLUGINS_DIR << ThirdPluginId << "ThirdPlugin"
                                 << "Third plugin of the tests, with two components" << "2.0.0" << 2;
    QTest::newRow("SecondPlugin") << NEBULA_OTHER_TEST_PLUGINS_DIR << SecondPluginId << "SecondPlugin"
                                  << "Second plugin of the tests, in another directory" << "1.1.0" << 1;
}

void TestPluginManager::scan()
{
    QFETCH(QString, directory);
    QFETCH(QString, id);
    QFETCH(QString, name);
    QFETCH(QString, description);
    QFETCH(QString, version);
    QFETCH(int, count);

    PluginManager plugins;
    const QList<PluginInfo> found = plugins.scan(directory);
    QCOMPARE(found.size(), count);
    QCOMPARE(plugins.plugins().size(), count);

    const std::optional<PluginInfo> info = plugins.plugin(id);
    QVERIFY(info.has_value());
    QCOMPARE(info->id, id);
    QCOMPARE(info->name, name);
    QCOMPARE(info->description, description);
    QCOMPARE(info->version, QVersionNumber::fromString(version));
    QCOMPARE(info->coreVersion, QVersionNumber::fromString(QStringLiteral(NEBULA_VERSION)));
    QCOMPARE(info->filePath, QFileInfo(directory + u'/' + name + QStringLiteral(".nbdl")).canonicalFilePath());
    // Read without loading it.
    QVERIFY(!info->buildDate.isValid());
    QVERIFY(!plugins.isLoaded(id));
}

void TestPluginManager::scanSkipsWhatItKnows()
{
    PluginManager plugins;
    QCOMPARE(plugins.scan(QStringLiteral(NEBULA_TEST_PLUGINS_DIR)).size(), 2);
    QVERIFY(plugins.scan(QStringLiteral(NEBULA_TEST_PLUGINS_DIR)).isEmpty());

    // The same plugin in another file: skipped, its id being known.
    const QTemporaryDir copies;
    QVERIFY(QFile::copy(firstPluginFile(), copies.filePath(QStringLiteral("Copy.nbdl"))));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("^Skipping .*Copy\\.nbdl - %1 is already "
                                                                         "provided by")
                                                              .arg(QRegularExpression::escape(FirstPluginId))));
    QVERIFY(plugins.scan(copies.path()).isEmpty());
    QCOMPARE(plugins.plugins().size(), 2);
}

void TestPluginManager::scanReadsOnlyPlugins()
{
    const QTemporaryDir directory;
    for(const QString& name : {QStringLiteral("readme.txt"), QStringLiteral("fake.nbdl")})
    {
        QFile file(directory.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("Not a plugin");
    }

    PluginManager plugins;
    // fake.nbdl only: the others are not even read.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("^Skipping .*fake\\.nbdl - ")));
    QVERIFY(plugins.scan(directory.path()).isEmpty());
    QVERIFY(plugins.scan(directory.filePath(QStringLiteral("missing"))).isEmpty());
    QVERIFY(plugins.plugins().isEmpty());
}

void TestPluginManager::addFile_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<bool>("added");
    // A part of errorString(); empty when it is.
    QTest::addColumn<QString>("error");

    QTest::newRow("plugin") << "first" << true << "";
    QTest::newRow("known already") << "first again" << true << "";
    QTest::newRow("missing") << "missing" << false << "is not a file";
    QTest::newRow("directory") << "directory" << false << "is not a file";
    QTest::newRow("not a plugin") << "text" << false << "";
    QTest::newRow("id of another file") << "copy" << false << FirstPluginId + QStringLiteral(" is already provided by");
}

void TestPluginManager::addFile()
{
    QFETCH(QString, file);
    QFETCH(bool, added);
    QFETCH(QString, error);

    const QTemporaryDir directory;
    const QString copy = directory.filePath(QStringLiteral("Copy.nbdl"));
    const QString text = directory.filePath(QStringLiteral("text.nbdl"));
    QVERIFY(QFile::copy(firstPluginFile(), copy));
    QFile textFile(text);
    QVERIFY(textFile.open(QIODevice::WriteOnly));
    textFile.write("Not a plugin");
    textFile.close();
    const QHash<QString, QString> paths{
        {QStringLiteral("first"), firstPluginFile()},
        {QStringLiteral("first again"), firstPluginFile()},
        {QStringLiteral("missing"), directory.filePath("none")},
        {QStringLiteral("directory"), directory.path()},
        {QStringLiteral("text"), text},
        {QStringLiteral("copy"), copy},
    };

    PluginManager plugins;
    if(file == u"first again" || file == u"copy")
    {
        QVERIFY(plugins.addFile(firstPluginFile()).has_value());
    }
    const std::optional<PluginInfo> info = plugins.addFile(paths.value(file));
    QCOMPARE(info.has_value(), added);
    if(added)
    {
        QCOMPARE(info->id, FirstPluginId);
        QVERIFY(plugins.errorString().isEmpty());
    }
    else
    {
        QVERIFY(!plugins.errorString().isEmpty());
        QVERIFY2(plugins.errorString().contains(error), qPrintable(plugins.errorString()));
    }
    QCOMPARE(plugins.plugins().size(), file == u"first again" || file == u"copy" || added ? 1 : 0);
}

void TestPluginManager::loadAndUnload_data()
{
    QTest::addColumn<QString>("directory");
    QTest::addColumn<QString>("id");
    QTest::addColumn<QStringList>("components");

    QTest::newRow("FirstPlugin") << NEBULA_TEST_PLUGINS_DIR << FirstPluginId << QStringList{"Alpha"};
    QTest::newRow("SecondPlugin") << NEBULA_OTHER_TEST_PLUGINS_DIR << SecondPluginId << QStringList{"Beta"};
    QTest::newRow("ThirdPlugin, two components")
        << NEBULA_TEST_PLUGINS_DIR << ThirdPluginId << QStringList{"Gamma", "Delta"};
}

void TestPluginManager::loadAndUnload()
{
    QFETCH(QString, directory);
    QFETCH(QString, id);
    QFETCH(QStringList, components);

    PluginManager plugins;
    plugins.scan(directory);
    // Twice: unloaded, it can be loaded again.
    for(int time = 0; time < 2; ++time)
    {
        QVERIFY2(plugins.load(id), qPrintable(plugins.errorString()));
        QVERIFY(plugins.isLoaded(id));
        QVERIFY(plugins.plugin(id)->buildDate.isValid());
        for(const QString& component : components)
        {
            QVERIFY2(isRegistered(id + u'.' + component), qPrintable(component));
        }

        QVERIFY(plugins.unload(id));
        QVERIFY(plugins.errorString().isEmpty());
        QVERIFY(!plugins.isLoaded(id));
        for(const QString& component : components)
        {
            QVERIFY2(!isRegistered(id + u'.' + component), qPrintable(component));
        }
    }
}

void TestPluginManager::rootObjectCreatesItsComponents_data()
{
    loadAndUnload_data();
}

// What nebula_add_plugin generates, loaded without a PluginManager: the root object of the plugin, which lists and
// creates its components.
void TestPluginManager::rootObjectCreatesItsComponents()
{
    QFETCH(QString, directory);
    QFETCH(QString, id);
    QFETCH(QStringList, components);

    QPluginLoader loader(directory + u'/' + id.section(u'.', -1) + QStringLiteral(".nbdl"));
    auto* plugin = qobject_cast<Nebula::IPlugin*>(loader.instance());
    QVERIFY2(plugin != nullptr, qPrintable(loader.errorString()));
    QStringList names;
    for(const QMetaObject* metaObject : plugin->components())
    {
        names.append(Nebula::componentName(*metaObject));
    }
    // In the order the linker initializes their source files.
    names.sort();
    components.sort();
    QCOMPARE(names, components);
    for(const QString& name : components)
    {
        const std::unique_ptr<Nebula::Component> component = plugin->create(name);
        QVERIFY2(component != nullptr, qPrintable(name));
        QCOMPARE(Nebula::componentName(*component->metaObject()), name);
    }
    QVERIFY(plugin->create(QStringLiteral("Unknown")) == nullptr);
    QVERIFY(plugin->create(id + QStringLiteral(".") + components.first()) == nullptr);
    QVERIFY(loader.unload());
}

void TestPluginManager::unknownPlugin()
{
    PluginManager plugins;
    QVERIFY(!plugins.load(QStringLiteral("Unknown")));
    QCOMPARE(plugins.errorString(), QStringLiteral("Unknown plugin Unknown"));
    QVERIFY(!plugins.unload(QStringLiteral("Unknown")));
    QCOMPARE(plugins.errorString(), QStringLiteral("Unknown plugin Unknown"));
    QVERIFY(!plugins.isLoaded(QStringLiteral("Unknown")));
    QVERIFY(!plugins.plugin(QStringLiteral("Unknown")).has_value());
}

void TestPluginManager::unloadWhatIsNotLoaded()
{
    PluginManager plugins;
    plugins.scan(QStringLiteral(NEBULA_TEST_PLUGINS_DIR));
    QVERIFY(plugins.unload(FirstPluginId));
    QVERIFY(plugins.errorString().isEmpty());
}

void TestPluginManager::pluginLoadedByTwoManagers()
{
    const QString alpha = FirstPluginId + QStringLiteral(".Alpha");
    PluginManager first;
    PluginManager second;
    first.scan(QStringLiteral(NEBULA_TEST_PLUGINS_DIR));
    second.scan(QStringLiteral(NEBULA_TEST_PLUGINS_DIR));
    QVERIFY(first.load(FirstPluginId));
    QVERIFY(second.load(FirstPluginId));

    // It stays loaded for the second one, its components with it.
    QVERIFY(!first.unload(FirstPluginId));
    QCOMPARE(first.errorString(), FirstPluginId + QStringLiteral(" is still loaded by another plugin manager"));
    QVERIFY(first.isLoaded(FirstPluginId));
    QVERIFY(isRegistered(alpha));

    QVERIFY(second.unload(FirstPluginId));
    QVERIFY(!second.isLoaded(FirstPluginId));
    QVERIFY(!first.isLoaded(FirstPluginId));
    QVERIFY(!isRegistered(alpha));
}

void TestPluginManager::pluginLoadedByAnotherManager()
{
    PluginManager loading;
    PluginManager other;
    loading.scan(QStringLiteral(NEBULA_TEST_PLUGINS_DIR));
    other.scan(QStringLiteral(NEBULA_TEST_PLUGINS_DIR));
    QVERIFY(loading.load(FirstPluginId));

    // Only the one that loaded it unloads it.
    QVERIFY(other.isLoaded(FirstPluginId));
    QVERIFY(!other.unload(FirstPluginId));
    QCOMPARE(other.errorString(), FirstPluginId + QStringLiteral(" was loaded by another plugin manager"));
    QVERIFY(loading.isLoaded(FirstPluginId));
    QVERIFY(loading.unload(FirstPluginId));
}

void TestPluginManager::destructorUnloads()
{
    {
        PluginManager plugins;
        plugins.scan(QStringLiteral(NEBULA_TEST_PLUGINS_DIR));
        QVERIFY(plugins.load(FirstPluginId));
        QVERIFY(isRegistered(FirstPluginId + QStringLiteral(".Alpha")));
    }
    QVERIFY(!isRegistered(FirstPluginId + QStringLiteral(".Alpha")));

    PluginManager again;
    again.scan(QStringLiteral(NEBULA_TEST_PLUGINS_DIR));
    QVERIFY(!again.isLoaded(FirstPluginId));
}

QTEST_GUILESS_MAIN(TestPluginManager)
#include "TestPluginManager.moc"
