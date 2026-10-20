// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Nebula/Component/Component.h"

// A component of SecondPlugin that does nothing: the tests create it from the plugin file, and unload it with it.
class Beta : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Component of SecondPlugin, for the tests")
};
