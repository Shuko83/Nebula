// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>
#include <atomic>
#include <memory>
#include <thread>

#include "Nebula/Application/WholeFile.h"

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace
{

// What the file filePath holds; empty when it cannot be read.
QByteArray readFile(const QString& filePath)
{
    QFile file(filePath);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

// Writes content to the file filePath, as another program would.
bool writeFile(const QString& filePath, const QByteArray& content)
{
    QFile file(filePath);
    return file.open(QIODevice::WriteOnly) && file.write(content) == content.size();
}

#ifdef Q_OS_WIN
// The file filePath held open, as another program can hold it, without sharing its deletion: Windows does not replace
// it, as long as it lives.
class Held
{
  public:
    explicit Held(const QString& filePath)
        : m_handle(CreateFileW(reinterpret_cast<const wchar_t*>(QDir::toNativeSeparators(filePath).utf16()),
                               GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL, nullptr))
    {
    }
    ~Held()
    {
        if(isHeld())
        {
            CloseHandle(m_handle);
        }
    }
    Held(const Held&) = delete;
    Held& operator=(const Held&) = delete;

    [[nodiscard]] bool isHeld() const
    {
        return m_handle != INVALID_HANDLE_VALUE;
    }

  private:
    HANDLE m_handle;
};
#endif

} // namespace

// Nebula::WholeFile::write(): a file written whole or not at all, again while Windows refuses to replace it.
class TestWholeFile : public QObject
{
    Q_OBJECT

  private slots:
    void init();
    void cleanup();

    void write_data();
    void write();
    void directoryCannotBeCreated();
    void whileLookedAt();
    void refusedForAMoment();
    void refusedTooLong();

  private:
    // A directory of its own for each test, and the file filePath in it.
    std::unique_ptr<QTemporaryDir> m_directory;
    QString m_filePath;
};

void TestWholeFile::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
    m_directory = std::make_unique<QTemporaryDir>();
    QVERIFY(m_directory->isValid());
    m_filePath = m_directory->filePath(QStringLiteral("settings.json"));
}

void TestWholeFile::cleanup()
{
    m_directory.reset();
}

void TestWholeFile::write_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QByteArray>("existing");

    QTest::newRow("new") << "settings.json" << QByteArray();
    QTest::newRow("new, in a directory to create") << "config/user/settings.json" << QByteArray();
    QTest::newRow("replaced") << "settings.json" << QByteArray("{\"older\": true}");
}

// The file holds what is written, its directory created when needed, what it held before replaced.
void TestWholeFile::write()
{
    QFETCH(QString, fileName);
    QFETCH(QByteArray, existing);
    const QString filePath = m_directory->filePath(fileName);
    if(!existing.isEmpty())
    {
        QVERIFY(writeFile(filePath, existing));
    }

    QString error;
    QVERIFY(Nebula::WholeFile::write(filePath, "{\"count\": 3}", error));

    QCOMPARE(readFile(filePath), QByteArray("{\"count\": 3}"));
    QCOMPARE(error, QString());
}

// A directory that cannot be created, a file being there, is told.
void TestWholeFile::directoryCannotBeCreated()
{
    QVERIFY(writeFile(m_directory->filePath(QStringLiteral("config")), "not a directory"));
    const QString filePath = m_directory->filePath(QStringLiteral("config/settings.json"));

    QString error;
    QVERIFY(!Nebula::WholeFile::write(filePath, "{}", error));

    QCOMPARE(error, QStringLiteral("Its directory cannot be created"));
    QVERIFY(!QFile::exists(filePath));
}

// While another thread looks at the file, as the thread of a QFileSystemWatcher does on each change, each writing is
// done: on Windows, QFileInfo holds the file open for a moment, which some of the replacements run into (#152).
void TestWholeFile::whileLookedAt()
{
    constexpr int Writings = 300;
    QVERIFY(writeFile(m_filePath, "{}"));
    std::atomic_bool done = false;
    std::thread looking(
        [&]
        {
            while(!done)
            {
                (void)QFileInfo(m_filePath).exists();
            }
        });

    int failed = 0;
    QString error;
    for(int writing = 0; writing < Writings; ++writing)
    {
        failed += Nebula::WholeFile::write(m_filePath, QByteArray::number(writing), error) ? 0 : 1;
    }
    done = true;
    looking.join();

    QVERIFY2(failed == 0, qPrintable(QStringLiteral("%1 of %2 not written: %3").arg(failed).arg(Writings).arg(error)));
    QCOMPARE(readFile(m_filePath), QByteArray::number(Writings - 1));
}

// A file another program holds open for less than the attempts last is written once it lets it go.
void TestWholeFile::refusedForAMoment()
{
#ifdef Q_OS_WIN
    QVERIFY(writeFile(m_filePath, "{}"));
    std::atomic_bool held = false;
    std::thread holding(
        [&]
        {
            const Held file(m_filePath);
            held = file.isHeld();
            std::this_thread::sleep_for(Nebula::WholeFile::Pause * 3);
        });
    while(!held)
    {
        std::this_thread::yield();
    }

    QString error;
    const bool written = Nebula::WholeFile::write(m_filePath, "{\"count\": 3}", error);
    holding.join();

    QVERIFY2(written, qPrintable(error));
    QCOMPARE(readFile(m_filePath), QByteArray("{\"count\": 3}"));
#else
    QSKIP("Only Windows refuses to replace a file another program holds open");
#endif
}

// A file another program holds open for longer than the attempts last is not written, which is told, and keeps what
// it holds; the temporary file of the writing is gone.
void TestWholeFile::refusedTooLong()
{
#ifdef Q_OS_WIN
    QVERIFY(writeFile(m_filePath, "{}"));
    const Held file(m_filePath);
    QVERIFY(file.isHeld());

    QString error;
    QVERIFY(!Nebula::WholeFile::write(m_filePath, "{\"count\": 3}", error));

    QVERIFY(!error.isEmpty());
    QCOMPARE(readFile(m_filePath), QByteArray("{}"));
    QCOMPARE(QDir(m_directory->path()).entryList(QDir::Files), QStringList({"settings.json"}));
#else
    QSKIP("Only Windows refuses to replace a file another program holds open");
#endif
}

QTEST_APPLESS_MAIN(TestWholeFile)
#include "TestWholeFile.moc"
