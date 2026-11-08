// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ValueEditors.h"

#include <QAction>
#include <QFileDialog>
#include <QMetaEnum>
#include <QSignalBlocker>
#include <QStyle>
#include <QUrl>

#include "PropertyValues.h"

namespace
{

// The role of the value of a row of a FlagsEditor.
constexpr int FlagRole = Qt::UserRole;

// Calls add with the name and the value of each value of enumeration, in the order it declares them.
template <typename Add> void forEachValue(const QMetaEnum& enumeration, Add add)
{
    for(int index = 0; index < enumeration.keyCount(); ++index)
    {
        add(QString::fromLatin1(enumeration.key(index)), enumeration.value(index));
    }
}

// Adds value, named name, to list, as a row to check, unless it is 0.
void addFlag(QListWidget& list, const QString& name, int value)
{
    if(value == 0)
    {
        return;
    }
    auto* row = new QListWidgetItem(name, &list);
    row->setData(FlagRole, value);
    row->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
    row->setCheckState(Qt::Unchecked);
}

// Has dialog open on url, when it is a local file.
void startIn(QFileDialog& dialog, const QUrl& url)
{
    if(url.isLocalFile())
    {
        dialog.selectUrl(url);
    }
}

} // namespace

EnumEditor::EnumEditor(QMetaType type, QWidget* parent) : QComboBox(parent), m_type(type)
{
    forEachValue(metaEnumOf(type), [this](const QString& name, int value) { addItem(name, value); });
    connect(this, &QComboBox::activated, this, &EnumEditor::edited);
}

QVariant EnumEditor::value() const
{
    return enumValue(m_type, currentData().toInt());
}

void EnumEditor::setValue(const QVariant& value)
{
    setCurrentIndex(findData(value.toInt()));
}

FlagsEditor::FlagsEditor(QMetaType type, QWidget* parent) : QListWidget(parent), m_type(type)
{
    forEachValue(metaEnumOf(type), [this](const QString& name, int value) { addFlag(*this, name, value); });
    connect(this, &QListWidget::itemChanged, this, &FlagsEditor::toggle);
}

QVariant FlagsEditor::value() const
{
    return enumValue(m_type, m_value);
}

void FlagsEditor::setValue(const QVariant& value)
{
    m_value = value.toInt();
    showChecks();
}

QSize FlagsEditor::sizeHint() const
{
    int height = 2 * frameWidth();
    for(int row = 0; row < count(); ++row)
    {
        height += sizeHintForRow(row);
    }
    return {QListWidget::sizeHint().width(), height};
}

void FlagsEditor::toggle(const QListWidgetItem* row)
{
    const int flag = row->data(FlagRole).toInt();
    m_value = row->checkState() == Qt::Checked ? m_value | flag : m_value & ~flag;
    showChecks();
    emit edited();
}

void FlagsEditor::showChecks()
{
    const QSignalBlocker blocker(this); // itemChanged is for the values the user checks
    for(int row = 0; row < count(); ++row)
    {
        const int flag = item(row)->data(FlagRole).toInt();
        item(row)->setCheckState((m_value & flag) == flag ? Qt::Checked : Qt::Unchecked);
    }
}

UrlEditor::UrlEditor(QWidget* parent) : QLineEdit(parent)
{
    QAction* chooser = addAction(style()->standardIcon(QStyle::SP_DirOpenIcon), QLineEdit::TrailingPosition);
    chooser->setToolTip(tr("Choose a local file"));
    connect(chooser, &QAction::triggered, this, &UrlEditor::browse);
}

QVariant UrlEditor::value() const
{
    return urlOf(text());
}

void UrlEditor::setValue(const QVariant& value)
{
    setText(value.toUrl().toString());
}

bool UrlEditor::isBrowsing() const
{
    return m_dialog != nullptr && m_dialog->isVisible();
}

void UrlEditor::browse()
{
    m_dialog = new QFileDialog(this, tr("Choose a file"));
    m_dialog->setAttribute(Qt::WA_DeleteOnClose);
    m_dialog->setFileMode(QFileDialog::ExistingFile);
    startIn(*m_dialog, urlOf(text()));
    connect(m_dialog, &QFileDialog::urlSelected, this, &UrlEditor::choose);
    m_dialog->open();
}

void UrlEditor::choose(const QUrl& url)
{
    setValue(url);
    emit edited();
}
