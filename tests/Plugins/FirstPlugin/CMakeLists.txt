# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# Alpha, which the tests create, place in galaxies and unload.
nebula_add_test_plugin(
    NAME FirstPlugin
    DIRECTORY "${NEBULA_TEST_PLUGINS_DIR}"
    DESCRIPTION "First plugin of the tests"
    VERSION 1.0.0
)
