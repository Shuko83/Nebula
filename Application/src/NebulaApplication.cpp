// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Nebula/Application/NebulaApplication.h"

#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QMetaProperty>
#include <algorithm>
#include <iterator>
#include <optional>
#include <ranges>

#include "MessageLog.h"
#include "Nebula/Component/Component.h"

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

// Gives the windows of the application the icon of the file filePath, read from the directory of the executable when
// it is relative.
void setIconFile(const QString& filePath)
{
    QApplication::setWindowIcon(QIcon(absolutePath(executableDirectory(), filePath)));
}

// Adds to parser what options tell of the command line: what the application is for, when they tell it, and its
// arguments, each one optional.
void describe(QCommandLineParser& parser, const ApplicationOptions& options)
{
    if(!options.executable.description.isEmpty())
    {
        parser.setApplicationDescription(options.executable.description);
    }
    for(const CommandLineArgument& argument : options.arguments)
    {
        parser.addPositionalArgument(argument.name, argument.description, QStringLiteral("[%1]").arg(argument.name));
    }
}

// Writes value to the property of instance named name, converted to its type; false when it has none, or one that
// does not take it.
bool writeProperty(Component& instance, const QString& name, const QString& value)
{
    const QMetaObject* metaObject = instance.metaObject();
    const int index = metaObject->indexOfProperty(name.toUtf8().constData());
    return index >= 0 && metaObject->property(index).write(&instance, value);
}

// Why argument cannot be given to the property of the instance it tells: why.
QString cannotGive(const CommandLineArgument& argument, const QString& why)
{
    return QStringLiteral("The argument %1 cannot be given to %2.%3: %4")
        .arg(argument.name, argument.instance, argument.property, why);
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
    QCommandLineParser parser; // All that is read from it is read here
    build(galaxy, options, options.commandLine ? &parser : nullptr);
}

NebulaApplication::NebulaApplication(int& argc, char** argv, QCommandLineParser& parser, const QString& galaxy,
                                     const ApplicationOptions& options)
    : QApplication(argc, argv)
{
    build(galaxy, options, &parser);
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
        return tellFailure(m_loaded ? StartFailed : LoadFailed);
    }
    const int code = QApplication::exec();
    m_universe->stop(); // While the application is whole: its settings files are written
    return code;
}

void NebulaApplication::build(const QString& galaxy, ApplicationOptions options, QCommandLineParser* commandLine)
{
    setExecutableInfo(options.executable); // Before the command line is processed: --version tells it
    if(commandLine != nullptr)
    {
        processCommandLine(*commandLine, options);
        readCommandLine(*commandLine, options);
    }
    m_failureBox = options.failureBox;
    load(galaxy, options, commandLine);
}

void NebulaApplication::setExecutableInfo(const ExecutableInfo& info)
{
    setUnlessEmpty(info.name, &QCoreApplication::setApplicationName);
    setUnlessEmpty(info.version, &QCoreApplication::setApplicationVersion);
    setUnlessEmpty(info.organization, &QCoreApplication::setOrganizationName);
    setUnlessEmpty(info.organizationDomain, &QCoreApplication::setOrganizationDomain);
    setUnlessEmpty(info.icon, &setIconFile);
}

void NebulaApplication::processCommandLine(QCommandLineParser& parser, const ApplicationOptions& options)
{
    describe(parser, options);
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
        m_loaded = openGalaxy(galaxy, options.galaxyDirectories) && giveArguments(options.arguments, commandLine);
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

bool NebulaApplication::giveArguments(const QList<CommandLineArgument>& arguments,
                                      const QCommandLineParser* commandLine)
{
    const QStringList values = commandLine != nullptr ? commandLine->positionalArguments() : QStringList();
    const qsizetype given = std::min(values.size(), arguments.size());
    return std::ranges::all_of(std::views::iota(qsizetype{0}, given), [this, &arguments, &values](qsizetype index)
                               { return giveArgument(arguments.at(index), values.at(index)); });
}

bool NebulaApplication::giveArgument(const CommandLineArgument& argument, const QString& value)
{
    Component* instance = m_universe->component(argument.instance);
    if(instance == nullptr)
    {
        return refuse(cannotGive(argument, QStringLiteral("the galaxy has no instance %1").arg(argument.instance)));
    }
    const QString given = argument.path ? absolutePath(QDir::current(), value) : value;
    return writeProperty(*instance, argument.property, given) ||
           refuse(cannotGive(argument, QStringLiteral("this property does not exist, or does not take %1").arg(given)));
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

int NebulaApplication::tellFailure(int code)
{
    emit failed(m_errorString);
    if(m_failureBox)
    {
        QMessageBox::critical(nullptr, applicationDisplayName(), m_errorString);
    }
    return code;
}

bool NebulaApplication::refuse(const QString& why)
{
    m_errorString = why;
    qCCritical(lcApplication).noquote() << why;
    return false;
}

} // namespace Nebula
