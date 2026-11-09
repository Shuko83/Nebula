# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The name of the package of Nebula, Nebula_<version>_<platform>_<compiler>_Qt<major>.<minor>, which its zip and the
# folder in it take: what its binaries are built for and with (#139). Its parts are split by '_', which none holds. And
# the name of the installer of NebulaStudio, made from it (#140).
# Only functions of their arguments, so that tests/Package/TestPackageName.cmake runs them in script mode.

include_guard(GLOBAL)

# Sets <result> to TRUE when <suffix> can follow the version: empty, or a dash then letters or digits (-rc0).
function(nebula_is_version_suffix result suffix)
    if("${suffix}" MATCHES "^(-[0-9A-Za-z]+)?$")
        set(${result} TRUE PARENT_SCOPE)
    else()
        set(${result} FALSE PARENT_SCOPE)
    endif()
endfunction()

# Sets <result> to the year of the Visual Studio whose MSVC has the _MSC_VER <msc_version> (1944: 2022), empty when
# it is older than Visual Studio 2017.
function(nebula_visual_studio_year result msc_version)
    set(${result} "" PARENT_SCOPE)
    foreach(_first_and_year IN ITEMS 1950:2026 1930:2022 1920:2019 1910:2017)
        string(REPLACE ":" ";" _first_and_year "${_first_and_year}")
        list(GET _first_and_year 0 _first)
        if(msc_version GREATER_EQUAL _first)
            list(GET _first_and_year 1 _year)
            set(${result} "${_year}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
endfunction()

# Sets <result> to the name of the compiler <id> (CMAKE_CXX_COMPILER_ID) of version <version>, <msc_version> being
# MSVC_VERSION: msvc and the year of its Visual Studio (msvc2022), whose kits Qt is built with, otherwise the id and
# the major of the compiler (gcc14, clang19).
function(nebula_compiler_name result id version msc_version)
    string(TOLOWER "${id}" _name)
    string(REGEX MATCH "^[0-9]+" _number "${version}")
    if(id STREQUAL "MSVC")
        nebula_visual_studio_year(_year "${msc_version}")
        if(NOT _year STREQUAL "")
            set(_number "${_year}")
        endif()
    elseif(id STREQUAL "GNU")
        set(_name "gcc")
    endif()
    set(${result} "${_name}${_number}" PARENT_SCOPE)
endfunction()

# Sets <result> to the name of the package of Nebula <version> built for <platform> with <compiler> and Qt
# <qt_version>, of which it keeps the major and the minor: a patch of Qt changes nothing a plugin is built against.
function(nebula_package_name result version platform compiler qt_version)
    string(REGEX MATCH "^[0-9]+\\.[0-9]+" _qt "${qt_version}")
    set(${result} "Nebula_${version}_${platform}_${compiler}_Qt${_qt}" PARENT_SCOPE)
endfunction()

# Sets <result> to the name of the installer of NebulaStudio built with the package <package>, without its extension
# (#140): NebulaStudio in place of Nebula, then setup, NebulaStudio_2.2.0_win-x64_msvc2022_Qt6.12_setup. What it is
# built for and with is what a plugin opened in it should be built for and with.
function(nebula_installer_name result package)
    string(REGEX REPLACE "^Nebula_(.+)$" "NebulaStudio_\\1_setup" _name "${package}")
    set(${result} "${_name}" PARENT_SCOPE)
endfunction()

# Sets <result> to the four numbers of <version>, the numbers of a version of project() (2.2.0), 0 for those it does not
# give (2.2.0.0): those a Windows binary has in its details.
function(nebula_version_numbers result version)
    string(REPLACE "." ";" _numbers "${version}")
    list(LENGTH _numbers _count)
    while(_count LESS 4)
        list(APPEND _numbers 0)
        math(EXPR _count "${_count} + 1")
    endwhile()
    list(JOIN _numbers "." _joined)
    set(${result} "${_joined}" PARENT_SCOPE)
endfunction()
