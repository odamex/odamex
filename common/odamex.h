// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//     This header must be included first by all Odamex/Odasrv TU's.
//
//-----------------------------------------------------------------------------

#pragma once

#if defined(_MSC_VER)
	#if _MSC_VER >= 1600
		#define USE_STDINT_H
	#endif
#else
	#define USE_STDINT_H
#endif

#if defined(USE_STDINT_H)
	#include <stdint.h>
	#undef USE_STDINT_H
#else
	#include "pstdint.h"
#endif

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <string>
#include <vector>

#include "fmt/format.h"
#include "fmt/printf.h"

#include "doomtype.h"
#include "doomdef.h"
#include "doomstat.h"
