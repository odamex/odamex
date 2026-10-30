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

#include <functional>
#include <memory>
#include <vector>

#include "doomtype.h"
#include "m_mempool.h"
#include "m_vectors.h"
#include "r_defs.h"
#include "r_draw.h"

class Texture;

// A reasonable maximum for render slices
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

	std::vector<uint16_t> plane_lightoff;
	bool                  plane_constlight;

	std::vector<int>      group_runstart;

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

	// [RH] ignore extralight and fullbright.
	bool    foggy;

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
// spritecontext_t
//
// The vissprite pool each slice fills, and the masked-column state the sprite
// and masked-midtexture drawers read.
//
struct spritecontext_t
{
	// the vissprite pool, grown on demand by R_NewVisSprite
	vissprite_t* vissprites;
	vissprite_t* firstvissprite;
	vissprite_t* lastvissprite;
	vissprite_t* vissprite_p;
	int          maxvissprites;
	int          newvissprite;

	int*         spritelights;

	// weapon bob for the player sprite
	fixed_t bobx;
	fixed_t boby;

	// vars for R_DrawMaskedColumn
	const int* mfloorclip;
	const int* mceilingclip;
	fixed_t    spryscale;
	fixed_t    sprtopscreen;

	int32_t            projspritenum = -1;
	const spritedef_t* projsprdef = NULL;
	bool               projlerp = false;

	const palindex_t* spriteposts[MAXWIDTH];
};

struct rendercontext_t;

//
// drawcontext_t
//
// What the drawers read: the four argument blocks, the colormap of whatever is
// currently being drawn, the fuzz cursor, and drawer selection.
//
struct drawcontext_t
{
	drawcolumn_t     dcol;
	drawspan_t       dspan;
	drawplanecol_t   dpcol;
	drawplanegroup_t dpgroup;

	// [RH] colormap of the sector/wall/plane currently being drawn
	shaderef_t  basecolormap;

	FuzzTable   fuzztable;

	// The selected drawers.
	void (*colfunc)(rendercontext_t&);
	// Fills or blanks one row of a plane under r_drawflat or nodrawers
	void (*spanfunc)(rendercontext_t&);

	// Draws one column of a level plane, or NULL when planes must go through
	// the span path.
	void (*levelcolfunc)(rendercontext_t&);

	// Draws one column of a sloped plane, or NULL when sloped planes must go
	// through the span path.
	void (*slopecolfunc)(rendercontext_t&);

	// dcol and dspan carry pointers back into this struct, so a copy would
	// point at the wrong context
	drawcontext_t()
	{
		dcol.fuzz = &fuzztable;
		dcol.basecolormap = &basecolormap;
		dspan.basecolormap = &basecolormap;
	}

	drawcontext_t(const drawcontext_t&) = delete;
	drawcontext_t& operator=(const drawcontext_t&) = delete;
};

//
// rendercontext_t
//
// One vertical slice of the view.
//
struct rendercontext_t
{
	planecontext_t  plane;
	segcontext_t    seg;
	bspcontext_t    bsp;
	spritecontext_t sprite;
	drawcontext_t   draw;

	int	slice_start = 0;
	int	slice_stop = MAXWIDTH - 1;

	int sliceWidth() const
	{
		return slice_stop - slice_start + 1;
	}
};

// The main thread's (slice 0) context.
extern rendercontext_t rctx;

// Brings the context set in line with r_threads.
// Returns true when contexts were added.
bool R_SyncRenderContexts();

// How many slices the frame is being rendered as.
int R_ActiveSlices();

// Slice i's context. Slice 0 is rctx.
rendercontext_t& R_SliceContext(int i);

// Runs the same job for every slice at once.
// The calling thread and the render workers claim slices until none are left,
// in the order given (slice indices), or 0..nslices-1 when order is NULL.
// Returns when all have finished, and rethrows the first exception any slice raised.
void R_RunSlices(int nslices, const std::function<void(int)>& job, const int* order = NULL);

// How many slices the view is currently split into.
int R_SliceCount();

// The inclusive column range of slice i of nslices.
void R_SliceBounds(int i, int nslices, int& start, int& stop);

// marks the first and last column of every slice, for r_showcontexts.
void R_ShowContexts();
