// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QMessageBox>
#include <QString>

class QLabel;
class QStatusBar;
class QWidget;

// What NebulaStudio tells its user: in the status bar of its window, for a while, or in a box that waits for them.

// How long the status bar tells a message, in milliseconds.
constexpr int MessageDuration = 8000;

// Tells message in bar, for a while.
void tell(QStatusBar& bar, const QString& message);
// Tells text in a box titled title, over parent, which waits for the user.
void warn(QWidget* parent, const QString& title, const QString& text);
// Asks, with text, over parent, whether to save what changed: Save, Discard, or Cancel to do neither.
[[nodiscard]] QMessageBox::StandardButton askToSave(QWidget* parent, const QString& text);
// A label of the status bar that tells where the plugins are: plugins/ next to the application.
[[nodiscard]] QLabel* createPluginDirectoryLabel();
