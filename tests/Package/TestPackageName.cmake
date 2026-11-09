# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The name of the package (cmake/PackageName.cmake, #139), and of the installer of NebulaStudio (#140), run by ctest in
# script mode:
#   cmake -D NEBULA_SOURCE_DIR=<sources> -D PACKAGE_NAME=<name of the build> -D INSTALLER_NAME=<name of the build>
#       -P TestPackageName.cmake
# A check that fails stops the script with an error, which fails the test.

include("${NEBULA_SOURCE_DIR}/cmake/PackageName.cmake")

# Fails the test when <actual> is not <expected>, saying <what>.
function(expect_equal what actual expected)
    if(NOT "${actual}" STREQUAL "${expected}")
        message(FATAL_ERROR "${what}: '${actual}', expected '${expected}'")
    endif()
endfunction()

function(expect_suffix suffix expected)
    nebula_is_version_suffix(_valid "${suffix}")
    expect_equal("Suffix '${suffix}' accepted" "${_valid}" "${expected}")
endfunction()

function(expect_year msc_version expected)
    nebula_visual_studio_year(_year "${msc_version}")
    expect_equal("Visual Studio of _MSC_VER ${msc_version}" "${_year}" "${expected}")
endfunction()

function(expect_compiler id version msc_version expected)
    nebula_compiler_name(_name "${id}" "${version}" "${msc_version}")
    expect_equal("Name of ${id} ${version}" "${_name}" "${expected}")
endfunction()

function(expect_package version platform compiler qt_version expected)
    nebula_package_name(_name "${version}" "${platform}" "${compiler}" "${qt_version}")
    expect_equal("Package ${version} for ${platform}" "${_name}" "${expected}")
endfunction()

function(expect_installer package expected)
    nebula_installer_name(_name "${package}")
    expect_equal("Installer of ${package}" "${_name}" "${expected}")
endfunction()

function(expect_numbers version expected)
    nebula_version_numbers(_numbers "${version}")
    expect_equal("Numbers of ${version}" "${_numbers}" "${expected}")
endfunction()

expect_suffix("" TRUE)
expect_suffix("-rc0" TRUE)
expect_suffix("-beta2" TRUE)
expect_suffix("rc0" FALSE)
expect_suffix("-" FALSE)
expect_suffix("-rc_0" FALSE)
expect_suffix("-rc.0" FALSE)

expect_year(1950 2026)
expect_year(1944 2022)
expect_year(1930 2022)
expect_year(1929 2019)
expect_year(1916 2017)
expect_year(1900 "")

expect_compiler(MSVC 19.44.35211.0 1944 msvc2022)
expect_compiler(MSVC 19.0.24215.1 1900 msvc19)
expect_compiler(GNU 14.2.0 "" gcc14)
expect_compiler(Clang 19.1.7 "" clang19)

expect_package(2.2.0 win-x64 msvc2022 6.12.0 Nebula_2.2.0_win-x64_msvc2022_Qt6.12)
expect_package(2.2.0.7-rc0 linux gcc14 6.8.3 Nebula_2.2.0.7-rc0_linux_gcc14_Qt6.8)

expect_installer(Nebula_2.2.0_win-x64_msvc2022_Qt6.12 NebulaStudio_2.2.0_win-x64_msvc2022_Qt6.12_setup)
expect_installer(Nebula_2.2.0-rc0_win-arm64_msvc2026_Qt6.12 NebulaStudio_2.2.0-rc0_win-arm64_msvc2026_Qt6.12_setup)

expect_numbers(2.2.0 2.2.0.0)
expect_numbers(2.2.0.7 2.2.0.7)
expect_numbers(3 3.0.0.0)

# The one of this build: CMake has no {2,3}, hence the third number of the version written apart.
set(_pattern "^Nebula_[0-9]+\\.[0-9]+\\.[0-9]+(\\.[0-9]+)?(-[0-9A-Za-z]+)?_(win-[0-9a-z]+|linux|macos)_[a-z]+[0-9]+_Qt[0-9]+\\.[0-9]+$")
if(NOT PACKAGE_NAME MATCHES "${_pattern}")
    message(FATAL_ERROR "The package of this build, '${PACKAGE_NAME}', does not match ${_pattern}")
endif()
# And its installer, the one CPack is given.
nebula_installer_name(_installer "${PACKAGE_NAME}")
expect_equal("The installer of this build" "${INSTALLER_NAME}" "${_installer}")
