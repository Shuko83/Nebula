// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QString>
#include <string_view>

#include "Nebula/Plugin/Plugin.h"

// The plugin of the components of the tests, in the process of the test rather than in a plugin file: see
// registerTestComponents().
class TestComponents final : public Nebula::Plugin<TestComponents>
{
  public:
    // The plugin id they are registered under: their ids are Test.<class name>.
    static constexpr QLatin1StringView Id{"Test"};

    [[nodiscard]] std::string_view buildDate() const override;
};

// Registers the components of the tests in ComponentFactory::instance(), under TestComponents::Id, the first time it is
// called: a test calls it before it creates any. Returns their plugin, which lives until the test exits.
TestComponents& registerTestComponents();

// The id of the component of the tests named name, e.g. Test.Probe.
[[nodiscard]] QString testComponentId(const QString& name);
