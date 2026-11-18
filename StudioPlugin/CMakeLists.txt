# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The parts of NebulaStudio, each a component, which its galaxy, NebulaStudio.nbp, assembles into the window of
# NebulaStudio. They are made of its classes, NebulaStudioClasses, compiled once in NebulaStudio/ (#146), with the
# icons and the font of its theme, NebulaStudioResources, and its translations, NebulaStudioTranslations.
nebula_add_plugin(
    NAME StudioPlugin
    DESCRIPTION "The parts of NebulaStudio, each a component, which its galaxy assembles"
    LINK_LIBRARIES PRIVATE NebulaStudioClasses NebulaStudioResources NebulaStudioTranslations Nebula::Standard
        Qt6::Widgets
)

# cmake --build <build> --target NebulaStudioUpdateTranslations writes in the .ts files what the sources of NebulaStudio
# and of its parts have to translate, once these targets all exist. TestTranslations tells what is left to translate.
qt_add_lupdate(TS_FILES ${NEBULA_STUDIO_TS_FILES} SOURCE_TARGETS NebulaStudio NebulaStudioClasses StudioPlugin
    LUPDATE_TARGET NebulaStudioUpdateTranslations NO_GLOBAL_TARGET OPTIONS -locations none -no-obsolete)

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
