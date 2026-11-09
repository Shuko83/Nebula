# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# What the installer of NebulaStudio installs (#140): the component NebulaStudio of the installation, which CPack packs,
# installed in PREFIX, emptied first, then checked. Run by ctest in script mode:
#   cmake -D BUILD_DIR=<build> -D CONFIG=<config> -D PREFIX=<folder> -D MSVC=<MSVC of the build>
#       -P TestInstallerContent.cmake
# A check that fails stops the script with an error, which fails the test.

# Fails the test when <actual> is not <expected>, saying <what>.
function(expect_equal what actual expected)
    if(NOT "${actual}" STREQUAL "${expected}")
        message(FATAL_ERROR "${what}: '${actual}', expected '${expected}'")
    endif()
endfunction()

# Installs the component in PREFIX, as CPack does in the folder it packs; windeployqt then adds Qt.
function(install_component)
    file(REMOVE_RECURSE "${PREFIX}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" --install "${BUILD_DIR}" --config "${CONFIG}" --component NebulaStudio
            --prefix "${PREFIX}"
        RESULT_VARIABLE _result OUTPUT_VARIABLE _output ERROR_VARIABLE _output)
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "The component NebulaStudio is not installed (${_result}):\n${_output}")
    endif()
endfunction()

# Fails the test unless each of <files>, relative to PREFIX, is installed.
function(expect_files)
    foreach(_file IN LISTS ARGN)
        if(NOT EXISTS "${PREFIX}/${_file}")
            message(FATAL_ERROR "${_file} is not installed in ${PREFIX}")
        endif()
    endforeach()
endfunction()

# Fails the test unless the files of PREFIX matching <pattern> are <expected>, sorted, relative to PREFIX.
function(expect_only pattern)
    file(GLOB_RECURSE _found RELATIVE "${PREFIX}" "${PREFIX}/${pattern}")
    list(SORT _found)
    expect_equal("Installed files ${pattern}" "${_found}" "${ARGN}")
endfunction()

install_component()

# Qt's DLLs and plugins, those of a debug build having a d after their name.
set(_d "")
if(CONFIG STREQUAL "Debug")
    set(_d d)
endif()
expect_files(NebulaStudio.exe Core.dll Application.dll LICENSE.txt SpaceGrotesk-OFL.txt
    Qt6Core${_d}.dll Qt6Gui${_d}.dll Qt6Widgets${_d}.dll Qt6Svg${_d}.dll
    platforms/qwindows${_d}.dll iconengines/qsvgicon${_d}.dll)
# The runtime of the compiler, next to it rather than the installer of Microsoft's redistributable.
if(MSVC)
    expect_files(vcruntime140.dll msvcp140.dll)
endif()
expect_only("vc_redist*")

# Its galaxy, and the plugins it is made of, nothing else: neither the example plugins, nor the tests'.
expect_only("plugins/*" plugins/StandardPlugin.nbdl plugins/StudioPlugin.nbdl)
expect_only("scene/*" scene/NebulaStudio.nbp)
# Nothing to build plugins with, which the package has, nor Qt's translations.
expect_only("*.lib")
expect_only("*.h")
expect_only("*.qm")
