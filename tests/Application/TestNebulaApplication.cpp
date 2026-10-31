// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <memory>
#include <vector>

#include "Nebula/Application/NebulaApplication.h"
#include "Probe.h"
#include "TestComponents.h"

using namespace Qt::StringLiterals;
using Nebula::ApplicationOptions;
using Nebula::NebulaApplication;

namespace
{

Q_LOGGING_CATEGORY(lcTest, "test.application")

const QString FirstPluginId = u"FabienDG.Nebula.FirstPlugin"_s;
const QString SecondPluginId = u"FabienDG.Nebula.SecondPlugin"_s;
const QString GalaxyName = u"Clock.nbp"_s;

// The command line of an application of the test, its program then arguments: it lives as long as the application, as
// Qt needs.
class CommandLine
{
  public:
    explicit CommandLine(const QStringList& arguments = {})
    {
        m_arguments.append(QByteArrayLiteral("TestNebulaApplication"));
        for(const QString& argument : arguments)
        {
            m_arguments.append(argument.toLocal8Bit());
        }
        for(QByteArray& argument : m_arguments)
        {
            m_values.push_back(argument.data());
        }
        m_values.push_back(nullptr);
        m_count = static_cast<int>(m_arguments.size());
    }

    int& count()
    {
        return m_count;
    }

    char** values()
    {
        return m_values.data();
    }

  private:
    QList<QByteArray> m_arguments;
    std::vector<char*> m_values;
    int m_count = 0;
};

// A Probe of a project, named name, which refuses to start or not.
QJsonObject probe(const QString& name, bool refusesToStart = false)
{
    return {{u"name"_s, name},
            {u"type"_s, testComponentId(u"Probe"_s)},
            {u"properties"_s, QJsonObject{{u"refusesToStart"_s, refusesToStart}}}};
}

// Writes the project of components, made of the plugins of the ids plugins, to the file filePath, its directory
// created; returns filePath.
QString writeGalaxy(const QString& filePath, const QJsonArray& components = {}, const QStringList& plugins = {})
{
    QJsonArray pluginsJson;
    for(const QString& plugin : plugins)
    {
        pluginsJson.append(QJsonObject{{u"id"_s, plugin}});
    }
    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile file(filePath);
    if(file.open(QIODevice::WriteOnly))
    {
        const QJsonObject project = {{u"version"_s, 1}, {u"plugins"_s, pluginsJson}, {u"components"_s, components}};
        file.write(QJsonDocument(project).toJson());
    }
    return filePath;
}

// A logger that counts in calls the messages of the test given to it.
Nebula::Logger counting(int& calls)
{
    return [&calls](QtMsgType /*type*/, const QMessageLogContext& context, const QString& /*message*/)
    {
        if(QLatin1StringView(context.category) == lcTest().categoryName())
        {
            ++calls;
        }
    };
}

} // namespace

// NebulaApplication: what it gives QCoreApplication, how it finds its galaxy and its plugins, the command line it reads
// with the one of the main, how it runs the galaxy, and where the messages go. Each test builds its own application,
// one after the other, as a main does.
class TestNebulaApplication : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void cleanup();

    void executableInfo_data();
    void executableInfo();
    void galaxyDirectories_data();
    void galaxyDirectories();
    void galaxyPath();
    void galaxyNotFound();
    void galaxyNotLoaded();
    void pluginDirectories();
    void pluginRoot_data();
    void pluginRoot();
    void commandLineOfTheMain();
    void runs();
    void refusesToStart();
    void logLevel_data();
    void logLevel();
    void unknownLogLevel();
    void logFile();
    void logFileRefused();
    void console();
    void loggerLogsItself();
    void messagesBackToQt();

  private:
    // The path of name in the temporary directory of the test.
    [[nodiscard]] QString path(const QString& name) const;

    std::unique_ptr<QTemporaryDir> m_directory;
};

void TestNebulaApplication::initTestCase()
{
    registerTestComponents();
}

void TestNebulaApplication::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
    m_directory = std::make_unique<QTemporaryDir>();
    QVERIFY(m_directory->isValid());
}

void TestNebulaApplication::cleanup()
{
    m_directory.reset();
    // What an application gave QCoreApplication stays for the next one.
    for(auto set : {&QCoreApplication::setApplicationName, &QCoreApplication::setApplicationVersion,
                    &QCoreApplication::setOrganizationName, &QCoreApplication::setOrganizationDomain})
    {
        set(QString());
    }
}

QString TestNebulaApplication::path(const QString& name) const
{
    return m_directory->filePath(name);
}

void TestNebulaApplication::executableInfo_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("version");
    QTest::addColumn<QString>("organization");
    QTest::addColumn<QString>("domain");
    QTest::addColumn<QString>("applicationName");

    QTest::newRow("all of it") << u"Clock"_s << u"1.2.3"_s << u"FabienDG"_s << u"example.org"_s << u"Clock"_s;
    QTest::newRow("none: Qt keeps the name of the executable")
        << QString() << QString() << QString() << QString() << u"TestNebulaApplication"_s;
}

void TestNebulaApplication::executableInfo()
{
    QFETCH(QString, name);
    QFETCH(QString, version);
    QFETCH(QString, organization);
    QFETCH(QString, domain);
    QFETCH(QString, applicationName);

    CommandLine line;
    const NebulaApplication app(
        line.count(), line.values(), writeGalaxy(path(GalaxyName)),
        {.executable = {.name = name, .version = version, .organization = organization, .organizationDomain = domain}});
    QCOMPARE(QCoreApplication::applicationName(), applicationName);
    QCOMPARE(QCoreApplication::applicationVersion(), version);
    QCOMPARE(QCoreApplication::organizationName(), organization);
    QCOMPARE(QCoreApplication::organizationDomain(), domain);
}

void TestNebulaApplication::galaxyDirectories_data()
{
    QTest::addColumn<QStringList>("holders"); // The directories that have the galaxy
    QTest::addColumn<QStringList>("code");
    QTest::addColumn<QStringList>("commandLine");
    QTest::addColumn<QString>("found");

    const QStringList both = {u"a"_s, u"b"_s};
    QTest::newRow("the first directory of the code") << both << both << QStringList() << u"a"_s;
    QTest::newRow("the second one") << QStringList{u"b"_s} << both << QStringList() << u"b"_s;
    QTest::newRow("the command line first") << both << QStringList{u"a"_s} << QStringList{u"b"_s} << u"b"_s;
    QTest::newRow("the code after the command line")
        << QStringList{u"a"_s} << QStringList{u"a"_s} << QStringList{u"b"_s} << u"a"_s;
}

void TestNebulaApplication::galaxyDirectories()
{
    QFETCH(QStringList, holders);
    QFETCH(QStringList, code);
    QFETCH(QStringList, commandLine);
    QFETCH(QString, found);

    for(const QString& holder : holders)
    {
        writeGalaxy(path(holder + u'/' + GalaxyName), {probe(u"Probe1"_s)});
    }
    QStringList arguments;
    for(const QString& directory : commandLine)
    {
        arguments << u"--galaxy-dir"_s << path(directory);
    }
    QStringList directories;
    for(const QString& directory : code)
    {
        directories.append(path(directory));
    }

    CommandLine line(arguments);
    QCommandLineParser parser;
    const NebulaApplication app(line.count(), line.values(), parser, GalaxyName, {.galaxyDirectories = directories});
    QVERIFY2(app.isLoaded(), qPrintable(app.errorString()));
    QCOMPARE(app.galaxyFile(), path(found + u'/' + GalaxyName));
    QVERIFY(app.universe().component(u"Probe1"_s) != nullptr);
    QCOMPARE(app.universe().filePath(), app.galaxyFile());
}

void TestNebulaApplication::galaxyPath()
{
    // An absolute path is not looked for in the directories, and a relative one is looked for in each of them.
    const QString galaxy = writeGalaxy(path(u"elsewhere/"_s + GalaxyName));
    {
        CommandLine line;
        const NebulaApplication app(line.count(), line.values(), galaxy, {.galaxyDirectories = {}});
        QVERIFY2(app.isLoaded(), qPrintable(app.errorString()));
        QCOMPARE(app.galaxyFile(), galaxy);
    }
    CommandLine line;
    const NebulaApplication app(line.count(), line.values(), u"elsewhere/"_s + GalaxyName,
                                {.galaxyDirectories = {path(u"a"_s), m_directory->path()}});
    QVERIFY2(app.isLoaded(), qPrintable(app.errorString()));
    QCOMPARE(app.galaxyFile(), galaxy);
}

void TestNebulaApplication::galaxyNotFound()
{
    CommandLine line;
    QTest::ignoreMessage(QtCriticalMsg, QRegularExpression(u"^The galaxy Clock\\.nbp is in none of "_s));
    NebulaApplication app(line.count(), line.values(), GalaxyName, {.galaxyDirectories = {path(u"a"_s)}});
    QVERIFY(!app.isLoaded());
    QVERIFY(app.galaxyFile().isEmpty());
    QVERIFY2(app.errorString().contains(QDir::toNativeSeparators(path(u"a"_s))), qPrintable(app.errorString()));
    QCOMPARE(app.exec(), NebulaApplication::LoadFailed); // At once
}

void TestNebulaApplication::galaxyNotLoaded()
{
    const QJsonObject unknown = {{u"name"_s, u"Nothing1"_s}, {u"type"_s, testComponentId(u"Nothing"_s)}};
    const QString galaxy = writeGalaxy(path(GalaxyName), {probe(u"Probe1"_s), unknown});
    CommandLine line;
    QTest::ignoreMessage(QtCriticalMsg, QRegularExpression(u"^The galaxy .* cannot be loaded:\n"_s));
    NebulaApplication app(line.count(), line.values(), galaxy);
    QVERIFY(!app.isLoaded());
    QCOMPARE(app.galaxyFile(), galaxy);
    QCOMPARE(app.exec(), NebulaApplication::LoadFailed);
    QVERIFY(!app.universe().isRunning()); // What could be loaded does not run
}

void TestNebulaApplication::pluginDirectories()
{
    const QJsonObject alpha = {{u"name"_s, u"Alpha1"_s}, {u"type"_s, FirstPluginId + u".Alpha"_s}};
    const QString galaxy = writeGalaxy(path(GalaxyName), {alpha}, {FirstPluginId});
    CommandLine line({u"--plugin-dir"_s, QStringLiteral(NEBULA_OTHER_TEST_PLUGINS_DIR)});
    QCommandLineParser parser;
    // test-plugins/ is next to the executable of the test, which the directories of the code are relative to.
    const NebulaApplication app(line.count(), line.values(), parser, galaxy,
                                {.pluginDirectories = {u"test-plugins"_s}});
    QVERIFY2(app.isLoaded(), qPrintable(app.errorString()));
    QVERIFY(app.universe().plugins().isLoaded(FirstPluginId)); // By its galaxy
    QVERIFY(app.universe().plugins().plugin(SecondPluginId).has_value());
    QVERIFY(app.universe().component(u"Alpha1"_s) != nullptr);
}

void TestNebulaApplication::pluginRoot_data()
{
    QTest::addColumn<QString>("code");
    QTest::addColumn<QStringList>("arguments");
    QTest::addColumn<QString>("root");

    QTest::newRow("none: the directory of the executable") << QString() << QStringList() << QString();
    QTest::newRow("relative: from the executable") << u"roots"_s << QStringList() << u"roots"_s;
    QTest::newRow("the command line replaces it")
        << u"roots"_s << QStringList{u"--plugin-root"_s, QDir::tempPath()} << QDir::tempPath();
}

void TestNebulaApplication::pluginRoot()
{
    QFETCH(QString, code);
    QFETCH(QStringList, arguments);
    QFETCH(QString, root);

    CommandLine line(arguments);
    QCommandLineParser parser;
    const NebulaApplication app(line.count(), line.values(), parser, writeGalaxy(path(GalaxyName)),
                                {.pluginRoot = code});
    const QDir executable(QCoreApplication::applicationDirPath());
    QCOMPARE(app.universe().pluginRoot(), QDir::cleanPath(executable.absoluteFilePath(root)));
}

void TestNebulaApplication::commandLineOfTheMain()
{
    CommandLine line({u"--name"_s, u"Ada"_s, u"--log-level"_s, u"info"_s, u"project.nbp"_s});
    QCommandLineParser parser;
    parser.addOption({u"name"_s, u"Who to greet."_s, u"name"_s});
    parser.addPositionalArgument(u"project"_s, u"Project file to open."_s);
    const NebulaApplication app(line.count(), line.values(), parser, writeGalaxy(path(GalaxyName)));
    QVERIFY2(app.isLoaded(), qPrintable(app.errorString()));
    // What the main declared, and what the application did, read from the same parser.
    QCOMPARE(parser.value(u"name"_s), u"Ada"_s);
    QCOMPARE(parser.positionalArguments(), QStringList{u"project.nbp"_s});
    QCOMPARE(parser.value(u"log-level"_s), u"info"_s);
}

void TestNebulaApplication::runs()
{
    CommandLine line;
    NebulaApplication app(line.count(), line.values(), writeGalaxy(path(GalaxyName), {probe(u"Probe1"_s)}));
    auto* probe = qobject_cast<Probe*>(app.universe().component(u"Probe1"_s));
    QVERIFY(probe != nullptr);
    QVERIFY(!app.universe().isRunning()); // Not before exec()

    bool running = false;
    QTimer::singleShot(0, &app,
                       [&app, &running]
                       {
                           running = app.universe().isRunning();
                           QCoreApplication::exit(7);
                       });
    QCOMPARE(app.exec(), 7); // The code it quit with
    QVERIFY(running);
    QVERIFY(!app.universe().isRunning()); // Stopped once it quit
    QCOMPARE(probe->starts(), 1);
    QVERIFY(app.errorString().isEmpty());
}

void TestNebulaApplication::refusesToStart()
{
    CommandLine line;
    NebulaApplication app(line.count(), line.values(), writeGalaxy(path(GalaxyName), {probe(u"Probe1"_s, true)}));
    QVERIFY(app.isLoaded());
    QTest::ignoreMessage(QtCriticalMsg, QRegularExpression(u"^The galaxy .* refused to start: "_s));
    QCOMPARE(app.exec(), NebulaApplication::StartFailed);
    QVERIFY(!app.universe().isRunning());
    QVERIFY(!app.errorString().isEmpty());
}

void TestNebulaApplication::logLevel_data()
{
    QTest::addColumn<QString>("level");
    QTest::addColumn<QList<int>>("written");

    QTest::newRow("debug: all of them") << u"debug"_s << QList<int>{QtDebugMsg, QtInfoMsg, QtWarningMsg, QtCriticalMsg};
    QTest::newRow("info") << u"info"_s << QList<int>{QtInfoMsg, QtWarningMsg, QtCriticalMsg};
    QTest::newRow("warning, whatever its case") << u"WARNING"_s << QList<int>{QtWarningMsg, QtCriticalMsg};
    QTest::newRow("critical") << u"critical"_s << QList<int>{QtCriticalMsg};
    QTest::newRow("fatal: only the fatal ones") << u"fatal"_s << QList<int>();
}

void TestNebulaApplication::logLevel()
{
    QFETCH(QString, level);
    QFETCH(QList<int>, written);

    QList<int> given;
    const Nebula::Logger logger =
        [&given](QtMsgType type, const QMessageLogContext& context, const QString& /*message*/)
    {
        if(QLatin1StringView(context.category) == lcTest().categoryName())
        {
            given.append(type);
        }
    };
    CommandLine line({u"--log-level"_s, level});
    QCommandLineParser parser;
    const NebulaApplication app(line.count(), line.values(), parser, writeGalaxy(path(GalaxyName)), {.logger = logger});
    for(const int type : written)
    {
        QTest::ignoreMessage(static_cast<QtMsgType>(type), "logged"); // Where Qt writes them, as well
    }
    qCDebug(lcTest, "logged");
    qCInfo(lcTest, "logged");
    qCWarning(lcTest, "logged");
    qCCritical(lcTest, "logged");
    QCOMPARE(given, written);
}

void TestNebulaApplication::unknownLogLevel()
{
    CommandLine line({u"--log-level"_s, u"verbose"_s});
    QCommandLineParser parser;
    QTest::ignoreMessage(QtCriticalMsg, "Unknown log level verbose: debug, info, warning, critical, fatal");
    NebulaApplication app(line.count(), line.values(), parser, writeGalaxy(path(GalaxyName)));
    QVERIFY(!app.isLoaded());
    QCOMPARE(app.exec(), NebulaApplication::LoadFailed);
}

void TestNebulaApplication::logFile()
{
    const QString logFile = path(u"logs/application.log"_s); // In a directory that does not exist yet
    writeGalaxy(path(GalaxyName));
    for(const QString& message : {u"first"_s, u"second"_s})
    {
        CommandLine line({u"--log-file"_s, logFile});
        QCommandLineParser parser;
        const NebulaApplication app(line.count(), line.values(), parser, path(GalaxyName));
        QVERIFY2(app.isLoaded(), qPrintable(app.errorString()));
        QTest::ignoreMessage(QtInfoMsg, qPrintable(message)); // Where Qt writes them, as well
        qCInfo(lcTest).noquote() << message;
    }
    // Added to it, each one after the other.
    QFile file(logFile);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString text = QString::fromUtf8(file.readAll());
    QVERIFY2(text.contains(u"test.application: first\ntest.application: second\n"_s), qPrintable(text));
}

void TestNebulaApplication::logFileRefused()
{
    CommandLine line({u"--log-file"_s, m_directory->path()}); // A directory
    QCommandLineParser parser;
    QTest::ignoreMessage(QtCriticalMsg, QRegularExpression(u"^Cannot open the log file "_s));
    NebulaApplication app(line.count(), line.values(), parser, writeGalaxy(path(GalaxyName)));
    QVERIFY(!app.isLoaded());
    QCOMPARE(app.exec(), NebulaApplication::LoadFailed);
}

void TestNebulaApplication::console()
{
    int calls = 0;
    CommandLine line;
    const NebulaApplication app(line.count(), line.values(), writeGalaxy(path(GalaxyName)),
                                {.console = true, .logger = counting(calls)});
    // Written to the error output in place of where Qt writes them: QTest, which would fail on it, does not see it.
    qCWarning(lcTest, "to the console");
    QCOMPARE(calls, 1);
}

void TestNebulaApplication::loggerLogsItself()
{
    int calls = 0;
    const Nebula::Logger logger =
        [&calls](QtMsgType /*type*/, const QMessageLogContext& context, const QString& /*message*/)
    {
        if(QLatin1StringView(context.category) == lcTest().categoryName())
        {
            ++calls;
            qCInfo(lcTest, "from the logger");
        }
    };
    CommandLine line;
    const NebulaApplication app(line.count(), line.values(), writeGalaxy(path(GalaxyName)), {.logger = logger});
    QTest::ignoreMessage(QtInfoMsg, "logged");
    qCInfo(lcTest, "logged"); // Then "from the logger", which Qt writes to the error output, not to the logger again
    QCOMPARE(calls, 1);
}

void TestNebulaApplication::messagesBackToQt()
{
    int calls = 0;
    {
        CommandLine line({u"--log-level"_s, u"warning"_s});
        QCommandLineParser parser;
        const NebulaApplication app(line.count(), line.values(), parser, writeGalaxy(path(GalaxyName)),
                                    {.logger = counting(calls)});
        QVERIFY(!lcTest().isDebugEnabled());
    }
    QVERIFY(lcTest().isDebugEnabled()); // Its filter rules are gone with it
    QTest::ignoreMessage(QtInfoMsg, "after");
    qCInfo(lcTest, "after");
    QCOMPARE(calls, 0);
}

QTEST_APPLESS_MAIN(TestNebulaApplication)
#include "TestNebulaApplication.moc"
