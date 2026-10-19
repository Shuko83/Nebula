// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "StargateDialog.h"

#include <QDialogButtonBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMetaProperty>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStringList>
#include <QStyle>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>
#include <memory>
#include <optional>

#include "InterfaceItem.h"
#include "Nebula/Application/Universe.h"
#include "Nebula/Component/ComponentFactory.h"
#include "Theme.h"

namespace
{

enum Column : std::uint8_t
{
    ExposesColumn,
    TypeColumn,
    NameColumn,
    ByColumn,
    ColumnCount,
};

// The stargates of application.
QList<const Nebula::Stargate*> stargatesOf(const Nebula::Universe& application)
{
    QList<const Nebula::Stargate*> stargates;
    for(const std::unique_ptr<Nebula::Component>& component : application.components())
    {
        if(const auto* stargate = qobject_cast<const Nebula::Stargate*>(component.get()))
        {
            stargates.append(stargate);
        }
    }
    return stargates;
}

} // namespace

StargateDialog::StargateDialog(Nebula::Stargate& stargate, const Nebula::Universe& application,
                               const Nebula::ComponentFactory& factory, QWidget* parent)
    : QDialog(parent), m_stargate(stargate), m_application(application)
{
    setWindowTitle(tr("Stargate %1").arg(stargate.objectName()));
    setWindowIcon(icon(GlossaryTerm::Stargate));

    auto* hint = new QLabel(
        tr("What the galaxy of this project has through %1 once it is placed in another one: the interfaces and the "
           "properties checked, each under the name next to it. Double-clicking one checks it and edits that name. "
           "What the galaxy provides, one of its components provides; what it consumes, it gives to those of its "
           "components that consume it, the same pointer to each of those checked under the same name.")
            .arg(stargate.objectName()));
    hint->setWordWrap(true);

    m_tree = new QTreeWidget;
    m_tree->setColumnCount(ColumnCount);
    m_tree->setHeaderLabels({tr("Interface or property"), tr("Type"), tr("Exposed as"), tr("Exposed by")});
    m_tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers); // Its name only, below
    fill(factory);
    connect(m_tree, &QTreeWidget::itemChanged, this, &StargateDialog::edited);
    // Exposed at once when it is not yet, under its name, which is edited then.
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this,
            [this](QTreeWidgetItem* row, int /*column*/)
            {
                if(!m_choices.contains(row))
                {
                    return;
                }
                row->setCheckState(ExposesColumn, Qt::Checked); // Named by edited() when it was not
                m_tree->editItem(row, NameColumn);
            });

    auto* problemIcon = new QLabel;
    problemIcon->setPixmap(
        style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(style()->pixelMetric(QStyle::PM_SmallIconSize)));
    m_problemText = new QLabel;
    m_problemText->setWordWrap(true);
    m_problem = new QWidget;
    auto* problemLayout = new QHBoxLayout(m_problem);
    problemLayout->setContentsMargins({});
    problemLayout->addWidget(problemIcon, 0, Qt::AlignTop);
    problemLayout->addWidget(m_problemText, 1);
    colorProblem();

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(this, &QDialog::accepted, this, &StargateDialog::apply);
    // Should the stargate go meanwhile, with what it would have been given.
    connect(&m_stargate, &QObject::destroyed, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(hint);
    layout->addWidget(m_tree, 1);
    layout->addWidget(m_problem);
    layout->addWidget(m_buttons);
    check();
    resize(680, 560);
}

void StargateDialog::changeEvent(QEvent* event)
{
    QDialog::changeEvent(event);
    if(event->type() == QEvent::PaletteChange)
    {
        colorProblem();
    }
}

void StargateDialog::fill(const Nebula::ComponentFactory& factory)
{
    const QList<const Nebula::Stargate*> stargates = stargatesOf(m_application);
    QList<std::pair<Nebula::Exposure, QString>> shown; // What the stargate exposes that a row tells
    for(const std::unique_ptr<Nebula::Component>& owned : m_application.components())
    {
        Nebula::Component* component = owned.get();
        if(qobject_cast<const Nebula::Stargate*>(component) != nullptr)
        {
            continue; // Bound to one another, not exposed
        }
        const auto* galaxy = qobject_cast<const Nebula::Galaxy*>(component);
        auto* componentRow = new QTreeWidgetItem({component->objectName()});
        componentRow->setFlags(Qt::ItemIsEnabled);
        componentRow->setToolTip(ExposesColumn, galaxy != nullptr ? galaxy->filePath() : component->componentId());

        // Its interfaces, those it consumes then those it provides, as its item shows them.
        const QList<Nebula::InterfaceInfo> consumed = component->consumedInterfaces();
        for(const Nebula::InterfaceInfo& info : consumed)
        {
            const Nebula::InterfaceBinding bound = component->binding(info.name);
            const auto* through = qobject_cast<const Nebula::Stargate*>(bound.provider);
            QString exposedAs;
            QString exposedBy;
            if(through == &m_stargate)
            {
                exposedAs = bound.providedName;
                shown.append({Nebula::Exposure::Consumed, bound.providedName});
            }
            else if(through != nullptr)
            {
                exposedBy = through->objectName();
            }
            addChoice(componentRow, info.name, tr("consumes %1").arg(InterfaceItem::typeName(info.type)),
                      Choice{.kind = Kind::Consumed, .component = component, .name = info.name, .type = info.type},
                      exposedAs, exposedBy);
            QTreeWidgetItem* row = componentRow->child(componentRow->childCount() - 1);
            if(bound.provider != nullptr && through == nullptr)
            {
                row->setToolTip(ExposesColumn,
                                tr("Bound to %1.%2: exposed, it is given what the galaxy consumes instead")
                                    .arg(bound.provider->objectName(), bound.providedName));
            }
        }
        const QList<Nebula::InterfaceInfo> provided = component->providedInterfaces();
        for(const Nebula::InterfaceInfo& info : provided)
        {
            // The stargate that consumes it, when one does.
            QString exposedAs;
            QString exposedBy;
            for(const Nebula::Stargate* stargate : stargates)
            {
                const QList<Nebula::InterfaceInfo> passed = stargate->consumedInterfaces();
                for(const Nebula::InterfaceInfo& pass : passed)
                {
                    const Nebula::InterfaceBinding bound = stargate->binding(pass.name);
                    if(bound.provider != component || bound.providedName != info.name)
                    {
                        continue;
                    }
                    if(stargate == &m_stargate)
                    {
                        exposedAs = pass.name;
                        shown.append({Nebula::Exposure::Provided, pass.name});
                    }
                    else
                    {
                        exposedBy = stargate->objectName();
                    }
                }
            }
            addChoice(componentRow, info.name, tr("provides %1").arg(InterfaceItem::typeName(info.type)),
                      Choice{.kind = Kind::Provided, .component = component, .name = info.name, .type = info.type},
                      exposedAs, exposedBy);
        }

        // Its properties, under parent, those of object among properties that a stargate can expose.
        const auto list = [this, &stargates, component](QTreeWidgetItem* parent, QObject* object, const QString& name,
                                                        const QString& type)
        {
            const Nebula::ExposedProperty target{
                .name = {}, .component = component, .object = object, .property = name};
            QString exposedAs;
            QString exposedBy;
            for(const Nebula::Stargate* stargate : stargates)
            {
                for(const Nebula::ExposedProperty& exposed : stargate->properties())
                {
                    if(exposed.component != target.component || exposed.object != target.object ||
                       exposed.property != target.property)
                    {
                        continue;
                    }
                    if(stargate == &m_stargate)
                    {
                        exposedAs = exposed.name;
                    }
                    else
                    {
                        exposedBy = stargate->objectName();
                    }
                }
            }
            addChoice(parent, name, type,
                      Choice{.kind = Kind::Property, .component = component, .object = object, .name = name}, exposedAs,
                      exposedBy);
        };
        const auto listAll =
            [&list, component](QTreeWidgetItem* parent, QObject* object, const QList<QMetaProperty>& properties)
        {
            for(const QMetaProperty& property : properties)
            {
                const QString name = QString::fromLatin1(property.name());
                if(Nebula::SettingsFile::canHold(*component, object, name)) // What a project holds
                {
                    list(parent, object, name, QString::fromLatin1(property.typeName()));
                }
            }
        };
        if(galaxy != nullptr)
        {
            // Those it exposes, which a stargate of its own exposes again.
            for(const Nebula::ExposedProperty& exposed : galaxy->exposedProperties())
            {
                const QMetaObject* metaObject = exposed.object->metaObject();
                const QMetaProperty property =
                    metaObject->property(metaObject->indexOfProperty(exposed.property.toLatin1().constData()));
                list(componentRow, component, exposed.name, QString::fromLatin1(property.typeName()));
            }
        }
        else if(const std::optional<Nebula::ComponentInfo> info = factory.component(component->componentId()))
        {
            listAll(componentRow, component, info->properties);
        }
        // Then those of its objects, as the property editor shows them: with its own when they have no name, under a
        // row of their name otherwise.
        const QList<Nebula::PropertyObject> objects = component->propertyObjects();
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
            auto* group = name.isEmpty() ? componentRow : new QTreeWidgetItem({name});
            group->setFlags(Qt::ItemIsEnabled);
            for(const Nebula::PropertyObject& added : objects)
            {
                if(added.name == name)
                {
                    listAll(group, added.object, added.properties());
                }
            }
            if(group != componentRow)
            {
                if(group->childCount() > 0)
                {
                    componentRow->addChild(group);
                }
                else
                {
                    delete group; // Nothing to expose
                }
            }
        }

        if(componentRow->childCount() > 0)
        {
            m_tree->addTopLevelItem(componentRow);
            componentRow->setExpanded(true);
        }
        else
        {
            delete componentRow;
        }
    }

    // What it exposes that none of the rows tells: bound to nothing, as the component it was bound to went.
    auto* keptRow = new QTreeWidgetItem({tr("Bound to none of these components")});
    keptRow->setFlags(Qt::ItemIsEnabled);
    for(const Nebula::ExposedInterface& exposed : m_stargate.interfaces())
    {
        if(shown.contains(std::pair{exposed.exposure, exposed.name}))
        {
            continue;
        }
        const QString type = InterfaceItem::typeName(exposed.type);
        addChoice(keptRow, exposed.name,
                  exposed.exposure == Nebula::Exposure::Provided ? tr("the galaxy provides %1").arg(type)
                                                                 : tr("the galaxy consumes %1").arg(type),
                  Choice{.kind = Kind::Kept, .name = exposed.name, .type = exposed.type, .exposure = exposed.exposure},
                  exposed.name);
    }
    if(keptRow->childCount() > 0)
    {
        m_tree->addTopLevelItem(keptRow);
        keptRow->setExpanded(true);
    }
    else
    {
        delete keptRow;
    }

    // The groups of the objects, which can have many properties, a widget say, are only open when one is exposed.
    for(QTreeWidgetItemIterator row(m_tree); *row != nullptr; ++row)
    {
        const bool exposed = !(*row)->text(NameColumn).isEmpty() || !(*row)->text(ByColumn).isEmpty();
        for(QTreeWidgetItem* parent = (*row)->parent(); exposed && parent != nullptr; parent = parent->parent())
        {
            parent->setExpanded(true);
        }
    }
}

void StargateDialog::addChoice(QTreeWidgetItem* parent, const QString& text, const QString& kind, const Choice& choice,
                               const QString& exposedAs, const QString& exposedBy)
{
    auto* row = new QTreeWidgetItem(parent, {text, kind, exposedAs, exposedBy});
    if(!exposedBy.isEmpty())
    {
        row->setFlags(Qt::NoItemFlags); // A single stargate exposes it
        return;
    }
    row->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEditable);
    row->setCheckState(ExposesColumn, exposedAs.isEmpty() ? Qt::Unchecked : Qt::Checked);
    m_choices.insert(row, choice);
}

QString StargateDialog::defaultName(const QTreeWidgetItem* row) const
{
    const Choice choice = m_choices.value(row);
    const std::optional<Nebula::Exposure> exposure = exposureOf(choice);
    const QList<std::pair<const QTreeWidgetItem*, Choice>> rows = checked();
    // Taken by another stargate, or by another row checked the same way, unless they share it.
    const auto taken = [this, &rows, row, &choice, exposure](const QString& name)
    {
        return isTaken(exposure, name) ||
               std::ranges::any_of(
                   rows,
                   [row, &choice, exposure, &name](const std::pair<const QTreeWidgetItem*, Choice>& other)
                   {
                       return other.first != row && exposureOf(other.second) == exposure &&
                              other.first->text(NameColumn) == name && !canShare(choice, other.second);
                   });
    };
    QString name = choice.name;
    for(int number = 2; taken(name); ++number)
    {
        name = choice.name + QString::number(number);
    }
    return name;
}

void StargateDialog::edited(QTreeWidgetItem* row, int column)
{
    if(!m_choices.contains(row))
    {
        return;
    }
    {
        const QSignalBlocker blocker(m_tree); // What is set here is not edited
        const bool isChecked = row->checkState(ExposesColumn) == Qt::Checked;
        if(column == ExposesColumn && isChecked && row->text(NameColumn).trimmed().isEmpty())
        {
            row->setText(NameColumn, defaultName(row));
        }
        else if(column == ExposesColumn && !isChecked)
        {
            row->setText(NameColumn, {});
        }
        else if(column == NameColumn)
        {
            row->setText(NameColumn, row->text(NameColumn).simplified()); // On one line, whatever was pasted
        }
    }
    m_tree->viewport()->update(); // What the blocker kept from being shown
    check();
}

void StargateDialog::check()
{
    QString problem;
    const QList<std::pair<const QTreeWidgetItem*, Choice>> rows = checked();
    for(qsizetype index = 0; index < rows.size() && problem.isEmpty(); ++index)
    {
        const auto& [row, choice] = rows.at(index);
        const QString name = row->text(NameColumn);
        const std::optional<Nebula::Exposure> exposure = exposureOf(choice);
        if(name.isEmpty())
        {
            problem = tr("%1 needs a name to be exposed as: double-click it to type one").arg(row->text(ExposesColumn));
            continue;
        }
        if(isTaken(exposure, name))
        {
            problem = tr("Another stargate exposes %1 the same way: %2 needs another name")
                          .arg(name, row->text(ExposesColumn));
            continue;
        }
        for(qsizetype next = index + 1; next < rows.size(); ++next)
        {
            const auto& [other, otherChoice] = rows.at(next);
            if(exposureOf(otherChoice) == exposure && other->text(NameColumn) == name && !canShare(choice, otherChoice))
            {
                problem = tr("%1 and %2 are both exposed as %3: only interfaces the galaxy consumes, of the same type, "
                             "share a name")
                              .arg(row->text(ExposesColumn), other->text(ExposesColumn), name);
                break;
            }
        }
    }
    m_problemText->setText(problem);
    m_problem->setVisible(!problem.isEmpty());
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(problem.isEmpty());
}

void StargateDialog::colorProblem()
{
    QPalette palette = m_problemText->palette();
    palette.setColor(QPalette::WindowText, currentThemeColors().warning);
    m_problemText->setPalette(palette);
}

void StargateDialog::apply()
{
    if(!m_stargate.isChangeable()) // It started meanwhile
    {
        return;
    }
    const QList<std::pair<const QTreeWidgetItem*, Choice>> rows = checked();

    // The properties no longer exposed, or under another name, first: a name can pass from one to another.
    QList<Nebula::ExposedProperty> properties;
    for(const auto& [row, choice] : rows)
    {
        if(choice.kind == Kind::Property && choice.component != nullptr && choice.object != nullptr)
        {
            properties.append({.name = row->text(NameColumn),
                               .component = choice.component,
                               .object = choice.object,
                               .property = choice.name});
        }
    }
    const QList<Nebula::ExposedProperty> exposedProperties = m_stargate.properties();
    for(const Nebula::ExposedProperty& exposed : exposedProperties)
    {
        if(!properties.contains(exposed))
        {
            m_stargate.removeProperty(exposed.name);
        }
    }
    for(const Nebula::ExposedProperty& property : std::as_const(properties))
    {
        if(!m_stargate.properties().contains(property))
        {
            m_stargate.exposeProperty(property);
        }
    }

    // What consumes it, and no longer should under that name, is unbound from it.
    for(auto choice = m_choices.cbegin(); choice != m_choices.cend(); ++choice)
    {
        Nebula::Component* component = choice->component;
        if(choice->kind != Kind::Consumed || component == nullptr)
        {
            continue;
        }
        const Nebula::InterfaceBinding bound = component->binding(choice->name);
        const bool kept = choice.key()->checkState(ExposesColumn) == Qt::Checked &&
                          choice.key()->text(NameColumn) == bound.providedName;
        if(bound.provider == &m_stargate && !kept)
        {
            component->unbindInterface(choice->name);
        }
    }
    // Its interfaces no longer wanted, or wanted of another type, go; then those wanted are exposed, in the order of
    // the tree, and bound.
    const auto wanted = [&rows](const Nebula::ExposedInterface& exposed)
    {
        return std::ranges::any_of(rows,
                                   [&exposed](const std::pair<const QTreeWidgetItem*, Choice>& row)
                                   {
                                       return exposureOf(row.second) == exposed.exposure &&
                                              row.first->text(NameColumn) == exposed.name &&
                                              row.second.type == exposed.type;
                                   });
    };
    const QList<Nebula::ExposedInterface> exposedInterfaces = m_stargate.interfaces();
    for(const Nebula::ExposedInterface& exposed : exposedInterfaces)
    {
        if(!wanted(exposed))
        {
            m_stargate.removeInterface(exposed.exposure, exposed.name);
        }
    }
    for(const auto& [row, choice] : rows)
    {
        const std::optional<Nebula::Exposure> exposure = exposureOf(choice);
        const QString name = row->text(NameColumn);
        if(!exposure)
        {
            continue;
        }
        if(m_stargate.exposedInterface(*exposure, name) == nullptr)
        {
            m_stargate.exposeInterface(*exposure, name, choice.type);
        }
        Nebula::Component* component = choice.component;
        if(component == nullptr)
        {
            continue; // Kept as it is
        }
        if(choice.kind == Kind::Provided)
        {
            const Nebula::InterfaceBinding bound = m_stargate.binding(name);
            if(bound.provider != component || bound.providedName != choice.name)
            {
                m_stargate.bindInterface(name, *component, choice.name);
            }
        }
        else
        {
            const Nebula::InterfaceBinding bound = component->binding(choice.name);
            if(bound.provider != &m_stargate || bound.providedName != name)
            {
                component->bindInterface(choice.name, m_stargate, name);
            }
        }
    }
}

QList<std::pair<const QTreeWidgetItem*, StargateDialog::Choice>> StargateDialog::checked() const
{
    QList<std::pair<const QTreeWidgetItem*, Choice>> rows;
    for(QTreeWidgetItemIterator row(m_tree, QTreeWidgetItemIterator::Checked); *row != nullptr; ++row)
    {
        if(const auto choice = m_choices.constFind(*row); choice != m_choices.cend())
        {
            rows.append({*row, *choice});
        }
    }
    return rows;
}

std::optional<Nebula::Exposure> StargateDialog::exposureOf(const Choice& choice)
{
    switch(choice.kind)
    {
    case Kind::Provided:
        return Nebula::Exposure::Provided;
    case Kind::Consumed:
        return Nebula::Exposure::Consumed;
    case Kind::Kept:
        return choice.exposure;
    case Kind::Property:
        break;
    }
    return std::nullopt;
}

bool StargateDialog::canShare(const Choice& first, const Choice& second)
{
    return exposureOf(first) == Nebula::Exposure::Consumed && exposureOf(second) == Nebula::Exposure::Consumed &&
           first.type == second.type;
}

bool StargateDialog::isTaken(std::optional<Nebula::Exposure> exposure, const QString& name) const
{
    const QList<const Nebula::Stargate*> stargates = stargatesOf(m_application);
    return std::ranges::any_of(stargates,
                               [this, exposure, &name](const Nebula::Stargate* stargate)
                               {
                                   if(stargate == &m_stargate)
                                   {
                                       return false;
                                   }
                                   return exposure ? stargate->exposedInterface(*exposure, name) != nullptr
                                                   : stargate->property(name) != nullptr;
                               });
}
