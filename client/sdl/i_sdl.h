// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: i_sdl.h 4483 2014-01-08 01:02:21Z dr_sean $
//
// Copyright (C) 2006-2014 by The Odamex Team.
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

#ifndef __I_SDL_H__
#define __I_SDL_H__

#include <SDL.h>

#if (SDL_MAJOR_VERSION == 2 && SDL_MINOR_VERSION == 0)
	#define SDL20
#elif (SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION == 2)
	#define SDL12
#endif

#endif  // __I_SDL_H__
