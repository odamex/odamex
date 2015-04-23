// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2015 by The Odamex Team.
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
//-----------------------------------------------------------------------------

#include "doomtype.h"
#include "i_video.h"
#include "v_video.h"


// Functions for v_video.cpp support

void r_dimpatchD_c(IWindowSurface* surface, argb_t color, int alpha, int x1, int y1, int w, int h)
{
	const int surface_pitch_pixels = surface->getPitchInPixels();

	argb_t* line = (argb_t*)surface->getBuffer() + y1 * surface_pitch_pixels;

	for (int y = y1; y < y1 + h; y++)
	{
		for (int x = x1; x < x1 + w; x++)
			line[x] = alphablend1a(line[x], color, alpha);

		line += surface_pitch_pixels;
	}
}

	
VERSION_CONTROL (r_drawt_cpp, "$Id$")

