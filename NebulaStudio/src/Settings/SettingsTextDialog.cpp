// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "SettingsTextDialog.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QPlainTextEdit>
#include <QSaveFile>
#include <QVBoxLayout>

#include "DialogHeader.h"
#include "FileFollower.h"
#include "Nebula/Settings/SettingsFile.h"
#include "Theme.h"
#include "WarningLabel.h"

namespace
{

// Where the text is typed, in the fixed font of the system, its lines as long as they are.
QPlainTextEdit* newTextEdit()
{
    auto* edit = new QPlainTextEdit;
    edit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    edit->setLineWrapMode(QPlainTextEdit::NoWrap);
    edit->setPlaceholderText(SettingsTextDialog::tr("The file does not exist yet: the application writes it with the "
                                                    "default values when it starts"));
    return edit;
}

// The text of the file filePath; empty when it does not exist, or cannot be read.
QString readText(const QString& filePath)
{
    QFile file(filePath);
    return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
}

// Writes text to the file filePath, whole or not at all, creating its directory when needed; false, with why set, when
// it cannot.
bool writeText(const QString& filePath, const QString& text, QString& why)
{
    QSaveFile file(filePath);
    const bool written = QDir().mkpath(QFileInfo(filePath).absolutePath()) && file.open(QIODevice::WriteOnly) &&
                         file.write(text.toUtf8()) >= 0 && file.commit();
    why = written ? QString() : file.errorString();
    return written;
}

// Why json is not JSON, as error tells it, with the line where it stops being.
QString notJson(const QByteArray& json, const QJsonParseError& error)
{
    return SettingsTextDialog::tr("Not JSON, line %1: %2")
        .arg(json.left(error.offset).count('\n') + 1)
        .arg(error.errorString());
}

// Why text is not what a settings file holds, a JSON object; empty when it is.
QString jsonRefusal(const QString& text)
{
    const QByteArray json = text.toUtf8();
    QJsonParseError error{};
    const QJsonDocument document = QJsonDocument::fromJson(json, &error);
    if(error.error != QJsonParseError::NoError && !text.trimmed().isEmpty()) // Nothing typed is not an error
    {
        return notJson(json, error);
    }
    return document.isObject() ? QString()
                               : SettingsTextDialog::tr("A settings file holds a JSON object, between braces");
}

} // namespace

SettingsTextDialog::SettingsTextDialog(Nebula::SettingsFile& file, QWidget* parent)
    : EditDialog(file, icon(GlossaryTerm::Comet), parent), m_file(file), m_text(newTextEdit()),
      m_problem(new WarningLabel), m_follower(new FileFollower(this))
{
    setWindowTitle(tr("Settings file"));
    tellAbout();
    layOut(newContent());
    showText(readText(file.filePath()));
    connect(m_text, &QPlainTextEdit::textChanged, this, &SettingsTextDialog::check);
    connect(m_follower, &FileFollower::changed, this, &SettingsTextDialog::readAgain);
    m_follower->follow(file.filePath());
    resize(640, 560);
}

void SettingsTextDialog::tellAbout()
{
    header().setTitle(m_file.fileName());
    header().setText(tr("The values the user of the application keeps from one run to the next, under the names of "
                        "the instances, written as they are typed: %1")
                         .arg(QDir::toNativeSeparators(m_file.filePath())));
}

QWidget* SettingsTextDialog::newContent()
{
    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->addWidget(m_text, 1);
    layout->addWidget(m_problem);
    layout->addWidget(buttons());
    return content;
}

void SettingsTextDialog::readAgain()
{
    const QString text = readText(m_file.filePath());
    if(text != m_shown && m_text->document()->isModified())
    {
        m_outdated = true; // What was typed is kept
        check();
    }
    else if(text != m_shown) // Unchanged when another file of its directory changed
    {
        showText(text);
    }
}

void SettingsTextDialog::showText(const QString& text)
{
    m_shown = text;
    m_outdated = false;
    m_text->setPlainText(text);
    m_text->document()->setModified(false);
    check();
}

void SettingsTextDialog::check()
{
    const QString refusal = jsonRefusal(m_text->toPlainText());
    const QString outdated =
        m_outdated ? tr("The file changed since it was shown: OK writes what is typed over it") : QString();
    m_problem->setText(refusal.isEmpty() ? outdated : refusal);
    setAcceptable(refusal.isEmpty());
}

bool SettingsTextDialog::apply()
{
    QString why;
    if(writeText(m_file.filePath(), m_text->toPlainText(), why))
    {
        return true;
    }
    m_problem->setText(tr("The file cannot be written: %1").arg(why));
    return false;
}
