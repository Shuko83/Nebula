# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# Beta, in a directory of its own: found when that directory is scanned too.
nebula_add_test_plugin(
    NAME SecondPlugin
    DIRECTORY "${NEBULA_OTHER_TEST_PLUGINS_DIR}"
    DESCRIPTION "Second plugin of the tests, in another directory"
    VERSION 1.1.0
)
