# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The parts of NebulaStudio, each a component, which its galaxy, NebulaStudio.nbp, assembles into the window of
# NebulaStudio. They are made of its classes: NebulaStudio is an executable, which nothing links to, so these are
# compiled again here, all but its window, its splash screen and its main, which the galaxy and the application that
# runs it replace. The icons and the font of its theme come with them.
set(_studio "${PROJECT_SOURCE_DIR}/NebulaStudio")
file(GLOB_RECURSE _studio_sources CONFIGURE_DEPENDS "${_studio}/src/*.cpp")
list(REMOVE_ITEM _studio_sources
    "${_studio}/src/main.cpp" "${_studio}/src/MainWindow.cpp" "${_studio}/src/SplashScreen.cpp")
nebula_add_plugin(
    NAME StudioPlugin
    DESCRIPTION "The parts of NebulaStudio, each a component, which its galaxy assembles"
    SOURCES ${_studio_sources} "${_studio}/resources/nebulastudio.qrc"
    INCLUDE_DIRECTORIES PRIVATE "${_studio}/src"
    LINK_LIBRARIES PRIVATE Qt6::Widgets
)
set_target_properties(StudioPlugin PROPERTIES AUTORCC ON)

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
