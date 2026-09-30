// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2026 by The Odamex Team.
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


#include "odamex.h"

#include "i_video.h"
#include "v_video.h"


// Functions for v_video.cpp support

void r_dimpatchD_c(IWindowSurface* surface, argb_t color, int alpha, int x1, int y1, int w, int h)
{
	const int rowstep = surface->getRowStepInPixels();
	const int colstep = surface->getColStepInPixels();

	// A screen column is the contiguous run, so the columns are the outer walk
	// and the rows the inner one. line[j] steps a row because rowstep is 1.
	const int run = h;
	const int count = w;
	const int stride = colstep;

	argb_t* line = reinterpret_cast<argb_t*>(surface->getBuffer()) + y1 * rowstep + x1 * colstep;

	for (int i = 0; i < count; i++, line += stride)
	{
		for (int j = 0; j < run; j++)
			line[j] = alphablend1a(line[j], color, alpha);
	}
}


VERSION_CONTROL (r_drawt_cpp, "$Id$")

