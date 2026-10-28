// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ComponentItem.h"

#include <QApplication>
#include <QFontMetricsF>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QStyleOptionGraphicsItem>
#include <algorithm>
#include <utility>

#include "Colors.h"
#include "InterfaceItem.h"
#include "ItemShapes.h"
#include "NameEditor.h"
#include "Nebula/Component/Component.h"
#include "Theme.h"

namespace
{

// The names, centered on the position of the item, where the component was dropped; its interfaces are below.
constexpr QRectF Header(-80, -28, 160, 56);
constexpr qreal Padding = 8;
// The mark of its state, in the top right corner of its names.
constexpr qreal StateMark = 8;
// Where the names are written, one above the other.
constexpr QRectF Names = Header.adjusted(Padding, Padding, -Padding, -Padding);
// Its name leaves the corner to the mark, and as much on its left to stay centered.
constexpr qreal MarkWidth = StateMark + Padding / 2;
constexpr QRectF NameRect(Names.left() + MarkWidth, Names.top(), Names.width() - 2 * MarkWidth, Names.height() / 2);
// The mark of a component that does not work, in the top left corner of its names, as wide as the room left there:
// a disc, larger than the mark of its state for the exclamation mark written in it to be read.
constexpr qreal WarningMark = MarkWidth;
constexpr int WarningMarkFontSize = 10; // In pixels
// How much of the color of its name the name of its component keeps over the color of an item, as it is fainter over
// the ones of the style.
constexpr float ComponentNameOpacity = 0.7F;
// Between the icon of an item and the name of its component.
constexpr qreal IconSpacing = 4;
// Between the name of a row and what it is.
constexpr qreal RowSpacing = 6;
// The width of its rows, and of the names of its interfaces.
constexpr qreal RowWidth = Header.width() - 2 * Padding;
// Where the name of its component is written, under its name.
constexpr QRectF ComponentRect(Names.left(), NameRect.bottom(), Names.width(), Names.height() / 2);
// The marks, on the line of its name: its state on the right, whether it works on the left.
constexpr QRectF StateMarkRect(Names.right() - StateMark, Names.top() + (NameRect.height() - StateMark) / 2, StateMark,
                               StateMark);
constexpr QRectF WarningMarkRect(Names.left(), Names.top() + (NameRect.height() - WarningMark) / 2, WarningMark,
                                 WarningMark);

// What the name of an item of color is written in: of the style when it has none; read over its color otherwise, as
// it shows over the item, which it lets through when it is translucent.
QColor nameColor(const QColor& color, const QPalette& palette)
{
    if(!color.isValid())
    {
        return palette.color(QPalette::ButtonText);
    }
    const QColor background = over(palette.color(QPalette::Button), palette.color(QPalette::Base));
    return textColorOver(over(color, background));
}

// The names of items, in their order.
QStringList namesOf(const QList<InterfaceItem*>& items)
{
    QStringList names;
    for(const InterfaceItem* item : items)
    {
        names.append(item->name());
    }
    return names;
}

// The names of the first count of interfaces.
QStringList namesOf(const QList<Nebula::InterfaceInfo>& interfaces, qsizetype count)
{
    QStringList names;
    for(qsizetype index = 0; index < count && index < interfaces.size(); ++index)
    {
        names.append(interfaces.at(index).name);
    }
    return names;
}

// Whether items show the first of interfaces, in their order.
bool showsFirst(const QList<InterfaceItem*>& items, const QList<Nebula::InterfaceInfo>& interfaces)
{
    return items.size() <= interfaces.size() && namesOf(items) == namesOf(interfaces, items.size());
}

// What the name of its component is written in, under the name of an item of color written in nameInk: fainter than
// its name over its color, in the color of the hints over the one of the style.
QColor componentInk(const QColor& color, const QPalette& palette, QColor nameInk)
{
    if(!color.isValid())
    {
        return palette.color(QPalette::PlaceholderText);
    }
    nameInk.setAlphaF(ComponentNameOpacity);
    return nameInk;
}

// The exclamation mark of an item that does not work, from font, the one of the item.
QFont warningMarkFont(QFont font)
{
    font.setBold(true);
    font.setPixelSize(WarningMarkFontSize);
    return font;
}

// What the tooltip of an item tells of unbound, the interfaces it misses.
QString unboundText(const QStringList& unbound)
{
    return ComponentItem::tr("Does not work: %1 bound to nothing").arg(unbound.join(QStringLiteral(", "))) + u'\n' +
           ComponentItem::tr("Drag from the dot of each to an interface of its type another component provides");
}

} // namespace

ComponentItem::ComponentItem(Nebula::Component& component, QString componentName)
    : m_component(component), m_componentName(std::move(componentName))
{
    setFlags(ItemIsMovable | ItemIsSelectable);

    connect(&m_component, &Nebula::Component::interfacesChanged, this, &ComponentItem::refreshInterfaces);
    connect(&m_component, &Nebula::Component::stateChanged, this, [this] { update(); }); // Its mark
    connect(&m_component, &QObject::objectNameChanged, this, [this] { update(); });      // Its name
    refreshInterfaces();
}

Nebula::Component& ComponentItem::component() const
{
    return m_component;
}

const QList<InterfaceItem*>& ComponentItem::consumedInterfaces() const
{
    return m_consumedInterfaces;
}

const QList<InterfaceItem*>& ComponentItem::providedInterfaces() const
{
    return m_providedInterfaces;
}

const QColor& ComponentItem::color() const
{
    return m_color;
}

void ComponentItem::setColor(const QColor& color)
{
    m_color = color;
    update();
}

void ComponentItem::editName()
{
    if(m_nameEditor == nullptr)
    {
        m_nameEditor = new NameEditor(*this, [this](const QString& name) { emit nameEdited(name); });
    }
    m_nameEditor->edit(m_component.objectName(), nameColor(m_color, QApplication::palette()), NameRect,
                       Qt::AlignHCenter);
}

void ComponentItem::setEditable(bool editable)
{
    m_editable = editable;
    setFlag(ItemIsMovable, editable);
}

void ComponentItem::setIcon(const QIcon& icon)
{
    m_icon = icon;
    update();
}

void ComponentItem::setOpenable(const QString& about, const QString& howToOpen)
{
    m_openable = true;
    m_about = about;
    m_howToOpen = howToOpen;
    refreshInterfaces(); // Its tooltip
}

void ComponentItem::setRows(const QList<Row>& rows)
{
    if(rows.size() != m_rows.size())
    {
        prepareGeometryChange();
    }
    m_rows = rows;
    update();
}

void ComponentItem::setInterfacesRenamable(bool renamable)
{
    m_interfacesRenamable = renamable;
}

void ComponentItem::editInterfaceName(const InterfaceItem& interfaceItem)
{
    m_editedKind = interfaceItem.kind();
    m_editedName = interfaceItem.name();
    const bool provided = interfaceItem.kind() == InterfaceItem::Kind::Provided;
    interfaceEditor().edit(interfaceItem.name(), QApplication::palette().color(QPalette::ButtonText),
                           interfaceItem.mapRectToParent(interfaceItem.nameRect()),
                           provided ? Qt::AlignRight : Qt::AlignLeft);
}

bool ComponentItem::isNameEdited(const InterfaceItem& interfaceItem) const
{
    return m_interfaceEditor != nullptr && m_interfaceEditor->isVisible() && interfaceItem.kind() == m_editedKind &&
           interfaceItem.name() == m_editedName;
}

QRectF ComponentItem::boundingRect() const
{
    const qsizetype rows = m_consumedInterfaces.size() + m_providedInterfaces.size() + m_rows.size();
    return Header.adjusted(0, 0, 0, static_cast<qreal>(rows) * InterfaceItem::RowHeight);
}

void ComponentItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    const QPalette palette = widget != nullptr ? widget->palette() : QApplication::palette();
    const QColor nameInk = nameColor(m_color, palette);
    const QFont font = painter->font();
    painter->setRenderHint(QPainter::Antialiasing);
    paintBody(*painter, palette, option->state.testFlag(QStyle::State_Selected));
    paintStateMark(*painter, nameInk);
    paintWarningMark(*painter, palette, font);
    paintName(*painter, nameInk, font);
    paintRows(*painter, palette, font);
    paintComponentName(*painter, componentInk(m_color, palette, nameInk), font);
}

void ComponentItem::paintBody(QPainter& painter, const QPalette& palette, bool selected) const
{
    const QPainterPath outline = roundedOutline(boundingRect());
    painter.fillPath(outline, palette.color(QPalette::Button));
    paintColor(painter, outline);
    // Once it is filled, for its color not to cover the half of its outline that is inside.
    painter.setPen(outlinePen(palette, selected));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(outline);
}

void ComponentItem::paintColor(QPainter& painter, const QPainterPath& outline) const
{
    if(m_color.isValid())
    {
        fillWithin(painter, outline, Header, m_color);
    }
}

void ComponentItem::paintStateMark(QPainter& painter, const QColor& ink) const
{
    if(m_component.state() != Nebula::ComponentState::Running)
    {
        return;
    }
    const QRectF& mark = StateMarkRect;
    painter.setPen(Qt::NoPen);
    painter.setBrush(ink);
    painter.drawPolygon(QPolygonF({mark.topLeft(), mark.bottomLeft(), QPointF(mark.right(), mark.center().y())}));
}

void ComponentItem::paintWarningMark(QPainter& painter, const QPalette& palette, const QFont& font) const
{
    if(m_unbound.isEmpty())
    {
        return;
    }
    painter.setPen(QPen(palette.color(QPalette::Button), 1));
    painter.setBrush(currentThemeColors().warning);
    painter.drawEllipse(WarningMarkRect);
    painter.setFont(warningMarkFont(font));
    painter.setPen(currentThemeColors().onWarning);
    painter.drawText(WarningMarkRect, Qt::AlignCenter, QStringLiteral("!"));
}

void ComponentItem::paintName(QPainter& painter, const QColor& ink, const QFont& font) const
{
    if(m_nameEditor != nullptr && m_nameEditor->isVisible()) // Its name is the text being edited
    {
        return;
    }
    const QFont title = titleFont(font);
    painter.setFont(title);
    painter.setPen(ink);
    painter.drawText(NameRect, Qt::AlignCenter,
                     QFontMetricsF(title).elidedText(m_component.objectName(), Qt::ElideRight, NameRect.width()));
}

void ComponentItem::paintRows(QPainter& painter, const QPalette& palette, const QFont& font) const
{
    painter.setFont(font);
    qreal top = rowsTop();
    for(const Row& row : m_rows)
    {
        paintRow(painter, palette, QRectF(Header.left() + Padding, top, RowWidth, InterfaceItem::RowHeight), row);
        top += InterfaceItem::RowHeight;
    }
}

void ComponentItem::paintRow(QPainter& painter, const QPalette& palette, const QRectF& rect, const Row& row)
{
    const auto& [name, what] = row;
    const QFontMetricsF metrics(painter.font());
    const QString shownName = metrics.elidedText(name, Qt::ElideRight, (rect.width() - RowSpacing) / 2);
    const qreal whatWidth = rect.width() - metrics.horizontalAdvance(shownName) - RowSpacing;
    painter.setPen(palette.color(QPalette::ButtonText));
    painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, shownName);
    painter.setPen(palette.color(QPalette::PlaceholderText));
    painter.drawText(rect, Qt::AlignRight | Qt::AlignVCenter, metrics.elidedText(what, Qt::ElideLeft, whatWidth));
}

void ComponentItem::paintComponentName(QPainter& painter, const QColor& ink, const QFont& font) const
{
    painter.setFont(font);
    painter.setPen(ink);
    if(!m_icon.isNull())
    {
        paintIconAndComponentName(painter, QFontMetricsF(font));
        return;
    }
    painter.drawText(ComponentRect, Qt::AlignCenter,
                     QFontMetricsF(font).elidedText(m_componentName, Qt::ElideRight, ComponentRect.width()));
}

void ComponentItem::paintIconAndComponentName(QPainter& painter, const QFontMetricsF& metrics) const
{
    const qreal iconSize = ComponentRect.height();
    const QString componentName =
        metrics.elidedText(m_componentName, Qt::ElideRight, ComponentRect.width() - iconSize - IconSpacing);
    const qreal width = iconSize + IconSpacing + metrics.horizontalAdvance(componentName);
    const QRectF iconRect(ComponentRect.center().x() - width / 2, ComponentRect.top(), iconSize, iconSize);
    m_icon.paint(&painter, iconRect.toAlignedRect());
    const QRectF nameRect(QPointF(iconRect.right() + IconSpacing, ComponentRect.top()), ComponentRect.bottomRight());
    painter.drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, componentName);
}

void ComponentItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if(!isNameEdited())
    {
        QGraphicsObject::mouseMoveEvent(event);
    }
}

void ComponentItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    if(event->button() == Qt::LeftButton && openOrEditAt(event->pos()))
    {
        event->accept();
        return;
    }
    QGraphicsObject::mouseDoubleClickEvent(event); // A press, or nothing to open or edit
}

NameEditor& ComponentItem::interfaceEditor()
{
    if(m_interfaceEditor == nullptr)
    {
        // Its child rather than the one of the interface: renaming it shows the interfaces again, in new items.
        m_interfaceEditor = new NameEditor(*this, [this](const QString& name)
                                           { emit interfaceNameEdited(m_editedKind, m_editedName, name); });
        m_interfaceEditor->setFont(QApplication::font()); // The one the interfaces are painted in
    }
    return *m_interfaceEditor;
}

QRectF ComponentItem::nameArea() const
{
    return m_openable ? QRectF(Header.topLeft(), QPointF(Header.right(), NameRect.bottom())) : Header;
}

bool ComponentItem::openOrEditAt(const QPointF& position)
{
    if(m_editable && nameArea().contains(position))
    {
        editName();
        return true;
    }
    return editInterfaceNameAt(position) || open();
}

bool ComponentItem::editInterfaceNameAt(const QPointF& position)
{
    const InterfaceItem* named = m_editable && m_interfacesRenamable ? interfaceNameAt(position) : nullptr;
    if(named != nullptr)
    {
        editInterfaceName(*named);
    }
    return named != nullptr;
}

bool ComponentItem::open()
{
    if(!m_openable)
    {
        return false;
    }
    // No longer held by the double-click: what opens, a dialog or another scene, takes its release away, and the scene
    // would keep it held, with neither context menu nor Delete.
    ungrabMouse();
    emit openRequested();
    return true;
}

bool ComponentItem::isNameEdited() const
{
    return (m_nameEditor != nullptr && m_nameEditor->isVisible()) ||
           (m_interfaceEditor != nullptr && m_interfaceEditor->isVisible());
}

const InterfaceItem* ComponentItem::interfaceNameAt(const QPointF& position) const
{
    for(const QList<InterfaceItem*>* items : {&m_consumedInterfaces, &m_providedInterfaces})
    {
        const auto found = std::ranges::find_if(*items, [&position](const InterfaceItem* item)
                                                { return item->mapRectToParent(item->nameRect()).contains(position); });
        if(found != items->end())
        {
            return *found;
        }
    }
    return nullptr;
}

void ComponentItem::refreshInterfaces()
{
    const QList<Nebula::InterfaceInfo> consumed = m_component.consumedInterfaces();
    const QList<Nebula::InterfaceInfo> provided = m_component.providedInterfaces();
    const bool rebuilt = clearIfChanged(consumed, provided); // Shown again from the start then
    showInterfaces(consumed, provided);
    refreshUnbound();
    if(rebuilt)
    {
        update(); // Its rows, lower or higher
        emit interfacesRebuilt();
    }
}

bool ComponentItem::clearIfChanged(const QList<Nebula::InterfaceInfo>& consumed,
                                   const QList<Nebula::InterfaceInfo>& provided)
{
    if(showsFirst(m_consumedInterfaces, consumed) && showsFirst(m_providedInterfaces, provided))
    {
        return false;
    }
    prepareGeometryChange();
    qDeleteAll(std::exchange(m_consumedInterfaces, {})); // With their links
    qDeleteAll(std::exchange(m_providedInterfaces, {}));
    return true;
}

void ComponentItem::showInterfaces(const QList<Nebula::InterfaceInfo>& consumed,
                                   const QList<Nebula::InterfaceInfo>& provided)
{
    // The rows start in the padding under the names and leave as much under the last one; the dots are on the edges
    // drawn by paint(), one pixel inside its bounds.
    const qreal top = Header.bottom() - Padding / 2 + InterfaceItem::RowHeight / 2;
    const qreal providedTop = top + static_cast<qreal>(consumed.size()) * InterfaceItem::RowHeight;
    showInterfaces(m_consumedInterfaces, consumed, InterfaceItem::Kind::Consumed, QPointF(Header.left() + 1, top));
    showInterfaces(m_providedInterfaces, provided, InterfaceItem::Kind::Provided,
                   QPointF(Header.right() - 1, providedTop));
}

void ComponentItem::showInterfaces(QList<InterfaceItem*>& items, const QList<Nebula::InterfaceInfo>& interfaces,
                                   InterfaceItem::Kind kind, QPointF at)
{
    for(qsizetype index = 0; index < interfaces.size(); ++index, at.ry() += InterfaceItem::RowHeight)
    {
        if(index == items.size())
        {
            prepareGeometryChange(); // A row more
            items.append(new InterfaceItem(kind, interfaces.at(index), RowWidth, *this));
        }
        items.at(index)->setAvailable(interfaces.at(index).available);
        items.at(index)->setPos(at); // Lower than it was when an interface was declared above it
    }
}

void ComponentItem::refreshUnbound()
{
    const QStringList unbound = m_component.unboundMandatoryInterfaces();
    for(InterfaceItem* consumed : std::as_const(m_consumedInterfaces))
    {
        consumed->setMissing(unbound.contains(consumed->name()));
    }
    setToolTip(toolTipText(unbound));
    if(std::exchange(m_unbound, unbound) != unbound)
    {
        update(); // Its mark
    }
}

QString ComponentItem::toolTipText(const QStringList& unbound) const
{
    QString toolTip = m_about.isEmpty() ? m_component.componentId() : m_about;
    if(m_openable)
    {
        toolTip += u'\n' + m_howToOpen;
    }
    if(!unbound.isEmpty())
    {
        toolTip += u'\n' + unboundText(unbound);
    }
    return toolTip;
}

qreal ComponentItem::rowsTop() const
{
    const qsizetype interfaces = m_consumedInterfaces.size() + m_providedInterfaces.size();
    return Header.bottom() - Padding / 2 + static_cast<qreal>(interfaces) * InterfaceItem::RowHeight;
}

QList<ComponentItem*> componentItems(const QGraphicsScene& scene)
{
    QList<ComponentItem*> found;
    const QList<QGraphicsItem*> items = scene.items();
    for(QGraphicsItem* item : items)
    {
        if(auto* component = qobject_cast<ComponentItem*>(item->toGraphicsObject()))
        {
            found.append(component);
        }
    }
    return found;
}
