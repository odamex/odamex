// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2026 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
//-----------------------------------------------------------------------------

#pragma once

#include <cstdint>

#include "doomtype.h"

class AActor;
struct vissprite_t;

struct r_voxelvis_s
{
	const void* model;
	angle_t angle;
	fixed_t TL_x;
	fixed_t TL_y;
	fixed_t c;
	fixed_t s;
	fixed_t pitchSlope;
};

void VX_Init();
void VX_ClearVoxels();
void VX_NearbySprites();
bool VX_ProjectVoxel(const AActor* thing, int frame, vissprite_t* vis);
void VX_DrawVoxel(vissprite_t* spr);
