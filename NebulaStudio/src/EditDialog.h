// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QDialog>

class DialogHeader;
class QDialogButtonBox;
class QIcon;

// A dialog that edits something, a settings file or a stargate, and gives it what it was given once it is accepted,
// by OK, which is disabled while it cannot be. A header across its top tells what it edits and what that is for. It is
// rejected should what it edits go meanwhile, with what it would have been given.
class EditDialog : public QDialog
{
    Q_OBJECT

  public:
    // edited outlives the dialog, unless it goes, which rejects it. icon is the one of the dialog and of its header.
    EditDialog(QObject& edited, const QIcon& icon, QWidget* parent);

  protected:
    // Its title and its text tell what it edits and what that is for.
    [[nodiscard]] DialogHeader& header() const;
    // OK and Cancel, laid out last.
    [[nodiscard]] QDialogButtonBox* buttons() const;
    // Lays out the header across its top, then content, which keeps its margins.
    void layOut(QWidget* content);
    // Whether it can be accepted: OK is enabled then.
    void setAcceptable(bool acceptable);
    // Gives what it edits what it was given; once it is accepted.
    virtual void apply() = 0;

  private:
    DialogHeader* m_header = nullptr;
    QDialogButtonBox* m_buttons = nullptr;
};
