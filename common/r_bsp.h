// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2012 by The Odamex Team.
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
//	Refresh module, BSP traversal and handling.
//
//-----------------------------------------------------------------------------


#ifndef __R_BSP__
#define __R_BSP__

extern const fixed_t NEARCLIP;

extern seg_t*		curline;
extern side_t*		sidedef;
extern line_t*		linedef;
extern sector_t*	frontsector;
extern sector_t*	backsector;

extern BOOL			skymap;

extern drawseg_t	*drawsegs;
extern drawseg_t*	ds_p;

extern byte *solidcol;

typedef void (*drawfunc_t) (int start, int stop);

EXTERN_CVAR (r_drawflat)		// [RH] Don't texture segs?

// BSP?
void R_ClearClipSegs (void);
void R_ReallocDrawSegs(void);
void R_ClearDrawSegs (void);
void R_RenderBSPNode (int bspnum);
bool R_DoorClosed(void);	// killough 1/17/98

// killough 4/13/98: fake floors/ceilings for deep water / fake ceilings:
sector_t *R_FakeFlat(sector_t *, sector_t *, int *, int *, BOOL);


#endif


