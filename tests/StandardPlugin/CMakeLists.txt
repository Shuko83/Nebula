# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The components of StandardPlugin, loaded from its file as an application loads them, and consumed from the test
# through the interfaces of Nebula::Standard, as another plugin consumes them. The directory of the plugin, built
# before the test, is in NEBULA_STANDARD_PLUGINS_DIR.
foreach(_test TestLogComponent TestHistoryComponent)
    nebula_add_test(${_test} LINK_LIBRARIES Nebula::Core Nebula::Standard)
    target_compile_definitions(${_test} PRIVATE
        "NEBULA_STANDARD_PLUGINS_DIR=\"$<TARGET_FILE_DIR:StandardPlugin>\"")
    add_dependencies(${_test} StandardPlugin)
endforeach()
