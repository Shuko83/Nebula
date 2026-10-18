// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QString>
#include <QWidget>

class GalaxyModel;
class QLabel;
class QPushButton;
class QStackedWidget;

// Shows a galaxy of a GalaxyModel: its name, its description, its file, its plugins and its state, with a button to
// open it. When it cannot be loaded, an error tells why, in the warning color: the plugins it misses, and how to make
// them known, or why its file cannot be read. It follows the model.
class GalaxyViewer : public QWidget
{
    Q_OBJECT

  public:
    explicit GalaxyViewer(const GalaxyModel& model, QWidget* parent = nullptr);

    // Shows the galaxy of the file filePath; nothing when it is empty or not listed.
    void setFilePath(const QString& filePath);
    // Whether a galaxy that can be loaded can be opened now: it can until told otherwise. Its Open button tells why
    // not, in its tooltip.
    void setOpenAllowed(bool allowed);

  signals:
    // The Open button was clicked: GalaxyViewer does not open the galaxy itself.
    void openRequested(const QString& filePath);

  protected:
    // The error takes the warning color of the scheme the system has now.
    void changeEvent(QEvent* event) override;

  private:
    // Reads the galaxy shown from the model again.
    void refresh();
    // Gives the error the warning color of the current scheme: the palette has no role for it.
    void colorError();

    const GalaxyModel& m_model;
    QString m_filePath;
    bool m_openAllowed = true;

    QStackedWidget* m_stack = nullptr;
    QLabel* m_placeholder = nullptr; // Shown when there is no galaxy
    QWidget* m_page = nullptr;       // Shown otherwise
    QLabel* m_name = nullptr;
    QLabel* m_description = nullptr;
    QLabel* m_file = nullptr;
    QLabel* m_plugins = nullptr;
    QLabel* m_state = nullptr;
    QWidget* m_error = nullptr; // Shown when it cannot be loaded
    QLabel* m_errorText = nullptr;
    QPushButton* m_openButton = nullptr;
};
