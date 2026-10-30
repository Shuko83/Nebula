// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Project.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <optional>

#include "EditionScene.h"
#include "EditionZone.h"
#include "Messages.h"
#include "PanelWidgets.h"
#include "ProjectFiles.h"
#include "Studio.h"
#include "Theme.h"

Project::Project(Studio& studio, EditionZone& zone, QObject* parent) : QObject(parent), m_studio(studio), m_zone(zone)
{
    setProjectFile({});
}

void Project::addFileActions(QMenu& menu)
{
    addProjectActions(menu);
    menu.addAction(m_zone.saveGalaxyAction());
    menu.addSeparator();
    // The description of the galaxy its file is.
    disableWhileRunning(m_studio.application(), *menu.addAction(icon(GlossaryTerm::Galaxy), tr("&Description..."), this,
                                                                &Project::editDescription));
}

void Project::addProjectActions(QMenu& menu)
{
    const Nebula::Universe& application = m_studio.application();
    // An empty application in its place.
    disableWhileRunning(application, *menu.addAction(icon(GlossaryTerm::Universe), tr("&New"), QKeySequence::New, this,
                                                     &Project::newProject));
    disableWhileRunning(application, *menu.addAction(tr("&Open..."), QKeySequence::Open, this, &Project::openProject));
    menu.addAction(tr("&Save"), QKeySequence::Save, this, &Project::saveProject);
    menu.addAction(tr("Save &As..."), QKeySequence::SaveAs, this, &Project::saveProjectAs);
}

QString Project::title() const
{
    return m_projectFile.isEmpty() ? tr("NebulaStudio")
                                   : tr("%1 - NebulaStudio").arg(QFileInfo(m_projectFile).fileName());
}

QWidget* Project::window() const
{
    return m_zone.window();
}

void Project::warnOpening(const QString& text)
{
    warn(window(), tr("Open project"), text);
}

bool Project::openProjectFile(const QString& filePath)
{
    QString error;
    const std::optional<QJsonObject> project = Nebula::Universe::read(filePath, error);
    if(!project)
    {
        warnOpening(tr("%1 cannot be opened:\n%2").arg(QDir::toNativeSeparators(filePath), error)); // Nothing changed
        return false;
    }
    showProject(filePath, *project);
    return true;
}

void Project::showProject(const QString& filePath, const QJsonObject& project)
{
    m_studio.application().setFilePath(filePath); // Its galaxies are relative to it
    const QStringList problems = m_zone.applicationScene().load(project);
    m_studio.refreshPlugins(); // Those it loaded, and their components, as the galaxies they were missing from
    m_studio.refreshGalaxies();
    setProjectFile(problems.isEmpty() ? filePath : QString()); // Saving would write over what is missing
    if(!problems.isEmpty())
    {
        warnOpening(
            tr("%1 could not be entirely opened:\n%2").arg(QDir::toNativeSeparators(filePath), problems.join(u'\n')));
    }
}

bool Project::saveProjectFile(const QString& filePath)
{
    m_zone.applicationScene().setFocusItem(nullptr); // What was typed in a name is taken before it is saved
    if(!writeProject(filePath))
    {
        return false;
    }
    setProjectFile(filePath);
    m_studio.refreshGalaxies(); // Its directory may have just been made, which nothing followed yet
    return true;
}

bool Project::writeProject(const QString& filePath)
{
    Nebula::Universe& application = m_studio.application();
    const QString previousFile = application.filePath();
    application.setFilePath(filePath);
    const bool written = writeProjectFile(window(), tr("Save project"), filePath, m_zone.applicationScene().save());
    if(!written)
    {
        application.setFilePath(previousFile);
    }
    return written;
}

void Project::setProjectFile(const QString& filePath)
{
    m_projectFile = filePath;
    m_studio.application().setFilePath(filePath); // Before it is saved: its galaxies are relative to it
    m_savedProject = m_zone.applicationScene().save();
    emit titleChanged(title());
}

bool Project::saveChanges()
{
    return m_zone.leaveGalaxies(0, true) &&
           (m_zone.applicationScene().save() == m_savedProject || saveOrDiscardProject());
}

bool Project::saveOrDiscardProject()
{
    const QMessageBox::StandardButton answer =
        askToSave(window(), tr("The application has been modified.\nDo you want to save it?"));
    return answer == QMessageBox::Save ? saveProject() : answer == QMessageBox::Discard;
}

void Project::newProject()
{
    if(saveChanges())
    {
        m_zone.applicationScene().reset();
        setProjectFile({});
    }
}

void Project::openProject()
{
    const QString filePath = saveChanges() ? askForProjectFile() : QString();
    if(!filePath.isEmpty())
    {
        openProjectFile(filePath);
    }
}

QString Project::askForProjectFile()
{
    return QFileDialog::getOpenFileName(window(), tr("Open project"), projectDialogPath(),
                                        tr("Nebula projects (*.nbp);;All files (*)"));
}

QString Project::projectDialogPath() const
{
    return m_projectFile.isEmpty() ? defaultProjectDirectory() : m_projectFile;
}

bool Project::saveProject()
{
    return m_projectFile.isEmpty() ? saveProjectAs() : saveProjectFile(m_projectFile);
}

bool Project::saveProjectAs()
{
    if(m_projectFile.isEmpty())
    {
        QDir().mkpath(
            defaultProjectDirectory()); // There the first time a project is saved: the dialog would start elsewhere
    }
    const QString filePath =
        QFileDialog::getSaveFileName(window(), tr("Save project"), projectDialogPath(), tr("Nebula projects (*.nbp)"));
    return !filePath.isEmpty() && saveProjectFile(filePath);
}

void Project::openGalaxy(const QString& filePath)
{
    if(m_studio.application().isRunning() || !saveChanges())
    {
        return;
    }
    openProjectFile(filePath);
}

void Project::editDescription()
{
    bool accepted = false;
    const QString description = QInputDialog::getMultiLineText(
        window(), tr("Description"), tr("What the application is for, shown with its galaxy in the Galaxies tab:"),
        m_studio.application().description(), &accepted);
    if(accepted)
    {
        m_studio.application().setDescription(description.trimmed()); // None when only blanks are left
    }
}
