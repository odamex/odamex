// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//
// SDL header wrapper
//
//-----------------------------------------------------------------------------

#pragma once

#include <SDL.h>

#if (SDL_MAJOR_VERSION == 2)
	#define SDL20
	#if (SDL_MINOR_VERSION > 0 || SDL_PATCHLEVEL >= 16)
		#define SDL2016
	#endif
#elif (SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION == 2)
	#define SDL12
#endif

#if (SDL_VERSION > SDL_VERSIONNUM(1, 2, 7))
	#include "SDL_cpuinfo.h"
#endif
