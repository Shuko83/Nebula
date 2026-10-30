// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>

class EditionZone;
class QMenu;
class QWidget;
class Studio;

// The project file of the application of a Studio, as its edition zone shows it, see EditionScene::save(): New empties
// the application, Open... replaces it with the one of a file, Save and Save As... write it to its file, and
// Description... edits the description of the galaxy its file is. What would replace it asks first whether to save
// it, when it changed since it was saved or opened, once the galaxies shown in the zone are left, see
// EditionZone::leaveGalaxies(). While the application runs, it is not replaced, and it is saved with the values of the
// edition. Its dialogs and its boxes open over the window of the zone, whose title it tells, see title().
class Project : public QObject
{
    Q_OBJECT

  public:
    Project(Studio& studio, EditionZone& zone, QObject* parent = nullptr);

    // Adds to menu New, Open..., Save and Save As..., Save Galaxy, which saves the galaxy shown in the zone, then
    // Description....
    void addFileActions(QMenu& menu);
    // The title of the window: NebulaStudio, after the name of its file when it has one. See titleChanged().
    [[nodiscard]] QString title() const;

    // Replaces the application with the one of the project file filePath, whose plugins it loads, and tells what
    // could not be restored. Tells why, changes nothing and returns false when the file cannot be read.
    bool openProjectFile(const QString& filePath);
    // Writes the application to the project file filePath, from then on its file; tells why and returns false when it
    // cannot be written.
    bool saveProjectFile(const QString& filePath);
    // Asks, when the galaxies shown changed since they were, then when the application changed since its file holds
    // it, whether to save them. Returns false when what was about to replace it is given up: the answer was to
    // cancel, or it could not be saved.
    bool saveChanges();
    // Opens the galaxy of the file filePath, one whose plugins the application knows, once the changes of the
    // application are saved or given up. Does nothing while the application runs.
    void openGalaxy(const QString& filePath);

  signals:
    void titleChanged(const QString& title);

  private:
    // New, Open..., Save and Save As....
    void addProjectActions(QMenu& menu);
    // Where its dialogs and boxes open: over the window of the zone.
    [[nodiscard]] QWidget* window() const;
    // Tells text, what opening a project file did not do, in a box.
    void warnOpening(const QString& text);
    // filePath is the file of the application, none when empty, which holds it as it is now.
    void setProjectFile(const QString& filePath);
    // Asks whether to save the application, which changed; false when it is not saved, nor its changes given up.
    bool saveOrDiscardProject();
    // Empties the application, once its changes are saved or given up.
    void newProject();
    // Asks for a project file and opens it, once the changes of the application are saved or given up.
    void openProject();
    // Asks for the project file to open; none when none is chosen.
    QString askForProjectFile();
    // Where the dialogs of the project files start: the file of the application, when it has one; the default
    // directory otherwise.
    [[nodiscard]] QString projectDialogPath() const;
    // Shows project, read from the file filePath, in place of the application, and tells what could not be restored.
    void showProject(const QString& filePath, const QJsonObject& project);
    // Saves the application to its file, asked for when it has none; false when it is not saved.
    bool saveProject();
    // Asks for the file to save the application to; false when it is not saved.
    bool saveProjectAs();
    // Writes the application to the file filePath, which its galaxies are written relative to; tells why and returns
    // false when it cannot be written.
    bool writeProject(const QString& filePath);
    // Asks for the description of the application, the one its galaxy has once it is saved.
    void editDescription();

    Studio& m_studio;
    EditionZone& m_zone;
    QString m_projectFile;      // Empty until the application is saved to a file, or opened from one
    QJsonObject m_savedProject; // What the scene of the application saved when m_projectFile was set
};
