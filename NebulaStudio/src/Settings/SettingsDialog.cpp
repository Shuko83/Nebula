// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "SettingsDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>

#include "DialogHeader.h"
#include "Nebula/Application/Universe.h"
#include "PlaceholderStack.h"
#include "PropertyTree.h"
#include "Theme.h"
#include "WarningLabel.h"

namespace
{

enum Column : std::uint8_t
{
    PropertyColumn,
    ValueColumn,
    FileColumn,
};

// The space between the fields and the properties.
constexpr int SectionSpacing = 12;

// The directory of a file, directory, telling what an empty or a relative one is; a button empties it.
QLineEdit* newDirectoryEdit(const QString& directory)
{
    auto* edit = new QLineEdit(directory);
    edit->setPlaceholderText(SettingsDialog::tr("Configuration directory of the user"));
    edit->setToolTip(
        SettingsDialog::tr("Empty: the configuration directory of the user, for the application that runs\n"
                           "Relative: from the directory of the application that runs"));
    edit->setClearButtonEnabled(true);
    return edit;
}

// A text in the muted color of the theme.
QLabel* newMutedLabel()
{
    auto* label = new QLabel;
    label->setForegroundRole(QPalette::PlaceholderText);
    return label;
}

// Where a file is written.
QLabel* newPathLabel()
{
    QLabel* label = newMutedLabel();
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setToolTip(SettingsDialog::tr("Where the application writes it when it runs here, in NebulaStudio"));
    return label;
}

// What filters the properties; a button empties it.
QLineEdit* newFilterEdit()
{
    auto* edit = new QLineEdit;
    edit->setPlaceholderText(SettingsDialog::tr("Filter properties"));
    edit->setToolTip(SettingsDialog::tr("Shows the properties whose name, or the one of their instance or of their "
                                        "group, contains this text"));
    edit->setClearButtonEnabled(true);
    return edit;
}

QCheckBox* newHeldOnlyBox()
{
    auto* box = new QCheckBox(SettingsDialog::tr("Held only"));
    box->setToolTip(SettingsDialog::tr("Shows only the properties the file holds"));
    return box;
}

// The title of a part of the dialog.
QLabel* newSectionTitle(const QString& text)
{
    auto* label = new QLabel(text);
    label->setFont(titleFont(label->font()));
    return label;
}

// Disables row, a property the file named holder holds, which it tells.
void disableFor(QTreeWidgetItem& row, const QString& holder)
{
    row.setFlags(Qt::NoItemFlags);
    row.setText(FileColumn, holder);
    row.setIcon(FileColumn, icon(GlossaryTerm::Comet));
    row.setToolTip(FileColumn, SettingsDialog::tr("A property is held by a single settings file"));
}

bool isChecked(const QTreeWidgetItem& row)
{
    return row.checkState(PropertyColumn) == Qt::Checked;
}

// Whether row is of a property a file holds: the one of the dialog, or another one.
bool isHeld(const QTreeWidgetItem& row)
{
    return isChecked(row) || !row.text(FileColumn).isEmpty();
}

} // namespace

SettingsDialog::SettingsDialog(Nebula::SettingsFile& file, const Nebula::Universe& application,
                               const Nebula::ComponentFactory& factory, QWidget* parent)
    : EditDialog(file, parent), m_file(file), m_application(application),
      m_header(new DialogHeader(icon(GlossaryTerm::Comet))),
      m_tree(newChoiceTree({tr("Property"), tr("Default value"), tr("Held by")}))
{
    setWindowTitle(tr("Settings file"));
    setWindowIcon(icon(GlossaryTerm::Comet));
    createFields();
    createFilter();
    fill(application, factory);
    layOut();
    refreshFields();
    refreshCount();
    filter();
    resize(640, 640);
}

void SettingsDialog::createFields()
{
    m_fileName = new QLineEdit(m_file.name());
    m_directory = newDirectoryEdit(m_file.directory());
    m_path = newPathLabel();
    m_refusal = new WarningLabel;
    connect(m_fileName, &QLineEdit::textChanged, this, &SettingsDialog::refreshFields);
    connect(m_directory, &QLineEdit::textChanged, this, &SettingsDialog::refreshFields);
}

void SettingsDialog::createFilter()
{
    m_filter = newFilterEdit();
    m_heldOnly = newHeldOnlyBox();
    m_count = newMutedLabel();
    connect(m_filter, &QLineEdit::textChanged, this, &SettingsDialog::filter);
    connect(m_heldOnly, &QCheckBox::toggled, this, &SettingsDialog::filter);
}

void SettingsDialog::layOut()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins({}); // The header across the dialog
    layout->setSpacing(0);
    layout->addWidget(m_header);
    layout->addWidget(newContent(), 1);
}

QWidget* SettingsDialog::newContent()
{
    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->addLayout(newForm());
    layout->addWidget(m_refusal);
    layout->addSpacing(SectionSpacing);
    layout->addLayout(newPropertiesTitle());
    layout->addLayout(newFilterRow());
    layout->addWidget(newPropertyArea(), 1);
    layout->addWidget(buttons());
    return content;
}

QFormLayout* SettingsDialog::newForm()
{
    auto* form = new QFormLayout;
    form->addRow(tr("File name:"), m_fileName);
    form->addRow(tr("Directory:"), newDirectoryRow());
    form->addRow(tr("Written to:"), m_path);
    return form;
}

QHBoxLayout* SettingsDialog::newDirectoryRow()
{
    auto* browseButton = new QPushButton(tr("Browse..."));
    connect(browseButton, &QPushButton::clicked, this, &SettingsDialog::browse);
    auto* row = new QHBoxLayout;
    row->addWidget(m_directory, 1);
    row->addWidget(browseButton);
    return row;
}

QHBoxLayout* SettingsDialog::newPropertiesTitle()
{
    auto* row = new QHBoxLayout;
    row->addWidget(newSectionTitle(tr("Properties")));
    row->addStretch();
    row->addWidget(m_count);
    return row;
}

QHBoxLayout* SettingsDialog::newFilterRow()
{
    auto* row = new QHBoxLayout;
    row->addWidget(m_filter, 1);
    row->addWidget(m_heldOnly);
    return row;
}

QWidget* SettingsDialog::newPropertyArea()
{
    auto* area = new QWidget;
    m_properties = new PlaceholderStack(m_tree, *area);
    m_properties->placeholder()->setForegroundRole(QPalette::PlaceholderText);
    return area;
}

void SettingsDialog::fill(const Nebula::Universe& application, const Nebula::ComponentFactory& factory)
{
    // Those of its galaxy: it holds no other.
    for(const std::unique_ptr<Nebula::Component>& component : application.components(m_file.galaxy()))
    {
        addIfFilled(*m_tree, componentRow(*component, factory));
    }
    openAbove(*m_tree, isHeld);
    makeGroupsCheckable(*m_tree, PropertyColumn);
    connect(m_tree, &QTreeWidget::itemChanged, this, &SettingsDialog::checkChanged);
}

QTreeWidgetItem* SettingsDialog::componentRow(Nebula::Component& component, const Nebula::ComponentFactory& factory)
{
    QTreeWidgetItem* row = groupRow(component.objectName());
    row->setToolTip(PropertyColumn, component.componentId());
    row->setIcon(PropertyColumn, icon(GlossaryTerm::Astre));
    row->setFont(PropertyColumn, titleFont(m_tree->font()));
    const ListProperties list = std::bind_front(&SettingsDialog::addProperties, this, std::ref(component));
    list(row, &component, ownProperties(factory, component));
    addObjectGroups(*row, component, list);
    return row;
}

void SettingsDialog::addProperties(Nebula::Component& component, QTreeWidgetItem* parent, QObject* object,
                                   const QList<QMetaProperty>& properties)
{
    for(const QMetaProperty& property : heldProperties(component, object, properties))
    {
        const QString name = QString::fromLatin1(property.name());
        addProperty(parent, {.component = &component, .object = object, .property = name}, property);
    }
}

void SettingsDialog::addProperty(QTreeWidgetItem* parent, const Nebula::SettingsEntry& entry,
                                 const QMetaProperty& property)
{
    auto* row = new QTreeWidgetItem(parent, {entry.property, property.read(entry.object).toString()});
    row->setToolTip(PropertyColumn, QString::fromLatin1(property.typeName()));
    const Nebula::SettingsFile* holder = m_application.settingsFile(entry);
    if(holder != nullptr && holder != &m_file) // Only one file holds a property
    {
        disableFor(*row, holder->fileName());
        return;
    }
    makeCheckable(*row, PropertyColumn, holder != nullptr);
    m_entries.insert(row, entry);
}

QString SettingsDialog::fileName() const
{
    // After the galaxies it is in, as its own is.
    return m_file.fileName().chopped(m_file.name().size()) + m_fileName->text().trimmed();
}

QString SettingsDialog::nameRefusal() const
{
    if(m_fileName->text().trimmed().isEmpty())
    {
        return tr("A file name is needed");
    }
    const Nebula::SettingsFile* holder = m_application.settingsFile(fileName());
    // Its own name, whatever its case, is its own.
    return holder != nullptr && holder != &m_file ? tr("Another settings file is named %1").arg(holder->fileName())
                                                  : QString();
}

QString SettingsDialog::about() const
{
    QString about = tr("Keeps the values the user of the application gives the properties checked below, from one run "
                       "to the next. Those of the project are their default values.");
    if(m_file.galaxy() == nullptr)
    {
        return about;
    }
    return about + u' ' +
           tr("Each instance of the galaxy writes its own file, named after it: %1 for this one.").arg(fileName());
}

void SettingsDialog::refreshFields()
{
    const QString name = m_fileName->text().trimmed();
    m_header->setTitle(name.isEmpty() ? tr("Settings file") : name);
    m_header->setText(about());
    const QString filePath = Nebula::SettingsFile::filePath(fileName(), m_directory->text().trimmed());
    m_path->setText(QDir::toNativeSeparators(filePath));
    const QString refusal = nameRefusal();
    m_refusal->setText(refusal);
    setAcceptable(refusal.isEmpty());
}

void SettingsDialog::browse()
{
    // From where the file is now.
    const QString current = Nebula::SettingsFile::filePath(
        m_fileName->text().trimmed().isEmpty() ? m_file.fileName() : fileName(), m_directory->text().trimmed());
    const QString directory = QFileDialog::getExistingDirectory(this, tr("Directory of the settings file"),
                                                                QFileInfo(current).absolutePath());
    if(!directory.isEmpty())
    {
        m_directory->setText(directory);
    }
}

void SettingsDialog::checkChanged(QTreeWidgetItem* row, int column)
{
    if(column == PropertyColumn)
    {
        followCheck(*row, column);
        refreshCount();
    }
}

void SettingsDialog::refreshCount()
{
    const QList<const QTreeWidgetItem*> rows = m_entries.keys();
    const auto held = std::ranges::count_if(rows, [](const QTreeWidgetItem* row) { return isChecked(*row); });
    m_count->setText(tr("%1 of %2 held").arg(held).arg(rows.size()));
}

void SettingsDialog::filter()
{
    const bool heldOnly = m_heldOnly->isChecked();
    const bool found = filterRows(*m_tree, PropertyColumn, m_filter->text().trimmed(),
                                  [heldOnly](const QTreeWidgetItem& row) { return !heldOnly || isChecked(row); });
    m_properties->placeholder()->setText(whyNoProperty());
    m_properties->showPageIf(found);
}

QString SettingsDialog::whyNoProperty() const
{
    if(m_tree->topLevelItemCount() == 0)
    {
        return tr("None of the instances has a property a settings file can hold");
    }
    // Only those held are shown, then.
    return m_filter->text().trimmed().isEmpty() ? tr("The file holds no property yet") : tr("No property found");
}

void SettingsDialog::apply()
{
    m_file.setName(m_fileName->text().trimmed());
    m_file.setDirectory(m_directory->text().trimmed());
    // In the order of the tree: the one the file holds them in.
    for(QTreeWidgetItemIterator row(m_tree); *row != nullptr; ++row)
    {
        applyRow(**row);
    }
}

void SettingsDialog::applyRow(const QTreeWidgetItem& row)
{
    const auto entry = m_entries.constFind(&row);
    if(entry != m_entries.cend() && row.checkState(PropertyColumn) == Qt::Checked)
    {
        m_file.add(*entry); // Nothing when it holds it already
    }
    else if(entry != m_entries.cend())
    {
        m_file.remove(*entry);
    }
}
