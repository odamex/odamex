// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: r_interp.cpp 3798 2013-04-24 03:09:33Z dr_sean $
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
//	Interpolation of moving ceiling/floor planes, scrolling texture, etc
//	for uncapped framerates.
//
//-----------------------------------------------------------------------------

#include "m_fixed.h"
#include "r_state.h"
#include "p_local.h"

#include <vector>

typedef std::pair<fixed_t, unsigned int> fixed_uint_pair;

static std::vector<fixed_uint_pair> prev_ceilingheight;
static std::vector<fixed_uint_pair> saved_ceilingheight;
static std::vector<fixed_uint_pair> prev_floorheight;
static std::vector<fixed_uint_pair> saved_floorheight;

//
// R_InterpolationTicker
//
// Records the current height of all moving planes and position of scrolling
// textures, which will be used as the previous position during iterpolation.
// This should be called once per gametic.
//
void R_InterpolationTicker()
{
	prev_ceilingheight.clear();
	prev_floorheight.clear();

	for (int i = 0; i < numsectors; i++)
	{
		if (sectors[i].ceilingdata)
			prev_ceilingheight.push_back(std::make_pair(P_CeilingHeight(&sectors[i]), i));
		if (sectors[i].floordata)
			prev_floorheight.push_back(std::make_pair(P_FloorHeight(&sectors[i]), i));
	}
}


//
// R_BeginInterpolation
//
// Saves the current height of all moving planes and position of scrolling
// textures, which will be restored by R_EndInterpolation. The height of a
// moving plane will be interpolated between the previous height and this
// current height. This should be called every time a frame is rendered.
//
void R_BeginInterpolation(fixed_t amount)
{
	saved_ceilingheight.clear();
	saved_floorheight.clear();

	for (std::vector<fixed_uint_pair>::const_iterator ceiling_it = prev_ceilingheight.begin();
		 ceiling_it != prev_ceilingheight.end(); ++ceiling_it)
	{
		unsigned int secnum = ceiling_it->second;
		sector_t* sector = &sectors[secnum];

		fixed_t old_value = ceiling_it->first;
		fixed_t cur_value = P_CeilingHeight(sector);

		saved_ceilingheight.push_back(std::make_pair(cur_value, secnum));
		
		fixed_t new_value = old_value + FixedMul(cur_value - old_value, amount);
		P_SetCeilingHeight(sector, new_value);
	}

	for (std::vector<fixed_uint_pair>::const_iterator floor_it = prev_floorheight.begin();
		 floor_it != prev_floorheight.end(); ++floor_it)
	{
		unsigned int secnum = floor_it->second;
		sector_t* sector = &sectors[secnum];

		fixed_t old_value = floor_it->first;
		fixed_t cur_value = P_FloorHeight(sector);

		saved_floorheight.push_back(std::make_pair(cur_value, secnum));
		
		fixed_t new_value = old_value + FixedMul(cur_value - old_value, amount);
		P_SetFloorHeight(sector, new_value);
	}

}

//
// R_EndInterpolation
//
// Restores the saved height of all moving planes and position of scrolling
// textures. This should be called at the end of every frame rendered.
//
void R_EndInterpolation()
{
	for (std::vector<fixed_uint_pair>::const_iterator ceiling_it = saved_ceilingheight.begin();
		 ceiling_it != saved_ceilingheight.end(); ++ceiling_it)
	{
		sector_t* sector = &sectors[ceiling_it->second];
		P_SetCeilingHeight(sector, ceiling_it->first);
	}

	for (std::vector<fixed_uint_pair>::const_iterator floor_it = saved_floorheight.begin();
		 floor_it != saved_floorheight.end(); ++floor_it)
	{
		sector_t* sector = &sectors[floor_it->second];
		P_SetFloorHeight(sector, floor_it->first);
	}
}

//
// R_InterpolateCamera
//
// Interpolate between the current position and the previous position
// of the camera. If not using uncapped framerate / interpolation,
// render_lerp_amount will be FRACUNIT.
//
void R_InterpolateCamera(fixed_t amount)
{
	// interpolate amount/FRACUNIT percent between previous value and current value
	viewangle = viewangleoffset + camera->prevangle +
			FixedMul(amount, camera->angle - camera->prevangle);
	viewx = camera->prevx + FixedMul(amount, camera->x - camera->prevx);
	viewy = camera->prevy + FixedMul(amount, camera->y - camera->prevy);
	if (camera->player)
		viewz = camera->player->prevviewz +
				FixedMul(amount, camera->player->viewz - camera->player->prevviewz);
	else
		viewz = camera->prevz +
				FixedMul(amount, camera->z - camera->prevz);
}

VERSION_CONTROL (r_interp_cpp, "$Id: r_interp.cpp 3798 2013-04-24 03:09:33Z dr_sean $")

