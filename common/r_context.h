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
#include "m_mempool.h"
#include "m_vectors.h"
#include "r_defs.h"

class Texture;

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
// segcontext_t
//
// Wall state for the segs currently being stored, and the per-column arrays,
// one per context.
//
struct segcontext_t
{
	// pools for the clipping arrays hanging off drawsegs
	// these are half a megabyte each, so be sure to use nslices
	// to allocate them instead of MAXRENDERSLICES
	Pool<const palindex_t*> masked_midposts_pool{4096};
	Pool<int>               sprclip_pool{4096};
	Pool<fixed_t>           midscales_pool{4096};

	bool    segtextured;    // true if any of the seg's textures might be visible
	bool    markfloor;      // false if the back side is the same plane
	bool    markceiling;
	bool    didsolidcol;

	const Texture* toptexture;
	const Texture* bottomtexture;
	const Texture* midtexture;
	const Texture* maskedtexture;

	int*    walllights;

	fixed_t rw_light;       // [RH] use different scaling for lights
	fixed_t rw_lightstep;
	fixed_t rw_scale;
	fixed_t rw_scalestep;
	fixed_t rw_midtexturemid;
	fixed_t rw_toptexturemid;
	fixed_t rw_bottomtexturemid;

	// floor and ceiling heights at the end points of a seg_t,
	// set by r_bsp and read by r_segs
	fixed_t rw_backcz1;
	fixed_t rw_backcz2;
	fixed_t rw_backfz1;
	fixed_t rw_backfz2;
	fixed_t rw_frontcz1;
	fixed_t rw_frontcz2;
	fixed_t rw_frontfz1;
	fixed_t rw_frontfz2;

	int     rw_start;
	int     rw_stop;
	bool    rw_hashigh;
	bool    rw_haslow;

	int     walltopf[MAXWIDTH];
	int     walltopb[MAXWIDTH];
	int     wallbottomf[MAXWIDTH];
	int     wallbottomb[MAXWIDTH];

	const palindex_t* topposts[MAXWIDTH];
	const palindex_t* midposts[MAXWIDTH];
	const palindex_t* bottomposts[MAXWIDTH];

	const palindex_t** masked_midposts;
	const fixed_t*     masked_midscales;  // used to know where to draw masked post positions

	// y-scale of the texture tier currently being drawn by the solid column blaster
	fixed_t wallscaley = FRACUNIT;
	fixed_t wallscalex[MAXWIDTH];
	int     texoffs[MAXWIDTH];

	// per-column scale and wall-parameter values
	double  wallscaled[MAXWIDTH];
	double  wallufrac[MAXWIDTH];
};

//
// bspcontext_t
//
// What the BSP traversal is currently looking at, the drawseg list it fills,
// and the per-column "already solid" flags.
//
struct bspcontext_t
{
	const seg_t* curline;
	side_t*      sidedef;
	line_t*      linedef;
	sector_t*    frontsector;
	sector_t*    backsector;

	// killough 4/7/98: indicates doors closed wrt automap bugfix
	bool    doorclosed;

	bool    r_fakingunderwater;  // TODO: This may need to be set per frame
	bool    r_underwater;        // instead of per context

	byte    fakeside;

	drawseg_t* ds_p;
	drawseg_t* drawsegs;
	drawseg_t* firstdrawseg;
	unsigned   maxdrawsegs;

	// CPhipps - instead of clipsegs, one entry per column indicating whether
	// it is blocked by a solid wall yet.
	// e6y: resolution limitation removed
	byte    solidcol[MAXWIDTH];
};

//
// rendercontext_t
//
// One vertical slice of the view.
//
struct rendercontext_t
{
	planecontext_t plane;
	segcontext_t   seg;
	bspcontext_t   bsp;

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

inline int*& walllights        = ::rctx.seg.walllights;
inline Pool<int>& sprclip_pool = ::rctx.seg.sprclip_pool;

inline fixed_t& rw_backcz1  = ::rctx.seg.rw_backcz1;
inline fixed_t& rw_backcz2  = ::rctx.seg.rw_backcz2;
inline fixed_t& rw_backfz1  = ::rctx.seg.rw_backfz1;
inline fixed_t& rw_backfz2  = ::rctx.seg.rw_backfz2;
inline fixed_t& rw_frontcz1 = ::rctx.seg.rw_frontcz1;
inline fixed_t& rw_frontcz2 = ::rctx.seg.rw_frontcz2;
inline fixed_t& rw_frontfz1 = ::rctx.seg.rw_frontfz1;
inline fixed_t& rw_frontfz2 = ::rctx.seg.rw_frontfz2;

inline const seg_t*& curline     = ::rctx.bsp.curline;
inline side_t*&      sidedef     = ::rctx.bsp.sidedef;
inline line_t*&      linedef     = ::rctx.bsp.linedef;
inline sector_t*&    frontsector = ::rctx.bsp.frontsector;
inline sector_t*&    backsector  = ::rctx.bsp.backsector;

inline bool& doorclosed         = ::rctx.bsp.doorclosed;
inline bool& r_fakingunderwater = ::rctx.bsp.r_fakingunderwater;
inline bool& r_underwater       = ::rctx.bsp.r_underwater;

inline drawseg_t*& ds_p         = ::rctx.bsp.ds_p;
inline drawseg_t*& drawsegs     = ::rctx.bsp.drawsegs;
inline drawseg_t*& firstdrawseg = ::rctx.bsp.firstdrawseg;

inline byte (&solidcol)[MAXWIDTH] = ::rctx.bsp.solidcol;

// How many slices the view is currently split into.
int R_SliceCount();

// The inclusive column range of slice i of nslices.
void R_SliceBounds(int i, int nslices, int& start, int& stop);

// marks the first and last column of every slice, for r_showcontexts.
void R_ShowContexts();
