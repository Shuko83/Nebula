# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The components of an example plugin, loaded from its file as an application loads them: the only tests that depend on
# an example, the one they test. The directory of the example plugins, built before the test, is in
# NEBULA_EXAMPLE_PLUGINS_DIR.
nebula_add_test(TestWidgetsPlugin LINK_LIBRARIES Nebula::Core Qt6::Widgets)
target_compile_definitions(TestWidgetsPlugin PRIVATE
    "NEBULA_EXAMPLE_PLUGINS_DIR=\"$<TARGET_FILE_DIR:WidgetsPlugin>\"")
add_dependencies(TestWidgetsPlugin WidgetsPlugin)
