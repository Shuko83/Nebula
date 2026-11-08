// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "PropertyDelegate.h"

#include <QAbstractItemView>
#include <QColorDialog>
#include <QIcon>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPersistentModelIndex>
#include <QPixmap>
#include <algorithm>

#include "PropertyTable.h"
#include "PropertyValues.h"
#include "ValueEditors.h"

namespace
{

// Whether a value of type is a color, which a dialog edits.
bool isColor(QMetaType type)
{
    return type == QMetaType::fromType<QColor>();
}

// The color index holds; not valid when it holds another value.
QColor colorOf(const QModelIndex& index)
{
    const QVariant value = index.data(Qt::EditRole);
    return isColor(value.metaType()) ? value.value<QColor>() : QColor();
}

// A swatch of color, of size, edged in edge, for a screen of ratio device pixels to a pixel.
QPixmap swatch(const QColor& color, const QColor& edge, QSize size, qreal ratio)
{
    QPixmap pixmap(size * ratio);
    pixmap.setDevicePixelRatio(ratio);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setPen(edge);
    painter.setBrush(color);
    painter.drawRect(QRectF(QPointF(), QSizeF(size)).adjusted(0.5, 0.5, -0.5, -0.5)); // Edged inside
    return pixmap;
}

// pixmap, as it is once its row is selected too: the style would tint it with the color of the selection.
QIcon iconOf(const QPixmap& pixmap)
{
    QIcon icon(pixmap);
    icon.addPixmap(pixmap, QIcon::Selected);
    return icon;
}

// Whether event asks to edit a value: a double-click, or F2 or Space, as they edit the value of a cell.
bool asksToEdit(const QEvent& event)
{
    if(event.type() == QEvent::MouseButtonDblClick)
    {
        return static_cast<const QMouseEvent&>(event).button() == Qt::LeftButton;
    }
    const auto* key = event.type() == QEvent::KeyPress ? static_cast<const QKeyEvent*>(&event) : nullptr;
    return key != nullptr && (key->key() == Qt::Key_F2 || key->key() == Qt::Key_Space);
}

// Whether the value of index, shown with option, is edited: it is editable, and its view has edit triggers.
bool isEdited(const QStyleOptionViewItem& option, const QModelIndex& index)
{
    const auto* view = qobject_cast<const QAbstractItemView*>(option.widget);
    return index.flags().testFlag(Qt::ItemIsEditable) && view != nullptr &&
           view->editTriggers() != QAbstractItemView::NoEditTriggers;
}

// cell, as high as height, moved up as much as it needs to stay in room, as high as it at most.
QRect unrolled(QRect cell, int height, const QRect& room)
{
    cell.setHeight(std::max(cell.height(), std::min(height, room.height())));
    cell.moveBottom(std::min(cell.bottom(), room.bottom()));
    return cell;
}

// A dialog on parent that chooses a color from color, alpha channel included, titled after the property named name. It
// is destroyed once closed.
QColorDialog* newColorDialog(const QColor& color, const QString& name, QWidget* parent)
{
    auto* dialog = new QColorDialog(parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setOption(QColorDialog::ShowAlphaChannel);
    dialog->setCurrentColor(color); // Once it shows the alpha channel: it would lose it otherwise
    dialog->setWindowTitle(PropertyDelegate::tr("Choose %1").arg(name));
    return dialog;
}

// What gives model the color chosen for the value of index, unless its row is gone meanwhile.
auto colorSetter(QAbstractItemModel& model, const QModelIndex& index)
{
    return [&model, edited = QPersistentModelIndex(index)](const QColor& color)
    {
        if(edited.isValid())
        {
            model.setData(edited, color);
        }
    };
}

} // namespace

QString PropertyDelegate::displayText(const QVariant& value, const QLocale& locale) const
{
    if(isColor(value.metaType()))
    {
        return colorName(value.value<QColor>());
    }
    return QStyledItemDelegate::displayText(value, locale);
}

QWidget* PropertyDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                                        const QModelIndex& index) const
{
    const QMetaType type = index.data(Qt::EditRole).metaType();
    if(isColor(type))
    {
        return nullptr; // Edited in a dialog, see editorEvent()
    }
    QWidget* editor = newEditor(type, parent);
    return editor != nullptr ? editor : QStyledItemDelegate::createEditor(parent, option, index);
}

template <typename Editor> Editor* PropertyDelegate::writingAtOnce(Editor* editor) const
{
    connect(editor, &Editor::edited, this, &PropertyDelegate::commit);
    return editor;
}

QWidget* PropertyDelegate::newEditor(QMetaType type, QWidget* parent) const
{
    const QMetaEnum enumeration = metaEnumOf(type);
    if(enumeration.isFlag())
    {
        return writingAtOnce(new FlagsEditor(type, parent));
    }
    if(enumeration.isValid())
    {
        return writingAtOnce(new EnumEditor(type, parent));
    }
    return type == QMetaType::fromType<QUrl>() ? writingAtOnce(new UrlEditor(parent)) : nullptr;
}

void PropertyDelegate::commit()
{
    emit commitData(qobject_cast<QWidget*>(sender()));
}

void PropertyDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
                                            const QModelIndex& index) const
{
    QStyledItemDelegate::updateEditorGeometry(editor, option, index);
    if(qobject_cast<FlagsEditor*>(editor) != nullptr)
    {
        editor->setGeometry(unrolled(editor->geometry(), editor->sizeHint().height(), editor->parentWidget()->rect()));
    }
}

void PropertyDelegate::initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const
{
    QStyledItemDelegate::initStyleOption(option, index);
    const QColor color = colorOf(index);
    if(color.isValid())
    {
        const qreal ratio = option->widget != nullptr ? option->widget->devicePixelRatioF() : 1;
        option->features |= QStyleOptionViewItem::HasDecoration;
        option->icon = iconOf(swatch(color, option->palette.color(QPalette::Mid), option->decorationSize, ratio));
    }
}

bool PropertyDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option,
                                   const QModelIndex& index)
{
    if(isColor(index.data(Qt::EditRole).metaType()) && asksToEdit(*event) && isEdited(option, index))
    {
        chooseColor(*model, index);
        return true;
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

void PropertyDelegate::chooseColor(QAbstractItemModel& model, const QModelIndex& index)
{
    QColorDialog* dialog = newColorDialog(colorOf(index), index.siblingAtColumn(PropertyNameColumn).data().toString(),
                                          qobject_cast<QWidget*>(parent()));
    connect(dialog, &QColorDialog::colorSelected, &model, colorSetter(model, index));
    dialog->open();
}

bool PropertyDelegate::eventFilter(QObject* object, QEvent* event)
{
    const auto* url = qobject_cast<const UrlEditor*>(object);
    if(event->type() == QEvent::FocusOut && url != nullptr && url->isBrowsing())
    {
        return false; // Not closed: its dialog has the focus
    }
    return QStyledItemDelegate::eventFilter(object, event);
}
