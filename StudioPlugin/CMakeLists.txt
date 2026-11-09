# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The parts of NebulaStudio, each a component, which its galaxy, NebulaStudio.nbp, assembles into the window of
# NebulaStudio. They are made of its classes, NebulaStudioClasses, compiled once in NebulaStudio/ (#146), with the
# icons and the font of its theme, NebulaStudioResources.
nebula_add_plugin(
    NAME StudioPlugin
    DESCRIPTION "The parts of NebulaStudio, each a component, which its galaxy assembles"
    LINK_LIBRARIES PRIVATE NebulaStudioClasses NebulaStudioResources Nebula::Standard Qt6::Widgets
)

# The galaxy, in scene/ next to the applications, where NebulaStudio lists the galaxies, and where ExampleUniverseApp
# runs it from: ExampleUniverseApp scene/NebulaStudio.nbp. A copy, which each build gives the content of the one here
# again: this one is the galaxy to edit.
add_custom_target(StudioGalaxy ALL
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:StudioPlugin>/../scene"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/NebulaStudio.nbp"
        "$<TARGET_FILE_DIR:StudioPlugin>/../scene/NebulaStudio.nbp"
    COMMENT "Copying the galaxy of NebulaStudio to scene/"
    SOURCES NebulaStudio.nbp
    VERBATIM)
