// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QString>
#include <QWidget>

class GalaxyModel;
class GalaxyViewer;
class QLineEdit;
class QTreeView;
class Studio;

// The galaxies of a directory, scene/ next to the application until another one is given or chosen, each telling
// whether the application of a Studio knows its plugins, above the viewer of the one selected, under the directory
// and the button that chooses another one. A galaxy whose plugins are known is dragged into the edition zone, to be
// placed there, and opened from its viewer, or by activating it: see openRequested(). It is listed again when another
// part tells its studio that a project file was written, or a plugin opened from its file. Named Galaxies, beside the
// icon of the Galaxy: the title and the icon of its tab.
class GalaxyPanel : public QWidget
{
    Q_OBJECT

  public:
    explicit GalaxyPanel(Studio& studio, QWidget* parent = nullptr);

    // The directory listed, absolute.
    [[nodiscard]] QString directory() const;
    // Lists the galaxies of directory, tells which one above them, and shows the first one in its viewer: scene/ next
    // to the application when directory is empty, and a relative directory read from the one of the application.
    void setDirectory(const QString& directory);

  signals:
    // The galaxy of the file filePath, whose plugins the application knows, is asked to be opened: the panel does not
    // open it by itself.
    void openRequested(const QString& filePath);
    // The user chose to list the galaxies of directory, absolute, which it lists now.
    void directoryChosen(const QString& directory);

  private:
    // The galaxies, those the application knows the plugins of opened from their viewer, or activated, and dragged
    // into the edition zone; under the directory, and the button that chooses another one.
    QWidget* createList();
    QWidget* createDirectoryBar();
    // Follows what the studio tells: the galaxies to list again, and whether the application runs.
    void followStudio(const Studio& studio);
    // Asks for the directory whose galaxies are listed.
    void chooseDirectory();
    // Asks for the galaxy of the file filePath to be opened, when the application knows its plugins.
    void open(const QString& filePath);

    GalaxyModel* m_model = nullptr;
    GalaxyViewer* m_viewer = nullptr;
    QTreeView* m_view = nullptr;
    QLineEdit* m_directory = nullptr; // Tells the directory of m_model
};
