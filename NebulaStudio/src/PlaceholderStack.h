// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QStackedWidget>
#include <QString>

class QLabel;

// A page, or in its place a placeholder that tells why there is nothing to show on it: the sheets of Inspector, the
// list of ProblemList and the table of PropertyEditor. It fills its owner.
class PlaceholderStack : public QStackedWidget
{
    Q_OBJECT

  public:
    // Lays itself over the whole of owner, showing the placeholder, empty, until it is told otherwise.
    PlaceholderStack(QWidget* page, QWidget& owner);

    // The placeholder, centered and wrapping its text.
    [[nodiscard]] QLabel* placeholder() const;
    void showPage();
    // Shows the placeholder, telling text, in place of the page.
    void showPlaceholder(const QString& text);
    // Shows the page when it has something to show, the placeholder otherwise.
    void showPageIf(bool hasSomething);

  private:
    QWidget* m_page = nullptr;
    QLabel* m_placeholder = nullptr;
};
