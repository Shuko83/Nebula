// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "PropertyEditor.h"

#include <QEvent>
#include <QHeaderView>
#include <QLabel>
#include <QMetaMethod>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStringList>
#include <QTableWidget>
#include <QVBoxLayout>
#include <cstdint>
#include <optional>
#include <utility>

#include "Nebula/Application/Universe.h"
#include "Nebula/Component/Component.h"
#include "Nebula/Component/ComponentFactory.h"
#include "Theme.h"

namespace
{

enum Column : std::uint8_t
{
    NameColumn,
    ValueColumn,
    ColumnCount,
};

// Whether a cell edits property: it can be written, and its type is one the default editors of Qt take.
bool isEditable(const QMetaProperty& property)
{
    if(!property.isWritable())
    {
        return false;
    }
    switch(property.typeId())
    {
    case QMetaType::Bool:
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::Double:
    case QMetaType::QString:
    case QMetaType::QDate:
    case QMetaType::QTime:
    case QMetaType::QDateTime:
        return true;
    default:
        return false;
    }
}

} // namespace

PropertyEditor::PropertyEditor(const Nebula::Universe& application, const Nebula::ComponentFactory& factory,
                               QWidget* parent)
    : QWidget(parent), m_application(application), m_factory(factory)
{
    // Which properties are in italics, which ones a stargate exposes, and whether they are edited.
    connect(&m_application, &Nebula::Universe::settingsFilesChanged, this, &PropertyEditor::rebuild);
    connect(&m_application, &Nebula::Universe::componentsChanged, this, &PropertyEditor::rebuild);
    connect(&m_application, &Nebula::Universe::runningChanged, this, &PropertyEditor::rebuild);

    m_placeholder = new QLabel;
    m_placeholder->setAlignment(Qt::AlignCenter);

    m_table = new QTableWidget(0, ColumnCount);
    m_table->setHorizontalHeaderLabels({tr("Property"), tr("Value")});
    m_table->verticalHeader()->hide();
    m_table->horizontalHeader()->setSectionResizeMode(NameColumn, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_table, &QTableWidget::itemChanged, this, &PropertyEditor::write);

    m_stack = new QStackedWidget;
    m_stack->addWidget(m_placeholder);
    m_stack->addWidget(m_table);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->addWidget(m_stack);

    setComponent(nullptr);
}

void PropertyEditor::setComponent(Nebula::Component* component)
{
    if(m_component != nullptr)
    {
        m_component->disconnect(this);
    }
    m_component = component;
    if(component != nullptr)
    {
        connect(component, &QObject::objectNameChanged, this, &PropertyEditor::refreshPlaceholder);
        connect(component, &Nebula::Component::propertyObjectsChanged, this, &PropertyEditor::rebuild);
    }
    rebuild();
}

void PropertyEditor::setReadOnly(bool readOnly)
{
    if(readOnly != m_readOnly)
    {
        m_readOnly = readOnly;
        rebuild();
    }
}

void PropertyEditor::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if(event->type() != QEvent::PaletteChange)
    {
        return;
    }
    const QSignalBlocker blocker(m_table); // itemChanged is for the values edited in the table
    for(int row = 0; row < m_table->rowCount(); ++row)
    {
        if(!m_rows.at(row).property.isValid())
        {
            m_table->item(row, NameColumn)->setBackground(palette().alternateBase());
        }
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

void PropertyEditor::rebuild()
{
    // The objects shown until now are no longer listened to. Not the component, should it still be the one shown:
    // more than its properties is followed.
    for(const Row& shown : std::as_const(m_rows))
    {
        if(shown.object != nullptr && shown.object != m_component)
        {
            shown.object->disconnect(this);
        }
    }
    m_rows.clear();
    // What the stargates of the application expose, which the names tell.
    for(const std::unique_ptr<Nebula::Component>& component : m_application.components())
    {
        if(const auto* stargate = qobject_cast<const Nebula::Stargate*>(component.get()))
        {
            connect(stargate, &Nebula::Stargate::changed, this, &PropertyEditor::rebuild, Qt::UniqueConnection);
        }
    }

    // Only what can be edited here: the other properties of a component are in the viewer of its component.
    const auto append = [this](QObject* object, const QList<QMetaProperty>& properties)
    {
        for(const QMetaProperty& property : properties)
        {
            if(isEditable(property))
            {
                m_rows.append(
                    Row{.object = object, .property = property, .title = {}, .name = {}, .component = m_component});
            }
        }
    };
    if(const auto* galaxy = qobject_cast<const Nebula::Galaxy*>(m_component))
    {
        // What its stargates expose, under the names they expose them as.
        for(const Nebula::ExposedProperty& exposed : galaxy->exposedProperties())
        {
            const QMetaObject* metaObject = exposed.object->metaObject();
            const QMetaProperty property =
                metaObject->property(metaObject->indexOfProperty(exposed.property.toLatin1().constData()));
            if(isEditable(property))
            {
                m_rows.append(Row{.object = exposed.object,
                                  .property = property,
                                  .title = {},
                                  .name = exposed.name,
                                  .component = exposed.component});
            }
        }
    }
    else if(m_component != nullptr)
    {
        if(const std::optional<Nebula::ComponentInfo> info = m_factory.component(m_component->componentId()))
        {
            append(m_component, info->properties);
        }
        const QList<Nebula::PropertyObject> objects = m_component->propertyObjects();
        // No name first, which follows the properties of the component, then each name once, where it first came.
        QStringList names{QString()};
        for(const Nebula::PropertyObject& added : objects)
        {
            if(!names.contains(added.name))
            {
                names.append(added.name);
            }
        }
        for(const QString& name : std::as_const(names))
        {
            const qsizetype title = m_rows.size();
            for(const Nebula::PropertyObject& added : objects)
            {
                if(added.name == name)
                {
                    append(added.object, added.properties());
                }
            }
            // No title for a group with nothing to edit.
            if(!name.isEmpty() && m_rows.size() > title)
            {
                m_rows.insert(title,
                              Row{.object = nullptr, .property = {}, .title = name, .name = {}, .component = {}});
            }
        }
    }

    const QSignalBlocker blocker(m_table); // itemChanged is for the values edited in the table
    m_table->setRowCount(0);               // Closes the editor of a value shown until now
    // A click on a value edits it, while the application is edited.
    m_table->setEditTriggers(m_readOnly || m_application.isRunning() ? QAbstractItemView::NoEditTriggers
                                                                     : QAbstractItemView::AllEditTriggers);
    m_table->clearSpans();
    m_table->setRowCount(static_cast<int>(m_rows.size()));
    const QMetaMethod refresh = metaObject()->method(metaObject()->indexOfSlot("refreshValues()"));
    for(int row = 0; row < m_table->rowCount(); ++row)
    {
        const Row& shown = m_rows.at(row);
        if(!shown.property.isValid())
        {
            auto* title = new QTableWidgetItem(shown.title);
            title->setFlags(Qt::ItemIsEnabled);
            title->setFont(titleFont(title->font()));
            title->setBackground(palette().alternateBase()); // Again when the palette changes
            m_table->setItem(row, NameColumn, title);
            m_table->setSpan(row, NameColumn, 1, ColumnCount);
            continue;
        }

        const QString propertyName = QString::fromLatin1(shown.property.name());
        auto* name = new QTableWidgetItem(shown.name.isEmpty() ? propertyName : shown.name);
        name->setFlags(Qt::ItemIsEnabled);
        name->setToolTip(QString::fromLatin1(shown.property.typeName()));
        if(!shown.name.isEmpty() && shown.component != nullptr)
        {
            // Where it is in the galaxy.
            name->setToolTip(tr("%1\nThe property %2 of %3 in the galaxy")
                                 .arg(name->toolTip(), propertyName, shown.component->objectName()));
        }
        const Nebula::SettingsEntry entry{
            .component = shown.component, .object = shown.object, .property = propertyName};
        if(const Nebula::SettingsFile* file = m_application.settingsFile(entry))
        {
            QFont font = name->font();
            font.setItalic(true);
            name->setFont(font);
            name->setToolTip(tr("%1\nDefault value: the user keeps the one they give it in %2")
                                 .arg(name->toolTip(), file->fileName()));
        }
        if(const auto [stargate, exposedAs] = exposedBy(shown); stargate != nullptr)
        {
            name->setIcon(icon(GlossaryTerm::Stargate));
            name->setToolTip(tr("%1\nExposed by %2 as %3: each instance of the galaxy of the project sets it")
                                 .arg(name->toolTip(), stargate->objectName(), exposedAs));
        }
        m_table->setItem(row, NameColumn, name);
        m_table->setItem(row, ValueColumn, new QTableWidgetItem); // Editable, as an item is by default

        if(shown.property.hasNotifySignal())
        {
            connect(shown.object, shown.property.notifySignal(), this, refresh, Qt::UniqueConnection);
        }
    }
    refreshValues();

    refreshPlaceholder();
    m_stack->setCurrentWidget(m_rows.isEmpty() ? static_cast<QWidget*>(m_placeholder) : m_table);
}

void PropertyEditor::refreshPlaceholder()
{
    if(m_component == nullptr)
    {
        m_placeholder->setText(tr("Select a component of the application to edit its properties"));
    }
    else if(qobject_cast<const Nebula::Galaxy*>(m_component) != nullptr)
    {
        m_placeholder->setText(tr("%1 is a galaxy that exposes no property: double-click it to see its components")
                                   .arg(m_component->objectName()));
    }
    else if(qobject_cast<const Nebula::Stargate*>(m_component) != nullptr)
    {
        m_placeholder->setText(tr("%1 is a stargate: double-click it to choose what the galaxy of the project exposes "
                                  "through it")
                                   .arg(m_component->objectName()));
    }
    else
    {
        m_placeholder->setText(tr("%1 has no editable property").arg(m_component->objectName()));
    }
}

std::pair<const Nebula::Stargate*, QString> PropertyEditor::exposedBy(const Row& row) const
{
    for(const std::unique_ptr<Nebula::Component>& component : m_application.components())
    {
        const auto* stargate = qobject_cast<const Nebula::Stargate*>(component.get());
        if(stargate == nullptr)
        {
            continue;
        }
        for(const Nebula::ExposedProperty& exposed : stargate->properties())
        {
            if(exposed.component == row.component && exposed.object == row.object &&
               exposed.property == QLatin1StringView(row.property.name()))
            {
                return {stargate, exposed.name};
            }
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
