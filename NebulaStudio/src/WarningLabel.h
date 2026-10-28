// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QString>
#include <QWidget>

class QLabel;

// The warning icon of the style beside a text in the warning color of the theme, which follows the scheme of the
// system: what keeps something from being done. Hidden while its text is empty.
class WarningLabel : public QWidget
{
    Q_OBJECT

  public:
    explicit WarningLabel(QWidget* parent = nullptr);

    [[nodiscard]] QString text() const;
    // Shows text; hides it when text is empty.
    void setText(const QString& text);

  protected:
    // The text takes the warning color of the scheme the system has now.
    void changeEvent(QEvent* event) override;

  private:
    // Gives the text the warning color of the current scheme: the palette has no role for it.
    void colorText();

    QLabel* m_text = nullptr; // Can be selected and wraps
};
