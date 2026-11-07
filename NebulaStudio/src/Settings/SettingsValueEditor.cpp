// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "SettingsValueEditor.h"

#include <QDir>
#include <QEvent>
#include <QFile>
#include <QLabel>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QVBoxLayout>
#include <optional>

#include "FileFollower.h"
#include "Nebula/Application/Universe.h"
#include "Nebula/Component/Component.h"
#include "PlaceholderStack.h"
#include "PropertyTable.h"
#include "PropertyTree.h"
#include "WarningLabel.h"

namespace
{

// A text over the values, in the muted color of the theme, which wraps.
QLabel* newMutedLabel()
{
    auto* label = new QLabel;
    label->setForegroundRole(QPalette::PlaceholderText);
    label->setWordWrap(true);
    return label;
}

// Where the file is, which can be selected and copied.
QLabel* newPathLabel()
{
    QLabel* label = newMutedLabel();
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setToolTip(SettingsValueEditor::tr("Where the application writes it when it runs here, in NebulaStudio"));
    return label;
}

// The instances of entries, each once, in the order they come there.
QList<Nebula::Component*> instancesOf(const QList<Nebula::SettingsEntry>& entries)
{
    QList<Nebula::Component*> instances;
    for(const Nebula::SettingsEntry& entry : entries)
    {
        if(!instances.contains(entry.component))
        {
            instances.append(entry.component);
        }
    }
    return instances;
}

// The name of a property, its type in its tooltip.
QTableWidgetItem* newNameItem(const QString& name, const QMetaProperty& property)
{
    auto* item = new QTableWidgetItem(name);
    item->setFlags(Qt::ItemIsEnabled);
    item->setToolTip(QString::fromLatin1(property.typeName()));
    return item;
}

// The value of property, edited when its type has an editor, only shown otherwise.
QTableWidgetItem* newValueItem(const QMetaProperty& property)
{
    auto* item = new QTableWidgetItem; // Editable, as an item is by default
    if(!hasEditor(property.metaType()))
    {
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    }
    return item;
}

// What keeps the values of a file from being edited, though they are shown: the application runs, or the file does
// not exist yet; empty when nothing does.
QString noteFor(bool running, bool exists)
{
    if(running)
    {
        return SettingsValueEditor::tr("The application runs: these are the values it writes there, edited once it is "
                                       "stopped");
    }
    return exists ? QString()
                  : SettingsValueEditor::tr("It does not exist yet: the application writes it with the default values "
                                            "when it starts, or as soon as a value is given here");
}

} // namespace

SettingsValueEditor::SettingsValueEditor(const Nebula::Universe& application, QWidget* parent)
    : QWidget(parent), m_application(application), m_follower(new FileFollower(this)), m_path(newPathLabel()),
      m_note(newMutedLabel()), m_problem(new WarningLabel), m_table(newPropertyTable()),
      m_stack(new PlaceholderStack(newPage(), *this))
{
    // Whether the values are edited, and those it writes while it runs.
    connect(&m_application, &Nebula::Universe::runningChanged, this, &SettingsValueEditor::refresh);
    connect(m_follower, &FileFollower::changed, this, &SettingsValueEditor::refresh);
    connect(m_table, &QTableWidget::itemChanged, this, &SettingsValueEditor::write);
    setFile(nullptr);
}

QWidget* SettingsValueEditor::newPage()
{
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins({}); // The table across it, as the one of a component
    layout->addWidget(newHeader());
    layout->addWidget(m_table, 1);
    return page;
}

QWidget* SettingsValueEditor::newHeader()
{
    auto* header = new QWidget;
    auto* layout = new QVBoxLayout(header); // With the margins of the style
    layout->addWidget(m_path);
    layout->addWidget(m_note);
    layout->addWidget(m_problem);
    return header;
}

Nebula::SettingsFile* SettingsValueEditor::file() const
{
    return m_file;
}

void SettingsValueEditor::setFile(Nebula::SettingsFile* file)
{
    if(m_file != nullptr)
    {
        m_file->disconnect(this);
    }
    m_file = file;
    followFile();
    rebuild();
}

void SettingsValueEditor::followFile()
{
    if(m_file != nullptr)
    {
        connect(m_file, &Nebula::SettingsFile::changed, this, &SettingsValueEditor::rebuild);
        connect(m_file, &QObject::destroyed, this, [this] { setFile(nullptr); });
    }
}

void SettingsValueEditor::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if(event->type() == QEvent::PaletteChange)
    {
        colorTitles(*m_table);
    }
}

QList<Nebula::SettingsEntry> SettingsValueEditor::entries() const
{
    return m_file != nullptr ? m_file->entries() : QList<Nebula::SettingsEntry>();
}

void SettingsValueEditor::rebuild()
{
    m_rows = fileRows();
    forgetInstances();
    followInstances();
    showRows();
    refresh();
    refreshPlaceholder();
}

void SettingsValueEditor::forgetInstances()
{
    for(const QPointer<Nebula::Component>& instance : std::as_const(m_instances))
    {
        if(instance != nullptr)
        {
            instance->disconnect(this);
        }
    }
    m_instances.clear();
}

void SettingsValueEditor::followInstances()
{
    for(Nebula::Component* instance : instancesOf(entries()))
    {
        m_instances.append(instance);
        connect(instance, &QObject::objectNameChanged, this, &SettingsValueEditor::rebuild);
    }
}

QList<SettingsValueEditor::Row> SettingsValueEditor::fileRows() const
{
    QList<Row> rows;
    for(const Nebula::Component* instance : instancesOf(entries()))
    {
        rows.append(Row{.name = instance->objectName()});
        rows.append(entryRows(*instance));
    }
    return rows;
}

QList<SettingsValueEditor::Row> SettingsValueEditor::entryRows(const Nebula::Component& instance) const
{
    QList<Row> rows;
    const QList<Nebula::SettingsEntry>& entries = m_file->entries();
    for(qsizetype index = 0; index < entries.size(); ++index)
    {
        if(entries.at(index).component == &instance)
        {
            rows.append(entryRow(index, entries.at(index)));
        }
    }
    return rows;
}

SettingsValueEditor::Row SettingsValueEditor::entryRow(qsizetype index, const Nebula::SettingsEntry& entry)
{
    return Row{.index = index,
               .entry = entry,
               .property = metaPropertyOf(*entry.object, entry.property),
               .name = propertyPath(entry)};
}

void SettingsValueEditor::showRows()
{
    const QSignalBlocker blocker(m_table); // itemChanged is for the values edited in the table
    m_table->setRowCount(0);               // Closes the editor of a value shown until now
    m_table->clearSpans();
    m_table->setRowCount(static_cast<int>(m_rows.size()));
    for(int row = 0; row < m_table->rowCount(); ++row)
    {
        showRow(row);
    }
}

void SettingsValueEditor::showRow(int row)
{
    const Row& shown = m_rows.at(row);
    if(shown.index < 0)
    {
        showTitle(*m_table, row, shown.name);
        return;
    }
    m_table->setItem(row, PropertyNameColumn, newNameItem(shown.name, shown.property));
    m_table->setItem(row, PropertyValueColumn, newValueItem(shown.property));
}

void SettingsValueEditor::refresh()
{
    m_follower->follow(m_file != nullptr ? m_file->filePath() : QString()); // Its directory may exist now
    if(m_file != nullptr)
    {
        QString why;
        const std::optional<QVariantList> values = m_file->readValues(why);
        showState(values.has_value(), why);
        showValues(values.value_or(QVariantList(m_file->entries().size())));
        setEditable(values.has_value() && !m_application.isRunning());
    }
}

void SettingsValueEditor::setEditable(bool editable)
{
    // A click on a value edits it then.
    m_table->setEditTriggers(editable ? QAbstractItemView::AllEditTriggers : QAbstractItemView::NoEditTriggers);
}

void SettingsValueEditor::showState(bool readable, const QString& why)
{
    const QString filePath = m_file->filePath();
    m_path->setText(QDir::toNativeSeparators(filePath));
    m_note->setText(noteFor(m_application.isRunning(), QFile::exists(filePath)));
    m_note->setVisible(!m_note->text().isEmpty());
    m_problem->setText(readable ? QString()
                                : tr("It cannot be read (%1): the application sets it aside when it starts, then "
                                     "writes it again")
                                      .arg(why));
}

void SettingsValueEditor::showValues(const QVariantList& values)
{
    const QSignalBlocker blocker(m_table); // itemChanged is for the values edited in the table
    for(int row = 0; row < m_table->rowCount(); ++row)
    {
        const Row& shown = m_rows.at(row);
        if(shown.index >= 0 && shown.index < values.size())
        {
            showValue(*m_table->item(row, PropertyValueColumn), shown, values.at(shown.index));
        }
    }
}

void SettingsValueEditor::showValue(QTableWidgetItem& item, const Row& shown, const QVariant& value)
{
    const bool inFile = value.isValid();
    // The value itself rather than its text: its type is what gives the cell its editor.
    item.setData(Qt::EditRole, inFile ? value : shown.property.read(shown.entry.object));
    QFont font = item.font();
    font.setItalic(!inFile);
    item.setFont(font);
    item.setToolTip(inFile ? QString()
                           : tr("Its default value, the one of the project: the file holds none it can have, and the "
                                "application writes this one there when it starts"));
}

void SettingsValueEditor::refreshPlaceholder()
{
    m_stack->placeholder()->setText(
        m_file == nullptr
            ? QString()
            : tr("%1 holds no property yet: double-click it to choose those it holds").arg(m_file->fileName()));
    m_stack->showPageIf(!m_rows.isEmpty());
}

void SettingsValueEditor::write(const QTableWidgetItem* item)
{
    const Row& edited = m_rows.at(item->row());
    if(m_file == nullptr || edited.index < 0 || item->column() != PropertyValueColumn)
    {
        return;
    }
    if(!m_file->writeValue(edited.entry, item->data(Qt::EditRole)))
    {
        emit refused(tr("%1 is not written: %2").arg(edited.name, m_file->errorString()));
    }
    refresh(); // What the file holds now, or still
}
