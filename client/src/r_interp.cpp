// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
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


#include "odamex.h"

#include "m_fixed.h"
#include "r_state.h"
#include "r_sky.h"
#include "p_local.h"

EXTERN_CVAR(sv_allowmovebob)
EXTERN_CVAR(cl_movebob)

typedef std::pair<fixed_t, unsigned int> fixed_uint_pair;
typedef std::pair <std::pair<fixed_t, fixed_t>, unsigned int> fixed_fixed_uint_pair;

// Sector heights
static std::vector<fixed_uint_pair> prev_ceilingheight;
static std::vector<fixed_uint_pair> saved_ceilingheight;
static std::vector<fixed_uint_pair> prev_floorheight;
static std::vector<fixed_uint_pair> saved_floorheight;

// Line scrolling
static std::vector<fixed_fixed_uint_pair> prev_linescrollingtex; // <x offs, yoffs>, linenum
static std::vector<fixed_fixed_uint_pair> saved_linescrollingtex;

// Floor/Ceiling scrolling
static std::vector<fixed_fixed_uint_pair> prev_sectorceilingscrollingflat; // <x offs, yoffs>, sectornum
static std::vector<fixed_fixed_uint_pair> saved_sectorceilingscrollingflat;
static std::vector<fixed_fixed_uint_pair> prev_sectorfloorscrollingflat; // <x offs, yoffs>, sectornum
static std::vector<fixed_fixed_uint_pair> saved_sectorfloorscrollingflat;

// Skies
static fixed_t saved_sky1offset;
static fixed_t prev_sky1offset;
static fixed_t saved_sky2offset;
static fixed_t prev_sky2offset;

// Automap x/y/angle
static fixed_t saved_amx;
static fixed_t prev_amx;
static fixed_t saved_amy;
static fixed_t prev_amy;
static fixed_t saved_amangle;
static fixed_t prev_amangle;

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
	prev_linescrollingtex.clear();
	prev_sectorceilingscrollingflat.clear();
	prev_sectorfloorscrollingflat.clear();
	prev_sky1offset = 0;
	prev_sky2offset = 0;
	prev_amx = 0;
	prev_amy = 0;
	prev_amangle = 0;

	if (gamestate == GS_LEVEL)
	{
		for (int i = 0; i < numsectors; i++)
		{
			if (sectors[i].ceilingdata)
				prev_ceilingheight.push_back(std::make_pair(P_CeilingHeight(&sectors[i]), i));
			if (sectors[i].floordata)
				prev_floorheight.push_back(std::make_pair(P_FloorHeight(&sectors[i]), i));
		}

		// Handle the scrolling interpolation
		TThinkerIterator<DScroller> iterator;
		DScroller* scroller;

		while ((scroller = iterator.Next()))
		{
			DScroller::EScrollType type = scroller->GetType();
			int affectee = scroller->GetAffectee();
			if (P_WallScrollType(type))
			{
				int wallnum = scroller->GetWallNum();

				if (wallnum >= 0) // huh?!?
				{
					prev_linescrollingtex.push_back(
						std::make_pair(std::make_pair(
							sides[wallnum].textureoffset,
							sides[wallnum].rowoffset),
								wallnum));
				}
			}
			else if (P_CeilingScrollType(type))
			{
				prev_sectorceilingscrollingflat.push_back(
						std::make_pair(std::make_pair(
							sectors[affectee].ceiling_xoffs,
							sectors[affectee].ceiling_yoffs),
								affectee));
			}
			else if (P_FloorScrollType(type))
			{
				prev_sectorfloorscrollingflat.push_back(
						std::make_pair(std::make_pair(
							sectors[affectee].floor_xoffs,
							sectors[affectee].floor_yoffs),
								affectee));
			}
		}

		// Update sky offsets
		prev_sky1offset = sky1columnoffset;
		prev_sky2offset = sky2columnoffset;

		// Update automap coords
		player_t& player = displayplayer();

		prev_amx = player.camera->x;
		prev_amy = player.camera->y;
		prev_amangle = player.camera->angle;
	}
}

//
// R_AutomapInterpolationTicker()
// Always runs the first frame while automap is active
void R_AutomapInterpolationTicker()
{
	// Update automap coords
	player_t& player = displayplayer();

	prev_amx = player.camera->x;
	prev_amy = player.camera->y;
	prev_amangle = player.camera->angle;
}


//
// R_ResetInterpolation
//
// Clears any saved interpolation related data. This should be called whenever
// a map is loaded.
//
void R_ResetInterpolation()
{
	prev_ceilingheight.clear();
	prev_floorheight.clear();
	prev_linescrollingtex.clear();
	prev_sectorceilingscrollingflat.clear();
	prev_sectorfloorscrollingflat.clear();
	saved_ceilingheight.clear();
	saved_floorheight.clear();
	saved_linescrollingtex.clear();
	saved_sectorceilingscrollingflat.clear();
	saved_sectorfloorscrollingflat.clear();
	prev_sky1offset = 0;
	prev_sky2offset = 0;
	saved_sky1offset = 0;
	saved_sky2offset = 0;
	prev_amx = 0;
	prev_amy = 0;
	prev_amangle = 0;
	saved_amx = 0;
	saved_amy = 0;
	saved_amangle = 0;
	::localview.angle = 0;
	::localview.setangle = false;
	::localview.skipangle = false;
	::localview.pitch = 0;
	::localview.setpitch = false;
	::localview.skippitch = false;
}

//
// Functions that assist with the interpolation of certain game objects
void R_InterpolateCeilings(fixed_t amount)
{
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

	for (std::vector<fixed_fixed_uint_pair>::const_iterator ceilingscroll_it = prev_sectorceilingscrollingflat.begin();
		 ceilingscroll_it != prev_sectorceilingscrollingflat.end(); ++ceilingscroll_it)
	{
		unsigned int secnum = ceilingscroll_it->second;
		const sector_t* sector = &sectors[secnum];

		fixed_uint_pair offs = ceilingscroll_it->first;

		fixed_t cur_x = sector->ceiling_xoffs;
		fixed_t cur_y = sector->ceiling_yoffs;

		fixed_t old_x = offs.first;
		fixed_t old_y = offs.second;

		saved_sectorceilingscrollingflat.push_back(
		    std::make_pair(std::make_pair(cur_x, cur_y), secnum));

		fixed_t new_x = old_x + FixedMul(cur_x - old_x, amount);
		fixed_t new_y = old_y + FixedMul(cur_y - old_y, amount);

		sectors[secnum].ceiling_xoffs = new_x;
		sectors[secnum].ceiling_yoffs = new_y;
	}
}

void R_InterpolateFloors(fixed_t amount)
{
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

	for (std::vector<fixed_fixed_uint_pair>::const_iterator floorscroll_it = prev_sectorfloorscrollingflat.begin();
		 floorscroll_it != prev_sectorfloorscrollingflat.end(); ++floorscroll_it)
	{
		unsigned int secnum = floorscroll_it->second;
		const sector_t* sector = &sectors[secnum];

		fixed_uint_pair offs = floorscroll_it->first;

		fixed_t old_x = offs.first;
		fixed_t old_y = offs.second;

		fixed_t cur_x = sector->floor_xoffs;
		fixed_t cur_y = sector->floor_yoffs;

		saved_sectorfloorscrollingflat.push_back(
		    std::make_pair(std::make_pair(cur_x, cur_y), secnum));

		fixed_t new_x = old_x + FixedMul(cur_x - old_x, amount);
		fixed_t new_y = old_y + FixedMul(cur_y - old_y, amount);

		sectors[secnum].floor_xoffs = new_x;
		sectors[secnum].floor_yoffs = new_y;
	}
}

void R_InterpolateWalls(fixed_t amount)
{
	for (std::vector<fixed_fixed_uint_pair>::const_iterator side_it = prev_linescrollingtex.begin();
		 side_it != prev_linescrollingtex.end(); ++side_it)
	{
		unsigned int sidenum = side_it->second;
		const side_t* side = &sides[sidenum];

		fixed_uint_pair offs = side_it->first;

		fixed_t old_x = offs.first;
		fixed_t old_y = offs.second;

		fixed_t cur_x = side->textureoffset;
		fixed_t cur_y = side->rowoffset;

		saved_linescrollingtex.push_back(
		    std::make_pair(std::make_pair(cur_x, cur_y), sidenum));

		fixed_t new_x = old_x + FixedMul(cur_x - old_x, amount);
		fixed_t new_y = old_y + FixedMul(cur_y - old_y, amount);

		sides[sidenum].textureoffset = new_x;
		sides[sidenum].rowoffset = new_y;
	}
}

void R_InterpolateSkies(fixed_t amount)
{
	// sky interpolation
	fixed_t old_sky1offset = prev_sky1offset;
	fixed_t old_sky2offset = prev_sky2offset;

	fixed_t cur_sky1offset = sky1columnoffset;
	fixed_t cur_sky2offset = sky2columnoffset;

	fixed_t new_sky1offset =
	    old_sky1offset + FixedMul(cur_sky1offset - old_sky1offset, amount);
	fixed_t new_sky2offset =
	    old_sky2offset + FixedMul(cur_sky2offset - old_sky2offset, amount);

	saved_sky1offset = cur_sky1offset;
	saved_sky2offset = cur_sky2offset;

	sky1columnoffset = new_sky1offset;
	sky2columnoffset = new_sky2offset;
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
	saved_sectorceilingscrollingflat.clear();
	saved_sectorfloorscrollingflat.clear();
	saved_linescrollingtex.clear();
	saved_sky1offset = 0;
	saved_sky2offset = 0;

	if (gamestate == GS_LEVEL)
	{
		R_InterpolateCeilings(amount);

		R_InterpolateFloors(amount);

		R_InterpolateWalls(amount);

		R_InterpolateSkies(amount);
	}
}

//
// R_BeginInterpolateAutomap
// Starts the interpolation tic for automap if automap is active.
//
void R_BeginInterpolateAutomap(fixed_t amount)
{
	saved_amx = 0;
	saved_amy = 0;
	saved_amangle = 0;

	player_t& player = displayplayer();

	fixed_t old_amx = player.camera->prevx;
	fixed_t old_amy = player.camera->prevy;
	fixed_t old_amangle = player.camera->prevangle;

	fixed_t cur_amx = player.camera->x;
	fixed_t cur_amy = player.camera->y;
	fixed_t cur_amangle = player.camera->angle;

	fixed_t new_amx = old_amx + FixedMul(cur_amx - old_amx, amount);
	fixed_t new_amy = old_amy + FixedMul(cur_amy - old_amy, amount);
	fixed_t new_amangle = old_amangle + FixedMul(cur_amangle - old_amangle, amount);

	saved_amx = cur_amx;
	saved_amy = cur_amy;
	saved_amangle = cur_amangle;

	amx = new_amx;
	amy = new_amy;
	amangle = new_amangle;
}

//
// Functions to assist in the restoration of state after the gametic has ended.

void R_RestoreCeilings(void)
{
	// Ceiling heights
	for (std::vector<fixed_uint_pair>::const_iterator ceiling_it = saved_ceilingheight.begin();
		 ceiling_it != saved_ceilingheight.end(); ++ceiling_it)
	{
		sector_t* sector = &sectors[ceiling_it->second];
		P_SetCeilingHeight(sector, ceiling_it->first);
	}

	// Ceiling scrolling flats
	for (std::vector<fixed_fixed_uint_pair>::const_iterator ceilingscroll_it = saved_sectorceilingscrollingflat.begin();
		 ceilingscroll_it != saved_sectorceilingscrollingflat.end(); ++ceilingscroll_it)
	{
		sector_t* sector = &sectors[ceilingscroll_it->second];

		fixed_uint_pair offs = ceilingscroll_it->first;

		sector->ceiling_xoffs = offs.first;
		sector->ceiling_yoffs = offs.second;
	}
}

void R_RestoreFloors(void)
{
		// Floor heights
		for (std::vector<fixed_uint_pair>::const_iterator floor_it = saved_floorheight.begin();
			 floor_it != saved_floorheight.end(); ++floor_it)
		{
			sector_t* sector = &sectors[floor_it->second];
			P_SetFloorHeight(sector, floor_it->first);
		}

		// Floor scrolling flats
		for (std::vector<fixed_fixed_uint_pair>::const_iterator floorscroll_it =	saved_sectorfloorscrollingflat.begin();
			 floorscroll_it != saved_sectorfloorscrollingflat.end(); ++floorscroll_it)
		{
			sector_t* sector = &sectors[floorscroll_it->second];

			fixed_uint_pair offs = floorscroll_it->first;

			sector->floor_xoffs = offs.first;
			sector->floor_yoffs = offs.second;
		}
}

void R_RestoreWalls(void)
{
	// Scrolling textures
	for (std::vector<fixed_fixed_uint_pair>::const_iterator side_it = saved_linescrollingtex.begin();
		 side_it != saved_linescrollingtex.end(); ++side_it)
	{
		unsigned int sidenum = side_it->second;
		const side_t* side = &sides[sidenum];

		fixed_uint_pair offs = side_it->first;

		sides[sidenum].textureoffset = offs.first;
		sides[sidenum].rowoffset = offs.second;
	}
}

void R_RestoreSkies(void)
{
	sky1columnoffset = saved_sky1offset;
	sky2columnoffset = saved_sky2offset;
}

//
// R_EndInterpolation
//
// Restores the saved height of all moving planes and position of scrolling
// textures. This should be called at the end of every frame rendered.
//
void R_EndInterpolation()
{
	if (gamestate == GS_LEVEL)
	{
		R_RestoreCeilings();

		R_RestoreFloors();

		R_RestoreWalls();

		R_RestoreSkies();
	}
}

//
// R_EndAutomapInterpolation
//
// Restores the saved location automap location at the end of the frame.
//
void R_EndAutomapInterpolation(void)
{
	if (gamestate == GS_LEVEL)
	{
		amx = saved_amx;
		amy = saved_amy;
		amangle = saved_amangle;
	}
}

//
// R_InterpolateCamera
//
// Interpolate between the current position and the previous position
// of the camera. If not using uncapped framerate / interpolation,
// render_lerp_amount will be FRACUNIT.
//

void R_InterpolateCamera(fixed_t amount, bool use_localview, bool chasecam)
{
	if (gamestate == GS_LEVEL && camera)
	{
		fixed_t x = camera->x;
		fixed_t y = camera->y;
		fixed_t z = camera->z;

		if (chasecam)
		{
			// [RH] Use chasecam view
			P_AimCamera(camera);
			x = CameraX;
			y = CameraY;
			z = CameraZ;
		}

		if (use_localview && !::localview.skipangle)
		{
			viewangle = camera->angle + ::localview.angle;
		}
		else
		{
			// Only interpolate if we are spectating
			// interpolate amount/FRACUNIT percent between previous value and current value
			viewangle = camera->prevangle + FixedMul(amount, camera->angle - camera->prevangle);
		}

		viewx = camera->prevx + FixedMul(amount, x - camera->prevx);
		viewy = camera->prevy + FixedMul(amount, y - camera->prevy);

		if (camera->player)
			viewz = camera->player->prevviewz + FixedMul(amount, camera->player->viewz - camera->player->prevviewz);
		else
			viewz = camera->prevz + FixedMul(amount, z - camera->prevz);
	}
}

VERSION_CONTROL (r_interp_cpp, "$Id$")
