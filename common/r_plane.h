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
//	Refresh, visplane stuff (floor, ceilings).
//
//-----------------------------------------------------------------------------

#pragma once

#include "r_data.h"
#include "resources/res_resourceid.h"

struct rendercontext_t;

extern std::unique_ptr<int[]> floorclipinitial;
extern std::unique_ptr<int[]> ceilingclipinitial;

extern std::unique_ptr<fixed_t[]> yslope;

void R_InitPlanes (void);
void R_ClearPlanes (rendercontext_t& ctx, bool fullclear);

void R_DrawPlanes (rendercontext_t& ctx);
void R_DrawPortals (rendercontext_t& ctx);

bool R_IsStackBoundary(const AActor* mo);

visplane_t* R_FindPlane(
	rendercontext_t& ctx,
	const plane_t& secplane,
	ResourceId res_id,
	uint32_t sky_transfer,
	int lightlevel,
	fixed_t xoffs,		// killough 2/28/98: add x-y offsets
	fixed_t yoffs,
	fixed_t xscale,
	fixed_t yscale,
	angle_t angle,
	AActor::AActorPtr skybox);

visplane_t *R_CheckPlane (rendercontext_t& ctx, visplane_t *pl, int start, int stop);

// [RH] Added for multires support
bool R_PlaneInitData(rendercontext_t& ctx, IWindowSurface* surface);
