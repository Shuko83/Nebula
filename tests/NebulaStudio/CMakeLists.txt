# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The classes of NebulaStudio, which each test links from NebulaStudioClasses, compiled once (#146): the linker only
# keeps what the test uses. A test that shows the icons or the font of the theme also links NebulaStudioResources, and
# Qt Svg, which draws the icons.
set(_resources NebulaStudioResources Qt6::Svg)

nebula_add_test(TestColors LINK_LIBRARIES NebulaStudioClasses)
nebula_add_test(TestTheme LINK_LIBRARIES NebulaStudioClasses ${_resources})
nebula_add_test(TestPluginModel LINK_LIBRARIES NebulaStudioClasses)
nebula_add_test(TestComponentModel LINK_LIBRARIES NebulaStudioClasses TestComponents ${_resources})
nebula_add_test(TestEditionView LINK_LIBRARIES NebulaStudioClasses)
# The overview of the edition zone, which follows its view.
nebula_add_test(TestSceneOverview LINK_LIBRARIES NebulaStudioClasses)
# What makes room as rows are dragged from a list.
nebula_add_test(TestDragAside LINK_LIBRARIES NebulaStudioClasses)
nebula_add_test(TestPreferences LINK_LIBRARIES NebulaStudioClasses)
# The edition zone, with its items, the dialogs it opens and the steps of its history.
nebula_add_test(TestEditionScene LINK_LIBRARIES NebulaStudioClasses TestComponents ${_resources})
# The steps of the history of the edition zone: each edit undone, then done again.
nebula_add_test(TestSceneCommand LINK_LIBRARIES NebulaStudioClasses TestComponents ${_resources})
# The field that finds the instances of the edition zone.
nebula_add_test(TestComponentFinder LINK_LIBRARIES NebulaStudioClasses TestComponents ${_resources})
# The property editor, and the values of a settings file, which it shows in place of a component.
nebula_add_test(TestPropertyEditor LINK_LIBRARIES NebulaStudioClasses TestComponents ${_resources})
nebula_add_test(TestSettingsValueEditor LINK_LIBRARIES NebulaStudioClasses TestComponents ${_resources})
# The dialog that shows and edits the text of a settings file.
nebula_add_test(TestSettingsTextDialog LINK_LIBRARIES NebulaStudioClasses TestComponents ${_resources})
# The dialog of a settings file.
nebula_add_test(TestSettingsDialog LINK_LIBRARIES NebulaStudioClasses TestComponents ${_resources})
# The dialog of a stargate, with the items whose names it shows.
nebula_add_test(TestStargateDialog LINK_LIBRARIES NebulaStudioClasses TestComponents ${_resources})
nebula_add_test(TestGalaxyModel LINK_LIBRARIES NebulaStudioClasses ${_resources})
