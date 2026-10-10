// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Refresh module, drawing LineSegs from BSP.
//
//-----------------------------------------------------------------------------

#pragma once

struct rendercontext_t;

void R_PrepWall(rendercontext_t& ctx, fixed_t px1, fixed_t py1, fixed_t px2, fixed_t py2,
				fixed_t tx1, fixed_t ty1, fixed_t tx2, fixed_t ty2, int start, int stop);
void R_RenderMaskedSegRange (rendercontext_t& ctx, drawseg_t *ds, int x1, int x2);
void R_StoreWallRange(rendercontext_t& ctx, int start, int stop);
void R_ClearOpenings(rendercontext_t& ctx);

EXTERN_CVAR (r_columnmethod)
