// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QMetaProperty>
#include <QPointer>
#include <QString>
#include <QVariant>
#include <QWidget>

#include "Nebula/Settings/SettingsFile.h"

class FileFollower;
class PlaceholderStack;
class QLabel;
class QTableWidget;
class QTableWidgetItem;
class WarningLabel;

namespace Nebula
{
class Component;
class Universe;
} // namespace Nebula

// The values a settings file of the application, or of one of its galaxies, holds, edited where the user of the
// application finds them: in its file. A row per property the file holds, under the name of its instance, after the
// name of its object when it has one, shows the value the file holds for it, of the type of the property; one the file
// holds no value for, or a value the property cannot have, shows its default value, the one of the project, in
// italics: the application writes it there when it starts. A value edited is written to the file at once, see
// Nebula::SettingsFile::writeValue(), and refused() tells why when it cannot be; one whose type has no editor is only
// shown. Above them come where the file is, and whether it does not exist yet, or cannot be read, which keeps them from
// being edited. It follows the file: what another program writes there, as the application does while it runs, is
// shown at once. While the application runs, it only shows the values.
class SettingsValueEditor : public QWidget
{
    Q_OBJECT

  public:
    // The files are those of application, or of its galaxies.
    explicit SettingsValueEditor(const Nebula::Universe& application, QWidget* parent = nullptr);

    // The file it shows; nullptr when none.
    [[nodiscard]] Nebula::SettingsFile* file() const;
    // Shows the values file holds; none when it is nullptr.
    void setFile(Nebula::SettingsFile* file);

  signals:
    // A value edited could not be written to the file; why is a sentence to show.
    void refused(const QString& why);

  protected:
    // Gives the titles of the instances the background of the palette when it changes, as the color scheme does.
    void changeEvent(QEvent* event) override;

  private:
    // A row of m_table: an entry of the file, or the title of the instance of the rows under it, which has none.
    struct Row
    {
        qsizetype index = -1; // Of its entry among those of the file; -1 for a title
        Nebula::SettingsEntry entry;
        QMetaProperty property;
        QString name; // What it shows: the name of an instance, or of the property of its entry in it
    };

    // Where the file is and what keeps its values from being edited, over the table.
    [[nodiscard]] QWidget* newPage();
    // Where the file is and what keeps its values from being edited.
    [[nodiscard]] QWidget* newHeader();
    // The entries of the file; none without it.
    [[nodiscard]] QList<Nebula::SettingsEntry> entries() const;
    // Rebuilds it as the entries of the file, its name or its directory change; shows nothing once it is destroyed.
    void followFile();
    // Shows the entries the file has now, and what it holds for them.
    void rebuild();
    // No longer follows the instances of the rows shown until now.
    void forgetInstances();
    // Has the rows rebuilt when an instance of theirs is renamed: the file holds its values under its name.
    void followInstances();
    // The rows of the entries of the file, under the names of their instances, in the order the instances come there.
    [[nodiscard]] QList<Row> fileRows() const;
    // The rows of the entries of the file whose instance is instance, in their order.
    [[nodiscard]] QList<Row> entryRows(const Nebula::Component& instance) const;
    // The row of entry, the index-th of the file.
    [[nodiscard]] static Row entryRow(qsizetype index, const Nebula::SettingsEntry& entry);
    // Shows m_rows in the table, without their values.
    void showRows();
    // Shows the row row of m_rows.
    void showRow(int row);
    // Reads the file again, and shows what it holds for each entry, where it is and what it is: its values are edited
    // while it can be read and the application does not run. Follows the file, which can exist now.
    void refresh();
    // Whether a click on a value edits it.
    void setEditable(bool editable);
    // Tells where the file is, and what keeps its values from being shown or edited: it does not exist yet, it cannot
    // be read, for why, unless readable, or the application runs.
    void showState(bool readable, const QString& why);
    // Shows values, those the file holds for its entries, in their order, or their default values where they are
    // invalid.
    void showValues(const QVariantList& values);
    // Shows in item value, the one the file holds for the entry of shown, or, when it is invalid, the default value of
    // that entry, in italics.
    static void showValue(QTableWidgetItem& item, const Row& shown, const QVariant& value);
    // Tells why no value is shown, when none is.
    void refreshPlaceholder();
    // Writes the value edited in item to the file, then shows what the file holds.
    void write(const QTableWidgetItem* item);

    const Nebula::Universe& m_application;
    QPointer<Nebula::SettingsFile> m_file;
    QList<Row> m_rows;                              // One per row of m_table
    QList<QPointer<Nebula::Component>> m_instances; // Those of m_rows, followed
    FileFollower* m_follower = nullptr;             // Of the file of m_file
    QLabel* m_path = nullptr;                       // Where the file is
    QLabel* m_note = nullptr;                       // It does not exist yet, or the application runs
    WarningLabel* m_problem = nullptr;              // It cannot be read
    QTableWidget* m_table = nullptr;                // Under them
    PlaceholderStack* m_stack = nullptr;            // Of the page of m_table, and of what tells why it has no row
};
