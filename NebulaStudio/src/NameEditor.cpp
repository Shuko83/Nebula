// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "NameEditor.h"

#include <QKeyEvent>
#include <QTextCursor>
#include <QTextDocument>
#include <utility>

#include "Theme.h"

NameEditor::NameEditor(QGraphicsItem& parent, std::function<void(const QString&)> named)
    : QGraphicsTextItem(&parent), m_named(std::move(named))
{
    setFont(titleFont(font())); // The one the name is painted in
    setTextInteractionFlags(Qt::TextEditorInteraction);
    hide(); // Until a name is edited
    connect(document(), &QTextDocument::contentsChanged, this, [this] { place(); });
}

void NameEditor::edit(const QString& name, const QColor& color, const QRectF& rect, Qt::Alignment alignment)
{
    m_rect = rect;
    m_alignment = alignment;
    setDefaultTextColor(color);
    setPlainText(name);
    place();
    show();
    setFocus();
    selectAll();            // What is typed replaces it
    parentItem()->update(); // Its name is no longer painted
}

void NameEditor::keyPressEvent(QKeyEvent* event)
{
    const int key = event->key();
    if(key == Qt::Key_Escape || key == Qt::Key_Return || key == Qt::Key_Enter)
    {
        end(key != Qt::Key_Escape);
        return;
    }
    QGraphicsTextItem::keyPressEvent(event);
}

void NameEditor::focusOutEvent(QFocusEvent* event)
{
    QGraphicsTextItem::focusOutEvent(event);
    end(true);
}

void NameEditor::place()
{
    const QSizeF size = document()->size();
    setPos(leftFor(size.width()), m_rect.center().y() - size.height() / 2);
}

qreal NameEditor::leftFor(qreal width) const
{
    const qreal margin = document()->documentMargin();
    if(m_alignment.testFlag(Qt::AlignHCenter))
    {
        return m_rect.center().x() - width / 2;
    }
    if(m_alignment.testFlag(Qt::AlignRight))
    {
        return m_rect.right() + margin - width;
    }
    return m_rect.left() - margin;
}

void NameEditor::selectAll()
{
    QTextCursor cursor = textCursor();
    cursor.select(QTextCursor::Document);
    setTextCursor(cursor);
}

void NameEditor::end(bool named)
{
    if(!isVisible())
    {
        return; // Ended already: hiding it takes its focus, which ends it too
    }
    hide();
    parentItem()->update(); // Its name is painted again
    if(named)
    {
        m_named(toPlainText().simplified()); // On one line, whatever was pasted
    }
}
