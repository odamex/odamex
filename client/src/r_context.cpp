// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
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
//	Per-thread render context.
//
//	Split the screen into vertical slices, and render each of them
//	concurrently before compositing the results into the final frame.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include <algorithm>

#include "r_context.h"

#include "c_cvars.h"
#include "doomstat.h"
#include "i_video.h"
#include "r_main.h"
#include "v_video.h"

EXTERN_CVAR(r_threads)
EXTERN_CVAR(r_showcontexts)

rendercontext_t rctx;

//
// R_SliceCount
//
// One slice per render thread, limited to >4 columns.
//
int R_SliceCount()
{
	int nslices = std::clamp(r_threads.asInt(), 1, MAXRENDERSLICES);

	while (nslices > 1 && (viewwidth >> 4) / nslices < 4)
		nslices--;

	return nslices;
}

//
// R_SliceBounds
//
// Distributes whole 16-pixel column groups evenly with any remainder
// going to the last slice.
//
void R_SliceBounds(int i, int nslices, int& start, int& stop)
{
	const int ngroups = viewwidth >> 4;

	start = (ngroups * i / nslices) << 4;
	stop = (i == nslices - 1) ? viewwidth - 1
	                          : ((ngroups * (i + 1) / nslices) << 4) - 1;
}

//
// R_ShowContexts
//
// Marks the first and last column of every slice.
// (And clears the top/bottom of the slice)
//
void R_ShowContexts()
{
	if (!r_showcontexts)
		return;

	IWindowSurface* surface = R_GetRenderingSurface();
	DCanvas* canvas = surface->getDefaultCanvas();

	const argb_t slicecolor(255, 0, 255);		// pink: nothing else draws it
	const int y1 = viewwindowy;
	const int y2 = viewwindowy + viewheight;

	const int nslices = R_SliceCount();

	for (int i = 0; i < nslices; i++)
	{
		int start, stop;
		R_SliceBounds(i, nslices, start, stop);

		start += viewwindowx;
		stop += viewwindowx;

		canvas->Clear(start, y1, start + 1, y2, slicecolor);
		canvas->Clear(stop, y1, stop + 1, y2, slicecolor);
	}
}

VERSION_CONTROL (r_context_cpp, "$Id$")
