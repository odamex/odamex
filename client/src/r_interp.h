// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2024 by The Odamex Team.
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

#pragma once

#include "m_fixed.h"
#include "r_defs.h"
#include <map>

typedef std::pair<fixed_t, unsigned int> fixed_uint_pair;
typedef std::pair<std::pair<fixed_t, fixed_t>, unsigned int> fixed_fixed_uint_pair;

class OInterpolation
{
public:
	~OInterpolation();
	static OInterpolation& getInstance(); // returns the instantiated OInterpolation object
	bool enabled() { return interpolationEnabled; }; // Interpolate game stuff at this moment?
	// Tickers (for gamesim interpolation)
	void resetGameInterpolation(); // R_ResetInterpolation() // called when starting/resetting a level
	void resetBobInterpolation(); // Reset bob after the player dies
	void beginGameInterpolation(fixed_t amount); // R_BeginInterpolation(fixed_t amount)
	void ticGameInterpolation(); // R_InterpolationTicker()
	void endGameInterpolation(); // R_EndInterpolation()
	// --- end game objects ---
	void beginConsoleInterpolation(fixed_t amount);
	fixed_t getInterpolatedConsoleBottom(fixed_t amount);
	// View interpolation
	void interpolateView(player_t* player, fixed_t amount);
	// State 
	void disable(); // enable gamesim interpolation
	void enable();  // disable gamesim interpolation

private:
	// private helper functions
	void interpolateSkies(fixed_t amount);
	void interpolateCeilings(fixed_t amount);
	void interpolateFloors(fixed_t amount);
	void interpolateWalls(fixed_t amount);
	void interpolateBob(fixed_t amount);

	void restoreSkies();
	void restoreCeilings();
	void restoreFloors();
	void restoreWalls();
	void restoreBob();

	// camera interpolation
	void interpolateCamera(fixed_t amount, bool use_localview, bool chasecam);

	// private interpolation state variables
	// Sector heights
	std::vector<fixed_uint_pair> prev_ceilingheight;
	std::vector<fixed_uint_pair> saved_ceilingheight;
	std::vector<fixed_uint_pair> prev_floorheight;
	std::vector<fixed_uint_pair> saved_floorheight;

	// Line scrolling
	std::vector<fixed_fixed_uint_pair> prev_linescrollingtex; // <x offs, yoffs>, linenum
	std::vector<fixed_fixed_uint_pair> saved_linescrollingtex;

	// Floor/Ceiling scrolling
	std::vector<fixed_fixed_uint_pair> prev_sectorceilingscrollingflat; // <x offs, yoffs>, sectornum
	std::vector<fixed_fixed_uint_pair> saved_sectorceilingscrollingflat;
	std::vector<fixed_fixed_uint_pair> prev_sectorfloorscrollingflat; // <x offs, yoffs>, sectornum
	std::vector<fixed_fixed_uint_pair> saved_sectorfloorscrollingflat;

	// Skies
	fixed_t saved_sky1offset;
	fixed_t prev_sky1offset;
	fixed_t saved_sky2offset;
	fixed_t prev_sky2offset;

	// Weapon bob x/y
	fixed_t saved_bobx;
	fixed_t prev_bobx;
	fixed_t saved_boby;
	fixed_t prev_boby;

	// Console
	fixed_t saved_conbottomstep;
	fixed_t prev_conbottomstep;

	// Chasecam
	fixed_t prev_camerax;
	fixed_t prev_cameray;
	fixed_t prev_cameraz;

	// Should we interpolate in-game objects?
	bool interpolationEnabled;

	OInterpolation();                          // private contsructor (part of Singleton)
	OInterpolation(const OInterpolation& rhs); // private copy constructor
};
