# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The components of StandardPlugin, loaded from its file as an application loads them, and consumed from the test
# through the interfaces of Nebula::Standard, as another plugin consumes them. The directory of the plugin, built
# before the test, is in NEBULA_STANDARD_PLUGINS_DIR.
nebula_add_test(TestLogComponent LINK_LIBRARIES Nebula::Core Nebula::Standard)
target_compile_definitions(TestLogComponent PRIVATE
    "NEBULA_STANDARD_PLUGINS_DIR=\"$<TARGET_FILE_DIR:StandardPlugin>\"")
add_dependencies(TestLogComponent StandardPlugin)
