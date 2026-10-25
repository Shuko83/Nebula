// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "EditionScene.h"

#include <QColorDialog>
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
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <QPalette>
#include <QToolTip>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "ComponentItem.h"
#include "ComponentModel.h"
#include "Galaxy/GalaxyModel.h"
#include "Galaxy/StargateDialog.h"
#include "Galaxy/StargateItem.h"
#include "InterfaceItem.h"
#include "LinkItem.h"
#include "Nebula/Application/Universe.h"
#include "Nebula/Component/ComponentFactory.h"
#include "Settings/SettingsDialog.h"
#include "Settings/SettingsItem.h"
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

// Where an item is, as the "x" and "y" of what a project file holds about it.
void writePosition(QJsonObject& json, const QPointF& position)
{
    json.insert(u"x", position.x());
    json.insert(u"y", position.y());
}

QPointF readPosition(const QJsonObject& json)
{
    return {json.value(u"x").toDouble(), json.value(u"y").toDouble()};
}

// The one of interfaces named name; nullptr when none is.
InterfaceItem* interfaceNamed(const QList<InterfaceItem*>& interfaces, const QString& name)
{
    const auto found = std::ranges::find(interfaces, name, &InterfaceItem::name);
    return found != interfaces.end() ? *found : nullptr;
}

} // namespace

EditionScene::EditionScene(Nebula::Universe& application, const Nebula::ComponentFactory& factory, QObject* parent)
    : QGraphicsScene(SceneRect, parent), m_application(application), m_factory(factory)
{
    connect(&m_application, &Nebula::Universe::runningChanged, this, [this] { setEditable(isEditable()); });
}

EditionScene::EditionScene(Nebula::Universe& application, const Nebula::ComponentFactory& factory,
                           Nebula::Galaxy& galaxy, QObject* parent)
    : QGraphicsScene(SceneRect, parent), m_application(application), m_factory(factory), m_galaxy(&galaxy)
{
    connect(&m_application, &Nebula::Universe::runningChanged, this, [this] { setEditable(isEditable()); });
    // Its items go before what they show, and come again with what its file holds now.
    connect(&galaxy, &Nebula::Galaxy::aboutToBeReloaded, this,
            [this]
            {
                m_selected.clear();
                for(const QGraphicsItem* item : selectedItems())
                {
                    if(const auto* component = qobject_cast<const ComponentItem*>(item->toGraphicsObject()))
                    {
                        m_selected.append(component->component().objectName());
                    }
                }
                removeItems();
            });
    connect(&galaxy, &Nebula::Galaxy::reloaded, this, &EditionScene::showGalaxyAgain);
    show(galaxy.components(), galaxy.settingsFiles(), galaxy.project());
    setEditable(isEditable());
    m_saved = save();
}

Nebula::Galaxy* EditionScene::galaxy() const
{
    return m_galaxy;
}

bool EditionScene::isModified() const
{
    return m_galaxy != nullptr && save() != m_saved;
}

void EditionScene::showGalaxyAgain()
{
    show(m_galaxy->components(), m_galaxy->settingsFiles(), m_galaxy->project());
    setEditable(isEditable());
    for(QGraphicsItem* item : items())
    {
        auto* component = qobject_cast<ComponentItem*>(item->toGraphicsObject());
        if(component != nullptr && m_selected.contains(component->component().objectName()))
        {
            component->setSelected(true);
        }
    }
    m_selected.clear();
    m_saved = save();
}

Nebula::Component* EditionScene::selectedComponent() const
{
    // Not a settings file nor a link, which can be selected too.
    const QList<QGraphicsItem*> selected = selectedItems();
    const auto* item =
        selected.size() == 1 ? qobject_cast<const ComponentItem*>(selected.constFirst()->toGraphicsObject()) : nullptr;
    return item != nullptr ? &item->component() : nullptr;
}

ComponentItem* EditionScene::select(const Nebula::Component& component)
{
    clearSelection();
    for(QGraphicsItem* item : items())
    {
        auto* componentItem = qobject_cast<ComponentItem*>(item->toGraphicsObject());
        if(componentItem != nullptr && &componentItem->component() == &component)
        {
            componentItem->setSelected(true);
            return componentItem;
        }
    }
    return nullptr;
}

QJsonObject EditionScene::save() const
{
    QHash<QString, const ComponentItem*> shown; // The components, by name
    QHash<const Nebula::SettingsFile*, const SettingsItem*> shownSettings;
    QJsonArray zones;
    // From the first one added: load() adds them in this order, which is then the same again.
    const QList<QGraphicsItem*> added = items(Qt::AscendingOrder);
    for(const QGraphicsItem* item : added)
    {
        if(const auto* componentItem = qobject_cast<const ComponentItem*>(item->toGraphicsObject()))
        {
            shown.insert(componentItem->component().objectName(), componentItem);
        }
        else if(const auto* settingsItem = qobject_cast<const SettingsItem*>(item->toGraphicsObject()))
        {
            shownSettings.insert(&settingsItem->file(), settingsItem);
        }
        else if(const auto* zone = qgraphicsitem_cast<const ZoneItem*>(item))
        {
            QJsonObject json;
            json.insert(u"title", zone->title());
            json.insert(u"color", zone->color().name());
            writePosition(json, zone->pos());
            json.insert(u"width", zone->boundingRect().width());
            json.insert(u"height", zone->boundingRect().height());
            zones.append(json);
        }
    }

    QJsonObject project = m_galaxy != nullptr ? m_application.saveGalaxy(*m_galaxy) : m_application.save();
    QJsonArray components = project.value(u"components").toArray();
    for(QJsonValueRef component : components)
    {
        QJsonObject json = component.toObject();
        if(const ComponentItem* item = shown.value(json.value(u"name").toString()))
        {
            writePosition(json, item->pos());
            if(item->color().isValid()) // None while it has the colors of the style
            {
                json.insert(u"color", item->color().name(QColor::HexArgb)); // With its opacity
            }
        }
        component = json;
    }
    project.insert(u"components", components);

    // In the order of the files of what it shows, as the application wrote them.
    QJsonArray settings = project.value(u"settings").toArray();
    const std::vector<std::unique_ptr<Nebula::SettingsFile>>& files =
        m_galaxy != nullptr ? m_galaxy->settingsFiles() : m_application.settingsFiles();
    for(qsizetype index = 0; index < settings.size(); ++index)
    {
        QJsonObject json = settings.at(index).toObject();
        if(const SettingsItem* item = shownSettings.value(files.at(static_cast<std::size_t>(index)).get()))
        {
            writePosition(json, item->pos());
        }
        settings.replace(index, json);
    }
    if(!settings.isEmpty())
    {
        project.insert(u"settings", settings);
    }
    project.insert(u"zones", zones);
    return project;
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
    QHash<QString, QJsonObject> saved; // What the project holds of the components, by name
    const QJsonArray savedComponents = project.value(u"components").toArray();
    for(const QJsonValue& value : savedComponents)
    {
        const QJsonObject json = value.toObject();
        saved.insert(json.value(u"name").toString(), json);
    }
    QList<StargateItem*> stargates;
    for(const std::unique_ptr<Nebula::Component>& component : components)
    {
        const QJsonObject json = saved.value(component->objectName());
        ComponentItem* item = showComponent(*component, readPosition(json));
        item->setColor(QColor::fromString(json.value(u"color").toString())); // Not valid when it has none
        if(auto* stargate = qobject_cast<StargateItem*>(item))
        {
            stargates.append(stargate);
        }
    }
    // The links of what is bound, once every component is shown, and of what the stargates expose.
    refreshLinks();
    for(StargateItem* stargate : std::as_const(stargates))
    {
        stargate->refreshLinks();
    }
    // Once the components they are joined to are shown: one per file of the project, in its order.
    const QJsonArray settings = project.value(u"settings").toArray();
    for(std::size_t index = 0; index < files.size(); ++index)
    {
        showSettings(*files.at(index), readPosition(settings.at(static_cast<qsizetype>(index)).toObject()));
    }

    const QJsonArray zones = project.value(u"zones").toArray();
    for(const QJsonValue& value : zones)
    {
        const QJsonObject json = value.toObject();
        addZone(readPosition(json), QSizeF(json.value(u"width").toDouble(), json.value(u"height").toDouble()),
                json.value(u"title").toString(), QColor::fromString(json.value(u"color").toString()));
    }
}

void EditionScene::reset()
{
    removeItems();
    m_application.clear();
}

void EditionScene::remove(const QList<Nebula::Component*>& components)
{
    // Found before any goes: its interfaces and their links, which are items too, go with it.
    QList<ComponentItem*> removed;
    const QList<QGraphicsItem*> shown = items();
    for(QGraphicsItem* item : shown)
    {
        auto* componentItem = qobject_cast<ComponentItem*>(item->toGraphicsObject());
        if(componentItem != nullptr && components.contains(&componentItem->component()))
        {
            removed.append(componentItem);
        }
    }
    for(ComponentItem* item : std::as_const(removed))
    {
        remove(*item);
    }
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
    const QMimeData* data = event->mimeData();
    Nebula::Component* component = nullptr;
    if(data->hasFormat(GalaxyModel::MimeType))
    {
        const QString filePath = QString::fromUtf8(data->data(GalaxyModel::MimeType));
        Nebula::Galaxy* galaxy = m_application.instantiateGalaxy(filePath, {}, m_galaxy);
        if(galaxy == nullptr)
        {
            emit refused(
                tr("%1 cannot be placed: %2").arg(QFileInfo(filePath).completeBaseName(), m_application.errorString()));
        }
        else
        {
            emit galaxyPlaced(*galaxy);
            if(!galaxy->problems().isEmpty())
            {
                emit told(tr("%1 is placed without what its file misses: %2")
                              .arg(galaxy->objectName(), galaxy->problems().join(u' ')));
            }
        }
        component = galaxy;
    }
    else
    {
        component = m_application.instantiate(QString::fromUtf8(data->data(ComponentModel::MimeType)), {}, m_galaxy);
    }
    if(component == nullptr)
    {
        event->ignore();
        return;
    }
    ComponentItem* item = showComponent(*component, event->scenePos());
    clearSelection();
    item->setSelected(true);
    if(event->widget() != nullptr)
    {
        event->widget()->setFocus(); // The view: the keys are for what was just dropped, as Delete is
    }
    event->setDropAction(Qt::CopyAction);
    event->accept();
}

void EditionScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    InterfaceItem* source =
        isEditable() && event->button() == Qt::LeftButton ? interfaceAt(event->scenePos()) : nullptr;
    if(source == nullptr)
    {
        QGraphicsScene::mousePressEvent(event);
        return;
    }
    // Not passed to the items: its component would move.
    m_linkSource = source;
    m_pendingLink = addPath({}, QPen(palette().color(QPalette::ButtonText), 1.5, Qt::DashLine));
    m_pendingLink->setZValue(1); // Over the components it is drawn across
    dimLinkTargets(source);
    event->accept();
}

void EditionScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if(m_linkSource == nullptr)
    {
        QGraphicsScene::mouseMoveEvent(event);
        return;
    }
    // What the dot under the cursor is: no tooltip shows by itself while a button is pressed, and its type is what
    // tells whether the link can end there.
    const InterfaceItem* hovered = interfaceAt(event->scenePos());
    QToolTip::showText(event->screenPos(),
                       hovered != nullptr && hovered != m_linkSource ? hovered->toolTip() : QString(), event->widget());
    // To the dot it would be bound to rather than to the cursor, which tells that releasing there binds them.
    const InterfaceItem* target = hovered != nullptr && m_linkSource->canLinkTo(*hovered) ? hovered : nullptr;
    const QPointF end = target != nullptr ? target->scenePos() : event->scenePos();
    const QPointF start = m_linkSource->scenePos();
    const bool provided = m_linkSource->kind() == InterfaceItem::Kind::Provided;
    m_pendingLink->setPath(provided ? LinkItem::curve(start, end) : LinkItem::curve(end, start));
    event->accept();
}

void EditionScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if(m_linkSource == nullptr || event->button() != Qt::LeftButton)
    {
        QGraphicsScene::mouseReleaseEvent(event);
        return;
    }
    InterfaceItem* source = m_linkSource;
    InterfaceItem* target = linkTargetAt(event->scenePos());
    m_linkSource = nullptr;
    delete m_pendingLink;
    m_pendingLink = nullptr;
    dimLinkTargets(nullptr);
    QToolTip::hideText();
    if(target != nullptr && isEditable()) // Not when it was started while the link was drawn
    {
        link(*source, *target);
    }
    event->accept();
}

void EditionScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    SettingsItem* settings =
        isEditable() && event->button() == Qt::LeftButton ? settingsAt(event->scenePos()) : nullptr;
    if(settings == nullptr)
    {
        QGraphicsScene::mouseDoubleClickEvent(event);
        return;
    }
    editSettings(*settings, event->widget());
    event->accept();
}

void EditionScene::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    // Not while a link is drawn or an item is dragged: what is dragged could be removed from under the cursor.
    if(m_linkSource != nullptr || mouseGrabberItem() != nullptr)
    {
        return;
    }
    if(!isEditable())
    {
        refuseEdition(); // Everything it does edits the application
        event->accept();
        return;
    }
    ComponentItem* component = componentAt(event->scenePos());
    SettingsItem* settings = component == nullptr ? settingsAt(event->scenePos()) : nullptr;
    LinkItem* link = component == nullptr && settings == nullptr ? linkAt(event->scenePos()) : nullptr;
    ZoneItem* zone = zoneAt(event->scenePos());
    QMenu menu(event->widget());
    const QAction* renameComponent = nullptr;
    const QAction* recolorComponent = nullptr;
    QAction* uncolorComponent = nullptr;
    const QAction* removeComponent = nullptr;
    const QAction* editSettingsFile = nullptr;
    QAction* writeDefaultValues = nullptr;
    const QAction* removeSettingsFile = nullptr;
    const QAction* removeLink = nullptr;
    const QAction* add = nullptr;
    const QAction* addSettingsFile = nullptr;
    const QAction* addStargate = nullptr;
    const QAction* openGalaxy = nullptr;
    const QAction* editStargateAction = nullptr;
    QAction* exposeInterface = nullptr;
    auto* galaxy = component != nullptr ? qobject_cast<Nebula::Galaxy*>(&component->component()) : nullptr;
    auto* stargate = qobject_cast<StargateItem*>(component);
    // The interface whose dot is under the cursor, of a component that is not a stargate: it exposes its own.
    const InterfaceItem* exposed =
        component != nullptr && stargate == nullptr ? interfaceAt(event->scenePos()) : nullptr;
    if(exposed != nullptr)
    {
        exposeInterface = menu.addAction(icon(GlossaryTerm::Stargate), tr("Expose..."));
        exposeInterface->setToolTip(
            tr("Adds a stargate that exposes %1, for the galaxy of the project to have it").arg(exposed->name()));
        menu.addSeparator();
    }
    if(galaxy != nullptr)
    {
        openGalaxy = menu.addAction(icon(GlossaryTerm::Galaxy), tr("Open galaxy"));
        menu.addSeparator();
    }
    if(stargate != nullptr)
    {
        editStargateAction = menu.addAction(icon(GlossaryTerm::Stargate), tr("Edit stargate..."));
        menu.addSeparator();
    }
    if(component != nullptr)
    {
        renameComponent = menu.addAction(tr("Rename component"));
        recolorComponent = menu.addAction(tr("Change component color..."));
        uncolorComponent = menu.addAction(tr("Reset component color"));
        uncolorComponent->setEnabled(component->color().isValid());
        removeComponent = menu.addAction(tr("Remove component"));
    }
    else if(settings != nullptr)
    {
        editSettingsFile = menu.addAction(icon(GlossaryTerm::Comet), tr("Edit settings file..."));
        writeDefaultValues = menu.addAction(tr("Write default values"));
        writeDefaultValues->setToolTip(
            tr("Writes the values of the project to the file, in place of those of the user"));
        removeSettingsFile = menu.addAction(tr("Remove settings file"));
    }
    else if(link != nullptr)
    {
        removeLink = menu.addAction(tr("Remove link"));
    }
    else
    {
        // Not on a component: right-clicking it is about it, and about the zone it is in.
        add = menu.addAction(icon(GlossaryTerm::Galaxy), tr("Add zone"));
        addSettingsFile = menu.addAction(icon(GlossaryTerm::Comet), tr("Add settings file"));
        addStargate = menu.addAction(icon(GlossaryTerm::Stargate), tr("Add stargate"));
    }
    const QAction* renameZone = nullptr;
    const QAction* recolorZone = nullptr;
    const QAction* removeZone = nullptr;
    if(zone != nullptr)
    {
        menu.addSeparator();
        renameZone = menu.addAction(tr("Rename zone"));
        recolorZone = menu.addAction(tr("Change zone color..."));
        removeZone = menu.addAction(tr("Remove zone"));
    }
    // Done once the menu is closed, and the focus back in the view: editing a name takes it.
    const QAction* chosen = menu.exec(event->screenPos());
    event->accept();
    if(chosen == nullptr)
    {
        return;
    }
    if(chosen == exposeInterface)
    {
        expose(*exposed);
    }
    else if(chosen == openGalaxy)
    {
        emit galaxyOpened(*galaxy);
    }
    else if(chosen == editStargateAction)
    {
        editStargate(*stargate, event->widget());
    }
    else if(chosen == renameComponent)
    {
        component->editName();
    }
    else if(chosen == recolorComponent)
    {
        // From the color it has: the one of the style until it is given one.
        const QColor current = component->color().isValid() ? component->color() : palette().color(QPalette::Button);
        const QColor color = QColorDialog::getColor(current, event->widget(), tr("Component color"),
                                                    QColorDialog::ShowAlphaChannel); // Its opacity too
        if(color.isValid())
        {
            component->setColor(color);
        }
    }
    else if(chosen == uncolorComponent)
    {
        component->setColor({});
    }
    else if(chosen == removeComponent)
    {
        remove(*component);
    }
    else if(chosen == editSettingsFile)
    {
        editSettings(*settings, event->widget());
    }
    else if(chosen == writeDefaultValues)
    {
        writeDefaults(*settings);
    }
    else if(chosen == removeSettingsFile)
    {
        remove(*settings);
    }
    else if(chosen == removeLink)
    {
        unlink({link});
    }
    else if(chosen == add)
    {
        addZone(event->scenePos(), ZoneSize, tr("Zone"))->editTitle(); // Named at once
    }
    else if(chosen == addSettingsFile)
    {
        // What it holds chosen at once.
        Nebula::SettingsFile& file =
            m_galaxy != nullptr ? *m_application.addSettingsFile(*m_galaxy) : m_application.addSettingsFile();
        editSettings(*showSettings(file, event->scenePos()), event->widget());
    }
    else if(chosen == addStargate)
    {
        // What it exposes chosen at once.
        auto* added =
            qobject_cast<StargateItem*>(showComponent(*m_application.addStargate({}, m_galaxy), event->scenePos()));
        clearSelection();
        added->setSelected(true);
        editStargate(*added, event->widget());
    }
    else if(chosen == renameZone)
    {
        zone->editTitle();
    }
    else if(chosen == recolorZone)
    {
        const QColor color = QColorDialog::getColor(zone->color(), event->widget(), tr("Zone color"));
        if(color.isValid())
        {
            zone->setColor(color);
        }
    }
    else if(chosen == removeZone)
    {
        delete zone; // Not what is in it
        update(HintRect);
    }
}

void EditionScene::keyPressEvent(QKeyEvent* event)
{
    // Not while a link is drawn or an item is dragged either: what is dragged would be removed from under the cursor.
    if(event->key() != Qt::Key_Delete || focusItem() != nullptr || m_linkSource != nullptr ||
       mouseGrabberItem() != nullptr)
    {
        QGraphicsScene::keyPressEvent(event);
        return;
    }
    if(!isEditable())
    {
        refuseEdition();
        event->accept();
        return;
    }
    QList<ComponentItem*> removed;
    QList<SettingsItem*> removedSettings;
    QList<LinkItem*> removedLinks;
    const QList<QGraphicsItem*> selected = selectedItems();
    for(QGraphicsItem* item : selected)
    {
        if(auto* component = qobject_cast<ComponentItem*>(item->toGraphicsObject()))
        {
            removed.append(component);
        }
        else if(auto* settings = qobject_cast<SettingsItem*>(item->toGraphicsObject()))
        {
            removedSettings.append(settings);
        }
        else if(auto* link = qgraphicsitem_cast<LinkItem*>(item))
        {
            removedLinks.append(link);
        }
    }
    clearSelection(); // Told once, rather than as each of them goes
    // The links before the components, which they would go with.
    unlink(removedLinks);
    // The files first: they would otherwise join themselves again to what is left as each of their components goes.
    for(SettingsItem* settings : std::as_const(removedSettings))
    {
        remove(*settings);
    }
    for(ComponentItem* component : std::as_const(removed))
    {
        remove(*component);
    }
    event->accept();
}

bool EditionScene::isEditable() const
{
    return !m_application.isRunning();
}

void EditionScene::setEditable(bool editable)
{
    for(QGraphicsItem* item : items())
    {
        if(auto* component = qobject_cast<ComponentItem*>(item->toGraphicsObject()))
        {
            component->setEditable(editable);
        }
        else if(auto* settings = qobject_cast<SettingsItem*>(item->toGraphicsObject()))
        {
            settings->setFlag(QGraphicsItem::ItemIsMovable, editable);
        }
        else if(auto* zone = qgraphicsitem_cast<ZoneItem*>(item))
        {
            zone->setEditable(editable);
        }
    }
}

void EditionScene::refuseEdition()
{
    emit refused(tr("The application is not edited while it runs: stop it first"));
}

InterfaceItem* EditionScene::interfaceAt(const QPointF& position) const
{
    for(QGraphicsItem* item : items(position))
    {
        if(auto* interfaceItem = qgraphicsitem_cast<InterfaceItem*>(item))
        {
            return interfaceItem;
        }
    }
    return nullptr;
}

InterfaceItem* EditionScene::linkTargetAt(const QPointF& position) const
{
    InterfaceItem* target = interfaceAt(position);
    return target != nullptr && m_linkSource != nullptr && m_linkSource->canLinkTo(*target) ? target : nullptr;
}

void EditionScene::dimLinkTargets(const InterfaceItem* source)
{
    for(QGraphicsItem* item : items())
    {
        if(auto* interfaceItem = qgraphicsitem_cast<InterfaceItem*>(item))
        {
            interfaceItem->setDimmed(source != nullptr && interfaceItem != source &&
                                     !source->canLinkTo(*interfaceItem));
        }
    }
}

ComponentItem* EditionScene::showComponent(Nebula::Component& component, const QPointF& position)
{
    ComponentItem* item = nullptr;
    if(auto* stargate = qobject_cast<Nebula::Stargate*>(&component))
    {
        auto* stargateItem = new StargateItem(*stargate);
        // Edited in a dialog of the view it was double-clicked in.
        connect(stargateItem, &ComponentItem::openRequested, this,
                [this, stargateItem] { editStargate(*stargateItem, views().value(0)); });
        connect(stargateItem, &ComponentItem::interfaceNameEdited, this,
                [this, stargate](InterfaceItem::Kind kind, const QString& name, const QString& newName)
                { renameExposed(*stargate, kind, name, newName); });
        item = stargateItem;
    }
    else if(auto* galaxy = qobject_cast<Nebula::Galaxy*>(&component))
    {
        item = new ComponentItem(component, galaxy->galaxyName());
        QString about = QDir::toNativeSeparators(galaxy->filePath());
        if(!galaxy->description().isEmpty())
        {
            about += u'\n' + galaxy->description();
        }
        item->setIcon(icon(GlossaryTerm::Galaxy));
        item->setOpenable(about, tr("Double-click below its name to open it"));
        connect(item, &ComponentItem::openRequested, this, [this, galaxy] { emit galaxyOpened(*galaxy); });
    }
    else
    {
        const std::optional<Nebula::ComponentInfo> info = m_factory.component(component.componentId());
        item = new ComponentItem(component, info ? info->name : QString());
    }
    // Its links again, once its interfaces were shown again.
    connect(item, &ComponentItem::interfacesRebuilt, this, &EditionScene::refreshLinks);
    item->setPos(position);
    addItem(item);
    update(HintRect); // Hidden once anything is in the edition zone
    connect(item, &ComponentItem::nameEdited, this,
            [this, &component](const QString& name) { rename(component, name); });
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
    m_application.remove(component);
    update(HintRect); // Shown again once nothing is left
}

void EditionScene::removeItems()
{
    // Nothing is selected, nor being linked, any longer: who listens lets go of the components before they go.
    clearSelection();
    m_linkSource = nullptr;
    m_pendingLink = nullptr;
    clear();
    m_zoneCount = 0;
    update(HintRect); // Shown again
}

void EditionScene::link(InterfaceItem& source, InterfaceItem& target)
{
    const bool sourceProvided = source.kind() == InterfaceItem::Kind::Provided;
    InterfaceItem& provided = sourceProvided ? source : target;
    InterfaceItem& consumed = sourceProvided ? target : source;
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

void EditionScene::unlink(const QList<LinkItem*>& links)
{
    if(links.isEmpty())
    {
        return;
    }
    // Read before any is unbound: a component shows its interfaces again, without their links, as they change.
    // Each one alone: a list keeps what else it is bound to.
    struct Bound
    {
        Nebula::Component* consumer;
        QString consumed;
        const Nebula::Component* provider;
        QString provided;
    };
    QList<Bound> bound;
    for(const LinkItem* link : links)
    {
        bound.append({.consumer = &link->consumed().component(),
                      .consumed = link->consumed().name(),
                      .provider = &link->provided().component(),
                      .provided = link->provided().name()});
    }
    for(const Bound& link : std::as_const(bound))
    {
        link.consumer->unbindInterface(link.consumed, *link.provider, link.provided);
    }
    refreshLinks(); // Which no longer draws them
}

ZoneItem* EditionScene::zoneAt(const QPointF& position) const
{
    // By their bounds, as their shape is only their title and their edges; the topmost one is the smallest.
    for(QGraphicsItem* item : items(position, Qt::IntersectsItemBoundingRect))
    {
        if(auto* zone = qgraphicsitem_cast<ZoneItem*>(item))
        {
            return zone;
        }
    }
    return nullptr;
}

ZoneItem* EditionScene::addZone(const QPointF& position, const QSizeF& size, const QString& title, const QColor& color)
{
    const QColor next(ZoneColors.at(m_zoneCount++ % ZoneColors.size()));
    auto* zone = new ZoneItem(size, title, color.isValid() ? color : next);
    zone->setPos(position);
    addItem(zone);
    update(HintRect); // Hidden once anything is in the edition zone
    return zone;
}

SettingsItem* EditionScene::showSettings(Nebula::SettingsFile& file, const QPointF& position)
{
    auto* item = new SettingsItem(file);
    item->setPos(position);
    addItem(item);
    update(HintRect); // Hidden once anything is in the edition zone
    return item;
}

SettingsItem* EditionScene::settingsAt(const QPointF& position) const
{
    QGraphicsItem* item = itemAt(position, QTransform()); // The topmost one
    return item != nullptr ? qobject_cast<SettingsItem*>(item->toGraphicsObject()) : nullptr;
}

void EditionScene::editSettings(const SettingsItem& item, QWidget* parent)
{
    // Not run here: the event that opens it ends first, while it blocks the window.
    auto* dialog = new SettingsDialog(item.file(), m_application, m_factory, parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->open();
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
    QHash<const Nebula::Component*, const ComponentItem*> shown;
    for(const QGraphicsItem* item : items())
    {
        if(const auto* componentItem = qobject_cast<const ComponentItem*>(item->toGraphicsObject()))
        {
            shown.insert(&componentItem->component(), componentItem);
        }
    }
    for(const ComponentItem* item : std::as_const(shown))
    {
        for(InterfaceItem* consumed : item->consumedInterfaces())
        {
            // What it is bound to that is shown: one interface, or several for a list.
            QList<InterfaceItem*> provided;
            const QList<Nebula::InterfaceBinding> bindings = item->component().bindings(consumed->name());
            for(const Nebula::InterfaceBinding& bound : bindings)
            {
                const ComponentItem* provider = shown.value(bound.provider);
                if(InterfaceItem* providedItem =
                       provider != nullptr ? interfaceNamed(provider->providedInterfaces(), bound.providedName)
                                           : nullptr)
                {
                    provided.append(providedItem);
                }
            }
            // The one of each binding stays; the others go, and so does the copy of its list.
            const QList<LinkItem*> links = consumed->links();
            for(LinkItem* link : links)
            {
                if(!provided.removeOne(&link->provided()))
                {
                    delete link;
                }
            }
            for(InterfaceItem* providedItem : std::as_const(provided))
            {
                addItem(new LinkItem(*providedItem, *consumed));
            }
        }
    }
}

void EditionScene::editStargate(const StargateItem& item, QWidget* parent)
{
    if(!isEditable())
    {
        refuseEdition();
        return;
    }
    // Not run here, as the dialog of a settings file: the event that opens it ends first.
    auto* dialog = new StargateDialog(item.stargate(), m_application, m_factory, parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &QDialog::accepted, this, &EditionScene::refreshLinks); // What it bound and unbound
    dialog->open();
}

void EditionScene::expose(const InterfaceItem& interfaceItem)
{
    Nebula::Component& component = interfaceItem.component();
    const QString interfaceName = interfaceItem.name();
    const bool provided = interfaceItem.kind() == InterfaceItem::Kind::Provided;
    const Nebula::Exposure exposure = provided ? Nebula::Exposure::Provided : Nebula::Exposure::Consumed;
    const QList<Nebula::InterfaceInfo> interfaces =
        provided ? component.providedInterfaces() : component.consumedInterfaces();
    const auto info = std::ranges::find(interfaces, interfaceName, &Nebula::InterfaceInfo::name);
    // Next to it, on the side of the interface, which the dot of the stargate faces.
    const QPointF position =
        interfaceItem.parentItem()->pos() + QPointF(provided ? ExposedDistance : -ExposedDistance, interfaceItem.y());
    Nebula::Stargate* stargate = m_application.addStargate({}, m_galaxy);
    if(info == interfaces.end() || stargate == nullptr)
    {
        return;
    }
    // Under its name, unless another stargate exposes one so: with the first number free then.
    QString name = interfaceName;
    for(int number = 2; !stargate->exposeInterface(exposure, name, info->type); ++number)
    {
        name = interfaceName + QString::number(number);
    }
    if(provided)
    {
        stargate->bindInterface(name, component, interfaceName);
    }
    else
    {
        component.bindInterface(interfaceName, *stargate, name);
    }
    auto* item = qobject_cast<StargateItem*>(showComponent(*stargate, position));
    refreshLinks();
    clearSelection();
    item->setSelected(true);
    // The name it exposes it as typed at once: the dot of the stargate is on the other edge.
    const QList<InterfaceItem*>& shown = provided ? item->consumedInterfaces() : item->providedInterfaces();
    if(const InterfaceItem* named = interfaceNamed(shown, name))
    {
        item->editInterfaceName(*named);
    }
}

void EditionScene::renameExposed(Nebula::Stargate& stargate, InterfaceItem::Kind kind, const QString& name,
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

    QColor lineColor = palette().color(QPalette::Mid);
    lineColor.setAlphaF(0.25F);
    painter->setPen(QPen(lineColor, 0));
    QList<QLineF> lines;
    for(qreal x = std::floor(rect.left() / GridStep) * GridStep; x <= rect.right(); x += GridStep)
    {
        lines.append(QLineF(x, rect.top(), x, rect.bottom()));
    }
    for(qreal y = std::floor(rect.top() / GridStep) * GridStep; y <= rect.bottom(); y += GridStep)
    {
        lines.append(QLineF(rect.left(), y, rect.right(), y));
    }
    painter->drawLines(lines);

    if(items().isEmpty())
    {
        painter->setPen(palette().color(QPalette::PlaceholderText));
        painter->drawText(
            HintRect, Qt::AlignLeft | Qt::AlignVCenter,
            m_galaxy != nullptr
                ? tr("Drag a component from the list of components and drop it here to add it to the galaxy\n"
                     "Right-click to add a zone, which groups components, a settings file or a stargate")
                : tr("Drag a component from the list of components and drop it here to instantiate it\n"
                     "Right-click to add a zone, which groups components, a settings file or a stargate"));
    }
    painter->restore();
}
