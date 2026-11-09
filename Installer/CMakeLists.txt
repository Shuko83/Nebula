# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The installer of NebulaStudio (#140), a Windows setup that installs it with what it needs to start, and nothing else:
# NebulaStudio.exe, the DLLs of Nebula, of Qt and of the compiler, the plugins its galaxy is made of in plugins/, its
# galaxy in scene/, and the licenses. CPack builds it with Inno Setup (ISCC, in its folder or in PATH), from the
# component NebulaStudio of the installation, which a full installation (the package of release.yml) leaves out:
#   cpack -C Release, from the build directory, writes <NEBULA_INSTALLER_NAME>.exe there.
# Installed for the user who runs it unless asked otherwise, in %LOCALAPPDATA%\Programs\NebulaStudio, where NebulaStudio
# can write: its dialogs start from scene/, next to it.

# Inno Setup is a Windows tool, and windeployqt the one of Qt that finds what NebulaStudio needs of Qt.
if(NOT WIN32)
    return()
endif()
if(NOT TARGET Qt6::windeployqt)
    message(WARNING "Nebula: Qt has no windeployqt, the installer of NebulaStudio is not configured")
    return()
endif()

set(_component NebulaStudio)

# NebulaStudio, and the DLLs it and its plugins link, at the root of the folder it is installed to; the plugins of its
# galaxy in plugins/, where it scans them, and its galaxy in scene/, where it finds it.
install(PROGRAMS "$<TARGET_FILE:NebulaStudio>" "$<TARGET_FILE:Core>" "$<TARGET_FILE:Application>"
    DESTINATION . COMPONENT ${_component} EXCLUDE_FROM_ALL)
install(FILES "$<TARGET_FILE:StudioPlugin>" "$<TARGET_FILE:StandardPlugin>"
    DESTINATION plugins COMPONENT ${_component} EXCLUDE_FROM_ALL)
install(FILES "${PROJECT_SOURCE_DIR}/StudioPlugin/NebulaStudio.nbp"
    DESTINATION scene COMPONENT ${_component} EXCLUDE_FROM_ALL)

# The licenses of what it is made of: Nebula's, and the one of Space Grotesk, the font of its titles.
install(FILES "${PROJECT_SOURCE_DIR}/LICENSE" DESTINATION . RENAME LICENSE.txt
    COMPONENT ${_component} EXCLUDE_FROM_ALL)
install(FILES "${PROJECT_SOURCE_DIR}/NebulaStudio/resources/fonts/OFL.txt" DESTINATION . RENAME SpaceGrotesk-OFL.txt
    COMPONENT ${_component} EXCLUDE_FROM_ALL)

# The DLLs of the compiler's runtime, next to it rather than the installer of Microsoft's redistributable, which
# windeployqt would copy: NebulaStudio starts on a computer that has none. Listed, not installed, by the module, whose
# rules would be part of a full installation.
set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP ON)
include(InstallRequiredSystemLibraries)
if(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
    install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION . COMPONENT ${_component} EXCLUDE_FROM_ALL)
endif()

# Qt, once NebulaStudio is in place: the DLLs of the modules it uses, which its plugins use as well, and the plugins of
# Qt they need. Neither translations, NebulaStudio being in English, nor what draws with OpenGL or Direct3D, which
# widgets do not.
set(_deploy_options --no-compiler-runtime --no-translations --no-opengl-sw --no-system-d3d-compiler
    --no-system-dxc-compiler)
install(CODE "
    execute_process(COMMAND \"$<TARGET_FILE:Qt6::windeployqt>\" ${_deploy_options}
        \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/$<TARGET_FILE_NAME:NebulaStudio>\"
        COMMAND_ERROR_IS_FATAL ANY)
" COMPONENT ${_component} EXCLUDE_FROM_ALL)

# What CPack packs: the component NebulaStudio alone.
set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};${PROJECT_NAME};${_component};/")
set(CPACK_GENERATOR INNOSETUP)
# Written to CPackConfig.cmake as they are: the backslash of a path of Inno Setup included.
set(CPACK_VERBATIM_VARIABLES ON)

# What the installer is: its file, NebulaStudio_2.2.0_win-x64_msvc2022_Qt6.12_setup.exe, its version, the one of the
# project with its suffix, and its id, which stays the same from one version to the next, so that installing a version
# replaces the one installed. The folder keeps its name for the same reason.
nebula_version_numbers(_numbers "${PROJECT_VERSION}")
set(CPACK_PACKAGE_NAME NebulaStudio)
set(CPACK_PACKAGE_VENDOR "Fabien Di Grande")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}${NEBULA_VERSION_SUFFIX}")
set(CPACK_PACKAGE_FILE_NAME "${NEBULA_INSTALLER_NAME}")
set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${cmu_organization}.NebulaStudio")
set(CPACK_PACKAGE_INSTALL_DIRECTORY NebulaStudio)
set(CPACK_INNOSETUP_SETUP_VersionInfoVersion "${_numbers}")
set(CPACK_INNOSETUP_SETUP_VersionInfoTextVersion "${CPACK_PACKAGE_VERSION}")
set(CPACK_INNOSETUP_SETUP_AppPublisher "${CPACK_PACKAGE_VENDOR}")
set(CPACK_INNOSETUP_SETUP_AppPublisherURL "https://github.com/Shuko83/Nebula")
set(CPACK_INNOSETUP_SETUP_AppUpdatesURL "https://github.com/Shuko83/Nebula/releases")

# For the user who runs it, without asking to be an administrator, unless the first page, or /ALLUSERS, asks for all
# the users: {autopf} is then Program Files, where NebulaStudio cannot write.
set(CPACK_INNOSETUP_SETUP_PrivilegesRequired lowest)
set(CPACK_INNOSETUP_SETUP_PrivilegesRequiredOverridesAllowed "dialog commandline")
# 64-bit Windows, x64 or an ARM64 that runs x64 binaries.
set(CPACK_INNOSETUP_SETUP_ArchitecturesAllowed x64compatible)
set(CPACK_INNOSETUP_SETUP_ArchitecturesInstallIn64BitMode x64compatible)

# The shortcut in the Start menu, at its top level, one on the desktop if the user checks it (the installer always has
# the task, unchecked), and NebulaStudio started once installed, unless the user unchecks it; the uninstaller in
# Settings > Apps, with the icon of NebulaStudio, as the installer.
set(CPACK_PACKAGE_EXECUTABLES NebulaStudio NebulaStudio)
set(CPACK_INNOSETUP_PROGRAM_MENU_FOLDER .)
set(CPACK_CREATE_DESKTOP_LINKS NebulaStudio)
set(CPACK_INNOSETUP_RUN_EXECUTABLES NebulaStudio)
set(CPACK_INNOSETUP_ICON_FILE "${PROJECT_SOURCE_DIR}/NebulaStudio/resources/nebula.ico")
set(CPACK_INNOSETUP_SETUP_UninstallDisplayIcon "{app}\\NebulaStudio.exe")
set(CPACK_INNOSETUP_USE_MODERN_WIZARD ON)
# Nothing to accept: the licenses are installed with it.
set(CPACK_INNOSETUP_IGNORE_LICENSE_PAGE ON)

include(CPack)
