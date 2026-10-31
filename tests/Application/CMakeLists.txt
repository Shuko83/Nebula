# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The QApplication of an executable whose content is a galaxy: each test builds its own, one after the other.
nebula_add_test(TestNebulaApplication LINK_LIBRARIES Nebula::Application TestComponents)
