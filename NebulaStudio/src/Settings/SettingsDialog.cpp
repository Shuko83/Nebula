// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "SettingsDialog.h"

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
#include <cstdint>
#include <functional>
#include <memory>

#include "Nebula/Application/Universe.h"
#include "PropertyTree.h"
#include "Theme.h"

namespace
{

enum Column : std::uint8_t
{
    PropertyColumn,
    ValueColumn,
    FileColumn,
};

// The name of file, telling in its tooltip what it is in a galaxy.
QLineEdit* newNameEdit(const Nebula::SettingsFile& file)
{
    auto* edit = new QLineEdit(file.name());
    if(file.galaxy() != nullptr)
    {
        edit->setToolTip(SettingsDialog::tr("The name the file of the galaxy gives it: each instance of the galaxy "
                                            "writes its own, named after it, %1 for this one")
                             .arg(file.fileName()));
    }
    return edit;
}

// The directory of a file, directory, telling what an empty or a relative one is.
QLineEdit* newDirectoryEdit(const QString& directory)
{
    auto* edit = new QLineEdit(directory);
    edit->setPlaceholderText(SettingsDialog::tr("Configuration directory of the user"));
    edit->setToolTip(
        SettingsDialog::tr("Empty: the configuration directory of the user, for the application that runs\n"
                           "Relative: from the directory of the application that runs"));
    return edit;
}

// Where a file is written.
QLabel* newPathLabel()
{
    auto* label = new QLabel;
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setToolTip(SettingsDialog::tr("Where the application writes it when it runs here, in NebulaStudio"));
    return label;
}

// Disables row, a property the file named holder holds, which it tells.
void disableFor(QTreeWidgetItem& row, const QString& holder)
{
    row.setFlags(Qt::NoItemFlags);
    row.setText(FileColumn, holder);
}

// Whether row is of a property a file holds: the one of the dialog, or another one.
bool isHeld(const QTreeWidgetItem& row)
{
    return row.checkState(PropertyColumn) == Qt::Checked || !row.text(FileColumn).isEmpty();
}

} // namespace

SettingsDialog::SettingsDialog(Nebula::SettingsFile& file, const Nebula::Universe& application,
                               const Nebula::ComponentFactory& factory, QWidget* parent)
    : EditDialog(file, parent), m_file(file), m_application(application),
      m_tree(newChoiceTree({tr("Property"), tr("Value"), tr("Held by")}))
{
    setWindowTitle(tr("Settings file"));
    setWindowIcon(icon(GlossaryTerm::Comet));
    createFields();
    fill(application, factory);
    layOut();
    refreshPath();
    resize(560, 520);
}

void SettingsDialog::createFields()
{
    m_fileName = newNameEdit(m_file);
    m_directory = newDirectoryEdit(m_file.directory());
    m_path = newPathLabel();
    connect(m_fileName, &QLineEdit::textChanged, this, &SettingsDialog::refreshPath);
    connect(m_directory, &QLineEdit::textChanged, this, &SettingsDialog::refreshPath);
}

void SettingsDialog::layOut()
{
    auto* layout = new QVBoxLayout(this);
    layout->addLayout(newForm());
    layout->addWidget(newHint(tr("The properties checked keep the values the user of the application gives them, "
                                 "from one run to the next. The values of the project are their default values.")));
    layout->addWidget(m_tree, 1);
    layout->addWidget(buttons());
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

void SettingsDialog::fill(const Nebula::Universe& application, const Nebula::ComponentFactory& factory)
{
    // Those of its galaxy: it holds no other.
    for(const std::unique_ptr<Nebula::Component>& component : application.components(m_file.galaxy()))
    {
        addIfFilled(*m_tree, componentRow(*component, factory));
    }
    openAbove(*m_tree, isHeld);
}

QTreeWidgetItem* SettingsDialog::componentRow(Nebula::Component& component, const Nebula::ComponentFactory& factory)
{
    QTreeWidgetItem* row = groupRow(component.objectName());
    row->setToolTip(PropertyColumn, component.componentId());
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

void SettingsDialog::refreshPath()
{
    const QString refusal = nameRefusal();
    const QString filePath = Nebula::SettingsFile::filePath(fileName(), m_directory->text().trimmed());
    m_path->setText(refusal.isEmpty() ? QDir::toNativeSeparators(filePath) : refusal);
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
