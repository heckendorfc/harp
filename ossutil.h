/*
 *  Copyright (C) 2009-2012  Christian Heckendorf <heckendorfc@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _OSSUTIL_H
#define _OSSUTIL_H

#include "defs.h"
#include <errno.h>

/* OSSv3 missing:
 * SNDCTL_DSP_SKIP
 * SNDCTL_DSP_SILENCE
 * SNDCTL_DSP_GETPLAYVOLUME
 * SNDCTL_DSP_SETPLAYVOLUME
 */

#if SOUND_VERSION >= 0x040000
#define OSSV4_DEFS
#else
#undef OSSV4_DEFS
#endif

#endif
