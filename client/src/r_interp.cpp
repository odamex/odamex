// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2025 by The Odamex Team.
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
//   Interpolation system turned into a singleton to keep track of interpolation state.
//   This system works with the frame scheduler to come up with interpolation values
//   for many Doom things, to make the higher framerate make things look smoother.
//   Originally developed by Sean Leonard to handle player camera and sector
//   heights, this has been expanded to sector and floor scrolling, automap,
//   viewsector (as the camera lags behind the view for a few rendering frames)
//   and weapon bob and chasecam. Console rise/fall and wipes are also
//   interpolated but it can't be disabled here, only gamesim things can.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "m_fixed.h"
#include "cl_demo.h"
#include "r_state.h"
#include "r_sky.h"
#include "p_local.h"
#include "r_interp.h"

EXTERN_CVAR(sv_allowmovebob)
EXTERN_CVAR(cl_movebob)

extern NetDemo netdemo;
extern int ConBottomStep;

extern fixed_t bobx;
extern fixed_t boby;

void R_InterpolateSkyDefs(fixed_t amount);
void R_TicSkyDefInterpolation();
void R_RestoreSkyDefs();

//
// OInterpolation::getInstance
//
// Singleton pattern
// Returns a pointer to the only allowable OInterpolation object
//

OInterpolation& OInterpolation::getInstance()
{
	static OInterpolation instance;
	return instance;
}

OInterpolation::~OInterpolation()
{
	OInterpolation::resetGameInterpolation();
}

OInterpolation::OInterpolation()
{
	// Sky 2 -- sky 1 uses skydefs and must be handled differently
	saved_sky2offset = 0;
	prev_sky2offset = 0;

	// Weapon bob x/y
	saved_bobx = 0;
	prev_bobx = 0;
	saved_boby = 0;
	prev_boby = 0;

	// Console
	saved_conbottomstep = 0;
	prev_conbottomstep = 0;

	// Chasecam
	prev_camerax = 0;
	prev_cameray = 0;
	prev_cameraz = 0;

	// Should we interpolate in-game objects?
	interpolationEnabled = true;
}

//
// OInterpolation::resetGameInterpolation()
//
// Clears any saved interpolation related data. This should be called whenever
// a map is loaded.
//
void OInterpolation::resetGameInterpolation()
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
	prev_sky2offset = 0;
	saved_sky2offset = 0;
	prev_bobx = 0;
	prev_boby = 0;
	saved_bobx = 0;
	saved_boby = 0;
	prev_camerax = 0;
	prev_cameray = 0;
	prev_cameraz = 0;
	// don't reset console params here
	::localview.angle = 0;
	::localview.setangle = false;
	::localview.skipangle = false;
	::localview.pitch = 0;
	::localview.setpitch = false;
	::localview.skippitch = false;
}

//
// OInterpolation::resetGameInterpolation()
//
// Clears any saved bob interpolation information
// That isn't needed because our player died
//
void OInterpolation::resetBobInterpolation()
{
	prev_bobx = 0;
	prev_boby = 0;
}

//
// OInterpolation::ticGameInterpolation()
//
// Records the current height of all moving planes and position of scrolling
// textures, which will be used as the previous position during iterpolation.
// This should be called once per gametic.
//
void OInterpolation::ticGameInterpolation()
{
	prev_ceilingheight.clear();
	prev_floorheight.clear();
	prev_linescrollingtex.clear();
	prev_sectorceilingscrollingflat.clear();
	prev_sectorfloorscrollingflat.clear();
	prev_sky2offset = 0;
	prev_camerax = 0;
	prev_cameray = 0;
	prev_cameraz = 0;
	prev_bobx = 0;
	prev_boby = 0;

	R_TicSkyDefInterpolation();

	if (gamestate == GS_LEVEL)
	{
		for (int i = 0; i < numsectors; i++)
		{
			if (sectors[i].ceilingdata)
				prev_ceilingheight.emplace_back(P_CeilingHeight(&sectors[i]), i);
			if (sectors[i].floordata)
				prev_floorheight.emplace_back(P_FloorHeight(&sectors[i]), i);
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
					prev_linescrollingtex.emplace_back(
						std::make_pair(
							sides[wallnum].textureoffset,
							sides[wallnum].rowoffset),
								wallnum);
				}
			}
			else if (P_CeilingScrollType(type))
			{
				prev_sectorceilingscrollingflat.emplace_back(
						std::make_pair(
							sectors[affectee].ceiling_xoffs,
							sectors[affectee].ceiling_yoffs),
								affectee);
			}
			else if (P_FloorScrollType(type))
			{
				prev_sectorfloorscrollingflat.emplace_back(
						std::make_pair(
							sectors[affectee].floor_xoffs,
							sectors[affectee].floor_yoffs),
								affectee);
			}
		}

		// Update sky offsets
		prev_sky2offset = sky2columnoffset;

		// Update bob - this happens once per gametic
		prev_bobx = bobx;
		prev_boby = boby;

		// Update chasecam
		prev_camerax = CameraX;
		prev_cameray = CameraY;
		prev_cameraz = CameraZ;
	}
}

//
// Functions that assist with the interpolation of certain game objects
void OInterpolation::interpolateCeilings(fixed_t amount)
{
	for (const auto [old_value, secnum] : prev_ceilingheight)
	{
		sector_t* sector = &sectors[secnum];

		const fixed_t cur_value = P_CeilingHeight(sector);

		saved_ceilingheight.emplace_back(cur_value, secnum);

		const fixed_t new_value = old_value + FixedMul(cur_value - old_value, amount);
		P_SetCeilingHeight(sector, new_value);
	}

	for (const auto& [offs, secnum] : prev_sectorceilingscrollingflat)
	{
		const sector_t* sector = &sectors[secnum];

		const fixed_t cur_x = sector->ceiling_xoffs;
		const fixed_t cur_y = sector->ceiling_yoffs;

		auto [old_x, old_y] = offs;

		saved_sectorceilingscrollingflat.emplace_back(std::make_pair(cur_x, cur_y), secnum);

		const fixed_t new_x = old_x + FixedMul(cur_x - old_x, amount);
		const fixed_t new_y = old_y + FixedMul(cur_y - old_y, amount);

		sectors[secnum].ceiling_xoffs = new_x;
		sectors[secnum].ceiling_yoffs = new_y;
	}
}

void OInterpolation::interpolateFloors(fixed_t amount)
{
	for (const auto [old_value, secnum] : prev_floorheight)
	{
		sector_t* sector = &sectors[secnum];

		const fixed_t cur_value = P_FloorHeight(sector);

		saved_floorheight.emplace_back(cur_value, secnum);

		const fixed_t new_value = old_value + FixedMul(cur_value - old_value, amount);
		P_SetFloorHeight(sector, new_value);
	}

	for (const auto& [offs, secnum] : prev_sectorfloorscrollingflat)
	{
		const sector_t* sector = &sectors[secnum];

		auto [old_x, old_y] = offs;

		const fixed_t cur_x = sector->floor_xoffs;
		const fixed_t cur_y = sector->floor_yoffs;

		saved_sectorfloorscrollingflat.emplace_back(std::make_pair(cur_x, cur_y), secnum);

		const fixed_t new_x = old_x + FixedMul(cur_x - old_x, amount);
		const fixed_t new_y = old_y + FixedMul(cur_y - old_y, amount);

		sectors[secnum].floor_xoffs = new_x;
		sectors[secnum].floor_yoffs = new_y;
	}
}

void OInterpolation::interpolateWalls(fixed_t amount)
{
	for (const auto& [offs, sidenum] : prev_linescrollingtex)
	{
		const side_t* side = &sides[sidenum];

		auto [old_x, old_y] = offs;

		const fixed_t cur_x = side->textureoffset;
		const fixed_t cur_y = side->rowoffset;

		saved_linescrollingtex.emplace_back(std::make_pair(cur_x, cur_y), sidenum);

		const fixed_t new_x = old_x + FixedMul(cur_x - old_x, amount);
		const fixed_t new_y = old_y + FixedMul(cur_y - old_y, amount);

		sides[sidenum].textureoffset = new_x;
		sides[sidenum].rowoffset = new_y;
	}
}

void OInterpolation::interpolateSkies(fixed_t amount)
{
	// Perform interp for any scrolling skies
	const fixed_t newsky2offset = prev_sky2offset +
	                    FixedMul(amount, sky2columnoffset - prev_sky2offset);

	saved_sky2offset = sky2columnoffset;

	sky2columnoffset = newsky2offset;
}

void OInterpolation::interpolateBob(fixed_t amount)
{
	// Perform interp on weapons bob
	fixed_t newbobx = prev_bobx + FixedMul(amount, bobx - prev_bobx);
	fixed_t newboby = prev_boby + FixedMul(amount, boby - prev_boby);

	saved_bobx = bobx;
	saved_boby = boby;

	bobx = newbobx;
	boby = newboby;
}

//
// R_BeginInterpolation
//
// Saves the current height of all moving planes and position of scrolling
// textures, which will be restored by R_EndInterpolation. The height of a
// moving plane will be interpolated between the previous height and this
// current height. This should be called every time a frame is rendered.
//
void OInterpolation::beginGameInterpolation(fixed_t amount)
{
	saved_ceilingheight.clear();
	saved_floorheight.clear();
	saved_sectorceilingscrollingflat.clear();
	saved_sectorfloorscrollingflat.clear();
	saved_linescrollingtex.clear();
	saved_sky2offset = 0;
	saved_bobx = 0;
	saved_boby = 0;

	R_InterpolateSkyDefs(amount);

	if (gamestate == GS_LEVEL)
	{
		interpolateCeilings(amount);

		interpolateFloors(amount);

		interpolateWalls(amount);

		interpolateSkies(amount);

		interpolateBob(amount);
	}
}

//
// Functions to assist in the restoration of state after the gametic has ended.

void OInterpolation::restoreCeilings(void)
{
	// Ceiling heights
	for (const auto [height, secnum] : saved_ceilingheight)
	{
		sector_t* sector = &sectors[secnum];
		P_SetCeilingHeight(sector, height);
	}

	// Ceiling scrolling flats
	for (const auto& [offs, secnum] : saved_sectorceilingscrollingflat)
	{
		sector_t* sector = &sectors[secnum];

		sector->ceiling_xoffs = offs.first;
		sector->ceiling_yoffs = offs.second;
	}
}

void OInterpolation::restoreFloors(void)
{
	// Floor heights
	for (const auto& [height, secnum] : saved_floorheight)
	{
		sector_t* sector = &sectors[secnum];
		P_SetFloorHeight(sector, height);
	}

	// Floor scrolling flats
	// Ceiling scrolling flats
	for (const auto& [offs, secnum] : saved_sectorfloorscrollingflat)
	{
		sector_t* sector = &sectors[secnum];

		sector->floor_xoffs = offs.first;
		sector->floor_yoffs = offs.second;
	}
}

void OInterpolation::restoreWalls(void)
{
	// Scrolling textures
	for (const auto& [offs, sidenum] : saved_linescrollingtex)
	{
		sides[sidenum].textureoffset = offs.first;
		sides[sidenum].rowoffset = offs.second;
	}
}

void OInterpolation::restoreSkies(void)
{
	// Restore scrolling skies
	sky2columnoffset = saved_sky2offset;
	R_RestoreSkyDefs();
}

void OInterpolation::restoreBob(void)
{
	// Restore weapon bob
	bobx = saved_bobx;
	boby = saved_boby;
}

//
// R_EndInterpolation
//
// Restores the saved height of all moving planes and position of scrolling
// textures. This should be called at the end of every frame rendered.
//
void OInterpolation::endGameInterpolation()
{
	if (gamestate == GS_LEVEL)
	{
		restoreCeilings();

		restoreFloors();

		restoreWalls();

		restoreSkies();

		restoreBob();
	}
}

//
// OInterpolation::interpolateCamera
//
// Interpolate between the current position and the previous position
// of the camera. If not using uncapped framerate / interpolation,
// render_lerp_amount will be FRACUNIT.
//

void OInterpolation::interpolateCamera(fixed_t amount, bool use_localview,
                                         bool chasecam)
{
	if (gamestate == GS_LEVEL && camera)
	{
		fixed_t x = camera->x;
		fixed_t y = camera->y;
		fixed_t z = camera->player ? camera->player->viewz : camera->z;
		fixed_t angle = camera->angle;
		sector_t* vs = camera->subsector->sector;

		if (chasecam)
		{
			// [RH] Use chasecam view
			P_AimCamera(camera);
			x = CameraX;
			y = CameraY;
			z = CameraZ;
			vs = CameraSector;
		}

		if (amount < FRACUNIT && interpolationEnabled)
		{
			if (use_localview && !::localview.skipangle)
			{
				viewangle = camera->angle + ::localview.angle;
			}
			else
			{
				// Only interpolate if we are spectating
				// interpolate amount/FRACUNIT percent between previous value and
				// current value
				viewangle = camera->prevangle +
				            FixedMul(amount, camera->angle - camera->prevangle);
			}

			if (chasecam)
			{
				viewx = prev_camerax + FixedMul(amount, x - prev_camerax);
				viewy = prev_cameray + FixedMul(amount, y - prev_cameray);
				viewz = prev_cameraz + FixedMul(amount, z - prev_cameraz);

				viewsector = R_PointInSubsector(viewx, viewy)->sector;
			}
			else
			{
				viewx = camera->prevx + FixedMul(amount, x - camera->prevx);
				viewy = camera->prevy + FixedMul(amount, y - camera->prevy);

				if (camera->player)
					viewz = camera->player->prevviewz +
					        FixedMul(amount,
					                 camera->player->viewz - camera->player->prevviewz);
				else
					viewz = camera->prevz + FixedMul(amount, z - camera->prevz);

				viewsector = R_PointInSubsector(viewx, viewy)->sector;
			}
		}
		else
		{
			viewx = x;
			viewy = y;
			viewz = z;
			viewangle = angle;
			viewsector = vs;
		}
	}
}

void OInterpolation::interpolateView(player_t* player, fixed_t amount)
{
	camera = player->camera; // [RH] Use camera instead of viewplayer

	if (!camera || !camera->subsector)
		return;

	player_t& consolePlayer = consoleplayer();
	const bool use_localview =
	    (consolePlayer.id == displayplayer().id && consolePlayer.health > 0 &&
	     !consolePlayer.mo->reactiontime && !netdemo.isPlaying() && !demoplayback);

	interpolateCamera(amount, use_localview, player->cheats & CF_CHASECAM);
}

//
// Handle interpolation state
//
void OInterpolation::enable()
{
	interpolationEnabled = true;
}

void OInterpolation::disable()
{
	interpolationEnabled = false;
}

//
// The stuff below runs even if R_RenderPlayerView won't run.
//

//
// Console Interpolation
//

//
// beginConsoleInterpolation()
// Always runs the first gametic while console is active
void OInterpolation::beginConsoleInterpolation(fixed_t amount)
{
	// Perform interp on console rise/drop
	prev_conbottomstep = saved_conbottomstep;

	saved_conbottomstep = ConBottomStep;
}

//
// getInterpolatedConsoleBottom()
// Always runs the first gametic while console is active
fixed_t OInterpolation::getInterpolatedConsoleBottom(fixed_t amount)
{
	// Perform interp on console rise/drop
	return prev_conbottomstep + FixedMul(amount, saved_conbottomstep - prev_conbottomstep);
}

VERSION_CONTROL (r_interp_cpp, "$Id$")
