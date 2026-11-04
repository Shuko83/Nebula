// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Nebula/Standard/ILogControl.h"

class LogComponent;

// What a LogComponent provides as "logControl": drives it through its properties, which tell what changes.
class LogControl : public Nebula::ILogControl
{
  public:
    explicit LogControl(LogComponent& component);

    void startLogging() override;
    void stopLogging() override;
    [[nodiscard]] bool isLogging() const override;
    void setLevel(Nebula::LogLevel level) override;
    [[nodiscard]] Nebula::LogLevel level() const override;

  private:
    LogComponent& m_component;
};
