# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The application that assembles applications from the components of the plugins, live. A NebulaApplication that runs
# the galaxy of NebulaStudio, which StudioPlugin/ assembles from the parts made of the classes of src/.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# The classes of NebulaStudio, in src/, compiled once (#146): the executable, StudioPlugin, whose parts are made of
# them, and the tests link them. A static library, of which each binary keeps what it uses, statics included: the
# executable and the plugin each have their own copy. Position independent, for StudioPlugin, a shared module.
file(GLOB_RECURSE _classes CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
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

# Its icons and the logo of Nebula are SVG files: Qt Svg draws the logo on the splash screen, and its icon engine,
# deployed with it, reads the icons.
find_package(Qt6 REQUIRED COMPONENTS Svg)
# Its own sources, in app/ rather than src/, which cmu_add_executable would compile again: its main, which runs the
# galaxy, and its splash screen, which both tell its version from NebulaStudio_info.h.
set(cmu_sources_dir app)
cmu_add_executable(
    NAME NebulaStudio
    NAMESPACE Nebula
    LINK_LIBRARIES PRIVATE NebulaStudioClasses NebulaStudioResources Nebula::Application Qt6::Svg
)
set_target_properties(NebulaStudio PROPERTIES WIN32_EXECUTABLE ON)
# And the icon of NebulaStudio.exe: nebula.ico, rendered from the logo, through nebulastudio.rc.
if(WIN32)
    target_sources(NebulaStudio PRIVATE resources/nebulastudio.rc)
endif()
