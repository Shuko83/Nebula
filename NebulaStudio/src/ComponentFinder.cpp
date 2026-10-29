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
#include <iterator>

#include "ComponentItem.h"
#include "Nebula/Component/Component.h"
#include "Theme.h"

namespace
{

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

// Whether item is named text, whatever its case.
bool isNamed(const ComponentItem& item, const QString& text)
{
    return item.component().objectName().compare(text, Qt::CaseInsensitive) == 0;
}

// Whether the name of item, or the name of its component, holds text, whatever its case.
bool holds(const ComponentItem& item, const QString& text)
{
    return item.component().objectName().contains(text, Qt::CaseInsensitive) ||
           item.componentName().contains(text, Qt::CaseInsensitive);
}

// The instances scene shows, in the order of their names, whatever their case, the numbers in them read as numbers:
// Greeter2 before Greeter10.
QList<ComponentItem*> sortedItems(const QGraphicsScene& scene)
{
    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    collator.setNumericMode(true);
    QList<ComponentItem*> items = componentItems(scene);
    std::ranges::sort(
        items, [&collator](const ComponentItem* left, const ComponentItem* right)
        { return collator.compare(left->component().objectName(), right->component().objectName()) < 0; });
    return items;
}

// What text finds among items: the one named text alone, when one is; those that hold it otherwise, in their order.
QList<ComponentItem*> findAmong(const QList<ComponentItem*>& items, const QString& text)
{
    const auto named = std::ranges::find_if(items, [&text](const ComponentItem* item) { return isNamed(*item, text); });
    if(named != items.end())
    {
        return {*named};
    }
    QList<ComponentItem*> holding;
    std::ranges::copy_if(items, std::back_inserter(holding),
                         [&text](const ComponentItem* item) { return holds(*item, text); });
    return holding;
}

// What text finds among the instances scene shows, see findAmong(); none when text is empty, or when there is no scene.
QList<ComponentItem*> findIn(const QGraphicsScene* scene, const QString& text)
{
    return text.isEmpty() || scene == nullptr ? QList<ComponentItem*>() : findAmong(sortedItems(*scene), text);
}

// The rank, among found, of the one after the first one selected: the first one when none is, or after the last one.
qsizetype nextRank(const QList<ComponentItem*>& found)
{
    const auto selected = std::ranges::find_if(found, &ComponentItem::isSelected);
    return selected == found.end() ? 0 : (std::distance(found.begin(), selected) + 1) % found.size();
}

// Tells that the one of found at rank is found, and, when there are others, which one it is and how to go to the next.
QString foundText(const QList<ComponentItem*>& found, qsizetype rank)
{
    const QString name = found.at(rank)->component().objectName();
    return found.size() == 1 ? ComponentFinder::tr("%1 found").arg(name)
                             : ComponentFinder::tr("%1 found, %2 of %3: Enter shows the next one")
                                   .arg(name)
                                   .arg(rank + 1)
                                   .arg(found.size());
}

// A row of the completions: the name of item, after the icon of its component, then the name of its component.
QList<QStandardItem*> completionRow(const ComponentItem& item)
{
    const QIcon& componentIcon = item.componentIcon();
    auto* name = new QStandardItem(componentIcon.isNull() ? icon(GlossaryTerm::Astre) : componentIcon,
                                   item.component().objectName());
    name->setData(name->text() + u'\n' + item.componentName(), SearchRole);
    return {name, new QStandardItem(item.componentName())};
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

ComponentFinder::ComponentFinder(QGraphicsView& view, QWidget* parent)
    : QLineEdit(parent), m_view(view), m_completions(new QStandardItemModel(this))
{
    setPlaceholderText(tr("Find instances"));
    setToolTip(tr("Find the instances of the edition zone by their name, or by the name of their component: Enter "
                  "shows the next one"));
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
    const QList<ComponentItem*> found = findIn(m_view.scene(), text);
    if(found.isEmpty() && !text.isEmpty())
    {
        emit told(
            tr("No instance in the edition zone has \"%1\" in its name or in the name of its component").arg(text));
    }
    return found.isEmpty() ? nullptr : showFound(found, nextRank(found));
}

void ComponentFinder::refreshCompletions()
{
    m_completions->setRowCount(0);
    const QGraphicsScene* scene = m_view.scene();
    const QList<ComponentItem*> items = scene != nullptr ? sortedItems(*scene) : QList<ComponentItem*>();
    for(const ComponentItem* item : items)
    {
        m_completions->appendRow(completionRow(*item));
    }
}

ComponentItem* ComponentFinder::showFound(const QList<ComponentItem*>& found, qsizetype rank)
{
    ComponentItem* item = found.at(rank);
    m_view.scene()->clearSelection();
    item->setSelected(true);
    m_view.centerOn(item);
    emit told(foundText(found, rank));
    return item;
}
