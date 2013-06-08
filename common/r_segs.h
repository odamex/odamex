// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2013 by The Odamex Team.
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
//	Refresh module, drawing LineSegs from BSP.
//
//-----------------------------------------------------------------------------


#ifndef __R_SEGS_H__
#define __R_SEGS_H__

#include "c_cvars.h"

void R_PrepWall(fixed_t px1, fixed_t py1, fixed_t px2, fixed_t py2, 
				fixed_t dist1, fixed_t dist2, int start, int stop);
void R_RenderMaskedSegRange (drawseg_t *ds, int x1, int x2);
void R_StoreWallRange(int start, int stop);
void R_RenderSegLoop();
void R_RenderSkyRange(visplane_t* pl);

EXTERN_CVAR (r_columnmethod)

#endif


