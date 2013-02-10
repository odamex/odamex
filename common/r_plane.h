// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: r_plane.h 1856 2010-09-05 03:14:13Z ladna $
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
//	Refresh, visplane stuff (floor, ceilings).
//
//-----------------------------------------------------------------------------


#ifndef __R_PLANE_H__
#define __R_PLANE_H__

#include "r_data.h"

// Visplane related.
extern	int*			lastopening;

typedef void (*planefunction_t) (int top, int bottom);

extern planefunction_t	floorfunc;
extern planefunction_t	ceilingfunc_t;

extern int  			*floorclip;
extern int  			*ceilingclip;

extern fixed_t			*yslope;

void R_InitPlanes (void);
void R_ClearPlanes (void);

void
R_MapPlane
( int		y,
  int		x1,
  int		x2 );

void
R_MakeSpans
( int		x,
  int		t1,
  int		b1,
  int		t2,
  int		b2 );
  
void R_DrawPlanes (void);

visplane_t *R_FindPlane
( plane_t		secplane,
  int			picnum,
  int			lightlevel,
  fixed_t		xoffs,		// killough 2/28/98: add x-y offsets
  fixed_t		yoffs,
  fixed_t		xscale,
  fixed_t		yscale,
  angle_t		angle);

visplane_t *R_CheckPlane (visplane_t *pl, int start, int stop);

// [RH] Added for multires support
BOOL R_PlaneInitData (void);


#endif // __R_PLANE_H__


