# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The plugins the tests load from their file, built by nebula_add_plugin() as a third party builds its own: the tests
# depend on them rather than on the example plugins, which change with what they show.
# By id: FirstPlugin, SecondPlugin, then ThirdPlugin; the second one alone in another directory.
add_subdirectory(FirstPlugin)
add_subdirectory(SecondPlugin)
add_subdirectory(ThirdPlugin)
