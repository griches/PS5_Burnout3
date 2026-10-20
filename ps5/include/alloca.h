/* Burnout 3 for the PlayStation 5 - <alloca.h>, which a FreeBSD has in <stdlib.h>.
 * SPDX-License-Identifier: MIT */
#pragma once
#include <stdlib.h>
#ifndef alloca
#define alloca(size) __builtin_alloca(size)
#endif
