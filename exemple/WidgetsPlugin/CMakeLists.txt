# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# A main window assembled from components, in <application directory>/plugins with ExamplePlugin. Its class
# (NEBULA_PLUGIN) only registers its components, in src/: each of them creates the objects it is made of.
nebula_add_plugin(
    NAME WidgetsPlugin
    DESCRIPTION "Example plugin: a main window, its top bar and its actions, each a component"
    VERSION 1.0.0
    # Its components are made of widgets.
    LINK_LIBRARIES PRIVATE Qt6::Widgets
)
