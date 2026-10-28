// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QHash>
#include <QList>
#include <QMetaProperty>
#include <QString>

#include "EditDialog.h"
#include "Nebula/Settings/SettingsFile.h"

class QFormLayout;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;

namespace Nebula
{
class ComponentFactory;
class Universe;
} // namespace Nebula

// Edits a settings file of the application, or of one of its galaxies: the name of its file, the one the file of its
// galaxy gives it in a galaxy, its directory, and the properties it holds, checked among those of the components of the
// application, or of its galaxy, then of their objects, that a settings file can hold. A property another file holds
// is listed with the name of that file, and cannot be checked. What was edited is given to the file once the dialog is
// accepted.
class SettingsDialog : public EditDialog
{
    Q_OBJECT

  public:
    // file, one of those of application or of its galaxies, outlives the dialog.
    SettingsDialog(Nebula::SettingsFile& file, const Nebula::Universe& application,
                   const Nebula::ComponentFactory& factory, QWidget* parent = nullptr);

  private:
    // The fields of its name and its directory, and where it is written, which follows them.
    void createFields();
    void layOut();
    [[nodiscard]] QFormLayout* newForm();
    // Its directory, with a button to choose another one.
    [[nodiscard]] QHBoxLayout* newDirectoryRow();
    // Lists the properties of the components of application, or of the galaxy of the file.
    void fill(const Nebula::Universe& application, const Nebula::ComponentFactory& factory);
    // The row of component, with those of the properties a file can hold under it.
    [[nodiscard]] QTreeWidgetItem* componentRow(Nebula::Component& component, const Nebula::ComponentFactory& factory);
    // Lists under parent those of properties of object, component or one of its objects, that a file can hold.
    void addProperties(Nebula::Component& component, QTreeWidgetItem* parent, QObject* object,
                       const QList<QMetaProperty>& properties);
    // The row of entry, whose property is property, under parent: checked when the file holds it, disabled when
    // another one does.
    void addProperty(QTreeWidgetItem* parent, const Nebula::SettingsEntry& entry, const QMetaProperty& property);
    // The file name the file would have with the name typed, after the galaxies it is in.
    [[nodiscard]] QString fileName() const;
    // Why the name typed cannot be given to the file: there is none, or it is the one of another file; empty when it
    // can.
    [[nodiscard]] QString nameRefusal() const;
    // Shows where the file is, as its name and its directory are edited; it is not accepted without a name, nor with
    // the one of another file of the application.
    void refreshPath();
    // Asks for the directory of the file.
    void browse();
    // Gives the file what was edited.
    void apply() override;
    // Has the file hold the property of row, or no longer, as row is checked.
    void applyRow(const QTreeWidgetItem& row);

    Nebula::SettingsFile& m_file;
    const Nebula::Universe& m_application;
    QHash<const QTreeWidgetItem*, Nebula::SettingsEntry> m_entries; // The entry each property of the tree is
    QLineEdit* m_fileName = nullptr;
    QLineEdit* m_directory = nullptr;
    QLabel* m_path = nullptr;
    QTreeWidget* m_tree = nullptr;
};
