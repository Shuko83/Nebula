# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The components of StudioPlugin, loaded from its file as an application loads them, and the galaxy of NebulaStudio
# that assembles them. The directory of the plugin, built before the test, is in NEBULA_STUDIO_PLUGINS_DIR, and the
# galaxy, the one of the sources, in NEBULA_STUDIO_GALAXY.
nebula_add_test(TestStudioPlugin LINK_LIBRARIES Nebula::Core Qt6::Widgets)
target_compile_definitions(TestStudioPlugin PRIVATE
    "NEBULA_STUDIO_PLUGINS_DIR=\"$<TARGET_FILE_DIR:StudioPlugin>\""
    "NEBULA_STUDIO_GALAXY=\"${PROJECT_SOURCE_DIR}/StudioPlugin/NebulaStudio.nbp\"")
add_dependencies(TestStudioPlugin StudioPlugin)
