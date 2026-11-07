// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QCoreApplication>
#include <QJsonObject>
#include <QPointer>
#include <QString>

#include "Nebula/Standard/Command.h"

class EditionScene;

// One step of the edition of what a scene shows, see EditionScene::edit(): what the scene showed before it, and what it
// showed after it, two of its snapshots. Undoing it gives the scene what it showed before, executing it what it showed
// after, see EditionScene::restore(), and tells it: what it acts on is found again by the names of the components,
// which a snapshot holds, rather than by pointers. Executing it as it is pushed changes nothing: the scene shows what
// it showed after already. Obsolete once its scene is gone, the one of a galaxy left say.
class SceneCommand : public Nebula::Command
{
    Q_DECLARE_TR_FUNCTIONS(SceneCommand)

  public:
    SceneCommand(EditionScene& scene, const QString& label, QJsonObject before, QJsonObject after);

    void execute() override;
    void undo() override;
    [[nodiscard]] bool isObsolete() const override;

  private:
    QPointer<EditionScene> m_scene;
    QJsonObject m_before;
    QJsonObject m_after;
};
