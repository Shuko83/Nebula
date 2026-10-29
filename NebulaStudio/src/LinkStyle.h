// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

// How the links of the edition zone are drawn from a provided interface to a consumed one, see LinkItem. Chosen in the
// View menu, for every scene at once; not saved with the project.
enum class LinkStyle
{
    // Out of the right edge of the provider, into the left edge of the consumer, bending between them.
    Curved,
    // From one dot to the other.
    Straight,
    // Out of the right edge of the provider, into the left edge of the consumer, in horizontal and vertical lines.
    RightAngled
};
