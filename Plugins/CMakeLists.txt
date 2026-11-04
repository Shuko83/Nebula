# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The standard plugins, shipped with Nebula for any application.
# The headers of the interfaces of their components, which a plugin links to consume them.
add_subdirectory(Standard)
# Its components, each in src/.
add_subdirectory(StandardPlugin)
