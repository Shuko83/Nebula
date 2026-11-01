// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

// How the links of the edition zone are drawn from a provided interface to a consumed one, see LinkItem. Chosen in the
// View menu, for every scene at once; not saved with the project, but kept for the next launch, see Preferences.h.
enum class LinkStyle
{
    // Out of the edge of the provider its dot is on, the right one until it is put on the other, into the edge of the
    // consumer its dot is on, the left one until then, bending between them.
    Curved,
    // From one dot to the other.
    Straight,
    // Out of the edge of the provider, into the edge of the consumer, as curves do, in horizontal and vertical lines.
    RightAngled
};
