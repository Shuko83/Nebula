# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# Gamma and Delta: a plugin registers each of its components.
nebula_add_test_plugin(
    NAME ThirdPlugin
    DIRECTORY "${NEBULA_TEST_PLUGINS_DIR}"
    DESCRIPTION "Third plugin of the tests, with two components"
    VERSION 2.0.0
)
