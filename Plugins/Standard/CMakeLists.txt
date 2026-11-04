# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The interfaces the standard components provide, headers only, in include/Nebula/Standard/: a plugin links
# Nebula::Standard to consume them, since a plugin, a MODULE, cannot be linked. From the package:
# find_package(Nebula COMPONENTS Core Standard).
cmu_add_interface_library(
    NAME Standard
    NAMESPACE Nebula
    LINK_LIBRARIES INTERFACE Qt6::Core
)
