// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <memory>

#include "DialogHeader.h"
#include "Nebula/Application/Universe.h"
#include "Nebula/Settings/SettingsFile.h"
#include "Settings/SettingsTextDialog.h"
#include "TestComponents.h"
#include "WarningLabel.h"

namespace
{

// What the file filePath holds; null when it cannot be read.
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

} // namespace

// SettingsTextDialog: the text of a settings file, shown, checked, written as it is typed, and following the file.
class TestSettingsTextDialog : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void cleanup();

    void showsTheFile();
    void noFileYet();
    void check_data();
    void check();
    void apply();
    void applyCreatesTheDirectory();
    void applyFails();
    void followsTheFile();
    void keepsWhatIsTyped();

  private:
    // Opens m_dialog on m_file.
    void open();
    // Replaces the text of the dialog with text, as if it were typed.
    void type(const QString& text) const;
    [[nodiscard]] QPlainTextEdit& text() const;
    // What the warning of the dialog tells; empty when it is hidden.
    [[nodiscard]] QString warning() const;
    [[nodiscard]] bool isAcceptable() const;

    // An application with a settings file in m_directory, whose text m_dialog shows once it is opened.
    std::unique_ptr<QTemporaryDir> m_directory;
    std::unique_ptr<Nebula::Universe> m_application;
    Nebula::SettingsFile* m_file = nullptr;
    std::unique_ptr<SettingsTextDialog> m_dialog;
};

void TestSettingsTextDialog::initTestCase()
{
    registerTestComponents();
    // Once, before any test fails on a warning: the offscreen platform warns that it has no fonts the first time a text
    // is laid out.
    QFontDatabase::families();
}

void TestSettingsTextDialog::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
    m_directory = std::make_unique<QTemporaryDir>();
    QVERIFY(m_directory->isValid());
    m_application = std::make_unique<Nebula::Universe>();
    m_file = m_application->addSettingsFile();
    m_file->setDirectory(m_directory->path());
}

void TestSettingsTextDialog::cleanup()
{
    m_dialog.reset(); // Before its directory, which it watches
    m_application.reset();
    m_directory.reset();
}

void TestSettingsTextDialog::open()
{
    m_dialog = std::make_unique<SettingsTextDialog>(*m_file);
}

void TestSettingsTextDialog::type(const QString& typed) const
{
    text().selectAll();
    text().insertPlainText(typed);
}

QPlainTextEdit& TestSettingsTextDialog::text() const
{
    return *m_dialog->findChild<QPlainTextEdit*>();
}

QString TestSettingsTextDialog::warning() const
{
    const auto* label = m_dialog->findChild<WarningLabel*>();
    return label->isVisibleTo(m_dialog.get()) ? label->text() : QString();
}

bool TestSettingsTextDialog::isAcceptable() const
{
    return m_dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->isEnabled();
}

// The text of the file, as it is, under its name and where it is.
void TestSettingsTextDialog::showsTheFile()
{
    const QByteArray content("{\n  \"Probe1\": {\"properties\": {\"count\": 7}}\n}\n");
    QVERIFY(writeFile(m_file->filePath(), content));
    open();

    QCOMPARE(text().toPlainText(), QString::fromUtf8(content));
    QVERIFY(!text().document()->isModified());
    const auto* header = m_dialog->findChild<DialogHeader*>();
    QCOMPARE(header->title(), QStringLiteral("settings.json"));
    QVERIFY(header->text().endsWith(QDir::toNativeSeparators(m_file->filePath())));
    QCOMPARE(warning(), QString());
    QVERIFY(isAcceptable());
}

// Without a file, there is no text to write yet.
void TestSettingsTextDialog::noFileYet()
{
    open();

    QCOMPARE(text().toPlainText(), QString());
    QVERIFY(text().placeholderText().startsWith(QStringLiteral("The file does not exist yet")));
    QCOMPARE(warning(), QStringLiteral("A settings file holds a JSON object, between braces"));
    QVERIFY(!isAcceptable());
}

void TestSettingsTextDialog::check_data()
{
    QTest::addColumn<QString>("typed");
    QTest::addColumn<QString>("warning"); // How it starts

    QTest::newRow("object") << R"({"Probe1": {"properties": {"count": 7}}})" << QString();
    QTest::newRow("empty object") << "{}" << QString();
    QTest::newRow("array") << "[1, 2]" << "A settings file holds a JSON object, between braces";
    QTest::newRow("blank") << "  \n" << "A settings file holds a JSON object, between braces";
    QTest::newRow("not JSON") << "{ count" << "Not JSON, line 1: ";
    QTest::newRow("line") << "{\n  \"count\": 7,\n  \"text\": \n}" << "Not JSON, line 4: ";
}

// OK is enabled while the text is a JSON object; the warning tells why it is not, and where.
void TestSettingsTextDialog::check()
{
    QFETCH(QString, typed);
    QFETCH(QString, warning);

    QVERIFY(writeFile(m_file->filePath(), "{}"));
    open();
    type(typed);

    QVERIFY2(warning.isEmpty() ? this->warning().isEmpty() : this->warning().startsWith(warning),
             qPrintable(this->warning()));
    QCOMPARE(isAcceptable(), warning.isEmpty());
}

// Accepted, the text is written to the file as it was typed.
void TestSettingsTextDialog::apply()
{
    QVERIFY(writeFile(m_file->filePath(), "{}"));
    open();
    // Neither indented nor sorted, and in UTF-8.
    const QString typed(QStringLiteral("{\"Probe1\":{\"properties\":{\"text\":\"été\", \"count\":2}}}"));
    type(typed);
    const QSignalSpy accepted(m_dialog.get(), &QDialog::accepted);

    m_dialog->accept();

    QCOMPARE(accepted.count(), 1);
    QCOMPARE(readFile(m_file->filePath()), typed.toUtf8());
}

// The directory of the file is created when it does not exist yet.
void TestSettingsTextDialog::applyCreatesTheDirectory()
{
    m_file->setDirectory(m_directory->filePath(QStringLiteral("deeper/still")));
    open();
    type(QStringLiteral("{}"));

    m_dialog->accept();

    QCOMPARE(readFile(m_file->filePath()), QByteArray("{}"));
}

// When the file cannot be written, the dialog stays open, and tells why.
void TestSettingsTextDialog::applyFails()
{
    // Its directory is a file.
    QVERIFY(writeFile(m_directory->filePath(QStringLiteral("blocked")), "not a directory"));
    m_file->setDirectory(m_directory->filePath(QStringLiteral("blocked")));
    open();
    type(QStringLiteral("{}"));
    const QSignalSpy accepted(m_dialog.get(), &QDialog::accepted);

    m_dialog->accept();

    QCOMPARE(accepted.count(), 0);
    QVERIFY2(warning().startsWith(QStringLiteral("The file cannot be written: ")), qPrintable(warning()));
    QCOMPARE(text().toPlainText(), QStringLiteral("{}"));
}

// What another program writes to the file is shown, from its creation on.
void TestSettingsTextDialog::followsTheFile()
{
    open();

    QVERIFY(writeFile(m_file->filePath(), R"({"Probe1": {}})"));
    QTRY_COMPARE(text().toPlainText(), QStringLiteral(R"({"Probe1": {}})"));
    QVERIFY(isAcceptable());
    QVERIFY(writeFile(m_file->filePath(), R"({"Probe1": {"properties": {}}})"));
    QTRY_COMPARE(text().toPlainText(), QStringLiteral(R"({"Probe1": {"properties": {}}})"));
    QVERIFY(!text().document()->isModified());
}

// What was typed is not replaced by what another program writes: the warning tells that OK writes over it.
void TestSettingsTextDialog::keepsWhatIsTyped()
{
    QVERIFY(writeFile(m_file->filePath(), "{}"));
    open();
    type(QStringLiteral(R"({"typed": 1})"));

    QVERIFY(writeFile(m_file->filePath(), R"({"written": 2})"));
    QTRY_COMPARE(warning(), QStringLiteral("The file changed since it was shown: OK writes what is typed over it"));
    QCOMPARE(text().toPlainText(), QStringLiteral(R"({"typed": 1})"));
    QVERIFY(isAcceptable());

    m_dialog->accept();
    QCOMPARE(readFile(m_file->filePath()), QByteArray(R"({"typed": 1})"));
}

QTEST_MAIN(TestSettingsTextDialog)
#include "TestSettingsTextDialog.moc"
