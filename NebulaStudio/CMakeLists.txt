# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The application that assembles applications from the components of the plugins, live. A NebulaApplication that runs
# the galaxy of NebulaStudio, which StudioPlugin/ assembles from the parts made of the classes of src/.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# The classes of NebulaStudio, in src/, compiled once (#146): the executable, StudioPlugin, whose parts are made of
# them, and the tests link them. All but its main, which runs the galaxy, and its splash screen, which both tell its
# version from NebulaStudio_info.h, the executable's own. A static library, of which each binary keeps what it uses,
# statics included: the executable and the plugin each have their own copy. Position independent, for StudioPlugin, a
# shared module.
set(_own "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/SplashScreen.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SplashScreen.h")
file(GLOB_RECURSE _classes CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
list(REMOVE_ITEM _classes ${_own})
add_library(NebulaStudioClasses STATIC ${_classes})
target_include_directories(NebulaStudioClasses PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(NebulaStudioClasses PUBLIC Nebula::Core Nebula::Standard Qt6::Widgets)
set_target_properties(NebulaStudioClasses PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Its icons, the logo of Nebula and the font of its titles, through nebulastudio.qrc, compiled once as well. An object
# library, whose objects each target that links it gets whole: from a static library, the linker would drop the
# registration of the resources, which nothing calls.
add_library(NebulaStudioResources OBJECT resources/nebulastudio.qrc)
target_link_libraries(NebulaStudioResources PUBLIC Qt6::Core)
set_target_properties(NebulaStudioResources PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Its translations (#169), one .ts file per language other than English, the language of its sources, compiled to .qm
# files and embedded with those of Qt, for its standard dialogs: nothing to deploy next to it, and windeployqt stays
# without translations. In their own object library, which StudioPlugin links: it is what applies the language, see
# StudioComponent, and neither the executable nor the tests need their size. The target NebulaStudioUpdateTranslations,
# in StudioPlugin/, brings the .ts files up to date with the sources.
find_package(Qt6 REQUIRED COMPONENTS LinguistTools)
set(NEBULA_STUDIO_TS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/translations/nebulastudio_fr.ts"
    CACHE INTERNAL "The translations of NebulaStudio")
qt_add_lrelease(TS_FILES ${NEBULA_STUDIO_TS_FILES} LRELEASE_TARGET NebulaStudioTranslationFiles NO_GLOBAL_TARGET
    QM_OUTPUT_DIRECTORY translations QM_FILES_OUTPUT_VARIABLE _qm_files)
set(_qt_translations "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_TRANSLATIONS}")
set(_translation_entries "")
foreach(_qm IN LISTS _qm_files)
    get_filename_component(_name "${_qm}" NAME)
    string(REPLACE "nebulastudio_" "qtbase_" _qt_name "${_name}")
    if(NOT EXISTS "${_qt_translations}/${_qt_name}")
        message(FATAL_ERROR "Nebula: ${_qt_name} is not in ${_qt_translations}: install the translations of Qt")
    endif()
    string(APPEND _translation_entries "<file alias=\"${_name}\">${_qm}</file>\n"
        "<file alias=\"${_qt_name}\">${_qt_translations}/${_qt_name}</file>\n")
endforeach()
set(_translations_qrc "${CMAKE_CURRENT_BINARY_DIR}/translations/translations.qrc")
file(CONFIGURE OUTPUT "${_translations_qrc}" @ONLY CONTENT
    "<RCC><qresource prefix=\"/translations\">\n@_translation_entries@</qresource></RCC>\n")
qt_add_resources(_translations_source "${_translations_qrc}")
add_library(NebulaStudioTranslations OBJECT ${_translations_source})
target_link_libraries(NebulaStudioTranslations PUBLIC Qt6::Core)
set_target_properties(NebulaStudioTranslations PROPERTIES POSITION_INDEPENDENT_CODE ON AUTOMOC OFF AUTORCC OFF)
add_dependencies(NebulaStudioTranslations NebulaStudioTranslationFiles)

# Its icons and the logo of Nebula are SVG files: Qt Svg draws the logo on the splash screen, and its icon engine,
# deployed with it, reads the icons.
find_package(Qt6 REQUIRED COMPONENTS Svg)
cmu_add_executable(
    NAME NebulaStudio
    NAMESPACE Nebula
    LINK_LIBRARIES PRIVATE NebulaStudioClasses NebulaStudioResources Nebula::Application Qt6::Svg
)
# cmu_add_executable gives it the whole of src/: it keeps its own sources, and links the classes rather than compile
# them again.
get_target_property(_sources NebulaStudio SOURCES)
list(REMOVE_ITEM _sources ${_classes})
set_target_properties(NebulaStudio PROPERTIES SOURCES "${_sources}" WIN32_EXECUTABLE ON)
# And the icon of NebulaStudio.exe: nebula.ico, rendered from the logo, through nebulastudio.rc.
if(WIN32)
    target_sources(NebulaStudio PRIVATE resources/nebulastudio.rc)
endif()
