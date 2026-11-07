// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "EditionScene.h"

#include <QActionGroup>
#include <QColorDialog>
#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QGraphicsPathItem>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QHash>
#include <QJsonArray>
#include <QKeyEvent>
#include <QList>
#include <QMimeData>
#include <QPainter>
#include <QPalette>
#include <QToolTip>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "AnchorItem.h"
#include "ComponentItem.h"
#include "ComponentModel.h"
#include "ContextMenu.h"
#include "Galaxy/GalaxyModel.h"
#include "Galaxy/StargateDialog.h"
#include "Galaxy/StargateItem.h"
#include "InterfaceItem.h"
#include "LinkItem.h"
#include "Nebula/Application/Universe.h"
#include "Nebula/Component/ComponentFactory.h"
#include "Nebula/Standard/IHistory.h"
#include "PositionJson.h"
#include "SceneCommand.h"
#include "SceneItems.h"
#include "Settings/SettingsDialog.h"
#include "Settings/SettingsItem.h"
#include "Settings/SettingsTextDialog.h"
#include "Theme.h"
#include "ZoneItem.h"

namespace
{

constexpr qreal GridStep = 20;
// Large enough not to grow, so that the view does not move when a component is dropped.
constexpr QRectF SceneRect(0, 0, 4000, 3000);
// Where the hint is drawn while nothing is in the edition zone, in its top left corner.
constexpr QRectF HintRect(GridStep, GridStep, 600, 2 * GridStep);
// A zone as it is added: room for a few components, until it is resized.
constexpr QSizeF ZoneSize(16 * GridStep, 10 * GridStep);
// How far from a component the stargate that exposes one of its interfaces is added, on the side of that interface.
constexpr qreal ExposedDistance = 14 * GridStep;
// The colors the zones are added with, in turn, which tells them apart until theirs is chosen: those of a nebula,
// violet, blue, pink, cyan, magenta and indigo, which their titles contrast with, in black or white, as WCAG AA asks.
constexpr std::array<QRgb, 6> ZoneColors = {0x7C3AED, 0x3B82F6, 0xEC4899, 0x06B6D4, 0xC026D3, 0x6366F1};

// The components shown, by what each item shows of them.
using ShownComponents = QHash<const Nebula::Component*, const ComponentItem*>;

// A binding a link shows, read before it is unbound: a component shows its interfaces again, without their links, as
// they change.
struct Binding
{
    Nebula::Component* consumer;
    QString consumed;
    const Nebula::Component* provider;
    QString provided;

    // Unbinds it alone: a list keeps what else it is bound to.
    void unbind() const
    {
        consumer->unbindInterface(consumed, *provider, provided);
    }
};

// Accepts a drag carrying a component, or a galaxy, as a copy: it stays in its list. Not while the application is not
// editable.
void acceptComponent(QGraphicsSceneDragDropEvent& event, bool editable)
{
    const QMimeData* data = event.mimeData();
    if(editable && (data->hasFormat(ComponentModel::MimeType) || data->hasFormat(GalaxyModel::MimeType)))
    {
        event.setDropAction(Qt::CopyAction);
        event.accept();
    }
    else
    {
        event.ignore();
    }
}

// Accepts event, the drop of what is now shown, as a copy, and gives the keys to the view it was dropped in: they are
// for what was just dropped, as Delete is.
void acceptDrop(QGraphicsSceneDragDropEvent& event)
{
    if(event.widget() != nullptr)
    {
        event.widget()->setFocus();
    }
    event.setDropAction(Qt::CopyAction);
    event.accept();
}

// json, what a project holds about item, with where it is.
QJsonObject withPosition(QJsonObject json, const QGraphicsItem& item)
{
    writePosition(json, item.pos());
    return json;
}

// The name of side in a project.
QString sideName(InterfaceItem::Side side)
{
    return side == InterfaceItem::Side::Left ? QStringLiteral("left") : QStringLiteral("right");
}

// The side named name in a project, the dot of an interface of kind being there; the one of kind when it is neither
// left nor right.
InterfaceItem::Side sideNamed(const QString& name, InterfaceItem::Kind kind)
{
    if(name == u"left")
    {
        return InterfaceItem::Side::Left;
    }
    return name == u"right" ? InterfaceItem::Side::Right : InterfaceItem::defaultSide(kind);
}

// What a project holds of the edges the dots of items are on, by the names of their interfaces: none for those on the
// edge of their kind.
QJsonObject sidesJson(const QList<InterfaceItem*>& items)
{
    QJsonObject json;
    for(const InterfaceItem* item : items)
    {
        if(item->side() != InterfaceItem::defaultSide(item->kind()))
        {
            json.insert(item->name(), sideName(item->side()));
        }
    }
    return json;
}

// json without keys.
QJsonObject without(QJsonObject json, std::initializer_list<QStringView> keys)
{
    for(const QStringView key : keys)
    {
        json.remove(key);
    }
    return json;
}

// Each object of array without keys.
QJsonArray withoutEach(const QJsonArray& array, std::initializer_list<QStringView> keys)
{
    QJsonArray stripped;
    for(const auto& value : array)
    {
        stripped.append(without(value.toObject(), keys));
    }
    return stripped;
}

// What project, a snapshot of a scene, holds of the application or of the galaxy itself: its components, their links
// and its settings files, as Universe saves them, without what only the items of the scene show.
QJsonObject contentOf(const QJsonObject& project)
{
    QJsonObject content = without(project, {u"zones"});
    content.insert(u"components",
                   withoutEach(project.value(u"components").toArray(), {u"x", u"y", u"color", u"sides", u"rows"}));
    content.insert(u"settings", withoutEach(project.value(u"settings").toArray(), {u"x", u"y"}));
    content.insert(u"bindings", withoutEach(project.value(u"bindings").toArray(), {u"anchors"}));
    return content;
}

// what, a step of the history, about subject, the component it was done to: Remove component (Greeter1). what alone
// when there is none.
QString about(const QString& what, const QString& subject)
{
    return what.isEmpty() || subject.isEmpty() ? what : EditionScene::tr("%1 (%2)").arg(what, subject);
}

// What dragging zone does.
QString zoneDragName(const ZoneItem& zone)
{
    return zone.isResized() ? EditionScene::tr("Resize zone %1").arg(zone.title())
                            : EditionScene::tr("Move zone %1").arg(zone.title());
}

// What dragging anchor does.
QString anchorDragName(const AnchorItem& anchor)
{
    return anchor.isJunction() ? EditionScene::tr("Move junction point") : EditionScene::tr("Move anchor point");
}

// What item, one the scene shows, is named in a step of the history: the name of its component, of its file or its
// title; what it is otherwise.
QString itemName(const QGraphicsItem& item)
{
    if(const auto* component = qobject_cast<const ComponentItem*>(item.toGraphicsObject()))
    {
        return component->component().objectName();
    }
    if(const auto* settings = qobject_cast<const SettingsItem*>(item.toGraphicsObject()))
    {
        return settings->file().fileName();
    }
    const auto* zone = qgraphicsitem_cast<const ZoneItem*>(&item);
    return zone != nullptr ? zone->title() : EditionScene::tr("link");
}

// Inserts value into json as key, unless it is empty.
void insertUnlessEmpty(QJsonObject& json, QStringView key, const QJsonObject& value)
{
    if(!value.isEmpty())
    {
        json.insert(key, value);
    }
}

// Writes to json, what a project holds about the component of item, the edges the dots of its interfaces are on, when
// one is not on the edge of its kind.
void writeSides(QJsonObject& json, const ComponentItem& item)
{
    QJsonObject sides;
    insertUnlessEmpty(sides, u"consumed", sidesJson(item.consumedInterfaces()));
    insertUnlessEmpty(sides, u"provided", sidesJson(item.providedInterfaces()));
    insertUnlessEmpty(json, u"sides", sides);
}

// Puts the dots of the interfaces of item of kind on the edges sides, what a project holds of them, tells.
void readSides(ComponentItem& item, InterfaceItem::Kind kind, const QJsonObject& sides)
{
    const QStringList names = sides.keys();
    for(const QString& name : names)
    {
        item.setInterfaceSide(kind, name, sideNamed(sides.value(name).toString(), kind));
    }
}

// Puts the dots of the interfaces of item on the edges json, what a project holds about its component, tells.
void readSides(ComponentItem& item, const QJsonObject& json)
{
    const QJsonObject sides = json.value(u"sides").toObject();
    readSides(item, InterfaceItem::Kind::Consumed, sides.value(u"consumed").toObject());
    readSides(item, InterfaceItem::Kind::Provided, sides.value(u"provided").toObject());
}

// The name of kind in a project, as the interfaces of a stargate are listed.
QString kindName(InterfaceItem::Kind kind)
{
    return kind == InterfaceItem::Kind::Provided ? QStringLiteral("provided") : QStringLiteral("consumed");
}

// What a project holds of the order of rows, the interfaces of a component from the top: the kind and the name of each.
QJsonArray rowsJson(const QList<InterfaceItem*>& rows)
{
    QJsonArray json;
    for(const InterfaceItem* row : rows)
    {
        json.append(QJsonObject{{kindName(row->kind()), row->name()}});
    }
    return json;
}

// Writes to json, what a project holds about the component of item, the order of the rows of its interfaces, when it
// is not the one of their kinds.
void writeRows(QJsonObject& json, const ComponentItem& item)
{
    const QList<InterfaceItem*> rows = item.interfaceRows();
    if(rows != item.consumedInterfaces() + item.providedInterfaces())
    {
        json.insert(u"rows", rowsJson(rows));
    }
}

// The interface row, what a project holds of a row, tells; none when it tells no kind.
std::optional<InterfaceItem::Key> rowKey(const QJsonObject& row)
{
    for(const InterfaceItem::Kind kind : {InterfaceItem::Kind::Consumed, InterfaceItem::Kind::Provided})
    {
        if(row.contains(kindName(kind)))
        {
            return InterfaceItem::Key{kind, row.value(kindName(kind)).toString()};
        }
    }
    return std::nullopt;
}

// Shows the interfaces of item in the order json, what a project holds about its component, tells.
void readRows(ComponentItem& item, const QJsonObject& json)
{
    QList<InterfaceItem::Key> keys;
    const QJsonArray rows = json.value(u"rows").toArray();
    for(const auto& row : rows)
    {
        if(const std::optional<InterfaceItem::Key> key = rowKey(row.toObject()))
        {
            keys.append(*key);
        }
    }
    item.setInterfaceOrder(keys);
}

// json, what a project holds about the component of item, with where it is, the color it was given, and the edges and
// the rows the dots of its interfaces are on.
QJsonObject placed(QJsonObject json, const ComponentItem& item)
{
    writePosition(json, item.pos());
    if(item.color().isValid()) // None while it has the colors of the style
    {
        json.insert(u"color", item.color().name(QColor::HexArgb)); // With its opacity
    }
    writeSides(json, item);
    writeRows(json, item);
    return json;
}

// items, by what key returns of each.
template <typename Key, typename Item, typename Function>
QHash<Key, const Item*> byKey(const QList<Item*>& items, Function key)
{
    QHash<Key, const Item*> found;
    for(const Item* item : items)
    {
        found.insert(key(*item), item);
    }
    return found;
}

// Writes where items, the components shown, are in project, and the color they were given.
void placeComponents(QJsonObject& project, const QList<ComponentItem*>& items)
{
    const QHash<QString, const ComponentItem*> shown =
        byKey<QString>(items, [](const ComponentItem& item) { return item.component().objectName(); });
    QJsonArray components = project.value(u"components").toArray();
    for(QJsonValueRef component : components)
    {
        const QJsonObject json = component.toObject();
        const ComponentItem* item = shown.value(json.value(u"name").toString());
        component = item != nullptr ? placed(json, *item) : json;
    }
    project.insert(u"components", components);
}

// The one of items that shows file; nullptr when none does.
const SettingsItem* settingsItemOf(const QList<SettingsItem*>& items, const Nebula::SettingsFile& file)
{
    const auto found = std::ranges::find(items, &file, [](const SettingsItem* item) { return &item->file(); });
    return found != items.end() ? *found : nullptr;
}

// settings, what a project holds about files, in their order, with where those items show are.
QJsonArray placedSettings(QJsonArray settings, const std::vector<std::unique_ptr<Nebula::SettingsFile>>& files,
                          const QList<SettingsItem*>& items)
{
    for(qsizetype index = 0; index < settings.size(); ++index)
    {
        const SettingsItem* item = settingsItemOf(items, *files.at(static_cast<std::size_t>(index)));
        const QJsonObject json = settings.at(index).toObject();
        settings.replace(index, item != nullptr ? withPosition(json, *item) : json);
    }
    return settings;
}

// What a project holds about zone.
QJsonObject zoneJson(const ZoneItem& zone)
{
    QJsonObject json;
    json.insert(u"title", zone.title());
    json.insert(u"color", zone.color().name());
    writePosition(json, zone.pos());
    json.insert(u"width", zone.boundingRect().width());
    json.insert(u"height", zone.boundingRect().height());
    return json;
}

QJsonArray zonesJson(const QList<ZoneItem*>& zones)
{
    QJsonArray json;
    for(const ZoneItem* zone : zones)
    {
        json.append(zoneJson(*zone));
    }
    return json;
}

// What project holds about its components, by name.
QHash<QString, QJsonObject> savedComponents(const QJsonObject& project)
{
    QHash<QString, QJsonObject> saved;
    const QJsonArray components = project.value(u"components").toArray();
    for(const auto& value : components)
    {
        const QJsonObject json = value.toObject();
        saved.insert(json.value(u"name").toString(), json);
    }
    return saved;
}

// The one of interfaces named name; nullptr when none is.
InterfaceItem* interfaceNamed(const QList<InterfaceItem*>& interfaces, const QString& name)
{
    const auto found = std::ranges::find(interfaces, name, &InterfaceItem::name);
    return found != interfaces.end() ? *found : nullptr;
}

bool isProvided(const InterfaceItem& item)
{
    return item.kind() == InterfaceItem::Kind::Provided;
}

// The interface shown that bound is to, through shown; nullptr when its component is not shown.
InterfaceItem* providedItem(const ShownComponents& shown, const Nebula::InterfaceBinding& bound)
{
    const ComponentItem* provider = shown.value(bound.provider);
    return provider != nullptr ? interfaceNamed(provider->providedInterfaces(), bound.providedName) : nullptr;
}

// The interfaces shown that bindings are to, through shown: one, or several for a list.
QList<InterfaceItem*> providedItems(const ShownComponents& shown, const QList<Nebula::InterfaceBinding>& bindings)
{
    QList<InterfaceItem*> provided;
    for(const Nebula::InterfaceBinding& bound : bindings)
    {
        if(InterfaceItem* item = providedItem(shown, bound))
        {
            provided.append(item);
        }
    }
    return provided;
}

// Those of provided consumed has no link to yet. Its links to others go, and so does the copy of one of a list.
QList<InterfaceItem*> unlinked(const InterfaceItem& consumed, QList<InterfaceItem*> provided)
{
    const QList<LinkItem*> links = consumed.links(); // A copy: each link deleted leaves the list
    for(LinkItem* link : links)
    {
        if(!provided.removeOne(&link->provided()))
        {
            delete link;
        }
    }
    return provided;
}

// What links show.
QList<Binding> bindingsOf(const QList<LinkItem*>& links)
{
    QList<Binding> bindings;
    for(const LinkItem* link : links)
    {
        bindings.append({.consumer = &link->consumed().component(),
                         .consumed = link->consumed().name(),
                         .provider = &link->provided().component(),
                         .provided = link->provided().name()});
    }
    return bindings;
}

// What a galaxy is about: its file, and its description.
QString aboutGalaxy(const Nebula::Galaxy& galaxy)
{
    const QString file = QDir::toNativeSeparators(galaxy.filePath());
    return galaxy.description().isEmpty() ? file : file + u'\n' + galaxy.description();
}

// Opens dialog, deleted once closed. Not run here: the event that opens it ends first, while it blocks the window.
void openDialog(QDialog& dialog)
{
    dialog.setAttribute(Qt::WA_DeleteOnClose);
    dialog.open();
}

// The type of the interface item shows; none when its component no longer has it.
std::optional<QMetaType> interfaceType(const InterfaceItem& item)
{
    const QList<Nebula::InterfaceInfo> interfaces =
        isProvided(item) ? item.component().providedInterfaces() : item.component().consumedInterfaces();
    const auto info = std::ranges::find(interfaces, item.name(), &Nebula::InterfaceInfo::name);
    return info != interfaces.end() ? std::optional(info->type) : std::nullopt;
}

// What provided is named in the menu of an interface it can be bound to: its component, then its name, as in
// Ticker1.display.
QString qualifiedName(const InterfaceItem& provided)
{
    return provided.component().objectName() + u'.' + provided.name();
}

// What binding one and other, an interface consumed and a provided one, whichever is which, is in the history: Bind
// Window1.content to Ticker1.display.
QString bindingName(const InterfaceItem& one, const InterfaceItem& other)
{
    const InterfaceItem& consumed = isProvided(one) ? other : one;
    const InterfaceItem& provided = isProvided(one) ? one : other;
    return EditionScene::tr("Bind %1 to %2").arg(qualifiedName(consumed), qualifiedName(provided));
}

// Tells in bindMenu, rather than leaving it empty, that no interface shown can be bound to consumed.
void addNoTarget(QMenu& bindMenu, const InterfaceItem& consumed)
{
    const QString type = InterfaceItem::typeName(interfaceType(consumed).value_or(QMetaType()));
    bindMenu.addAction(EditionScene::tr("No other component here provides %1").arg(type))->setEnabled(false);
}

// What the entries of bindMenu that bind consumed are in, which draws them as radio buttons: a single interface is
// bound to one of them at most. nullptr for a list, bound to several, whose entries are check boxes.
QActionGroup* choiceGroup(QMenu& bindMenu, const InterfaceItem& consumed)
{
    if(consumed.isList())
    {
        return nullptr;
    }
    auto* group = new QActionGroup(&bindMenu);
    group->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional); // Bound to nothing, none is checked
    return group;
}

// Where the stargate that exposes item is added: next to its component, on the edge its dot is on, which the dot of
// the stargate faces.
QPointF exposedPosition(const InterfaceItem& item)
{
    const bool right = item.side() == InterfaceItem::Side::Right;
    return item.parentItem()->pos() + QPointF(right ? ExposedDistance : -ExposedDistance, item.y());
}

// The kind of the interface of the stargate that exposes item: it consumes what a component provides, and the other
// way round.
InterfaceItem::Kind exposingKind(const InterfaceItem& item)
{
    return isProvided(item) ? InterfaceItem::Kind::Consumed : InterfaceItem::Kind::Provided;
}

// The edge across from side.
InterfaceItem::Side across(InterfaceItem::Side side)
{
    return side == InterfaceItem::Side::Left ? InterfaceItem::Side::Right : InterfaceItem::Side::Left;
}

// What the entry of the menu of a dot that puts it on side tells.
QString sideText(InterfaceItem::Side side)
{
    return side == InterfaceItem::Side::Left ? EditionScene::tr("Dot on the left edge")
                                             : EditionScene::tr("Dot on the right edge");
}

// What the entry of the menu of a dot that swaps its row with the one of row tells: its name and its kind, which two
// interfaces of a component can share a name across.
QString swapText(const InterfaceItem& row)
{
    return isProvided(row) ? EditionScene::tr("%1 (provided)").arg(row.name())
                           : EditionScene::tr("%1 (consumed)").arg(row.name());
}

// Adds to swapMenu, a submenu of menu, the entry that swaps the rows of dot and row, two interfaces of a component;
// nothing when row is dot.
void addSwapEntry(ContextMenu& menu, QMenu& swapMenu, const InterfaceItem& dot, const InterfaceItem& row)
{
    if(&row == &dot)
    {
        return;
    }
    menu.add(swapMenu, swapText(row),
             [item = &dot.componentItem(), one = dot.key(), other = row.key()] { item->swapInterfaces(one, other); });
}

// Has stargate expose item, an interface of a component, of type, and returns the name it exposes it as: its own,
// unless another stargate exposes one so, with the first number free then.
QString exposeAs(Nebula::Stargate& stargate, const InterfaceItem& item, QMetaType type)
{
    const Nebula::Exposure exposure = isProvided(item) ? Nebula::Exposure::Provided : Nebula::Exposure::Consumed;
    QString name = item.name();
    for(int number = 2; !stargate.exposeInterface(exposure, name, type); ++number)
    {
        name = item.name() + QString::number(number);
    }
    return name;
}

// Binds item, an interface of a component, to the one stargate exposes it as, name.
void bindExposed(Nebula::Stargate& stargate, const QString& name, const InterfaceItem& item)
{
    if(isProvided(item))
    {
        stargate.bindInterface(name, item.component(), item.name());
    }
    else
    {
        item.component().bindInterface(item.name(), stargate, name);
    }
}

// The lines of the grid across rect.
QList<QLineF> gridLines(const QRectF& rect)
{
    QList<QLineF> lines;
    for(qreal x = std::floor(rect.left() / GridStep) * GridStep; x <= rect.right(); x += GridStep)
    {
        lines.append(QLineF(x, rect.top(), x, rect.bottom()));
    }
    for(qreal y = std::floor(rect.top() / GridStep) * GridStep; y <= rect.bottom(); y += GridStep)
    {
        lines.append(QLineF(rect.left(), y, rect.right(), y));
    }
    return lines;
}

} // namespace

EditionScene::EditionScene(Nebula::Universe& application, const Nebula::ComponentFactory& factory, QObject* parent)
    : QGraphicsScene(SceneRect, parent), m_application(application), m_factory(factory)
{
    connect(&m_application, &Nebula::Universe::runningChanged, this, [this] { setEditable(isEditable()); });
}

EditionScene::EditionScene(Nebula::Universe& application, const Nebula::ComponentFactory& factory,
                           Nebula::Galaxy& galaxy, QObject* parent)
    : EditionScene(application, factory, parent)
{
    m_galaxy = &galaxy;
    // Its items go before what they show, and come again with what its file holds now.
    connect(&galaxy, &Nebula::Galaxy::aboutToBeReloaded, this, &EditionScene::hideGalaxy);
    connect(&galaxy, &Nebula::Galaxy::reloaded, this, &EditionScene::showGalaxyAgain);
    showGalaxy();
}

Nebula::Galaxy* EditionScene::galaxy() const
{
    return m_galaxy;
}

bool EditionScene::isModified() const
{
    return m_galaxy != nullptr && save() != m_saved;
}

void EditionScene::showGalaxy()
{
    show(m_galaxy->components(), m_galaxy->settingsFiles(), m_galaxy->project());
    setEditable(isEditable());
    m_saved = save();
}

void EditionScene::hideGalaxy()
{
    m_selected = selectedNames();
    removeItems();
}

void EditionScene::showGalaxyAgain()
{
    showGalaxy();
    selectNamed(m_selected);
    m_selected.clear();
}

QStringList EditionScene::selectedNames() const
{
    QStringList names;
    const QList<ComponentItem*> selected = objectsAmong<ComponentItem>(selectedItems());
    for(const ComponentItem* item : selected)
    {
        names.append(item->component().objectName());
    }
    return names;
}

void EditionScene::selectNamed(const QStringList& names)
{
    const QList<ComponentItem*> shown = objectsAmong<ComponentItem>(items());
    for(ComponentItem* item : shown)
    {
        item->setSelected(names.contains(item->component().objectName()));
    }
}

void EditionScene::selectOnly(QGraphicsItem& item)
{
    clearSelection();
    item.setSelected(true);
}

Nebula::Component* EditionScene::selectedComponent() const
{
    // Not a settings file nor a link, which can be selected too.
    const auto* item = onlyObjectAmong<const ComponentItem>(selectedItems());
    return item != nullptr ? &item->component() : nullptr;
}

Nebula::SettingsFile* EditionScene::selectedSettingsFile() const
{
    const auto* item = onlyObjectAmong<const SettingsItem>(selectedItems());
    return item != nullptr ? &item->file() : nullptr;
}

ComponentItem* EditionScene::select(const Nebula::Component& component)
{
    clearSelection();
    ComponentItem* item = itemOf(component);
    if(item != nullptr)
    {
        item->setSelected(true);
    }
    return item;
}

ComponentItem* EditionScene::itemOf(const Nebula::Component& component) const
{
    const QList<ComponentItem*> shown = objectsAmong<ComponentItem>(items());
    const auto found =
        std::ranges::find(shown, &component, [](const ComponentItem* item) { return &item->component(); });
    return found != shown.end() ? *found : nullptr;
}

const std::vector<std::unique_ptr<Nebula::SettingsFile>>& EditionScene::shownFiles() const
{
    return m_galaxy != nullptr ? m_galaxy->settingsFiles() : m_application.settingsFiles();
}

QJsonObject EditionScene::save() const
{
    QJsonObject project = m_galaxy != nullptr ? m_application.saveGalaxy(*m_galaxy) : m_application.save();
    // From the first one added: load() adds them in this order, which is then the same again.
    const QList<QGraphicsItem*> added = items(Qt::AscendingOrder);
    placeComponents(project, objectsAmong<ComponentItem>(added));
    placeSettings(project, objectsAmong<SettingsItem>(added));
    project.insert(u"bindings", m_anchors.written(project.value(u"bindings").toArray()));
    project.insert(u"zones", zonesJson(itemsAmong<ZoneItem>(added)));
    return project;
}

void EditionScene::placeSettings(QJsonObject& project, const QList<SettingsItem*>& items) const
{
    const QJsonArray settings = placedSettings(project.value(u"settings").toArray(), shownFiles(), items);
    if(!settings.isEmpty())
    {
        project.insert(u"settings", settings);
    }
}

QStringList EditionScene::load(const QJsonObject& project)
{
    removeItems(); // Before the components they show are replaced
    QStringList problems = m_application.load(project);
    show(m_application.components(), m_application.settingsFiles(), project);
    return problems;
}

void EditionScene::show(const std::vector<std::unique_ptr<Nebula::Component>>& components,
                        const std::vector<std::unique_ptr<Nebula::SettingsFile>>& files, const QJsonObject& project)
{
    const QHash<QString, QJsonObject> saved = savedComponents(project);
    for(const std::unique_ptr<Nebula::Component>& component : components)
    {
        showSaved(*component, saved.value(component->objectName()));
    }
    showLinks(components, project);                                 // Once every component is shown
    showSettingsFiles(files, project.value(u"settings").toArray()); // Joined to the components shown
    showZones(project.value(u"zones").toArray());
}

void EditionScene::showLinks(const std::vector<std::unique_ptr<Nebula::Component>>& components,
                             const QJsonObject& project)
{
    m_anchors.read(components, project.value(u"bindings").toArray());
    refreshLinks();
    refreshStargateLinks();
}

void EditionScene::showSaved(Nebula::Component& component, const QJsonObject& json)
{
    ComponentItem* item = showComponent(component, readPosition(json));
    item->setColor(QColor::fromString(json.value(u"color").toString())); // Not valid when it has none
    readSides(*item, json);
    readRows(*item, json);
}

void EditionScene::refreshStargateLinks()
{
    const QList<StargateItem*> stargates = objectsAmong<StargateItem>(items());
    for(StargateItem* stargate : stargates)
    {
        stargate->refreshLinks();
    }
}

void EditionScene::showSettingsFiles(const std::vector<std::unique_ptr<Nebula::SettingsFile>>& files,
                                     const QJsonArray& settings)
{
    for(std::size_t index = 0; index < files.size(); ++index)
    {
        showSettings(*files.at(index), readPosition(settings.at(static_cast<qsizetype>(index)).toObject()));
    }
}

void EditionScene::showZones(const QJsonArray& zones)
{
    for(const auto& value : zones)
    {
        const QJsonObject json = value.toObject();
        addZone(readPosition(json), QSizeF(json.value(u"width").toDouble(), json.value(u"height").toDouble()),
                json.value(u"title").toString(), QColor::fromString(json.value(u"color").toString()));
    }
}

void EditionScene::place(QGraphicsItem& item, const QPointF& position)
{
    item.setPos(position);
    addItem(&item);
    update(HintRect); // Hidden once anything is in the edition zone
}

void EditionScene::reset()
{
    removeItems();
    m_application.clear();
}

template <typename Item> void EditionScene::removeAll(const QList<Item*>& removed)
{
    for(Item* item : removed)
    {
        remove(*item);
    }
}

void EditionScene::remove(const QList<Nebula::Component*>& components)
{
    // Found before any goes: its interfaces and their links, which are items too, go with it.
    QList<ComponentItem*> removed = objectsAmong<ComponentItem>(items());
    removed.removeIf([&components](const ComponentItem* item) { return !components.contains(&item->component()); });
    removeAll(removed);
}

LinkStyle EditionScene::linkStyle() const
{
    return m_linkStyle;
}

void EditionScene::setLinkStyle(LinkStyle style)
{
    m_linkStyle = style; // The link being drawn has it as soon as the cursor moves
    const QList<LinkItem*> links = itemsAmong<LinkItem>(items());
    for(LinkItem* link : links)
    {
        link->setStyle(style);
    }
}

void EditionScene::setHistory(Nebula::IHistory* history)
{
    m_history = history;
}

void EditionScene::edit(const QString& what, const std::function<void()>& change)
{
    step(
        [&what, &change]
        {
            change();
            return what;
        });
}

void EditionScene::step(const std::function<QString()>& change)
{
    QJsonObject before = snapshot();
    m_pending.append(&before);
    const QString what = change();
    m_pending.removeOne(&before);
    record(what, before);
}

void EditionScene::record(const QString& what, const QJsonObject& before)
{
    const QJsonObject after = snapshot();
    if(after == before)
    {
        return;
    }
    startAfter(after);
    if(m_history != nullptr) // Done already: pushing it changes nothing
    {
        m_history->push(std::make_unique<SceneCommand>(*this, what.isEmpty() ? tr("Edit") : what, before, after));
    }
}

void EditionScene::startAfter(const QJsonObject& state)
{
    std::ranges::for_each(m_pending, [&state](QJsonObject* before) { *before = state; });
    if(m_gesture)
    {
        m_gesture->before = state;
    }
}

QJsonObject EditionScene::snapshot() const
{
    if(m_galaxy != nullptr)
    {
        return save(); // Its galaxies are relative to its file, which stays
    }
    const QString filePath = m_application.filePath();
    m_application.setFilePath({}); // Without a file, the files of its galaxies are whole
    QJsonObject state = save();
    m_application.setFilePath(filePath);
    return state;
}

bool EditionScene::restore(const QJsonObject& state, const QString& what)
{
    if(!checkEditable() || snapshot() == state)
    {
        return false;
    }
    const QStringList selected = selectedNames();
    m_gesture.reset(); // What it started from is no longer shown: it is not a step
    restoreState(state);
    selectNamed(selected);
    if(!what.isEmpty())
    {
        emit told(what);
    }
    return true;
}

bool EditionScene::isBusy() const
{
    return m_linkSource != nullptr || mouseGrabberItem() != nullptr;
}

const std::vector<std::unique_ptr<Nebula::Component>>& EditionScene::shownComponents() const
{
    return m_galaxy != nullptr ? m_galaxy->components() : m_application.components();
}

void EditionScene::restoreState(const QJsonObject& state)
{
    const QJsonObject current = contentOf(snapshot());
    const QJsonObject wanted = contentOf(state);
    if(current != wanted && !restoreDescription(current, wanted))
    {
        restoreContent(state);
    }
    else if(snapshot() != state)
    {
        restoreItems(state);
    }
}

bool EditionScene::restoreDescription(const QJsonObject& current, const QJsonObject& wanted)
{
    if(m_galaxy != nullptr || without(current, {u"description"}) != without(wanted, {u"description"}))
    {
        return false;
    }
    m_application.setDescription(wanted.value(u"description").toString());
    return true;
}

void EditionScene::restoreItems(const QJsonObject& state)
{
    removeItems();
    show(shownComponents(), shownFiles(), state);
    setEditable(isEditable());
}

void EditionScene::restoreContent(const QJsonObject& state)
{
    const QStringList problems = m_galaxy != nullptr ? restoreGalaxy(state) : load(state);
    if(!problems.isEmpty())
    {
        emit refused(tr("Not everything could be restored: %1").arg(problems.join(u' ')));
    }
}

QStringList EditionScene::restoreGalaxy(const QJsonObject& state)
{
    const QJsonObject saved = m_saved; // Still what its file holds: shown again, it is not saved
    QStringList problems = m_application.restoreGalaxy(*m_galaxy, state);
    m_saved = saved;
    return problems;
}

void EditionScene::beginGesture(const QGraphicsSceneMouseEvent& event)
{
    if(isEditable() && event.button() == Qt::LeftButton)
    {
        m_gesture = Gesture{.before = snapshot(), .what = {}};
    }
}

void EditionScene::nameGesture(const QString& what)
{
    if(m_gesture && !what.isEmpty())
    {
        m_gesture->what = what;
    }
}

void EditionScene::endGesture()
{
    if(m_gesture)
    {
        const Gesture gesture = *std::exchange(m_gesture, std::nullopt);
        record(gesture.what, gesture.before);
    }
}

QString EditionScene::dragName() const
{
    const QGraphicsItem* grabbed = mouseGrabberItem();
    if(const auto* zone = qgraphicsitem_cast<const ZoneItem*>(grabbed))
    {
        return zoneDragName(*zone);
    }
    if(const auto* anchor = qgraphicsitem_cast<const AnchorItem*>(grabbed))
    {
        return anchorDragName(*anchor);
    }
    return grabbed != nullptr ? tr("Move %1").arg(selectionName()) : QString();
}

QString EditionScene::selectionName() const
{
    const QList<QGraphicsItem*> selected = selectedItems();
    return selected.size() == 1 ? itemName(*selected.first()) : tr("%n items", nullptr, int(selected.size()));
}

void EditionScene::execMenu(ContextMenu& menu, const QGraphicsSceneContextMenuEvent& event)
{
    const ComponentItem* item = componentAt(event.scenePos()); // Before it is chosen: it may remove the component
    const QString subject = item != nullptr ? item->component().objectName() : QString();
    step([&menu, &event, &subject] { return about(menu.exec(event.screenPos()), subject); });
}

void EditionScene::dragEnterEvent(QGraphicsSceneDragDropEvent* event)
{
    acceptComponent(*event, isEditable());
}

void EditionScene::dragMoveEvent(QGraphicsSceneDragDropEvent* event)
{
    acceptComponent(*event, isEditable());
}

void EditionScene::dropEvent(QGraphicsSceneDragDropEvent* event)
{
    const QJsonObject before = snapshot();
    Nebula::Component* component = dropped(*event->mimeData());
    if(component == nullptr)
    {
        event->ignore();
        return;
    }
    selectOnly(*showComponent(*component, event->scenePos()));
    record(tr("Add %1").arg(component->objectName()), before);
    acceptDrop(*event);
}

Nebula::Component* EditionScene::dropped(const QMimeData& data)
{
    if(data.hasFormat(GalaxyModel::MimeType))
    {
        return placeGalaxy(QString::fromUtf8(data.data(GalaxyModel::MimeType)));
    }
    return m_application.instantiate(QString::fromUtf8(data.data(ComponentModel::MimeType)), {}, m_galaxy);
}

Nebula::Galaxy* EditionScene::placeGalaxy(const QString& filePath)
{
    Nebula::Galaxy* galaxy = m_application.instantiateGalaxy(filePath, {}, m_galaxy);
    if(galaxy == nullptr)
    {
        emit refused(
            tr("%1 cannot be placed: %2").arg(QFileInfo(filePath).completeBaseName(), m_application.errorString()));
        return nullptr;
    }
    emit galaxyPlaced(*galaxy);
    tellProblems(*galaxy);
    return galaxy;
}

void EditionScene::tellProblems(const Nebula::Galaxy& galaxy)
{
    if(!galaxy.problems().isEmpty())
    {
        emit told(
            tr("%1 is placed without what its file misses: %2").arg(galaxy.objectName(), galaxy.problems().join(u' ')));
    }
}

void EditionScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    beginGesture(*event);
    if(!isEditable() || event->button() != Qt::LeftButton || !startLinkAt(*event))
    {
        QGraphicsScene::mousePressEvent(event);
        nameGesture(dragName()); // What the item it pressed does, as it is dragged
        return;
    }
    event->accept(); // Not passed to the items: a component would move, a link would be selected
}

bool EditionScene::startLinkAt(const QGraphicsSceneMouseEvent& event)
{
    if(InterfaceItem* source = interfaceAt(event.scenePos()))
    {
        startLink(*source);
        return true;
    }
    return event.modifiers().testFlag(Qt::ShiftModifier) && startBranch(event.scenePos());
}

void EditionScene::startLink(InterfaceItem& source)
{
    m_linkSource = &source;
    m_pendingLink = addPath({}, QPen(palette().color(QPalette::ButtonText), 1.5, Qt::DashLine));
    m_pendingLink->setZValue(1); // Over the components it is drawn across
    dimLinkTargets(&source);
}

bool EditionScene::startBranch(const QPointF& position)
{
    AnchorItem* anchor = anchorAt(position);
    LinkItem* link = anchor != nullptr ? anchor->links().constFirst() : linkAt(position);
    if(link == nullptr)
    {
        return false;
    }
    m_branch = Branch{.link = link, .anchor = anchor, .position = anchor != nullptr ? anchor->pos() : position};
    startLink(link->provided());
    return true;
}

LinkItem::Stop EditionScene::pendingStart() const
{
    return m_branch ? m_branch->link->branchStop(m_branch->position) : LinkItem::stopAt(*m_linkSource);
}

void EditionScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if(m_linkSource == nullptr)
    {
        QGraphicsScene::mouseMoveEvent(event);
        return;
    }
    const InterfaceItem* hovered = interfaceAt(event->scenePos());
    showTypeOf(hovered, *event);
    drawPendingLink(hovered, event->scenePos());
    event->accept();
}

void EditionScene::showTypeOf(const InterfaceItem* hovered, const QGraphicsSceneMouseEvent& event) const
{
    QToolTip::showText(event.screenPos(),
                       hovered != nullptr && hovered != m_linkSource ? hovered->toolTip() : QString(), event.widget());
}

void EditionScene::drawPendingLink(const InterfaceItem* hovered, const QPointF& cursor)
{
    const InterfaceItem* target = hovered != nullptr && m_linkSource->canLinkTo(*hovered) ? hovered : nullptr;
    const LinkItem::Stop start = pendingStart();
    // The cursor is gone through the way its start is, as the dot of a component across from it would be.
    const LinkItem::Stop end =
        target != nullptr ? LinkItem::stopAt(*target) : LinkItem::Stop{.point = cursor, .leftwards = start.leftwards};
    m_pendingLink->setPath(isProvided(*m_linkSource) ? LinkItem::route(m_linkStyle, start, end)
                                                     : LinkItem::route(m_linkStyle, end, start));
}

void EditionScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    releaseButton(*event);
    if(event->button() == Qt::LeftButton)
    {
        endGesture(); // Once the items let go of what they dragged
    }
}

void EditionScene::releaseButton(QGraphicsSceneMouseEvent& event)
{
    if(m_linkSource == nullptr || event.button() != Qt::LeftButton)
    {
        QGraphicsScene::mouseReleaseEvent(&event);
        return;
    }
    finishLink(event.scenePos());
    event.accept();
}

void EditionScene::finishLink(const QPointF& position)
{
    InterfaceItem* source = m_linkSource;
    const std::optional<Branch> branch = m_branch;
    InterfaceItem* target = linkTargetAt(position);
    endLink();
    if(target != nullptr && isEditable()) // Not when it was started while the link was drawn
    {
        nameGesture(bindingName(*source, *target));
        branch ? linkThrough(*branch, *target) : link(*source, *target);
    }
}

void EditionScene::endLink()
{
    m_linkSource = nullptr;
    m_branch.reset();
    delete m_pendingLink;
    m_pendingLink = nullptr;
    dimLinkTargets(nullptr);
    QToolTip::hideText();
}

bool EditionScene::abandonLink(const QKeyEvent& event)
{
    const bool abandoned = event.key() == Qt::Key_Escape && m_linkSource != nullptr;
    if(abandoned)
    {
        endLink(); // The release of the button then goes to the scene, which nothing grabs it for
    }
    return abandoned;
}

void EditionScene::linkThrough(const Branch& branch, InterfaceItem& consumed)
{
    InterfaceItem& provided = branch.link->provided();
    if(consumed.linkTo(provided) == nullptr)
    {
        link(provided, consumed); // Which draws the links again: the one it branches off stays, still bound
    }
    LinkItem* joined = consumed.linkTo(provided);
    if(joined != nullptr && !goesThrough(branch, *joined))
    {
        joined->setAnchorItems(branch.link->anchorsUpTo(junctionOf(branch)));
    }
}

AnchorItem& EditionScene::junctionOf(const Branch& branch)
{
    return branch.anchor != nullptr ? *branch.anchor : branch.link->addAnchor(branch.position);
}

bool EditionScene::goesThrough(const Branch& branch, LinkItem& link)
{
    return branch.anchor != nullptr ? link.anchorItems().contains(branch.anchor)
                                    : branch.link->linksAlong(branch.position).contains(&link);
}

void EditionScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    beginGesture(*event); // The second press of the double-click, which its release ends
    if(!isEditable() || event->button() != Qt::LeftButton || !doubleClick(event->scenePos(), event->widget()))
    {
        QGraphicsScene::mouseDoubleClickEvent(event); // Which presses an item that does nothing else of it
        nameGesture(dragName());
        return;
    }
    event->accept();
}

bool EditionScene::doubleClick(const QPointF& position, QWidget* parent)
{
    return editSettingsAt(position, parent) || removeAnchorAt(position) || anchorLinkAt(position);
}

bool EditionScene::editSettingsAt(const QPointF& position, QWidget* parent)
{
    const SettingsItem* settings = settingsAt(position);
    if(settings != nullptr)
    {
        editSettings(*settings, parent);
    }
    return settings != nullptr;
}

bool EditionScene::removeAnchorAt(const QPointF& position)
{
    AnchorItem* anchor = anchorAt(position);
    if(anchor != nullptr)
    {
        nameGesture(anchor->isJunction() ? tr("Remove junction point") : tr("Remove anchor point"));
        anchor->isJunction() ? LinkItem::removeJunction(*anchor) : LinkItem::removeAnchor(*anchor);
    }
    return anchor != nullptr;
}

bool EditionScene::anchorLinkAt(const QPointF& position)
{
    LinkItem* link = linkAt(position);
    if(link != nullptr)
    {
        nameGesture(tr("Add anchor point"));
        link->addAnchor(position);
    }
    return link != nullptr;
}

void EditionScene::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    if(isBusy())
    {
        return;
    }
    event->accept();
    if(checkEditable() && !showDotMenu(*event)) // Everything it does edits the application
    {
        showMenu(*event);
    }
}

const InterfaceItem* EditionScene::shiftClickedDot(const QGraphicsSceneContextMenuEvent& event) const
{
    // Not from the keyboard, which Shift+F10 opens the menu from as well.
    const bool shifted =
        event.reason() == QGraphicsSceneContextMenuEvent::Mouse && event.modifiers().testFlag(Qt::ShiftModifier);
    return shifted ? interfaceAt(event.scenePos()) : nullptr;
}

bool EditionScene::showDotMenu(const QGraphicsSceneContextMenuEvent& event)
{
    const InterfaceItem* dot = shiftClickedDot(event);
    if(dot == nullptr)
    {
        return false;
    }
    ContextMenu menu(event.widget());
    addSideEntries(menu, *dot);
    addSwapEntries(menu, *dot);
    execMenu(menu, event);
    return true;
}

void EditionScene::addSideEntries(ContextMenu& menu, const InterfaceItem& dot)
{
    QActionGroup& group = menu.addGroup();
    for(const InterfaceItem::Side side : {InterfaceItem::Side::Left, InterfaceItem::Side::Right})
    {
        QAction* entry = menu.add(sideText(side), [item = &dot.componentItem(), kind = dot.kind(), name = dot.name(),
                                                   side] { item->setInterfaceSide(kind, name, side); });
        entry->setCheckable(true);
        entry->setChecked(dot.side() == side);
        entry->setActionGroup(&group);
    }
}

void EditionScene::addSwapEntries(ContextMenu& menu, const InterfaceItem& dot)
{
    menu.addSeparator();
    QMenu& swapMenu = menu.addMenu(tr("Swap with"));
    const QList<InterfaceItem*> rows = dot.componentItem().interfaceRows();
    if(rows.size() < 2)
    {
        swapMenu.addAction(tr("It has no other interface"))->setEnabled(false);
    }
    std::ranges::for_each(rows, [&menu, &swapMenu, &dot](const InterfaceItem* row)
                          { addSwapEntry(menu, swapMenu, dot, *row); });
}

void EditionScene::showMenu(const QGraphicsSceneContextMenuEvent& event)
{
    const QPointF position = event.scenePos();
    ContextMenu menu(event.widget());
    if(!addComponentEntries(menu, componentAt(position), position, event.widget()) &&
       !addSettingsEntries(menu, settingsAt(position), event.widget()) && !addAnchorEntries(menu, anchorAt(position)) &&
       !addLinkEntries(menu, linkAt(position), position))
    {
        addSceneEntries(menu, position, event.widget());
    }
    addZoneEntries(menu, position, event.widget());
    execMenu(menu, event);
}

bool EditionScene::addComponentEntries(ContextMenu& menu, ComponentItem* component, const QPointF& position,
                                       QWidget* parent)
{
    if(component == nullptr)
    {
        return false;
    }
    addBindEntries(menu, interfaceAt(position));
    addKindEntries(menu, *component, position, parent);
    addComponentActions(menu, *component, parent);
    return true;
}

void EditionScene::addBindEntries(ContextMenu& menu, InterfaceItem* consumed)
{
    if(consumed == nullptr || isProvided(*consumed))
    {
        return;
    }
    addBindTargets(menu, menu.addMenu(tr("Bind to")), *consumed);
    menu.addSeparator();
}

void EditionScene::addBindTargets(ContextMenu& menu, QMenu& bindMenu, InterfaceItem& consumed)
{
    const QList<InterfaceItem*> targets = linkTargets(consumed);
    if(targets.isEmpty())
    {
        addNoTarget(bindMenu, consumed);
    }
    QActionGroup* group = choiceGroup(bindMenu, consumed);
    for(InterfaceItem* provided : targets)
    {
        addBindTarget(menu, bindMenu, consumed, *provided)->setActionGroup(group);
    }
}

QAction* EditionScene::addBindTarget(ContextMenu& menu, QMenu& bindMenu, InterfaceItem& consumed,
                                     InterfaceItem& provided)
{
    QAction* entry = menu.add(bindMenu, qualifiedName(provided),
                              [this, consumer = &consumed, provider = &provided] { toggleLink(*consumer, *provider); });
    entry->setCheckable(true);
    entry->setChecked(consumed.linkTo(provided) != nullptr);
    return entry;
}

void EditionScene::addKindEntries(ContextMenu& menu, ComponentItem& component, const QPointF& position, QWidget* parent)
{
    auto* stargate = qobject_cast<StargateItem*>(&component);
    // The interface whose dot is under the cursor, of a component that is not a stargate: it exposes its own.
    addExposeEntry(menu, stargate == nullptr ? interfaceAt(position) : nullptr);
    addOpenEntry(menu, qobject_cast<Nebula::Galaxy*>(&component.component()));
    addEditEntry(menu, stargate, parent);
}

void EditionScene::addExposeEntry(ContextMenu& menu, const InterfaceItem* exposed)
{
    if(exposed == nullptr)
    {
        return;
    }
    menu.add(
        tr("Expose..."), [this, exposed] { expose(*exposed); }, icon(GlossaryTerm::Stargate),
        tr("Adds a stargate that exposes %1, for the galaxy of the project to have it").arg(exposed->name()));
    menu.addSeparator();
}

void EditionScene::addOpenEntry(ContextMenu& menu, Nebula::Galaxy* galaxy)
{
    if(galaxy == nullptr)
    {
        return;
    }
    menu.add(tr("Open galaxy"), [this, galaxy] { emit galaxyOpened(*galaxy); }, icon(GlossaryTerm::Galaxy));
    menu.addSeparator();
}

void EditionScene::addEditEntry(ContextMenu& menu, StargateItem* stargate, QWidget* parent)
{
    if(stargate == nullptr)
    {
        return;
    }
    menu.add(
        tr("Edit stargate..."), [this, stargate, parent] { editStargate(*stargate, parent); },
        icon(GlossaryTerm::Stargate));
    menu.addSeparator();
}

void EditionScene::addComponentActions(ContextMenu& menu, ComponentItem& component, QWidget* parent)
{
    menu.add(tr("Rename component"), [item = &component] { item->editName(); });
    menu.add(tr("Change component color..."), [this, item = &component, parent] { recolor(*item, parent); });
    menu.add(tr("Reset component color"), [item = &component] { item->setColor({}); })
        ->setEnabled(component.color().isValid());
    menu.add(tr("Remove component"), [this, item = &component] { remove(*item); });
}

bool EditionScene::addSettingsEntries(ContextMenu& menu, SettingsItem* settings, QWidget* parent)
{
    if(settings == nullptr)
    {
        return false;
    }
    addSettingsActions(menu, *settings, parent);
    return true;
}

void EditionScene::addSettingsActions(ContextMenu& menu, SettingsItem& settings, QWidget* parent)
{
    menu.add(
        tr("Edit settings file..."), [this, item = &settings, parent] { editSettings(*item, parent); },
        icon(GlossaryTerm::Comet));
    menu.add(
        tr("View and edit file..."), [item = &settings, parent] { editText(*item, parent); }, {},
        tr("Shows the text of the file, where the values of the user are, and writes it as it is typed"));
    menu.add(
        tr("Write default values"), [this, item = &settings] { writeDefaults(*item); }, {},
        tr("Writes the values of the project to the file, in place of those of the user"));
    menu.add(tr("Remove settings file"), [this, item = &settings] { remove(*item); });
}

bool EditionScene::addAnchorEntries(ContextMenu& menu, AnchorItem* anchor)
{
    if(anchor == nullptr)
    {
        return false;
    }
    addRemoveEntry(menu, *anchor);
    return true;
}

void EditionScene::addRemoveEntry(ContextMenu& menu, AnchorItem& anchor)
{
    if(!anchor.isJunction())
    {
        menu.add(tr("Remove anchor point"), [item = &anchor] { LinkItem::removeAnchor(*item); });
        return;
    }
    menu.add(
        tr("Remove junction point"), [item = &anchor] { LinkItem::removeJunction(*item); }, {},
        tr("Has each link through it go from its provided interface again, without the line they share"));
}

bool EditionScene::addLinkEntries(ContextMenu& menu, LinkItem* link, const QPointF& position)
{
    if(link == nullptr)
    {
        return false;
    }
    const QList<LinkItem*> along = link->linksAlong(position);
    addAnchoringEntries(menu, *link, position, along);
    menu.add(along.size() == 1 ? tr("Remove link") : tr("Remove links"), [this, along] { unlink(along); });
    return true;
}

void EditionScene::addAnchoringEntries(ContextMenu& menu, LinkItem& link, const QPointF& position,
                                       const QList<LinkItem*>& along)
{
    menu.add(
        tr("Add anchor point"), [item = &link, position] { item->addAnchor(position); }, {},
        tr("Has the line go through this point, wherever its components are"));
    menu.add(tr("Remove anchor points"),
             [along] { std::ranges::for_each(along, [](LinkItem* item) { item->setAnchors({}); }); })
        ->setEnabled(std::ranges::any_of(along, [](const LinkItem* item) { return !item->anchors().isEmpty(); }));
    menu.addSeparator();
}

void EditionScene::addSceneEntries(ContextMenu& menu, const QPointF& position, QWidget* parent)
{
    menu.add(
        tr("Add zone"), [this, position] { addZone(position, ZoneSize, tr("Zone"))->editTitle(); },
        icon(GlossaryTerm::Galaxy));
    menu.add(
        tr("Add settings file"), [this, position, parent] { addSettingsFileAt(position, parent); },
        icon(GlossaryTerm::Comet));
    menu.add(
        tr("Add stargate"), [this, position, parent] { addStargateAt(position, parent); },
        icon(GlossaryTerm::Stargate));
}

void EditionScene::addZoneEntries(ContextMenu& menu, const QPointF& position, QWidget* parent)
{
    ZoneItem* zone = zoneAt(position);
    if(zone == nullptr)
    {
        return;
    }
    menu.addSeparator();
    menu.add(tr("Rename zone"), [zone] { zone->editTitle(); });
    menu.add(tr("Change zone color..."), [this, zone, parent] { recolor(*zone, parent); });
    menu.add(tr("Remove zone"), [this, zone] { removeZone(*zone); });
}

void EditionScene::recolor(ComponentItem& component, QWidget* parent)
{
    const QColor current = component.color().isValid() ? component.color() : palette().color(QPalette::Button);
    const QColor color = QColorDialog::getColor(current, parent, tr("Component color"), QColorDialog::ShowAlphaChannel);
    if(color.isValid())
    {
        component.setColor(color);
    }
}

void EditionScene::recolor(ZoneItem& zone, QWidget* parent)
{
    const QColor color = QColorDialog::getColor(zone.color(), parent, tr("Zone color"));
    if(color.isValid())
    {
        zone.setColor(color);
    }
}

void EditionScene::removeZone(ZoneItem& zone)
{
    delete &zone;
    update(HintRect); // Shown again once nothing is left
}

void EditionScene::addSettingsFileAt(const QPointF& position, QWidget* parent)
{
    Nebula::SettingsFile& file =
        m_galaxy != nullptr ? *m_application.addSettingsFile(*m_galaxy) : m_application.addSettingsFile();
    editSettings(*showSettings(file, position), parent);
}

void EditionScene::addStargateAt(const QPointF& position, QWidget* parent)
{
    auto* added = qobject_cast<StargateItem*>(showComponent(*m_application.addStargate({}, m_galaxy), position));
    selectOnly(*added);
    editStargate(*added, parent);
}

void EditionScene::keyPressEvent(QKeyEvent* event)
{
    if(!abandonLink(*event) && !removeOnDelete(*event))
    {
        QGraphicsScene::keyPressEvent(event);
        return;
    }
    event->accept();
}

bool EditionScene::removeOnDelete(const QKeyEvent& event)
{
    if(event.key() != Qt::Key_Delete || focusItem() != nullptr || isBusy())
    {
        return false;
    }
    if(checkEditable())
    {
        edit(tr("Remove %1").arg(selectionName()), [this] { removeSelected(); });
    }
    return true;
}

void EditionScene::removeSelected()
{
    // Sorted before any goes: unlinking destroys the links, which are among the items selected.
    const QList<QGraphicsItem*> selected = selectedItems();
    const QList<LinkItem*> links = itemsAmong<LinkItem>(selected);
    const QList<SettingsItem*> files = objectsAmong<SettingsItem>(selected);
    const QList<ComponentItem*> components = objectsAmong<ComponentItem>(selected);
    clearSelection(); // Told once, rather than as each of them goes
    // The links before the components, which they would go with.
    unlink(links);
    // The files first: they would otherwise join themselves again to what is left as each of their components goes.
    removeAll(files);
    removeAll(components);
}

bool EditionScene::isEditable() const
{
    return !m_application.isRunning();
}

bool EditionScene::checkEditable()
{
    const bool editable = isEditable();
    if(!editable)
    {
        refuseEdition();
    }
    return editable;
}

void EditionScene::setEditable(bool editable)
{
    const QList<QGraphicsItem*> shown = items();
    std::ranges::for_each(objectsAmong<ComponentItem>(shown),
                          [editable](ComponentItem* item) { item->setEditable(editable); });
    std::ranges::for_each(objectsAmong<SettingsItem>(shown),
                          [editable](SettingsItem* item) { item->setFlag(QGraphicsItem::ItemIsMovable, editable); });
    std::ranges::for_each(itemsAmong<ZoneItem>(shown), [editable](ZoneItem* item) { item->setEditable(editable); });
    std::ranges::for_each(itemsAmong<LinkItem>(shown), [editable](LinkItem* item) { item->setEditable(editable); });
}

void EditionScene::refuseEdition()
{
    emit refused(tr("The application is not edited while it runs: stop it first"));
}

InterfaceItem* EditionScene::interfaceAt(const QPointF& position) const
{
    return firstAmong<InterfaceItem>(items(position));
}

InterfaceItem* EditionScene::linkTargetAt(const QPointF& position) const
{
    InterfaceItem* target = interfaceAt(position);
    return target != nullptr && m_linkSource != nullptr && m_linkSource->canLinkTo(*target) ? target : nullptr;
}

void EditionScene::dimLinkTargets(const InterfaceItem* source)
{
    const QList<InterfaceItem*> shown = itemsAmong<InterfaceItem>(items());
    for(InterfaceItem* item : shown)
    {
        item->setDimmed(source != nullptr && item != source && !source->canLinkTo(*item));
    }
}

QList<InterfaceItem*> EditionScene::linkTargets(const InterfaceItem& consumed) const
{
    QList<InterfaceItem*> targets;
    // From the first one added, as save() lists them.
    const QList<ComponentItem*> shown = objectsAmong<ComponentItem>(items(Qt::AscendingOrder));
    for(const ComponentItem* item : shown)
    {
        std::ranges::copy_if(item->providedInterfaces(), std::back_inserter(targets),
                             [&consumed](const InterfaceItem* provided) { return consumed.canLinkTo(*provided); });
    }
    return targets;
}

ComponentItem* EditionScene::showComponent(Nebula::Component& component, const QPointF& position)
{
    ComponentItem* item = newItem(component);
    // Its links again, once its interfaces were shown again.
    connect(item, &ComponentItem::interfacesRebuilt, this, &EditionScene::refreshLinks);
    connect(item, &ComponentItem::nameEdited, this,
            [this, &component](const QString& name) { renameEdited(component, name); });
    place(*item, position);
    return item;
}

ComponentItem* EditionScene::newItem(Nebula::Component& component)
{
    if(auto* stargate = qobject_cast<Nebula::Stargate*>(&component))
    {
        return newStargateItem(*stargate);
    }
    if(auto* galaxy = qobject_cast<Nebula::Galaxy*>(&component))
    {
        return newGalaxyItem(*galaxy);
    }
    const std::optional<Nebula::ComponentInfo> info = m_factory.component(component.componentId());
    return new ComponentItem(component, info ? info->name : QString());
}

ComponentItem* EditionScene::newStargateItem(Nebula::Stargate& stargate)
{
    auto* item = new StargateItem(stargate);
    // Edited in a dialog of the view it was double-clicked in.
    connect(item, &ComponentItem::openRequested, this, [this, item] { editStargate(*item, views().value(0)); });
    connect(item, &ComponentItem::interfaceNameEdited, this,
            [this, exposing = &stargate](InterfaceItem::Kind kind, const QString& name, const QString& newName)
            { renameExposed(*exposing, kind, name, newName); });
    return item;
}

ComponentItem* EditionScene::newGalaxyItem(Nebula::Galaxy& galaxy)
{
    auto* item = new ComponentItem(galaxy, galaxy.galaxyName());
    item->setIcon(icon(GlossaryTerm::Galaxy));
    item->setOpenable(aboutGalaxy(galaxy), tr("Double-click below its name to open it"));
    connect(item, &ComponentItem::openRequested, this, [this, opened = &galaxy] { emit galaxyOpened(*opened); });
    return item;
}

ComponentItem* EditionScene::componentAt(const QPointF& position) const
{
    for(const QGraphicsItem* item : items(position))
    {
        // Through what it is the parent of: the dots of its interfaces go past its edges.
        if(auto* component = qobject_cast<ComponentItem*>(item->topLevelItem()->toGraphicsObject()))
        {
            return component;
        }
    }
    return nullptr;
}

void EditionScene::renameEdited(Nebula::Component& component, const QString& name)
{
    edit(tr("Rename %1 to %2").arg(component.objectName(), name),
         [this, &component, &name] { rename(component, name); });
}

void EditionScene::rename(Nebula::Component& component, const QString& name)
{
    // No name is one given up rather than refused.
    if(!name.isEmpty() && !m_application.rename(component, name))
    {
        emit refused(tr("%1 is not renamed: another component is named %2").arg(component.objectName(), name));
    }
}

void EditionScene::remove(ComponentItem& item)
{
    Nebula::Component& component = item.component();
    item.setSelected(false); // Who listens lets go of the component before it goes
    delete &item;            // With its interfaces, and their links
    m_anchors.keepOnly(itemsAmong<LinkItem>(items()));
    m_application.remove(component);
    update(HintRect); // Shown again once nothing is left
}

void EditionScene::removeItems()
{
    // Nothing is selected, nor being linked, any longer: who listens lets go of the components before they go.
    clearSelection();
    m_linkSource = nullptr;
    m_branch.reset();
    m_pendingLink = nullptr;
    clear();
    m_anchors.clear();
    m_zoneCount = 0;
    update(HintRect); // Shown again
}

void EditionScene::link(InterfaceItem& source, InterfaceItem& target)
{
    InterfaceItem& provided = isProvided(source) ? source : target;
    InterfaceItem& consumed = isProvided(source) ? target : source;
    // What a component consumes is bound to one interface: the new one, which the links show. A list keeps the others.
    if(consumed.component().bindInterface(consumed.name(), provided.component(), provided.name()))
    {
        refreshLinks();
    }
}

LinkItem* EditionScene::linkAt(const QPointF& position) const
{
    return qgraphicsitem_cast<LinkItem*>(itemAt(position, QTransform())); // The topmost item
}

AnchorItem* EditionScene::anchorAt(const QPointF& position) const
{
    return qgraphicsitem_cast<AnchorItem*>(itemAt(position, QTransform())); // The topmost item
}

void EditionScene::unlink(const QList<LinkItem*>& links)
{
    if(links.isEmpty())
    {
        return;
    }
    const QList<Binding> bindings = bindingsOf(links);
    std::ranges::for_each(bindings, &Binding::unbind);
    refreshLinks(); // Which no longer draws them
}

void EditionScene::toggleLink(InterfaceItem& consumed, InterfaceItem& provided)
{
    if(LinkItem* shown = consumed.linkTo(provided))
    {
        unlink({shown});
    }
    else
    {
        link(consumed, provided);
    }
}

ZoneItem* EditionScene::zoneAt(const QPointF& position) const
{
    // By their bounds, as their shape is only their title and their edges; the topmost one is the smallest.
    return firstAmong<ZoneItem>(items(position, Qt::IntersectsItemBoundingRect));
}

ZoneItem* EditionScene::addZone(const QPointF& position, const QSizeF& size, const QString& title, const QColor& color)
{
    const QColor next(ZoneColors.at(m_zoneCount++ % ZoneColors.size()));
    auto* zone = new ZoneItem(size, title, color.isValid() ? color : next);
    zone->setTitleEdited([this, zone](const QString& typed) { retitle(*zone, typed); });
    place(*zone, position);
    return zone;
}

void EditionScene::retitle(ZoneItem& zone, const QString& title)
{
    edit(tr("Rename zone %1 to %2").arg(zone.title(), title), [&zone, &title] { zone.setTitle(title); });
}

SettingsItem* EditionScene::showSettings(Nebula::SettingsFile& file, const QPointF& position)
{
    auto* item = new SettingsItem(file);
    place(*item, position);
    return item;
}

SettingsItem* EditionScene::settingsAt(const QPointF& position) const
{
    QGraphicsItem* item = itemAt(position, QTransform()); // The topmost one
    return item != nullptr ? qobject_cast<SettingsItem*>(item->toGraphicsObject()) : nullptr;
}

void EditionScene::editSettings(const SettingsItem& item, QWidget* parent)
{
    auto* dialog = new SettingsDialog(item.file(), m_application, m_factory, parent);
    // What it gives the file, one step: nothing else is edited meanwhile, the dialog blocks its window.
    connect(dialog, &QDialog::accepted, this,
            [this, before = snapshot(), file = &item.file()] { record(tr("Edit %1").arg(file->fileName()), before); });
    openDialog(*dialog);
}

void EditionScene::editText(const SettingsItem& item, QWidget* parent)
{
    openDialog(*new SettingsTextDialog(item.file(), parent));
}

void EditionScene::writeDefaults(const SettingsItem& item)
{
    Nebula::SettingsFile& file = item.file();
    if(file.writeDefaults())
    {
        emit told(tr("The default values are written to %1").arg(QDir::toNativeSeparators(file.filePath())));
    }
    else
    {
        emit refused(tr("The default values are not written: %1").arg(file.errorString()));
    }
}

void EditionScene::remove(SettingsItem& item)
{
    Nebula::SettingsFile& file = item.file();
    item.setSelected(false);
    delete &item; // With its links
    m_application.removeSettingsFile(file);
    update(HintRect); // Shown again once nothing is left
}

void EditionScene::refreshLinks()
{
    drawBindings();
    m_anchors.keepOnly(itemsAmong<LinkItem>(items())); // Those of what is no longer bound go
}

void EditionScene::drawBindings()
{
    const ShownComponents shown = byKey<const Nebula::Component*>(
        objectsAmong<ComponentItem>(items()), [](const ComponentItem& item) { return &item.component(); });
    for(const ComponentItem* item : shown)
    {
        for(InterfaceItem* consumed : item->consumedInterfaces())
        {
            drawLinks(*consumed, providedItems(shown, item->component().bindings(consumed->name())));
        }
    }
}

void EditionScene::drawLinks(InterfaceItem& consumed, const QList<InterfaceItem*>& provided)
{
    const QList<InterfaceItem*> missing = unlinked(consumed, provided);
    for(InterfaceItem* item : missing)
    {
        drawLink(*item, consumed);
    }
}

void EditionScene::drawLink(InterfaceItem& provided, InterfaceItem& consumed)
{
    auto* link =
        new LinkItem(provided, consumed, m_linkStyle, [this](const LinkItem& anchored) { m_anchors.keep(anchored); });
    addItem(link); // Before its anchors, which join its scene
    link->setEditable(isEditable());
    // Those the links of provided already drawn go through first are the same points: their line is one up to there.
    link->setAnchors(m_anchors.of(link->ends()));
}

void EditionScene::editStargate(const StargateItem& item, QWidget* parent)
{
    if(!checkEditable())
    {
        return;
    }
    auto* dialog = new StargateDialog(item.stargate(), m_application, m_factory, parent);
    connect(dialog, &QDialog::accepted, this, &EditionScene::refreshLinks); // What it bound and unbound
    // Then what it changed, one step, as SettingsDialog does.
    connect(dialog, &QDialog::accepted, this, [this, before = snapshot(), stargate = &item.stargate()]
            { record(tr("Edit %1").arg(stargate->objectName()), before); });
    openDialog(*dialog);
}

void EditionScene::expose(const InterfaceItem& interfaceItem)
{
    const std::optional<QMetaType> type = interfaceType(interfaceItem);
    Nebula::Stargate* stargate = type ? m_application.addStargate({}, m_galaxy) : nullptr;
    if(!type || stargate == nullptr)
    {
        return;
    }
    const QString name = exposeAs(*stargate, interfaceItem, *type);
    bindExposed(*stargate, name, interfaceItem);
    showExposing(*stargate, name, interfaceItem);
}

void EditionScene::showExposing(Nebula::Stargate& stargate, const QString& name, const InterfaceItem& exposed)
{
    auto* item = qobject_cast<StargateItem*>(showComponent(stargate, exposedPosition(exposed)));
    item->setInterfaceSide(exposingKind(exposed), name, across(exposed.side())); // Facing the dot of exposed
    refreshLinks();
    selectOnly(*item);
    if(const InterfaceItem* named = item->interfaceItem(exposingKind(exposed), name))
    {
        item->editInterfaceName(*named);
    }
}

void EditionScene::renameExposed(Nebula::Stargate& stargate, InterfaceItem::Kind kind, const QString& name,
                                 const QString& newName)
{
    edit(tr("Rename %1 to %2").arg(name, newName),
         [this, &stargate, kind, &name, &newName] { renameInterface(stargate, kind, name, newName); });
}

void EditionScene::renameInterface(Nebula::Stargate& stargate, InterfaceItem::Kind kind, const QString& name,
                                   const QString& newName)
{
    // What the stargate consumes, on its left edge, the galaxy provides, and the other way round.
    const Nebula::Exposure exposure =
        kind == InterfaceItem::Kind::Consumed ? Nebula::Exposure::Provided : Nebula::Exposure::Consumed;
    // No name is one given up rather than refused.
    if(!newName.isEmpty() && !stargate.renameInterface(exposure, name, newName))
    {
        emit refused(tr("%1 is not renamed: another interface is exposed as %2 the same way").arg(name, newName));
    }
}

void EditionScene::drawBackground(QPainter* painter, const QRectF& rect)
{
    QGraphicsScene::drawBackground(painter, rect);
    painter->save();
    drawGrid(*painter, rect);
    if(items().isEmpty())
    {
        drawHint(*painter);
    }
    painter->restore();
}

void EditionScene::drawGrid(QPainter& painter, const QRectF& rect) const
{
    QColor lineColor = palette().color(QPalette::Mid);
    lineColor.setAlphaF(0.25F);
    painter.setPen(QPen(lineColor, 0));
    painter.drawLines(gridLines(rect));
}

void EditionScene::drawHint(QPainter& painter) const
{
    painter.setPen(palette().color(QPalette::PlaceholderText));
    painter.drawText(HintRect, Qt::AlignLeft | Qt::AlignVCenter,
                     m_galaxy != nullptr
                         ? tr("Drag a component from the list of components and drop it here to add it to the galaxy\n"
                              "Right-click to add a zone, which groups components, a settings file or a stargate")
                         : tr("Drag a component from the list of components and drop it here to instantiate it\n"
                              "Right-click to add a zone, which groups components, a settings file or a stargate"));
}
