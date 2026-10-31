# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The classes of NebulaStudio that do not need its window, compiled again with each test: NebulaStudio is an
# executable, nothing links to it.
set(_studio "${PROJECT_SOURCE_DIR}/NebulaStudio")
# The icons and the font of the theme.
set(_resources "${_studio}/resources/nebulastudio.qrc")

nebula_add_test(TestColors INCLUDE_DIRECTORIES "${_studio}/src" LINK_LIBRARIES Qt6::Gui)
nebula_add_test(TestTheme
    SOURCES "${_studio}/src/Theme.cpp" ${_resources}
    INCLUDE_DIRECTORIES "${_studio}/src"
    LINK_LIBRARIES Qt6::Widgets Qt6::Svg)
nebula_add_test(TestPluginModel
    SOURCES "${_studio}/src/PluginModel.cpp"
    INCLUDE_DIRECTORIES "${_studio}/src"
    LINK_LIBRARIES Nebula::Core Qt6::Gui)
nebula_add_test(TestComponentModel
    SOURCES "${_studio}/src/ComponentModel.cpp" "${_studio}/src/Theme.cpp" ${_resources}
    INCLUDE_DIRECTORIES "${_studio}/src"
    LINK_LIBRARIES TestComponents Qt6::Widgets Qt6::Svg)
nebula_add_test(TestEditionView
    SOURCES "${_studio}/src/EditionView.cpp"
    INCLUDE_DIRECTORIES "${_studio}/src"
    LINK_LIBRARIES Qt6::Widgets)
nebula_add_test(TestPreferences
    SOURCES "${_studio}/src/Preferences.cpp"
    INCLUDE_DIRECTORIES "${_studio}/src"
    LINK_LIBRARIES Qt6::Core)
# The edition zone, with its items and the dialogs it opens.
set(_scene
    "${_studio}/src/EditionScene.cpp" "${_studio}/src/ContextMenu.cpp" "${_studio}/src/ComponentItem.cpp"
    "${_studio}/src/InterfaceItem.cpp" "${_studio}/src/LinkItem.cpp" "${_studio}/src/AnchorItem.cpp"
    "${_studio}/src/LinkAnchors.cpp" "${_studio}/src/DashedLinkItem.cpp" "${_studio}/src/NameEditor.cpp"
    "${_studio}/src/ZoneItem.cpp" "${_studio}/src/Theme.cpp" "${_studio}/src/EditDialog.cpp"
    "${_studio}/src/PropertyTree.cpp" "${_studio}/src/WarningLabel.cpp" "${_studio}/src/DialogHeader.cpp"
    "${_studio}/src/PlaceholderStack.cpp" "${_studio}/src/ChoicePanel.cpp"
    "${_studio}/src/Galaxy/StargateItem.cpp" "${_studio}/src/Galaxy/StargateDialog.cpp"
    "${_studio}/src/Settings/SettingsItem.cpp" "${_studio}/src/Settings/SettingsDialog.cpp"
    ${_resources})
nebula_add_test(TestEditionScene
    SOURCES ${_scene}
    INCLUDE_DIRECTORIES "${_studio}/src"
    LINK_LIBRARIES TestComponents Qt6::Widgets Qt6::Svg)
# The field that finds the instances of the edition zone.
nebula_add_test(TestComponentFinder
    SOURCES "${_studio}/src/ComponentFinder.cpp" ${_scene}
    INCLUDE_DIRECTORIES "${_studio}/src"
    LINK_LIBRARIES TestComponents Qt6::Widgets Qt6::Svg)
nebula_add_test(TestPropertyEditor
    SOURCES
        "${_studio}/src/PropertyEditor.cpp" "${_studio}/src/PlaceholderStack.cpp" "${_studio}/src/PropertyTree.cpp"
        "${_studio}/src/Theme.cpp" ${_resources}
    INCLUDE_DIRECTORIES "${_studio}/src"
    LINK_LIBRARIES TestComponents Qt6::Widgets Qt6::Svg)
# The dialog of a settings file.
nebula_add_test(TestSettingsDialog
    SOURCES
        "${_studio}/src/Settings/SettingsDialog.cpp" "${_studio}/src/EditDialog.cpp" "${_studio}/src/DialogHeader.cpp"
        "${_studio}/src/PlaceholderStack.cpp" "${_studio}/src/PropertyTree.cpp" "${_studio}/src/WarningLabel.cpp"
        "${_studio}/src/ChoicePanel.cpp" "${_studio}/src/Theme.cpp" ${_resources}
    INCLUDE_DIRECTORIES "${_studio}/src"
    LINK_LIBRARIES TestComponents Qt6::Widgets Qt6::Svg)
nebula_add_test(TestGalaxyModel
    SOURCES "${_studio}/src/Galaxy/GalaxyModel.cpp" "${_studio}/src/Theme.cpp" ${_resources}
    INCLUDE_DIRECTORIES "${_studio}/src"
    LINK_LIBRARIES Nebula::Core Qt6::Widgets Qt6::Svg)
