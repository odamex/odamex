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
//	Refresh, visplane stuff (floor, ceilings).
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include "r_local.h"

BOOL R_AlignFlat (int linenum, int side, int fc)
{
	line_t *line = lines + linenum;
	sector_t *sec = side ? line->backsector : line->frontsector;

	if (!sec)
		return false;

	fixed_t x = line->v1->x;
	fixed_t y = line->v1->y;

	angle_t angle = R_PointToAngle2 (x, y, line->v2->x, line->v2->y);
	angle_t norm = (angle-ANG90) >> ANGLETOFINESHIFT;

	fixed_t dist = -FixedMul (finecosine[norm], x) - FixedMul (finesine[norm], y);

	if (side)
	{
		angle = angle + ANG180;
		dist = -dist;
	}

	if (fc)
	{
		sec->base_ceiling_angle = 0-angle;
		sec->base_ceiling_yoffs = dist & ((1<<(FRACBITS+8))-1);
	}
	else
	{
		sec->base_floor_angle = 0-angle;
		sec->base_floor_yoffs = dist & ((1<<(FRACBITS+8))-1);
	}

	return true;
}


VERSION_CONTROL (r_plane_cpp, "$Id$")

