# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.19)

project(Nebula VERSION 3.0.0)

# Before CMakeUtils is included, which writes them in the info files of each target. The copyright is the one of the
# license, not the one CMakeUtils would build from the organization.
set(cmu_organization "FabienDG")
set(cmu_copyright "Copyright (C) 2026 Fabien Di Grande")

# What follows the version of a build that is not a release (-rc0): in the details of each binary, the version
# NebulaStudio tells, and the name of the package. The numbers stay the ones of project().
set(NEBULA_VERSION_SUFFIX "" CACHE STRING "Suffix of the version of Nebula: a dash then letters or digits (-rc0)")
include(${CMAKE_CURRENT_LIST_DIR}/cmake/PackageName.cmake)
nebula_is_version_suffix(_valid "${NEBULA_VERSION_SUFFIX}")
if(NOT _valid)
    message(FATAL_ERROR "NEBULA_VERSION_SUFFIX '${NEBULA_VERSION_SUFFIX}' is not a dash then letters or digits (-rc0)")
endif()
set(cmu_version_suffix "${NEBULA_VERSION_SUFFIX}")

include(${CMAKE_CURRENT_LIST_DIR}/CMakeUtils/CMakeUtils.cmake)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# What release.yml names the zip of the package and the folder in it, which tests/Package checks.
nebula_compiler_name(_compiler "${CMAKE_CXX_COMPILER_ID}" "${CMAKE_CXX_COMPILER_VERSION}" "${MSVC_VERSION}")
nebula_package_name(_package "${PROJECT_VERSION}${NEBULA_VERSION_SUFFIX}" "${cmu_platform}" "${_compiler}"
    "${Qt6_VERSION}")
set(NEBULA_PACKAGE_NAME "${_package}" CACHE INTERNAL "Name of the package of Nebula: its zip and the folder in it")
message(STATUS "Nebula: the package is ${NEBULA_PACKAGE_NAME}")
# And the one of the installer of NebulaStudio, which Installer/ gives CPack, and release.yml publishes (#140).
nebula_installer_name(_installer "${NEBULA_PACKAGE_NAME}")
set(NEBULA_INSTALLER_NAME "${_installer}" CACHE INTERNAL "Name of the installer of NebulaStudio, without its extension")

# Executables and DLLs side by side, so that Core.dll is found when an application starts;
# nebula_add_plugin puts the plugins in plugins/ next to them.
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()

# Visual Studio compiles the files of a project one after the other, unless MSVC is given /MP: it then compiles them in
# parallel, in the IDE as with cmake --build (#146). Ninja already runs one compiler per file.
if(CMAKE_GENERATOR MATCHES "^Visual Studio")
    add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/MP>)
endif()

add_subdirectory(Core)
# The QApplication of an executable whose content is a galaxy, which it loads and runs.
add_subdirectory(Application)
# The standard plugins, shipped with Nebula for any application, and the headers of the interfaces of their components.
add_subdirectory(Plugins)
add_subdirectory(NebulaStudio)
# The parts of NebulaStudio, each a component, and the galaxy that assembles them.
add_subdirectory(StudioPlugin)
add_subdirectory(exemple)
# The installer of NebulaStudio, which CPack builds with Inno Setup: cpack -C Release, from the build directory.
add_subdirectory(Installer)

# The QTest executables of tests/, run by ctest from the build directory: see docs/testing.md.
option(NEBULA_BUILD_TESTS "Build the tests of Nebula" ON)
if(NEBULA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

