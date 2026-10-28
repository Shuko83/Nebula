// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QString>
#include <QWidget>
#include <functional>
#include <initializer_list>

class QFormLayout;
class QLabel;
class QPushButton;
class PlaceholderStack;
class QVBoxLayout;

// A sheet that shows what is selected on a page, a form with what is below it, or a placeholder that tells why there
// is nothing to show: PluginViewer, ComponentViewer and GalaxyViewer. A subclass builds its page once, then shows it
// or the placeholder.
class Inspector : public QWidget
{
    Q_OBJECT

  public:
    explicit Inspector(QWidget* parent = nullptr);

  protected:
    // A label whose text can be selected and wraps, as the values of the form show them.
    [[nodiscard]] static QLabel* newValueLabel();
    // A row of the form, named name, with a value label, which it returns.
    QLabel* addField(const QString& name);
    // A row of the form, named name, showing field.
    void addRow(const QString& name, QWidget* field);
    // Adds widget below what the page has.
    void addBelow(QWidget* widget);
    // Adds below what the page has a row of buttons, on the left.
    void addButtons(std::initializer_list<QPushButton*> buttons);
    // Ends the page: what it has stays at its top.
    void addStretch();
    // A button showing text, which calls clicked.
    QPushButton* newButton(const QString& text, const std::function<void()>& clicked);

    void showPage();
    // Shows the placeholder, telling text, in place of the page.
    void showPlaceholder(const QString& text);

  private:
    QWidget* m_page = nullptr; // Shown when there is something to show
    QVBoxLayout* m_pageLayout = nullptr;
    QFormLayout* m_form = nullptr;       // At the top of the page
    PlaceholderStack* m_stack = nullptr; // Of the page and of its placeholder
};
