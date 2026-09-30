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

#pragma once

#include "doomtype.h"
#include "r_defs.h"

// TODO: This should be automatically determined using hardware checks.
static constexpr int MAXRENDERSLICES = 64;

//
// rendercontext_t
//
// One vertical slice of the view.
//
struct rendercontext_t
{
	int	slice_start = 0;
	int	slice_stop = MAXWIDTH - 1;

	int sliceWidth() const
	{
		return slice_stop - slice_start + 1;
	}
};

// The main thread's context.
extern rendercontext_t rctx;

// How many slices the view is currently split into.
int R_SliceCount();

// The inclusive column range of slice i of nslices.
void R_SliceBounds(int i, int nslices, int& start, int& stop);

// marks the first and last column of every slice, for r_showcontexts.
void R_ShowContexts();
