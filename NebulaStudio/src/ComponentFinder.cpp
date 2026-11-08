// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ComponentFinder.h"

#include <QCollator>
#include <QCompleter>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QTreeView>
#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <memory>

#include "ComponentItem.h"
#include "EditionScene.h"
#include "Nebula/Application/Universe.h"
#include "Nebula/Component/Component.h"
#include "Nebula/Component/ComponentFactory.h"
#include "Nebula/Galaxy/Galaxy.h"
#include "SceneItems.h"
#include "Theme.h"

namespace
{

using Instance = ComponentFinder::Instance;

// Wide enough for the name of an instance and the name of its component, side by side in the popup, which is as wide.
constexpr int FieldWidth = 280;

// What is typed is looked for in this role of the first column of the completions: the name of the instance and the
// name of its component, on two lines, for it to be found in either of them but not across them.
constexpr int SearchRole = Qt::UserRole;

// Completes with the name of the instance a row offers, rather than with what the row is found by.
class NameCompleter : public QCompleter
{
  public:
    using QCompleter::QCompleter;

    [[nodiscard]] QString pathFromIndex(const QModelIndex& index) const override
    {
        return index.data(Qt::DisplayRole).toString();
    }
};

// The names of galaxies.
QStringList namesOf(const QList<Nebula::Galaxy*>& galaxies)
{
    QStringList names;
    for(const Nebula::Galaxy* galaxy : galaxies)
    {
        names.append(galaxy->objectName());
    }
    return names;
}

// component, in galaxies, one in the other from the application down: its name after theirs.
Instance instanceOf(Nebula::Component& component, const QList<Nebula::Galaxy*>& galaxies)
{
    return {.component = &component,
            .galaxies = galaxies,
            .name = (namesOf(galaxies) << component.objectName()).join(u'.'),
            .componentName = componentNameOf(component, Nebula::ComponentFactory::instance())};
}

// Appends to instances those in galaxies, one in the other from the application down, the last one, or the
// application when there is none, and those of the galaxies they hold, at any depth.
void appendInstances(const Nebula::Universe& application, const QList<Nebula::Galaxy*>& galaxies,
                     QList<Instance>& instances)
{
    for(const std::unique_ptr<Nebula::Component>& component :
        application.components(galaxies.isEmpty() ? nullptr : galaxies.constLast()))
    {
        instances.append(instanceOf(*component, galaxies));
        if(auto* galaxy = qobject_cast<Nebula::Galaxy*>(component.get()))
        {
            appendInstances(application, QList<Nebula::Galaxy*>(galaxies) << galaxy, instances);
        }
    }
}

// Whether left comes before right: the fewer galaxies it is in, the sooner, then in the order collator gives their
// names.
bool comesBefore(const QCollator& collator, const Instance& left, const Instance& right)
{
    if(left.galaxies.size() != right.galaxies.size())
    {
        return left.galaxies.size() < right.galaxies.size();
    }
    return collator.compare(left.name, right.name) < 0;
}

// The instances of application, those of its galaxies at any depth among them: its own first, then those one galaxy
// down, and so on, each level in the order of their names, whatever their case, the numbers in them read as numbers:
// Greeter2 before Greeter10.
QList<Instance> sortedInstances(const Nebula::Universe& application)
{
    QList<Instance> instances;
    appendInstances(application, {}, instances);
    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    collator.setNumericMode(true);
    std::ranges::sort(instances, [&collator](const Instance& left, const Instance& right)
                      { return comesBefore(collator, left, right); });
    return instances;
}

// Whether the name of instance, after those of its galaxies, is text, whatever its case.
bool isNamed(const Instance& instance, const QString& text)
{
    return instance.name.compare(text, Qt::CaseInsensitive) == 0;
}

// Whether its own name is text, whatever its case.
bool hasOwnName(const Instance& instance, const QString& text)
{
    return instance.component->objectName().compare(text, Qt::CaseInsensitive) == 0;
}

// Whether its name, after those of its galaxies, or the name of its component, holds text, whatever its case.
bool holds(const Instance& instance, const QString& text)
{
    return instance.name.contains(text, Qt::CaseInsensitive) ||
           instance.componentName.contains(text, Qt::CaseInsensitive);
}

// Those of instances that matches tells match text, in their order.
QList<Instance> matching(const QList<Instance>& instances, const QString& text,
                         bool (*matches)(const Instance&, const QString&))
{
    QList<Instance> found;
    std::ranges::copy_if(instances, std::back_inserter(found),
                         [&text, matches](const Instance& instance) { return matches(instance, text); });
    return found;
}

// What text finds among instances: the one named text, when one is; those whose own name it is otherwise, in galaxies;
// those that hold it otherwise. In their order.
QList<Instance> findAmong(const QList<Instance>& instances, const QString& text)
{
    for(const auto matches : {&isNamed, &hasOwnName})
    {
        if(QList<Instance> found = matching(instances, text, matches); !found.isEmpty())
        {
            return found;
        }
    }
    return matching(instances, text, &holds);
}

// The components whose items are selected in scene; none when there is no scene.
QList<const Nebula::Component*> selectedIn(const QGraphicsScene* scene)
{
    QList<const Nebula::Component*> selected;
    if(scene != nullptr)
    {
        for(const ComponentItem* item : objectsAmong<ComponentItem>(scene->selectedItems()))
        {
            selected.append(&item->component());
        }
    }
    return selected;
}

// Whether instance is in galaxy, or in a galaxy it holds, at any depth: whatever it is in when galaxy is nullptr, the
// application.
bool isIn(const Instance& instance, const Nebula::Galaxy* galaxy)
{
    return galaxy == nullptr || std::ranges::find(instance.galaxies, galaxy) != instance.galaxies.end();
}

// The rank, among found, of the one after the first one selected, the first one after the last one; when none is, the
// first one in shown, or in a galaxy it holds, see isIn(), or the first one when none is there.
qsizetype nextRank(const QList<Instance>& found, const QList<const Nebula::Component*>& selected,
                   const Nebula::Galaxy* shown)
{
    const auto first = std::ranges::find_if(found, [&selected](const Instance& instance)
                                            { return selected.contains(instance.component); });
    if(first != found.end())
    {
        return (std::distance(found.begin(), first) + 1) % found.size();
    }
    const auto in = std::ranges::find_if(found, [shown](const Instance& instance) { return isIn(instance, shown); });
    return in != found.end() ? std::distance(found.begin(), in) : 0;
}

// Tells that the one of found at rank is found, and, when there are others, which one it is and how to go to the next.
QString foundText(const QList<Instance>& found, qsizetype rank)
{
    const QString& name = found.at(rank).name;
    return found.size() == 1 ? ComponentFinder::tr("%1 found").arg(name)
                             : ComponentFinder::tr("%1 found, %2 of %3: Enter shows the next one")
                                   .arg(name)
                                   .arg(rank + 1)
                                   .arg(found.size());
}

// A row of the completions: the name of instance, after the icon of its component, then the name of its component.
QList<QStandardItem*> completionRow(const Instance& instance)
{
    const QIcon componentIcon = componentIconOf(*instance.component);
    auto* name = new QStandardItem(componentIcon.isNull() ? icon(GlossaryTerm::Astre) : componentIcon, instance.name);
    name->setData(instance.name + u'\n' + instance.componentName, SearchRole);
    return {name, new QStandardItem(instance.componentName)};
}

// The popup of the completions: the name of each instance, as wide as it needs, then the name of its component. Set
// for all its columns at once, those to come included: it has none until it is first filled.
QTreeView* createPopup()
{
    auto* popup = new QTreeView;
    popup->setRootIsDecorated(false);
    popup->setHeaderHidden(true);
    popup->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    popup->header()->setStretchLastSection(true);
    return popup;
}

// Offers to field the rows of completions whose search role holds what is typed, whatever its case, see SearchRole.
QCompleter* createCompleter(QStandardItemModel& completions, QLineEdit& field)
{
    auto* completer = new NameCompleter(&completions, &field);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCompletionRole(SearchRole);
    completer->setPopup(createPopup());
    return completer;
}

} // namespace

ComponentFinder::ComponentFinder(const Nebula::Universe& application, QGraphicsView& view, QWidget* parent)
    : QLineEdit(parent), m_application(application), m_view(view), m_completions(new QStandardItemModel(this))
{
    setPlaceholderText(tr("Find instances"));
    setToolTip(tr("Find the instances of the application, and of its galaxies, by their name, or by the name of their "
                  "component: Enter shows the next one, opening the galaxy it is in"));
    setClearButtonEnabled(true);
    setMinimumWidth(FieldWidth);
    // Before the completer looks among them: the line edit tells that its text was edited, then has it look.
    connect(this, &QLineEdit::textEdited, this, &ComponentFinder::refreshCompletions);
    setCompleter(createCompleter(*m_completions, *this));
    connect(completer(), qOverload<const QString&>(&QCompleter::activated), this, &ComponentFinder::find);
    connect(this, &QLineEdit::returnPressed, this, [this] { find(text()); });
}

ComponentItem* ComponentFinder::find(const QString& text)
{
    const QList<Instance> found = text.isEmpty() ? QList<Instance>() : findAmong(sortedInstances(m_application), text);
    if(found.isEmpty() && !text.isEmpty())
    {
        emit told(tr("No instance of the application, nor of its galaxies, has \"%1\" in its name or in the name of "
                     "its component")
                      .arg(text));
    }
    return found.isEmpty() ? nullptr : showFound(found, nextRank(found, selectedIn(m_view.scene()), shownGalaxy()));
}

void ComponentFinder::refreshCompletions()
{
    m_completions->setRowCount(0);
    for(const Instance& instance : sortedInstances(m_application))
    {
        m_completions->appendRow(completionRow(instance));
    }
}

Nebula::Galaxy* ComponentFinder::shownGalaxy() const
{
    const auto* scene = qobject_cast<const EditionScene*>(m_view.scene());
    return scene != nullptr ? scene->galaxy() : nullptr;
}

QStringList ComponentFinder::shownPath() const
{
    QStringList path;
    for(const Nebula::Galaxy* galaxy = shownGalaxy(); galaxy != nullptr; galaxy = m_application.galaxyOf(*galaxy))
    {
        path.prepend(galaxy->objectName());
    }
    return path;
}

ComponentItem* ComponentFinder::showFound(const QList<Instance>& found, qsizetype rank)
{
    // By their names: the galaxies left to show the one it is in may be read again, and what they hold with them.
    const Instance& instance = found.at(rank);
    return showFound(namesOf(instance.galaxies), instance.component->objectName(), foundText(found, rank));
}

ComponentItem* ComponentFinder::showFound(const QStringList& path, const QString& name, const QString& what)
{
    if(shownPath() != path)
    {
        emit galaxyRequested(path);
    }
    return select(shownItem(path, name), what);
}

ComponentItem* ComponentFinder::shownItem(const QStringList& path, const QString& name) const
{
    if(m_view.scene() == nullptr || shownPath() != path)
    {
        return nullptr;
    }
    const QList<ComponentItem*> items = componentItems(*m_view.scene());
    const auto named =
        std::ranges::find(items, name, [](const ComponentItem* item) { return item->component().objectName(); });
    return named != items.end() ? *named : nullptr;
}

ComponentItem* ComponentFinder::select(ComponentItem* item, const QString& what)
{
    if(item != nullptr)
    {
        m_view.scene()->clearSelection();
        item->setSelected(true);
        m_view.centerOn(item);
        emit told(what);
    }
    return item;
}
