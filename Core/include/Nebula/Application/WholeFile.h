// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QString>
#include <chrono>

#include "Core_export.h"

class QByteArray;

// Writing a file whole or not at all, as Nebula writes its settings files and NebulaStudio its projects.
namespace Nebula::WholeFile
{

// How many times write() writes a file whose replacement is refused, and how long it waits between two attempts.
inline constexpr int Attempts = 10;
inline constexpr std::chrono::milliseconds Pause{10};

// Writes data to the file filePath, whole or not at all (QSaveFile), creating its directory when needed. On Windows, a
// file is not replaced while another handle is open on it, which merely looking at it opens for a moment: QFileInfo,
// even exists(), the thread of a QFileSystemWatcher on each change of the files it watches, an antivirus. A refused
// replacement is therefore attempted again, Attempts times at most, Pause apart. false, with error set, when the file
// cannot be written.
CORE_EXPORT bool write(const QString& filePath, const QByteArray& data, QString& error);

} // namespace Nebula::WholeFile
