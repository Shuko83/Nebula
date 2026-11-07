# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The standard components, for any application, with the version of Nebula they are shipped with. Its class,
# StandardPlugin.h, which is its root object, and its metadata are generated; its components, each registered in it
# with NEBULA_REGISTER_COMPONENT, are in src/, and the interfaces they provide in Nebula::Standard.
nebula_add_plugin(
    NAME StandardPlugin
    DESCRIPTION "Standard components of Nebula, for any application: a log, which drives the one of the application, and a history of what can be undone"
    LINK_LIBRARIES PRIVATE Nebula::Standard
)
