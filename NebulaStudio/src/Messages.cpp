// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Messages.h"

#include <QCoreApplication>
#include <QDir>
#include <QLabel>
#include <QStatusBar>

#include "Nebula/Plugin/PluginManager.h"

void tell(QStatusBar& bar, const QString& message)
{
    bar.showMessage(message, MessageDuration);
}

void warn(QWidget* parent, const QString& title, const QString& text)
{
    QMessageBox::warning(parent, title, text);
}

QMessageBox::StandardButton askToSave(QWidget* parent, const QString& text)
{
    return QMessageBox::warning(parent, QCoreApplication::translate("Messages", "NebulaStudio"), text,
                                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
}

QLabel* createPluginDirectoryLabel()
{
    return new QLabel(QCoreApplication::translate("Messages", "Plugins of %1")
                          .arg(QDir::toNativeSeparators(Nebula::PluginManager::defaultDirectory())));
}
