// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QIcon>
#include <QString>
#include <QWidget>

class QLabel;

// The band across the top of a dialog: the icon of what it edits, its title, in the font of the titles and larger, and
// under it a muted text that tells what it is for, which wraps. Painted in the colors of the palette, on the alternate
// color of the fields, with an edge under it, it follows the scheme of the system.
class DialogHeader : public QWidget
{
    Q_OBJECT

  public:
    explicit DialogHeader(const QIcon& icon, QWidget* parent = nullptr);

    [[nodiscard]] QString title() const;
    // Shown as it is: a name typed is not read as rich text.
    void setTitle(const QString& title);
    [[nodiscard]] QString text() const;
    void setText(const QString& text);

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    QLabel* m_title = nullptr;
    QLabel* m_text = nullptr;
};
