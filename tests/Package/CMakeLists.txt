# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The name of the package and the details of the binaries (#139): checks of the build rather than of a class.
add_test(NAME TestPackageName
    COMMAND ${CMAKE_COMMAND} -D "NEBULA_SOURCE_DIR=${PROJECT_SOURCE_DIR}" -D "PACKAGE_NAME=${NEBULA_PACKAGE_NAME}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/TestPackageName.cmake")

# A version resource is a Windows one.
if(NOT WIN32)
    return()
endif()

# The four numbers of the version of the project, which a binary has in its details: 0 for those it does not give.
set(_number "${PROJECT_VERSION_MAJOR}")
foreach(_part IN ITEMS MINOR PATCH TWEAK)
    if("${PROJECT_VERSION_${_part}}" STREQUAL "")
        string(APPEND _number ".0")
    else()
        string(APPEND _number ".${PROJECT_VERSION_${_part}}")
    endif()
endforeach()

# Adds the test TestVersionDetails<TARGET>, which reads the details of the file of TARGET, a binary of the project
# that leaves its version to CMakeUtils.
function(nebula_add_details_test TARGET)
    set(_compiler "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    add_test(NAME TestVersionDetails${TARGET}
        COMMAND powershell -NoProfile -ExecutionPolicy Bypass -File "${CMAKE_CURRENT_SOURCE_DIR}/TestVersionDetails.ps1"
            -Binary "$<TARGET_FILE:${TARGET}>" -Version "${PROJECT_VERSION}${NEBULA_VERSION_SUFFIX}" -Number "${_number}"
            -Description "${TARGET} (${cmu_platform}, ${_compiler}, Qt ${Qt6_VERSION})")
endfunction()

# An executable, a library and a plugin.
nebula_add_details_test(NebulaStudio)
nebula_add_details_test(Core)
nebula_add_details_test(StandardPlugin)
