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

#include <memory>
#include <vector>

#include "doomtype.h"
#include "m_vectors.h"
#include "r_defs.h"

// TODO: This should be automatically determined using hardware checks.
static constexpr int MAXRENDERSLICES = 64;

// must be a power of 2: visplane hash slot count -- killough
static constexpr int MAXVISPLANES = 128;

// Visplane headers come from dense blocks so that walking a hash chain or the
// free list stays within a few pages.
static constexpr size_t VISPLANE_BLOCK = 64;

//
// planecontext_t
//
// Visplane bookkeeping and the plane texture-mapping state, all of which is
// written while a frame renders.
//
struct planecontext_t
{
	// visplane hash table and free list -- killough
	visplane_t*   visplanes[MAXVISPLANES + 1];
	visplane_t*   freetail;
	visplane_t**  freehead = &freetail;

	// the block allocator behind the hash table
	std::vector<visplane_t*>    visplane_blocks;
	size_t                      visplane_block_used = VISPLANE_BLOCK;
	std::vector<unsigned int*>  visplane_spans;

	visplane_t* floorplane;
	visplane_t* ceilingplane;
	visplane_t* skyplane;

	// clip values are the solid pixel bounding the range:
	// floorclip starts out viewheight, ceilingclip starts out -1
	std::unique_ptr<int[]> floorclip;
	std::unique_ptr<int[]> ceilingclip;

	// spanstart holds the start of a plane span at each row
	std::unique_ptr<int[]> spanstart;

	int*        planezlight;
	float       plight;
	float       shade;

	double      pl_xscale;
	double      pl_yscale;
	double      pl_viewsin;
	double      pl_viewcos;
	double      pl_viewxtrans;
	double      pl_viewytrans;
	double      pl_xstepscale;
	double      pl_ystepscale;
	double      pl_planeheight;

	// sloped plane texture-mapping vectors
	v3float_t   slope_a;
	v3float_t   slope_b;
	v3float_t   slope_c;
	float       ixscale;
	float       iyscale;

	// freehead points at freetail, so a copy would dangle.
	// So let's make sure nobody tries to copy one.
	planecontext_t() = default;
	planecontext_t(const planecontext_t&) = delete;
	planecontext_t& operator=(const planecontext_t&) = delete;
};

//
// rendercontext_t
//
// One vertical slice of the view.
//
struct rendercontext_t
{
	planecontext_t plane;

	int	slice_start = 0;
	int	slice_stop = MAXWIDTH - 1;

	int sliceWidth() const
	{
		return slice_stop - slice_start + 1;
	}
};

// The main thread's context.
extern rendercontext_t rctx;

inline visplane_t*&	floorplane   = ::rctx.plane.floorplane;
inline visplane_t*&	ceilingplane = ::rctx.plane.ceilingplane;
inline visplane_t*&	skyplane     = ::rctx.plane.skyplane;

inline std::unique_ptr<int[]>& floorclip   = ::rctx.plane.floorclip;
inline std::unique_ptr<int[]>& ceilingclip = ::rctx.plane.ceilingclip;

// How many slices the view is currently split into.
int R_SliceCount();

// The inclusive column range of slice i of nslices.
void R_SliceBounds(int i, int nslices, int& start, int& stop);

// marks the first and last column of every slice, for r_showcontexts.
void R_ShowContexts();
