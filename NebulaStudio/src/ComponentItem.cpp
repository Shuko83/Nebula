// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ComponentItem.h"

#include <QApplication>
#include <QFontMetricsF>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QStyleOptionGraphicsItem>
#include <algorithm>
#include <utility>

#include "Colors.h"
#include "InterfaceItem.h"
#include "NameEditor.h"
#include "Nebula/Component/Component.h"
#include "Theme.h"

namespace
{

// The names, centered on the position of the item, where the component was dropped; its interfaces are below.
constexpr QRectF Header(-80, -28, 160, 56);
constexpr qreal Padding = 8;
constexpr qreal CornerRadius = 6;
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
    if(m_interfaceEditor == nullptr)
    {
        // Its child rather than the one of the interface: renaming it shows the interfaces again, in new items.
        m_interfaceEditor = new NameEditor(*this, [this](const QString& name)
                                           { emit interfaceNameEdited(m_editedKind, m_editedName, name); });
        m_interfaceEditor->setFont(QApplication::font()); // The one the interfaces are painted in
    }
    m_editedKind = interfaceItem.kind();
    m_editedName = interfaceItem.name();
    const bool provided = interfaceItem.kind() == InterfaceItem::Kind::Provided;
    m_interfaceEditor->edit(interfaceItem.name(), QApplication::palette().color(QPalette::ButtonText),
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
    const bool selected = option->state.testFlag(QStyle::State_Selected);
    // One pixel inside its bounds.
    QPainterPath outline;
    outline.addRoundedRect(boundingRect().adjusted(1, 1, -1, -1), CornerRadius, CornerRadius);

    painter->setRenderHint(QPainter::Antialiasing);
    painter->fillPath(outline, palette.color(QPalette::Button));
    if(m_color.isValid())
    {
        // Behind its names only: its interfaces keep the colors of the style, which their dots are drawn with.
        QPainterPath header;
        header.addRect(Header);
        painter->fillPath(outline.intersected(header), m_color);
    }
    // Once it is filled, for its color not to cover the half of its outline that is inside.
    painter->setPen(selected ? QPen(palette.color(QPalette::Highlight), 2) : QPen(palette.color(QPalette::Mid), 1));
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(outline);

    const QRectF componentRect(Names.left(), NameRect.bottom(), Names.width(), Names.height() / 2);
    const QColor nameInk = nameColor(m_color, palette);
    QColor componentInk = palette.color(QPalette::PlaceholderText);
    if(m_color.isValid())
    {
        componentInk = nameInk;
        componentInk.setAlphaF(ComponentNameOpacity);
    }

    // A shape, as the dots of its interfaces: a triangle while it runs, nothing while it is edited.
    const QRectF mark(Names.right() - StateMark, Names.top() + (NameRect.height() - StateMark) / 2, StateMark,
                      StateMark);
    painter->setPen(Qt::NoPen);
    painter->setBrush(nameInk);
    if(m_component.state() == Nebula::ComponentState::Running)
    {
        painter->drawPolygon(QPolygonF({mark.topLeft(), mark.bottomLeft(), QPointF(mark.right(), mark.center().y())}));
    }

    const QFont font = painter->font();
    // In the corner across, while it does not work: an exclamation mark, which tells it without color, in a disc of the
    // warning color, outlined in the one of the item to stand apart from the color it was given.
    if(!m_unbound.isEmpty())
    {
        const ThemeColors& colors = currentThemeColors();
        const QRectF warning(Names.left(), Names.top() + (NameRect.height() - WarningMark) / 2, WarningMark,
                             WarningMark);
        painter->setPen(QPen(palette.color(QPalette::Button), 1));
        painter->setBrush(colors.warning);
        painter->drawEllipse(warning);
        QFont markFont = font;
        markFont.setBold(true);
        markFont.setPixelSize(WarningMarkFontSize);
        painter->setFont(markFont);
        painter->setPen(colors.onWarning);
        painter->drawText(warning, Qt::AlignCenter, QStringLiteral("!"));
    }

    if(m_nameEditor == nullptr || !m_nameEditor->isVisible()) // Its name is otherwise the text being edited
    {
        const QFont title = titleFont(font);
        painter->setFont(title);
        painter->setPen(nameInk);
        painter->drawText(NameRect, Qt::AlignCenter,
                          QFontMetricsF(title).elidedText(m_component.objectName(), Qt::ElideRight, NameRect.width()));
    }

    // Its rows, under its interfaces: the name on the left, what it is on the right, both elided to share the width.
    painter->setFont(font);
    const QFontMetricsF metrics(font);
    const qreal rowWidth = Header.width() - 2 * Padding;
    qreal rowTop = rowsTop();
    for(const auto& [name, what] : std::as_const(m_rows))
    {
        const QRectF rowRect(Header.left() + Padding, rowTop, rowWidth, InterfaceItem::RowHeight);
        const QString shownName = metrics.elidedText(name, Qt::ElideRight, (rowWidth - RowSpacing) / 2);
        const qreal nameWidth = metrics.horizontalAdvance(shownName);
        painter->setPen(palette.color(QPalette::ButtonText));
        painter->drawText(rowRect, Qt::AlignLeft | Qt::AlignVCenter, shownName);
        painter->setPen(palette.color(QPalette::PlaceholderText));
        painter->drawText(rowRect, Qt::AlignRight | Qt::AlignVCenter,
                          metrics.elidedText(what, Qt::ElideLeft, rowWidth - nameWidth - RowSpacing));
        rowTop += InterfaceItem::RowHeight;
    }

    painter->setPen(componentInk);
    if(m_icon.isNull())
    {
        painter->drawText(componentRect, Qt::AlignCenter,
                          metrics.elidedText(m_componentName, Qt::ElideRight, componentRect.width()));
        return;
    }
    // The icon before the name, both in the middle.
    const qreal iconSize = componentRect.height();
    const QString componentName =
        metrics.elidedText(m_componentName, Qt::ElideRight, componentRect.width() - iconSize - IconSpacing);
    const qreal width = iconSize + IconSpacing + metrics.horizontalAdvance(componentName);
    const QRectF iconRect(componentRect.center().x() - width / 2, componentRect.top(), iconSize, iconSize);
    m_icon.paint(painter, iconRect.toAlignedRect());
    painter->drawText(QRectF(iconRect.right() + IconSpacing, componentRect.top(),
                             componentRect.right() - iconRect.right() - IconSpacing, componentRect.height()),
                      Qt::AlignLeft | Qt::AlignVCenter, componentName);
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
    if(event->button() != Qt::LeftButton)
    {
        QGraphicsObject::mouseDoubleClickEvent(event); // A press
        return;
    }
    // One that opens keeps the line of its name to be renamed, the rest of it to be opened.
    const QRectF nameArea = m_openable ? QRectF(Header.topLeft(), QPointF(Header.right(), NameRect.bottom())) : Header;
    const InterfaceItem* named = m_editable && m_interfacesRenamable ? interfaceNameAt(event->pos()) : nullptr;
    if(m_editable && nameArea.contains(event->pos()))
    {
        editName();
    }
    else if(named != nullptr)
    {
        editInterfaceName(*named);
    }
    else if(m_openable)
    {
        emit openRequested();
    }
    else
    {
        QGraphicsObject::mouseDoubleClickEvent(event);
        return;
    }
    event->accept();
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
    // Those it shows, unless one was renamed or removed, as those of a stargate are: shown again from the start then.
    const auto shown = [](const QList<InterfaceItem*>& items, const QList<Nebula::InterfaceInfo>& interfaces)
    { return items.size() <= interfaces.size() && namesOf(items) == namesOf(interfaces, items.size()); };
    const bool rebuilt = !shown(m_consumedInterfaces, consumed) || !shown(m_providedInterfaces, provided);
    if(rebuilt)
    {
        prepareGeometryChange();
        qDeleteAll(std::exchange(m_consumedInterfaces, {})); // With their links
        qDeleteAll(std::exchange(m_providedInterfaces, {}));
    }

    // The rows start in the padding under the names and leave as much under the last one.
    qreal row = Header.bottom() - Padding / 2 + InterfaceItem::RowHeight / 2;
    // items shows interfaces, of kind, down from row, each on the edge at x.
    const auto refresh = [&](QList<InterfaceItem*>& items, const QList<Nebula::InterfaceInfo>& interfaces,
                             InterfaceItem::Kind kind, qreal x)
    {
        for(qsizetype index = 0; index < interfaces.size(); ++index, row += InterfaceItem::RowHeight)
        {
            if(index == items.size())
            {
                prepareGeometryChange(); // A row more
                items.append(new InterfaceItem(kind, interfaces.at(index), Header.width() - 2 * Padding, *this));
            }
            items.at(index)->setAvailable(interfaces.at(index).available);
            items.at(index)->setPos(x, row); // Lower than it was when an interface was declared above it
        }
    };
    // On the edges drawn by paint(), one pixel inside its bounds.
    refresh(m_consumedInterfaces, consumed, InterfaceItem::Kind::Consumed, Header.left() + 1);
    refresh(m_providedInterfaces, provided, InterfaceItem::Kind::Provided, Header.right() - 1);

    // Whether it works: what it misses is drawn in the warning color, and told by its mark and its tooltip.
    const QStringList unbound = m_component.unboundMandatoryInterfaces();
    for(InterfaceItem* consumedItem : std::as_const(m_consumedInterfaces))
    {
        consumedItem->setMissing(unbound.contains(consumedItem->name()));
    }
    QString toolTip = m_about.isEmpty() ? m_component.componentId() : m_about;
    if(m_openable)
    {
        toolTip += u'\n' + m_howToOpen;
    }
    if(!unbound.isEmpty())
    {
        toolTip += u'\n' + tr("Does not work: %1 bound to nothing").arg(unbound.join(QStringLiteral(", "))) + u'\n' +
                   tr("Drag from the dot of each to an interface of its type another component provides");
    }
    setToolTip(toolTip);
    if(unbound != m_unbound)
    {
        m_unbound = unbound;
        update(); // Its mark
    }
    if(rebuilt)
    {
        update(); // Its rows, lower or higher
        emit interfacesRebuilt();
    }
}

qreal ComponentItem::rowsTop() const
{
    const qsizetype interfaces = m_consumedInterfaces.size() + m_providedInterfaces.size();
    return Header.bottom() - Padding / 2 + static_cast<qreal>(interfaces) * InterfaceItem::RowHeight;
}
