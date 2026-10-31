// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Nebula/Application/NebulaApplication.h"

#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <algorithm>
#include <iterator>
#include <optional>

#include "MessageLog.h"

namespace Nebula
{

namespace
{

Q_LOGGING_CATEGORY(lcApplication, "nebula.application")

// The names of the options addCommandLineOptions() adds, beyond those of Qt and of Universe.
const QString GalaxyDirOption = QStringLiteral("galaxy-dir");
const QString LogLevelOption = QStringLiteral("log-level");
const QString LogFileOption = QStringLiteral("log-file");
const QString ConsoleOption = QStringLiteral("console");

// The directory of the executable, which the paths of ApplicationOptions are relative to.
QDir executableDirectory()
{
    return QDir(QCoreApplication::applicationDirPath());
}

// path, read from directory when it is relative.
QString absolutePath(const QDir& directory, const QString& path)
{
    return QDir::cleanPath(directory.absoluteFilePath(path));
}

// paths, each read from directory when it is relative.
QStringList absolutePaths(const QDir& directory, const QStringList& paths)
{
    QStringList absolute;
    std::ranges::transform(paths, std::back_inserter(absolute),
                           [&directory](const QString& path) { return absolutePath(directory, path); });
    return absolute;
}

// The file named name in each of directories, in their order.
QStringList filesIn(const QStringList& directories, const QString& name)
{
    QStringList files;
    std::ranges::transform(directories, std::back_inserter(files),
                           [&name](const QString& directory) { return QDir(directory).filePath(name); });
    return files;
}

// The file of galaxy, a file name, or a path, in the first of directories that has it; galaxy itself when it is an
// absolute path. Empty when there is none.
QString findGalaxy(const QString& galaxy, const QStringList& directories)
{
    const QStringList files = QDir::isAbsolutePath(galaxy) ? QStringList{galaxy} : filesIn(directories, galaxy);
    const auto found = std::ranges::find_if(files, [](const QString& file) { return QFileInfo(file).isFile(); });
    return found != files.end() ? QDir::cleanPath(*found) : QString();
}

// paths, as the system writes them, one after the other.
QString nativeList(const QStringList& paths)
{
    QStringList native;
    std::ranges::transform(paths, std::back_inserter(native), &QDir::toNativeSeparators);
    return native.join(QStringLiteral(", "));
}

// Why galaxy is not found, looked for in directories.
QString notFound(const QString& galaxy, const QStringList& directories)
{
    if(QDir::isAbsolutePath(galaxy) || directories.isEmpty())
    {
        return QStringLiteral("The galaxy %1 does not exist").arg(QDir::toNativeSeparators(galaxy));
    }
    return QStringLiteral("The galaxy %1 is in none of %2").arg(galaxy, nativeList(directories));
}

// Why the galaxy of the file filePath cannot be loaded: problems, as Universe::open() tells them.
QString cannotLoad(const QString& filePath, const QString& problems)
{
    return QStringLiteral("The galaxy %1 cannot be loaded:\n%2").arg(QDir::toNativeSeparators(filePath), problems);
}

// Gives QCoreApplication value through set, unless it is empty: Qt keeps the one it has then.
void setUnlessEmpty(const QString& value, void (*set)(const QString&))
{
    if(!value.isEmpty())
    {
        set(value);
    }
}

// Adds to parser the options of the messages: their level, their file and the console.
void addLogOptions(QCommandLineParser& parser)
{
    parser.addOption({LogLevelOption,
                      QStringLiteral("Least severe messages written: %1.").arg(MessageLog::levelNames().join(u", ")),
                      QStringLiteral("level")});
    parser.addOption({LogFileOption, QStringLiteral("File the messages are added to."), QStringLiteral("file")});
    parser.addOption({ConsoleOption, QStringLiteral("Writes the messages to the console: the one of the terminal the "
                                                    "application is started from, when it has none of its own.")});
}

} // namespace

NebulaApplication::NebulaApplication(int& argc, char** argv, const QString& galaxy, const ApplicationOptions& options)
    : QApplication(argc, argv)
{
    setExecutableInfo(options.executable);
    load(galaxy, options, nullptr);
}

NebulaApplication::NebulaApplication(int& argc, char** argv, QCommandLineParser& parser, const QString& galaxy,
                                     ApplicationOptions options)
    : QApplication(argc, argv)
{
    setExecutableInfo(options.executable); // Before the command line is processed: --version tells it
    processCommandLine(parser);
    readCommandLine(parser, options);
    load(galaxy, options, &parser);
}

NebulaApplication::~NebulaApplication() = default;

void NebulaApplication::addCommandLineOptions(QCommandLineParser& parser)
{
    parser.addHelpOption();
    parser.addVersionOption();
    Universe::addCommandLineOptions(parser);
    parser.addOption({GalaxyDirOption,
                      QStringLiteral("Directory to look for the galaxy in, before those of the application. Can be "
                                     "given several times: the first directory that has it is the one."),
                      QStringLiteral("directory")});
    addLogOptions(parser);
}

Universe& NebulaApplication::universe()
{
    return *m_universe;
}

const Universe& NebulaApplication::universe() const
{
    return *m_universe;
}

QString NebulaApplication::galaxyFile() const
{
    return m_galaxyFile;
}

bool NebulaApplication::isLoaded() const
{
    return m_loaded;
}

QString NebulaApplication::errorString() const
{
    return m_errorString;
}

int NebulaApplication::exec()
{
    if(!m_loaded || !startGalaxy())
    {
        return m_loaded ? StartFailed : LoadFailed;
    }
    const int code = QApplication::exec();
    m_universe->stop(); // While the application is whole: its settings files are written
    return code;
}

void NebulaApplication::setExecutableInfo(const ExecutableInfo& info)
{
    setUnlessEmpty(info.name, &QCoreApplication::setApplicationName);
    setUnlessEmpty(info.version, &QCoreApplication::setApplicationVersion);
    setUnlessEmpty(info.organization, &QCoreApplication::setOrganizationName);
    setUnlessEmpty(info.organizationDomain, &QCoreApplication::setOrganizationDomain);
}

void NebulaApplication::processCommandLine(QCommandLineParser& parser)
{
    addCommandLineOptions(parser);
    parser.process(*this);
}

void NebulaApplication::readCommandLine(const QCommandLineParser& parser, ApplicationOptions& options)
{
    // First, and from the current directory, as a shell gives them.
    options.galaxyDirectories =
        absolutePaths(QDir::current(), parser.values(GalaxyDirOption)) + options.galaxyDirectories;
    readLogOptions(parser, options);
}

void NebulaApplication::readLogOptions(const QCommandLineParser& parser, ApplicationOptions& options)
{
    if(parser.isSet(LogLevelOption))
    {
        readLogLevel(parser.value(LogLevelOption), options);
    }
    if(parser.isSet(LogFileOption))
    {
        options.logFile = absolutePath(QDir::current(), parser.value(LogFileOption));
    }
    options.console = options.console || parser.isSet(ConsoleOption);
}

void NebulaApplication::readLogLevel(const QString& name, ApplicationOptions& options)
{
    const std::optional<QtMsgType> level = MessageLog::levelNamed(name);
    if(!level)
    {
        refuse(QStringLiteral("Unknown log level %1: %2").arg(name, MessageLog::levelNames().join(u", ")));
        return;
    }
    options.logLevel = *level;
}

void NebulaApplication::load(const QString& galaxy, const ApplicationOptions& options,
                             const QCommandLineParser* commandLine)
{
    startLog(options);
    m_universe = std::make_unique<Universe>(); // Once the messages are written as asked: it tells the plugins it skips
    if(m_errorString.isEmpty())
    {
        addPlugins(options, commandLine);
        m_loaded = openGalaxy(galaxy, options.galaxyDirectories);
    }
}

void NebulaApplication::startLog(ApplicationOptions options)
{
    if(!options.logFile.isEmpty())
    {
        options.logFile = absolutePath(executableDirectory(), options.logFile);
    }
    QString error;
    m_log = std::make_unique<MessageLog>(options, error);
    if(!error.isEmpty())
    {
        refuse(error);
    }
}

void NebulaApplication::addPlugins(const ApplicationOptions& options, const QCommandLineParser* commandLine)
{
    for(const QString& directory : absolutePaths(executableDirectory(), options.pluginDirectories))
    {
        m_universe->scanPluginDirectory(directory);
    }
    m_universe->setPluginRoot(absolutePath(executableDirectory(), options.pluginRoot)); // Its directory when empty
    if(commandLine != nullptr)
    {
        m_universe->applyCommandLine(*commandLine); // After those of the code: --plugin-root replaces theirs
    }
}

bool NebulaApplication::openGalaxy(const QString& galaxy, const QStringList& directories)
{
    const QStringList searched = absolutePaths(executableDirectory(), directories);
    m_galaxyFile = findGalaxy(galaxy, searched);
    if(m_galaxyFile.isEmpty())
    {
        return refuse(notFound(galaxy, searched));
    }
    return m_universe->open(m_galaxyFile) || refuse(cannotLoad(m_galaxyFile, m_universe->errorString()));
}

bool NebulaApplication::startGalaxy()
{
    if(m_universe->start())
    {
        return true;
    }
    return refuse(QStringLiteral("The galaxy %1 refused to start: %2")
                      .arg(QDir::toNativeSeparators(m_galaxyFile), m_universe->errorString()));
}

bool NebulaApplication::refuse(const QString& why)
{
    m_errorString = why;
    qCCritical(lcApplication).noquote() << why;
    return false;
}

} // namespace Nebula
