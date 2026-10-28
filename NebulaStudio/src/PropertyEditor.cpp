// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "PropertyEditor.h"

#include <QEvent>
#include <QHeaderView>
#include <QLabel>
#include <QMetaMethod>
#include <QSignalBlocker>
#include <QStringList>
#include <QTableWidget>
#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <utility>

#include "Nebula/Application/Universe.h"
#include "Nebula/Component/Component.h"
#include "Nebula/Component/ComponentFactory.h"
#include "PlaceholderStack.h"
#include "PropertyTree.h"
#include "Theme.h"

namespace
{

enum Column : std::uint8_t
{
    NameColumn,
    ValueColumn,
    ColumnCount,
};

// The types the default editors of Qt take.
constexpr std::array EditableTypes{QMetaType::Bool,    QMetaType::Int,   QMetaType::UInt,  QMetaType::Double,
                                   QMetaType::QString, QMetaType::QDate, QMetaType::QTime, QMetaType::QDateTime};

// Whether a cell edits property: it can be written, and its type is one the default editors of Qt take.
bool isEditable(const QMetaProperty& property)
{
    return property.isWritable() &&
           std::ranges::find(EditableTypes, static_cast<QMetaType::Type>(property.typeId())) != EditableTypes.end();
}

// The table of the properties: their name, and their value.
QTableWidget* newPropertyTable()
{
    auto* table = new QTableWidget(0, ColumnCount);
    table->setHorizontalHeaderLabels({PropertyEditor::tr("Property"), PropertyEditor::tr("Value")});
    table->verticalHeader()->hide();
    table->horizontalHeader()->setSectionResizeMode(NameColumn, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    return table;
}

// Why component shows no property, and what to do instead, %1 standing for its name.
QString nothingToEdit(const Nebula::Component& component)
{
    if(qobject_cast<const Nebula::Galaxy*>(&component) != nullptr)
    {
        return PropertyEditor::tr("%1 is a galaxy that exposes no property: double-click it to open it");
    }
    if(qobject_cast<const Nebula::Stargate*>(&component) != nullptr)
    {
        return PropertyEditor::tr("%1 is a stargate: double-click it to choose what the galaxy of the project exposes "
                                  "through it");
    }
    return PropertyEditor::tr("%1 has no editable property");
}

// font, in italics.
QFont italic(QFont font)
{
    font.setItalic(true);
    return font;
}

} // namespace

PropertyEditor::PropertyEditor(const Nebula::Universe& application, const Nebula::ComponentFactory& factory,
                               QWidget* parent)
    : QWidget(parent), m_application(application), m_factory(factory), m_table(newPropertyTable()),
      m_stack(new PlaceholderStack(m_table, *this))
{
    // Which properties are in italics, which ones a stargate exposes, and whether they are edited.
    connect(&m_application, &Nebula::Universe::settingsFilesChanged, this, &PropertyEditor::rebuild);
    connect(&m_application, &Nebula::Universe::componentsChanged, this, &PropertyEditor::rebuild);
    connect(&m_application, &Nebula::Universe::runningChanged, this, &PropertyEditor::rebuild);
    connect(m_table, &QTableWidget::itemChanged, this, &PropertyEditor::write);
    setComponent(nullptr);
}

void PropertyEditor::setComponent(Nebula::Component* component)
{
    if(m_component != nullptr)
    {
        m_component->disconnect(this);
    }
    m_component = component;
    follow(component);
    rebuild();
}

void PropertyEditor::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if(event->type() == QEvent::PaletteChange)
    {
        colorTitles();
    }
}

void PropertyEditor::refreshValues()
{
    const QSignalBlocker blocker(m_table);
    for(int row = 0; row < m_table->rowCount(); ++row)
    {
        const Row& shown = m_rows.at(row);
        if(shown.object != nullptr && shown.property.isValid())
        {
            // The value itself rather than its text: its type is what gives the cell its editor.
            m_table->item(row, ValueColumn)->setData(Qt::EditRole, shown.property.read(shown.object));
        }
    }
}

void PropertyEditor::follow(Nebula::Component* component)
{
    // What a galaxy exposes is of its components, which are others once it is read again.
    if(auto* galaxy = qobject_cast<Nebula::Galaxy*>(component))
    {
        connect(galaxy, &Nebula::Galaxy::reloaded, this, &PropertyEditor::rebuild);
    }
    if(component != nullptr)
    {
        connect(component, &QObject::objectNameChanged, this, &PropertyEditor::refreshPlaceholder);
        connect(component, &Nebula::Component::propertyObjectsChanged, this, &PropertyEditor::rebuild);
    }
}

void PropertyEditor::colorTitles()
{
    const QSignalBlocker blocker(m_table); // itemChanged is for the values edited in the table
    for(int row = 0; row < m_table->rowCount(); ++row)
    {
        if(!m_rows.at(row).property.isValid())
        {
            m_table->item(row, NameColumn)->setBackground(palette().alternateBase());
        }
    }
}

void PropertyEditor::rebuild()
{
    forgetRows();
    followStargates();
    m_rows = componentRows();
    showRows();
    refreshPlaceholder();
    m_stack->showPageIf(!m_rows.isEmpty());
}

void PropertyEditor::forgetRows()
{
    for(const Row& shown : std::as_const(m_rows))
    {
        if(shown.object != nullptr && shown.object != m_component)
        {
            shown.object->disconnect(this);
        }
    }
    m_rows.clear();
}

void PropertyEditor::followStargates()
{
    for(const Nebula::Stargate* stargate : stargates())
    {
        connect(stargate, &Nebula::Stargate::changed, this, &PropertyEditor::rebuild, Qt::UniqueConnection);
    }
}

QList<PropertyEditor::Row> PropertyEditor::componentRows() const
{
    if(const auto* galaxy = qobject_cast<const Nebula::Galaxy*>(m_component))
    {
        return exposedRows(*galaxy);
    }
    return m_component != nullptr ? propertyRows(*m_component) : QList<Row>();
}

QList<PropertyEditor::Row> PropertyEditor::exposedRows(const Nebula::Galaxy& galaxy)
{
    QList<Row> rows;
    for(const Nebula::ExposedProperty& exposed : galaxy.exposedProperties())
    {
        rows.append(Row{.object = exposed.object,
                        .property = metaPropertyOf(*exposed.object, exposed.property),
                        .name = exposed.name,
                        .component = exposed.component});
    }
    rows.removeIf([](const Row& row) { return !isEditable(row.property); });
    return rows;
}

QList<PropertyEditor::Row> PropertyEditor::propertyRows(Nebula::Component& component) const
{
    QList<Row> rows = editableRows(&component, ownProperties(m_factory, component), component);
    const QList<Nebula::PropertyObject> objects = component.propertyObjects();
    rows.append(objectRows(objects, QString(), component)); // With its own
    for(const QString& name : groupNames(objects))
    {
        rows.append(groupRows(objects, name, component));
    }
    return rows;
}

QList<PropertyEditor::Row> PropertyEditor::groupRows(const QList<Nebula::PropertyObject>& objects, const QString& name,
                                                     Nebula::Component& component)
{
    QList<Row> rows = objectRows(objects, name, component);
    if(!rows.isEmpty()) // No title for a group with nothing to edit
    {
        rows.prepend(Row{.object = nullptr, .property = {}, .title = name});
    }
    return rows;
}

QList<PropertyEditor::Row> PropertyEditor::objectRows(const QList<Nebula::PropertyObject>& objects, const QString& name,
                                                      Nebula::Component& component)
{
    QList<Row> rows;
    for(const Nebula::PropertyObject& added : objects)
    {
        if(added.name == name)
        {
            rows.append(editableRows(added.object, added.properties(), component));
        }
    }
    return rows;
}

QList<PropertyEditor::Row> PropertyEditor::editableRows(QObject* object, const QList<QMetaProperty>& properties,
                                                        Nebula::Component& component)
{
    QList<Row> rows;
    for(const QMetaProperty& property : properties)
    {
        if(isEditable(property))
        {
            rows.append(Row{.object = object, .property = property, .component = &component});
        }
    }
    return rows;
}

void PropertyEditor::showRows()
{
    const QSignalBlocker blocker(m_table); // itemChanged is for the values edited in the table
    clearTable();
    m_table->setRowCount(static_cast<int>(m_rows.size()));
    for(int row = 0; row < m_table->rowCount(); ++row)
    {
        showRow(row);
    }
    refreshValues();
}

void PropertyEditor::clearTable()
{
    m_table->setRowCount(0); // Closes the editor of a value shown until now
    // A click on a value edits it, while the application is edited.
    m_table->setEditTriggers(m_application.isRunning() ? QAbstractItemView::NoEditTriggers
                                                       : QAbstractItemView::AllEditTriggers);
    m_table->clearSpans();
}

void PropertyEditor::showRow(int row)
{
    const Row& shown = m_rows.at(row);
    if(!shown.property.isValid())
    {
        showTitle(row, shown.title);
        return;
    }
    m_table->setItem(row, NameColumn, nameItem(shown));
    m_table->setItem(row, ValueColumn, new QTableWidgetItem); // Editable, as an item is by default
    followValue(shown);
}

void PropertyEditor::showTitle(int row, const QString& title)
{
    auto* item = new QTableWidgetItem(title);
    item->setFlags(Qt::ItemIsEnabled);
    item->setFont(titleFont(item->font()));
    item->setBackground(palette().alternateBase()); // Again when the palette changes
    m_table->setItem(row, NameColumn, item);
    m_table->setSpan(row, NameColumn, 1, ColumnCount);
}

QTableWidgetItem* PropertyEditor::nameItem(const Row& shown) const
{
    const QString propertyName = QString::fromLatin1(shown.property.name());
    auto* name = new QTableWidgetItem(shown.name.isEmpty() ? propertyName : shown.name);
    name->setFlags(Qt::ItemIsEnabled);
    name->setToolTip(QString::fromLatin1(shown.property.typeName()));
    tellPlace(*name, shown);
    tellHolder(*name, shown);
    tellExposure(*name, shown);
    return name;
}

void PropertyEditor::tellPlace(QTableWidgetItem& item, const Row& shown)
{
    if(!shown.name.isEmpty() && shown.component != nullptr)
    {
        item.setToolTip(
            tr("%1\nThe property %2 of %3 in the galaxy")
                .arg(item.toolTip(), QString::fromLatin1(shown.property.name()), shown.component->objectName()));
    }
}

void PropertyEditor::tellHolder(QTableWidgetItem& item, const Row& shown) const
{
    const Nebula::SettingsFile* file = m_application.settingsFile(Nebula::SettingsEntry{
        .component = shown.component, .object = shown.object, .property = QString::fromLatin1(shown.property.name())});
    if(file != nullptr)
    {
        item.setFont(italic(item.font()));
        item.setToolTip(
            tr("%1\nDefault value: the user keeps the one they give it in %2").arg(item.toolTip(), file->fileName()));
    }
}

void PropertyEditor::tellExposure(QTableWidgetItem& item, const Row& shown) const
{
    const auto [stargate, exposedAs] = exposedBy(shown);
    if(stargate != nullptr)
    {
        item.setIcon(icon(GlossaryTerm::Stargate));
        item.setToolTip(tr("%1\nExposed by %2 as %3: each instance of the galaxy of the project sets it")
                            .arg(item.toolTip(), stargate->objectName(), exposedAs));
    }
}

void PropertyEditor::followValue(const Row& shown)
{
    if(!shown.property.hasNotifySignal())
    {
        return;
    }
    const QMetaMethod refresh = metaObject()->method(metaObject()->indexOfSlot("refreshValues()"));
    connect(shown.object, shown.property.notifySignal(), this, refresh, Qt::UniqueConnection);
}

void PropertyEditor::refreshPlaceholder()
{
    m_stack->placeholder()->setText(m_component == nullptr
                                        ? tr("Select a component of the application to edit its properties")
                                        : nothingToEdit(*m_component).arg(m_component->objectName()));
}

QList<const Nebula::Stargate*> PropertyEditor::stargates() const
{
    if(m_component == nullptr)
    {
        return {};
    }
    return stargatesAmong(m_application.components(m_application.galaxyOf(*m_component)));
}

std::pair<const Nebula::Stargate*, QString> PropertyEditor::exposedBy(const Row& row) const
{
    const Nebula::ExposedProperty target{
        .component = row.component, .object = row.object, .property = QString::fromLatin1(row.property.name())};
    for(const Nebula::Stargate* stargate : stargates())
    {
        if(const std::optional<QString> name = exposedNameOf(*stargate, target))
        {
            return {stargate, *name};
        }
    }
    return {nullptr, {}};
}

void PropertyEditor::write(const QTableWidgetItem* item)
{
    const Row& edited = m_rows.at(item->row());
    if(edited.object == nullptr || item->column() != ValueColumn)
    {
        return;
    }
    edited.property.write(edited.object, item->data(Qt::EditRole));
    refreshValues(); // What the object made of it: it can adjust a value, or refuse it
}
