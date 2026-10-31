// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "StargateDialog.h"

#include <QDialogButtonBox>
#include <QSignalBlocker>
#include <QStringList>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>
#include <algorithm>
#include <functional>
#include <memory>
#include <ranges>

#include "DialogHeader.h"
#include "InterfaceItem.h"
#include "Nebula/Application/Universe.h"
#include "Nebula/Galaxy/Galaxy.h"
#include "PropertyTree.h"
#include "Theme.h"
#include "WarningLabel.h"

namespace
{

enum Column : std::uint8_t
{
    ExposesColumn,
    TypeColumn,
    NameColumn,
    ByColumn,
};

// What name, which component consumes, is bound to in stargate; nothing when it is not bound to it. A list can be
// bound to it besides what else it is bound to.
std::optional<QString> boundTo(const Nebula::Component& component, const QString& name,
                               const Nebula::Stargate& stargate)
{
    const QList<Nebula::InterfaceBinding> bindings = component.bindings(name);
    const auto bound = std::ranges::find_if(bindings, [&stargate](const Nebula::InterfaceBinding& binding)
                                            { return binding.provider == &stargate; });
    return bound != bindings.end() ? std::optional(bound->providedName) : std::nullopt;
}

// The stargate info, which component consumes, is bound to; nullptr when it is bound to none, or is a list.
const Nebula::Stargate* stargateBoundTo(const Nebula::Component& component, const Nebula::InterfaceInfo& info)
{
    const QList<Nebula::InterfaceBinding> bindings = component.bindings(info.name);
    return !info.list && !bindings.isEmpty() ? qobject_cast<const Nebula::Stargate*>(bindings.constFirst().provider)
                                             : nullptr;
}

// What exposing info, which component consumes, does besides: a list is also given what the galaxy consumes, one bound
// to a component is given it instead; empty otherwise.
QString consumedToolTip(const Nebula::Component& component, const Nebula::InterfaceInfo& info)
{
    if(info.list)
    {
        return StargateDialog::tr("A list: exposed, it is also given what the galaxy consumes");
    }
    const QList<Nebula::InterfaceBinding> bindings = component.bindings(info.name);
    const bool toComponent =
        !bindings.isEmpty() && qobject_cast<const Nebula::Stargate*>(bindings.constFirst().provider) == nullptr;
    return toComponent ? StargateDialog::tr("Bound to %1.%2: exposed, it is given what the galaxy consumes instead")
                             .arg(bindings.constFirst().provider->objectName(), bindings.constFirst().providedName)
                       : QString();
}

// The interface stargate consumes from name, which component provides, the last one when there are several; nullopt
// when it consumes none.
std::optional<QString> passOf(const Nebula::Stargate& stargate, const Nebula::Component& component, const QString& name)
{
    const QList<Nebula::InterfaceInfo> passed = stargate.consumedInterfaces();
    const auto isOf = [&stargate, &component, &name](const Nebula::InterfaceInfo& pass)
    {
        const Nebula::InterfaceBinding bound = stargate.binding(pass.name);
        return bound.provider == &component && bound.providedName == name;
    };
    const auto lastFirst = passed | std::views::reverse;
    const auto found = std::ranges::find_if(lastFirst, isOf);
    return found != lastFirst.end() ? std::optional(found->name) : std::nullopt;
}

// The type of exposed, a property a galaxy exposes.
QString typeNameOf(const Nebula::ExposedProperty& exposed)
{
    return QString::fromLatin1(metaPropertyOf(*exposed.object, exposed.property).typeName());
}

// Whether stargate exposes name, the way exposure tells; a property when it is nullopt.
bool exposes(const Nebula::Stargate& stargate, std::optional<Nebula::Exposure> exposure, const QString& name)
{
    return exposure ? stargate.exposedInterface(*exposure, name) != nullptr : stargate.property(name) != nullptr;
}

// Whether row tells what a stargate exposes: the stargate of the dialog, or another one.
bool isExposed(const QTreeWidgetItem& row)
{
    return !row.text(NameColumn).isEmpty() || !row.text(ByColumn).isEmpty();
}

} // namespace

void StargateDialog::Exposed::record(const Nebula::Stargate& stargate, const Nebula::Stargate& self,
                                     const QString& name)
{
    if(&stargate == &self)
    {
        as = name;
        return;
    }
    by = stargate.objectName();
}

StargateDialog::StargateDialog(Nebula::Stargate& stargate, const Nebula::Universe& application,
                               const Nebula::ComponentFactory& factory, QWidget* parent)
    : EditDialog(stargate, icon(GlossaryTerm::Stargate), parent), m_stargate(stargate), m_application(application),
      m_tree(newChoiceTree({tr("Interface or property"), tr("Type"), tr("Exposed as"), tr("Exposed by")})),
      m_problem(new WarningLabel)
{
    setWindowTitle(tr("Stargate %1").arg(stargate.objectName()));
    tellAbout();
    m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers); // Its name only, see editName()
    fill(factory);
    connect(m_tree, &QTreeWidget::itemChanged, this, &StargateDialog::edited);
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this, &StargateDialog::editName);
    layOut(newContent());
    check();
    resize(680, 560);
}

void StargateDialog::tellAbout()
{
    header().setTitle(m_stargate.objectName());
    header().setText(
        tr("What the galaxy of this project has through %1 once it is placed in another one: the interfaces and the "
           "properties checked, each under the name next to it. Double-clicking one checks it and edits that name. "
           "What the galaxy provides, one of its components provides; what it consumes, it gives to those of its "
           "components that consume it, the same pointer to each of those checked under the same name.")
            .arg(m_stargate.objectName()));
}

QWidget* StargateDialog::newContent()
{
    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->addWidget(m_tree, 1);
    layout->addWidget(m_problem);
    layout->addWidget(buttons());
    return content;
}

void StargateDialog::editName(QTreeWidgetItem* row)
{
    if(m_choices.contains(row))
    {
        row->setCheckState(ExposesColumn, Qt::Checked); // Named by edited() when it was not
        m_tree->editItem(row, NameColumn);
    }
}

void StargateDialog::fill(const Nebula::ComponentFactory& factory)
{
    for(Nebula::Component* component : exposable())
    {
        addIfFilled(*m_tree, componentRow(*component, factory));
    }
    addIfFilled(*m_tree, keptRow());
    openAbove(*m_tree, isExposed);
}

QTreeWidgetItem* StargateDialog::componentRow(Nebula::Component& component, const Nebula::ComponentFactory& factory)
{
    QTreeWidgetItem* row = groupRow(component.objectName());
    const auto* galaxy = qobject_cast<const Nebula::Galaxy*>(&component);
    row->setToolTip(ExposesColumn, galaxy != nullptr ? galaxy->filePath() : component.componentId());
    addConsumedChoices(row, component);
    addProvidedChoices(row, component);
    addPropertyChoices(row, component, factory);
    return row;
}

void StargateDialog::addConsumedChoices(QTreeWidgetItem* parent, Nebula::Component& component)
{
    const QList<Nebula::InterfaceInfo> consumed = component.consumedInterfaces();
    for(const Nebula::InterfaceInfo& info : consumed)
    {
        addConsumedChoice(parent, component, info);
    }
}

void StargateDialog::addConsumedChoice(QTreeWidgetItem* parent, Nebula::Component& component,
                                       const Nebula::InterfaceInfo& info)
{
    const QString kind =
        (info.list ? tr("consumes a list of %1") : tr("consumes %1")).arg(InterfaceItem::typeName(info.type));
    Exposed exposed{.as = boundTo(component, info.name, m_stargate).value_or(QString())};
    if(const Nebula::Stargate* other = stargateBoundTo(component, info); exposed.as.isEmpty() && other != nullptr)
    {
        exposed.record(*other, m_stargate, info.name);
    }
    const Choice choice{.kind = Kind::Consumed, .component = &component, .name = info.name, .type = info.type};
    addChoice(parent, info.name, kind, choice, exposed)->setToolTip(ExposesColumn, consumedToolTip(component, info));
}

void StargateDialog::addProvidedChoices(QTreeWidgetItem* parent, Nebula::Component& component)
{
    const QList<Nebula::InterfaceInfo> provided = component.providedInterfaces();
    for(const Nebula::InterfaceInfo& info : provided)
    {
        const Choice choice{.kind = Kind::Provided, .component = &component, .name = info.name, .type = info.type};
        addChoice(parent, info.name, tr("provides %1").arg(InterfaceItem::typeName(info.type)), choice,
                  providedExposure(component, info.name));
    }
}

void StargateDialog::addPropertyChoices(QTreeWidgetItem* parent, Nebula::Component& component,
                                        const Nebula::ComponentFactory& factory)
{
    const ListProperties list = std::bind_front(&StargateDialog::addHeldProperties, this, std::ref(component));
    if(auto* galaxy = qobject_cast<Nebula::Galaxy*>(&component))
    {
        addGalaxyProperties(parent, *galaxy);
    }
    else
    {
        list(parent, &component, ownProperties(factory, component));
    }
    addObjectGroups(*parent, component, list);
}

void StargateDialog::addGalaxyProperties(QTreeWidgetItem* parent, Nebula::Galaxy& galaxy)
{
    for(const Nebula::ExposedProperty& exposed : galaxy.exposedProperties())
    {
        addPropertyChoice(parent, galaxy, &galaxy, exposed.name, typeNameOf(exposed));
    }
}

void StargateDialog::addHeldProperties(Nebula::Component& component, QTreeWidgetItem* parent, QObject* object,
                                       const QList<QMetaProperty>& properties)
{
    for(const QMetaProperty& property : heldProperties(component, object, properties))
    {
        addPropertyChoice(parent, component, object, QString::fromLatin1(property.name()),
                          QString::fromLatin1(property.typeName()));
    }
}

void StargateDialog::addPropertyChoice(QTreeWidgetItem* parent, Nebula::Component& component, QObject* object,
                                       const QString& name, const QString& type)
{
    const Exposed exposed = propertyExposure({.name = {}, .component = &component, .object = object, .property = name});
    addChoice(parent, name, type,
              Choice{.kind = Kind::Property, .component = &component, .object = object, .name = name}, exposed);
}

QTreeWidgetItem* StargateDialog::keptRow()
{
    QTreeWidgetItem* row = groupRow(tr("Bound to none of these components"));
    const QList<std::pair<Nebula::Exposure, QString>> shown = shownInterfaces();
    for(const Nebula::ExposedInterface& exposed : m_stargate.interfaces())
    {
        if(!shown.contains(std::pair{exposed.exposure, exposed.name}))
        {
            addKeptChoice(row, exposed);
        }
    }
    return row;
}

void StargateDialog::addKeptChoice(QTreeWidgetItem* parent, const Nebula::ExposedInterface& exposed)
{
    const QString type = InterfaceItem::typeName(exposed.type);
    const QString kind = exposed.exposure == Nebula::Exposure::Provided ? tr("the galaxy provides %1").arg(type)
                                                                        : tr("the galaxy consumes %1").arg(type);
    addChoice(parent, exposed.name, kind,
              Choice{.kind = Kind::Kept, .name = exposed.name, .type = exposed.type, .exposure = exposed.exposure},
              Exposed{.as = exposed.name});
}

QTreeWidgetItem* StargateDialog::addChoice(QTreeWidgetItem* parent, const QString& text, const QString& kind,
                                           const Choice& choice, const Exposed& exposed)
{
    auto* row = new QTreeWidgetItem(parent, {text, kind, exposed.as, exposed.by});
    if(!exposed.by.isEmpty())
    {
        row->setFlags(Qt::NoItemFlags); // A single stargate exposes it
        return row;
    }
    makeCheckable(*row, ExposesColumn, !exposed.as.isEmpty(), Qt::ItemIsSelectable | Qt::ItemIsEditable);
    m_choices.insert(row, choice);
    return row;
}

StargateDialog::Exposed StargateDialog::providedExposure(const Nebula::Component& component, const QString& name) const
{
    Exposed exposed;
    for(const Nebula::Stargate* stargate : stargates())
    {
        if(const std::optional<QString> pass = passOf(*stargate, component, name))
        {
            exposed.record(*stargate, m_stargate, *pass);
        }
    }
    return exposed;
}

StargateDialog::Exposed StargateDialog::propertyExposure(const Nebula::ExposedProperty& target) const
{
    Exposed exposed;
    for(const Nebula::Stargate* stargate : stargates())
    {
        if(const std::optional<QString> name = exposedNameOf(*stargate, target))
        {
            exposed.record(*stargate, m_stargate, *name);
        }
    }
    return exposed;
}

QList<std::pair<Nebula::Exposure, QString>> StargateDialog::shownInterfaces() const
{
    QList<std::pair<Nebula::Exposure, QString>> shown;
    for(auto choice = m_choices.cbegin(); choice != m_choices.cend(); ++choice)
    {
        const std::optional<Nebula::Exposure> exposure = exposureOf(*choice);
        if(exposure && !choice.key()->text(NameColumn).isEmpty())
        {
            shown.append({*exposure, choice.key()->text(NameColumn)});
        }
    }
    return shown;
}

QString StargateDialog::defaultName(const QTreeWidgetItem* row) const
{
    const Choice choice = m_choices.value(row);
    const Rows rows = checked();
    QString name = choice.name;
    for(int number = 2; isNameTaken(rows, row, choice, name); ++number)
    {
        name = choice.name + QString::number(number);
    }
    return name;
}

bool StargateDialog::isNameTaken(const Rows& rows, const QTreeWidgetItem* row, const Choice& choice,
                                 const QString& name) const
{
    const auto clashing = [row, &choice, &name](const std::pair<const QTreeWidgetItem*, Choice>& other)
    { return other.first != row && clashes(choice, name, other); };
    return isTaken(exposureOf(choice), name) || std::ranges::any_of(rows, clashing);
}

void StargateDialog::edited(QTreeWidgetItem* row, int column)
{
    if(!m_choices.contains(row))
    {
        return;
    }
    tidyName(*row, column);
    m_tree->viewport()->update(); // What the blocker of tidyName() kept from being shown
    check();
}

void StargateDialog::tidyName(QTreeWidgetItem& row, int column)
{
    if(const std::optional<QString> name = nameAfterEdit(row, column))
    {
        const QSignalBlocker blocker(m_tree); // What is set here is not edited
        row.setText(NameColumn, *name);
    }
}

std::optional<QString> StargateDialog::nameAfterEdit(const QTreeWidgetItem& row, int column) const
{
    const bool isChecked = row.checkState(ExposesColumn) == Qt::Checked;
    if(column == ExposesColumn && isChecked && row.text(NameColumn).trimmed().isEmpty())
    {
        return defaultName(&row);
    }
    if(column == ExposesColumn && !isChecked)
    {
        return QString();
    }
    return column == NameColumn ? std::optional(row.text(NameColumn).simplified()) : std::nullopt;
}

void StargateDialog::check()
{
    const QString problem = firstProblem();
    m_problem->setText(problem);
    setAcceptable(problem.isEmpty());
}

QString StargateDialog::firstProblem() const
{
    const Rows rows = checked();
    for(qsizetype index = 0; index < rows.size(); ++index)
    {
        const QString own = ownProblem(*rows.at(index).first, rows.at(index).second);
        if(const QString problem = own.isEmpty() ? clashProblem(rows, index) : own; !problem.isEmpty())
        {
            return problem;
        }
    }
    return {};
}

QString StargateDialog::ownProblem(const QTreeWidgetItem& row, const Choice& choice) const
{
    const QString name = row.text(NameColumn);
    if(name.isEmpty())
    {
        return tr("%1 needs a name to be exposed as: double-click it to type one").arg(row.text(ExposesColumn));
    }
    return isTaken(exposureOf(choice), name) ? tr("Another stargate exposes %1 the same way: %2 needs another name")
                                                   .arg(name, row.text(ExposesColumn))
                                             : QString();
}

QString StargateDialog::clashProblem(const Rows& rows, qsizetype index)
{
    const auto& [row, choice] = rows.at(index);
    const QString name = row->text(NameColumn);
    const auto clash = std::find_if(rows.begin() + index + 1, rows.end(),
                                    [&choice, &name](const auto& other) { return clashes(choice, name, other); });
    if(clash == rows.end())
    {
        return {};
    }
    return tr("%1 and %2 are both exposed as %3: only interfaces the galaxy consumes, of the same type, share a name")
        .arg(row->text(ExposesColumn), clash->first->text(ExposesColumn), name);
}

void StargateDialog::apply()
{
    if(!m_stargate.isChangeable()) // It started meanwhile
    {
        return;
    }
    const Rows rows = checked();
    applyProperties(rows);
    unbindDropped();
    removeUnwanted(rows);
    exposeWanted(rows);
}

void StargateDialog::applyProperties(const Rows& rows)
{
    const QList<Nebula::ExposedProperty> properties = chosenProperties(rows);
    removePropertiesBut(properties);
    exposeProperties(properties);
}

void StargateDialog::removePropertiesBut(const QList<Nebula::ExposedProperty>& kept)
{
    const QList<Nebula::ExposedProperty> exposedProperties = m_stargate.properties();
    for(const Nebula::ExposedProperty& exposed : exposedProperties)
    {
        if(!kept.contains(exposed))
        {
            m_stargate.removeProperty(exposed.name);
        }
    }
}

void StargateDialog::exposeProperties(const QList<Nebula::ExposedProperty>& properties)
{
    for(const Nebula::ExposedProperty& property : properties)
    {
        if(!m_stargate.properties().contains(property))
        {
            m_stargate.exposeProperty(property);
        }
    }
}

void StargateDialog::unbindDropped()
{
    for(auto choice = m_choices.cbegin(); choice != m_choices.cend(); ++choice)
    {
        if(choice->kind == Kind::Consumed && choice->component != nullptr)
        {
            unbindIfDropped(*choice.key(), *choice);
        }
    }
}

void StargateDialog::unbindIfDropped(const QTreeWidgetItem& row, const Choice& choice)
{
    const std::optional<QString> toThis = boundTo(*choice.component, choice.name, m_stargate);
    const bool kept = row.checkState(ExposesColumn) == Qt::Checked && row.text(NameColumn) == toThis;
    if(toThis && !kept)
    {
        choice.component->unbindInterface(choice.name, m_stargate, *toThis); // A list keeps the others
    }
}

void StargateDialog::removeUnwanted(const Rows& rows)
{
    const QList<Nebula::ExposedInterface> exposedInterfaces = m_stargate.interfaces();
    for(const Nebula::ExposedInterface& exposed : exposedInterfaces)
    {
        if(!isWanted(rows, exposed))
        {
            m_stargate.removeInterface(exposed.exposure, exposed.name);
        }
    }
}

void StargateDialog::exposeWanted(const Rows& rows)
{
    for(const auto& [row, choice] : rows)
    {
        if(const std::optional<Nebula::Exposure> exposure = exposureOf(choice))
        {
            exposeAndBind(*exposure, row->text(NameColumn), choice);
        }
    }
}

void StargateDialog::exposeAndBind(Nebula::Exposure exposure, const QString& name, const Choice& choice)
{
    if(m_stargate.exposedInterface(exposure, name) == nullptr)
    {
        m_stargate.exposeInterface(exposure, name, choice.type);
    }
    bind(name, choice);
}

void StargateDialog::bind(const QString& name, const Choice& choice)
{
    if(choice.kind == Kind::Provided)
    {
        bindProvided(name, choice);
    }
    else if(choice.kind == Kind::Consumed)
    {
        bindConsumed(name, choice);
    }
}

void StargateDialog::bindProvided(const QString& name, const Choice& choice)
{
    const Nebula::InterfaceBinding bound = m_stargate.binding(name);
    if(choice.component != nullptr && (bound.provider != choice.component || bound.providedName != choice.name))
    {
        m_stargate.bindInterface(name, *choice.component, choice.name);
    }
}

void StargateDialog::bindConsumed(const QString& name, const Choice& choice)
{
    if(choice.component != nullptr && boundTo(*choice.component, choice.name, m_stargate) != name)
    {
        choice.component->bindInterface(choice.name, m_stargate, name);
    }
}

StargateDialog::Rows StargateDialog::checked() const
{
    Rows rows;
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
    if(choice.kind == Kind::Property)
    {
        return std::nullopt;
    }
    if(choice.kind == Kind::Kept)
    {
        return choice.exposure;
    }
    return choice.kind == Kind::Provided ? Nebula::Exposure::Provided : Nebula::Exposure::Consumed;
}

bool StargateDialog::canShare(const Choice& first, const Choice& second)
{
    return exposureOf(first) == Nebula::Exposure::Consumed && exposureOf(second) == Nebula::Exposure::Consumed &&
           first.type == second.type;
}

bool StargateDialog::clashes(const Choice& choice, const QString& name,
                             const std::pair<const QTreeWidgetItem*, Choice>& other)
{
    return exposureOf(other.second) == exposureOf(choice) && other.first->text(NameColumn) == name &&
           !canShare(choice, other.second);
}

bool StargateDialog::isWanted(const Rows& rows, const Nebula::ExposedInterface& exposed)
{
    return std::ranges::any_of(rows,
                               [&exposed](const std::pair<const QTreeWidgetItem*, Choice>& row)
                               {
                                   return exposureOf(row.second) == exposed.exposure &&
                                          row.first->text(NameColumn) == exposed.name &&
                                          row.second.type == exposed.type;
                               });
}

Nebula::ExposedProperty StargateDialog::chosenProperty(const QTreeWidgetItem& row, const Choice& choice)
{
    return {
        .name = row.text(NameColumn), .component = choice.component, .object = choice.object, .property = choice.name};
}

QList<Nebula::ExposedProperty> StargateDialog::chosenProperties(const Rows& rows)
{
    QList<Nebula::ExposedProperty> properties;
    for(const auto& [row, choice] : rows)
    {
        if(choice.kind == Kind::Property && choice.component != nullptr && choice.object != nullptr)
        {
            properties.append(chosenProperty(*row, choice));
        }
    }
    return properties;
}

bool StargateDialog::isTaken(std::optional<Nebula::Exposure> exposure, const QString& name) const
{
    QList<const Nebula::Stargate*> others = stargates();
    others.removeOne(&m_stargate);
    return std::ranges::any_of(others, [exposure, &name](const Nebula::Stargate* stargate)
                               { return exposes(*stargate, exposure, name); });
}

const std::vector<std::unique_ptr<Nebula::Component>>& StargateDialog::components() const
{
    return m_application.components(m_application.galaxyOf(m_stargate));
}

QList<Nebula::Component*> StargateDialog::exposable() const
{
    QList<Nebula::Component*> found;
    for(const std::unique_ptr<Nebula::Component>& component : components())
    {
        if(qobject_cast<const Nebula::Stargate*>(component.get()) == nullptr) // Bound to one another, not exposed
        {
            found.append(component.get());
        }
    }
    return found;
}

QList<const Nebula::Stargate*> StargateDialog::stargates() const
{
    return stargatesAmong(components());
}
