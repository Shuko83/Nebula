// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QDialog>
#include <QString>

class QDialogButtonBox;
class QLabel;

// A dialog that edits something, a settings file or a stargate, and gives it what it was given once it is accepted,
// by OK, which is disabled while it cannot be. It is rejected should what it edits go meanwhile, with what it would
// have been given.
class EditDialog : public QDialog
{
    Q_OBJECT

  public:
    // edited outlives the dialog, unless it goes, which rejects it.
    EditDialog(QObject& edited, QWidget* parent);

  protected:
    // A hint that wraps, telling text.
    [[nodiscard]] static QLabel* newHint(const QString& text);
    // OK and Cancel, laid out last.
    [[nodiscard]] QDialogButtonBox* buttons() const;
    // Whether it can be accepted: OK is enabled then.
    void setAcceptable(bool acceptable);
    // Gives what it edits what it was given; once it is accepted.
    virtual void apply() = 0;

  private:
    QDialogButtonBox* m_buttons = nullptr;
};
