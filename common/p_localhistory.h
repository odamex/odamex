// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//   Plane height history for sector movers the client simulates itself.
//
//-----------------------------------------------------------------------------

#pragma once

#include "doomdef.h"
#include "m_fixed.h"
#include "r_defs.h"

#include <array>
#include <vector>

// Plane height history for sector movers the client simulates itself.
//
// A broadcast mover is rewound during prediction by resetting its sector to the
// most recent SVC_MovingSector* snapshot and re-running the thinker once per
// replayed tic.
//
// A mover that is simulated locally rather than broadcast has no
// such snapshot, so the replay would run every past tic against the plane's
// present height - and since the prediction check compares positions for exact
// equality, anything standing on that plane mispredicts on every tic.
//
// Such a mover calls watch() for its sector, the height is recorded once a tic,
// and prediction hands it back for the tic being replayed.
class LocalSectorHistory
{
  public:
	static LocalSectorHistory& getInstance();

	static bool enabled();

	// Starts idempotently tracking a sector.
	void watch(sector_t* sector);

	// Forgets everything. Called when a level is torn down.
	void clear();

	// Is this sector's plane one we simulate locally?
	[[nodiscard]] bool watching(const sector_t* sector) const;

	// Stores the current height of every tracked plane under 'tic'. Called once
	// per tic, after the thinkers have run.
	void record(int tic);

	// Lends the planes to a prediction replay and takes them back afterwards.
	// restore() does nothing unless a replay is in progress.
	void beginReplay();
	void restore(int tic);
	void endReplay();

  private:
	LocalSectorHistory() = default;

	// Has to cover the client's whole prediction window (MAXSAVETICS), which is
	// not visible from here.
	static constexpr size_t MAX_HISTORY_TICS = static_cast<size_t>(2 * TICRATE);

	struct PlaneRecord
	{
		sector_t* sector = nullptr;

		// Which tic each slot holds, so a gap - a tic the client skipped, or a
		// sector watched moments ago - reads as empty rather than stale.
		std::array<int, MAX_HISTORY_TICS> tics{};
		std::array<fixed_t, MAX_HISTORY_TICS> floorheights{};
		std::array<fixed_t, MAX_HISTORY_TICS> ceilingheights{};

		fixed_t liveFloor = 0;
		fixed_t liveCeiling = 0;
	};

	std::vector<PlaneRecord> m_records;
	bool m_replaying = false;
};
