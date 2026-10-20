# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The tests of Nebula: a QTest executable per class tested, each one a test of ctest. See docs/testing.md.
# ENVIRONMENT_MODIFICATION, which tells them where Core and Qt are, needs CMake 3.22.
if(CMAKE_VERSION VERSION_LESS 3.22)
    message(WARNING "The tests of Nebula need CMake 3.22: they are not built")
    return()
endif()

find_package(Qt6 REQUIRED COMPONENTS Test Svg)

# Apart from the applications: their plugins/ would hold nothing the tests need, and what the tests build is not
# shipped. Multi-config generators add a <config> subdirectory: <build>/tests/bin/Debug with Visual Studio.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/bin")

# The components the tests instantiate, registered in ComponentFactory::instance() by the test itself.
add_subdirectory(Components)

# Where the plugins of the tests are written: next to the tests, but not in their plugins/, which every Universe scans
# as it is built, so that a test scans them itself. Two directories, as an application finds plugins in its plugins/
# and in another one. Multi-config generators add a <config> subdirectory before them, as for the tests:
# <build>/tests/bin/Debug/test-plugins with Visual Studio.
set(_output "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
get_property(_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(_multi_config)
    string(APPEND _output "/$<CONFIG>")
endif()
set(NEBULA_TEST_PLUGINS_DIR "${_output}/test-plugins")
set(NEBULA_OTHER_TEST_PLUGINS_DIR "${_output}/other-test-plugins")

# Adds the plugin <NAME> of the tests, built from the src/ of the current directory by nebula_add_plugin(), to which it
# forwards the other arguments, in DIRECTORY. Unlike the plugins of the applications, it is not in the package.
#   nebula_add_test_plugin(NAME <name> DIRECTORY <directory> [DESCRIPTION <text>] [VERSION <version>])
function(nebula_add_test_plugin)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "NAME;DIRECTORY" "")
    nebula_add_plugin(NAME ${ARG_NAME} ${ARG_UNPARSED_ARGUMENTS})
    # cmu_add_target puts every target it creates in the package, which installs them once the configuration is done.
    get_property(_packaged GLOBAL PROPERTY CMU_PACKAGE_TARGETS)
    list(REMOVE_ITEM _packaged ${ARG_NAME})
    set_property(GLOBAL PROPERTY CMU_PACKAGE_TARGETS ${_packaged})
    # With $<CONFIG> in it, multi-config generators add no other subdirectory.
    set_target_properties(${ARG_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${ARG_DIRECTORY}" FOLDER tests/Plugins)
    set_property(GLOBAL APPEND PROPERTY NEBULA_TEST_PLUGINS ${ARG_NAME})
endfunction()

# The plugins the tests load from their file, in place of the example plugins.
add_subdirectory(Plugins)

# Adds the test <NAME>, a QTest executable built from <NAME>.cpp, in the current directory, and SOURCES.
#   nebula_add_test(<NAME> [SOURCES <file>...] [LINK_LIBRARIES <library>...] [INCLUDE_DIRECTORIES <directory>...])
# It runs on the Core and the Qt of the build, with no window shown (QT_QPA_PLATFORM=offscreen): ctest, and the
# debugger of Visual Studio, find them through PATH. It has the directories of the plugins of the tests, built before
# it, in NEBULA_TEST_PLUGINS_DIR (FirstPlugin and ThirdPlugin) and NEBULA_OTHER_TEST_PLUGINS_DIR (SecondPlugin), and
# the version of the project in NEBULA_VERSION.
function(nebula_add_test NAME)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "SOURCES;LINK_LIBRARIES;INCLUDE_DIRECTORIES")
    add_executable(${NAME} ${NAME}.cpp ${ARG_SOURCES})
    target_link_libraries(${NAME} PRIVATE Qt6::Test ${ARG_LINK_LIBRARIES})
    target_include_directories(${NAME} PRIVATE ${ARG_INCLUDE_DIRECTORIES})
    target_compile_definitions(${NAME} PRIVATE
        "NEBULA_TEST_PLUGINS_DIR=\"${NEBULA_TEST_PLUGINS_DIR}\""
        "NEBULA_OTHER_TEST_PLUGINS_DIR=\"${NEBULA_OTHER_TEST_PLUGINS_DIR}\""
        "NEBULA_VERSION=\"${PROJECT_VERSION}\"")
    set_target_properties(${NAME} PROPERTIES AUTOMOC ON AUTORCC ON FOLDER tests)
    get_property(_plugins GLOBAL PROPERTY NEBULA_TEST_PLUGINS)
    add_dependencies(${NAME} ${_plugins})

    add_test(NAME ${NAME} COMMAND ${NAME})
    # Qt first: Core.dll is next to the Qt NebulaStudio was deployed with, which lacks the offscreen platform.
    # QT_FORCE_STDERR_LOGGING: on Windows, without a console, Qt writes its messages, the results of QTest included,
    # to the debugger only.
    set(_environment
        "PATH=path_list_prepend:$<TARGET_FILE_DIR:Nebula::Core>"
        "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        "QT_QPA_PLATFORM=set:offscreen"
        "QT_FORCE_STDERR_LOGGING=set:1")
    set_tests_properties(${NAME} PROPERTIES ENVIRONMENT_MODIFICATION "${_environment}")
    set_target_properties(${NAME} PROPERTIES VS_DEBUGGER_ENVIRONMENT
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$<TARGET_FILE_DIR:Nebula::Core>;%PATH%\nQT_QPA_PLATFORM=offscreen")
endfunction()

add_subdirectory(Core)
add_subdirectory(NebulaStudio)
add_subdirectory(exemple)
