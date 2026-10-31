# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The QApplication of an executable whose content is a galaxy: it loads it as it is built, then runs it.
set(CMAKE_AUTOMOC ON)

cmu_add_shared_library(
    NAME Application
    NAMESPACE Nebula
    # Widgets: it is a QApplication, for the components of the galaxy to show some.
    LINK_LIBRARIES PUBLIC Nebula::Core Qt6::Widgets
)
