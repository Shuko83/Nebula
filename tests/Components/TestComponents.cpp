// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "TestComponents.h"

#include "Nebula/Component/ComponentFactory.h"
#include "Outlets.h"
#include "Panel.h"
#include "Probe.h"
#include "Sink.h"
#include "Source.h"

// Here rather than next to each class: this file is linked as soon as a test calls registerTestComponents(), the
// others of the library only when the test uses them.
NEBULA_REGISTER_COMPONENT(TestComponents, Probe);
NEBULA_REGISTER_COMPONENT(TestComponents, Source);
NEBULA_REGISTER_COMPONENT(TestComponents, Sink);
NEBULA_REGISTER_COMPONENT(TestComponents, Panel);
NEBULA_REGISTER_COMPONENT(TestComponents, Outlets);

std::string_view TestComponents::buildDate() const
{
    return __DATE__ " " __TIME__;
}

TestComponents& registerTestComponents()
{
    Nebula::ComponentFactory& factory = Nebula::ComponentFactory::instance();
    // After the factory, created above: destroyed before it.
    static TestComponents plugin;
    factory.add(TestComponents::Id, plugin); // Does nothing once its components are registered
    return plugin;
}

QString testComponentId(const QString& name)
{
    return TestComponents::Id + u'.' + name;
}
