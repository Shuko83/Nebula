// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QJsonObject>
#include <QLatin1StringView>
#include <QString>
#include <QStringList>
#include <memory>
#include <vector>

#include "Core_export.h"
#include "Nebula/Component/Component.h"
#include "Nebula/Settings/SettingsFile.h"

namespace Nebula
{

class Universe;

// A galaxy placed in an application, see Universe::instantiateGalaxy(): a component Core builds itself, made of the
// components of a project file, which it owns, bound as the file binds them and with the values it gives them, and of
// the settings files of the file. In the application, or in another galaxy, it is one component among the others, as
// many times as wanted, each instance with its own components; it holds other galaxies the same way. Its content is
// the one of its file: it is not edited.
// Started, it starts its components, in their order, and stops them the other way round; its application starts and
// stops its settings files, with its own.
class CORE_EXPORT Galaxy : public Component
{
    Q_OBJECT

  public:
    // Its componentId(): Core creates the galaxies, they are not registered in ComponentFactory.
    static constexpr QLatin1StringView Id{"Nebula.Galaxy"};

    ~Galaxy() override;

    [[nodiscard]] QString componentId() const override;

    // Its file, absolute and canonical.
    [[nodiscard]] QString filePath() const;
    // The name of its file without its extension, as Universe::readGalaxy() gives it: what it is an instance of.
    [[nodiscard]] QString galaxyName() const;
    // The one of its file; empty when it has none.
    [[nodiscard]] QString description() const;
    // The project its file holds, as it was read when the galaxy was placed: what NebulaStudio adds to it tells where
    // its components are shown.
    [[nodiscard]] const QJsonObject& project() const;
    // What could not be restored of its file, and of the galaxies it holds, a sentence each: none when everything was.
    [[nodiscard]] const QStringList& problems() const;

    // In the order of its file.
    [[nodiscard]] const std::vector<std::unique_ptr<Component>>& components() const;
    // The one named name; nullptr when none is.
    [[nodiscard]] Component* component(const QString& name) const;
    // In the order of its file, named after the galaxy: see Universe::instantiateGalaxy().
    [[nodiscard]] const std::vector<std::unique_ptr<SettingsFile>>& settingsFiles() const;

  protected:
    // Starts its components in their order; when one refuses, stops those it started and refuses.
    bool onStart() override;
    void onStop() override;

  private:
    friend class Universe; // Creates it, and fills it from its file

    Galaxy();
    // Core builds it, not a plugin.
    void builtByPlugin() override;

    QString m_filePath;
    QString m_description;
    QJsonObject m_project;
    QStringList m_problems;
    std::vector<std::unique_ptr<Component>> m_components;
    std::vector<std::unique_ptr<SettingsFile>> m_settingsFiles; // Destroyed first: their entries are of m_components
    QStringList m_settingsFileNames; // As its file names them, without the name of the galaxy: one per settings file
};

} // namespace Nebula
