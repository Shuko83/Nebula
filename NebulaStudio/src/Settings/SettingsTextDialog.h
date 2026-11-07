// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QString>

#include "EditDialog.h"

class FileFollower;
class QPlainTextEdit;
class WarningLabel;

namespace Nebula
{
class SettingsFile;
} // namespace Nebula

// The text of a settings file, where the user of the application keeps the values they give, shown and edited as it
// is: once the dialog is accepted, the text is written to the file as it was typed, its directory created when needed.
// OK is disabled while the text is not a JSON object, the content of a settings file, which a warning tells, with the
// line where it stops being JSON; it stays open, telling why, when the file cannot be written. It follows the file:
// what another program writes there, as the application does while it runs, is shown at once, unless something was
// typed meanwhile, in which case the warning tells that OK writes over it.
class SettingsTextDialog : public EditDialog
{
    Q_OBJECT

  public:
    // file, one of those of the application or of its galaxies, outlives the dialog, unless it is removed, which
    // rejects it.
    explicit SettingsTextDialog(Nebula::SettingsFile& file, QWidget* parent = nullptr);

  private:
    // Its header: the name of the file, what it holds, and where it is.
    void tellAbout();
    // Under the header: the text, what keeps it from being written, and the buttons.
    [[nodiscard]] QWidget* newContent();
    // Shows what the file holds now, unless something was typed since it was last shown: tells then that it changed.
    void readAgain();
    // Shows text, what the file holds now, as nothing typed yet.
    void showText(const QString& text);
    // Enables OK while the text is a JSON object; tells why it is not, or that the file changed since it was shown.
    void check();
    // Writes the text to the file; tells why, and returns false, when it cannot be.
    bool apply() override;

    Nebula::SettingsFile& m_file;
    QPlainTextEdit* m_text = nullptr;
    WarningLabel* m_problem = nullptr;  // Why OK is disabled, or that the file changed
    FileFollower* m_follower = nullptr; // Of the file
    QString m_shown;                    // What the file held when it was last shown
    bool m_outdated = false;            // The file changed since, while something was typed
};
