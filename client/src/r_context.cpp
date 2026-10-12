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
#include <memory>
#include <vector>

#include "r_context.h"

#include "c_cvars.h"
#include "doomstat.h"
#include "i_video.h"
#include "r_main.h"
#include "r_things.h"
#include "v_video.h"

EXTERN_CVAR(r_threads)
EXTERN_CVAR(r_showcontexts)

rendercontext_t rctx;

// Slices 1..n-1.
// Slice 0 is rctx (main thread), so the one slice case allocates nothing.
static std::vector<std::unique_ptr<rendercontext_t>> extra_contexts;

static int activeslices = 1;

//
// R_ActiveSlices
//
int R_ActiveSlices()
{
	return activeslices;
}

//
// R_SliceContext
//
rendercontext_t& R_SliceContext(int i)
{
	return i == 0 ? rctx : *extra_contexts[i - 1];
}

//
// R_SyncRenderContexts
//
// Grows the context set to match r_threads.
// Contexts are never released in order to only allocate once.
//
bool R_SyncRenderContexts()
{
	const int want = R_SliceCount();

	bool grew = false;
	while (extra_contexts.size() + 1 < static_cast<size_t>(want))
	{
		extra_contexts.push_back(std::make_unique<rendercontext_t>());
		R_InitVisSprites(*extra_contexts.back());
		grew = true;
	}

	activeslices = want;
	return grew;
}

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

	const int nslices = R_ActiveSlices();

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
