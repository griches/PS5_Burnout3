/*
 * Doom 3 BFG for the PlayStation 5 - <malloc.h>, as a Linux game expects it.
 *
 * The console's C library is FreeBSD's, whose <malloc.h> is an #error: malloc
 * is in <stdlib.h> there, and alloca in <stdlib.h> too. RBDoom3BFG includes
 * <malloc.h> on every system but Windows and macOS (for alloca).
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once
#include <stdlib.h>
