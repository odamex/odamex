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

#include "odamex.h"

#include "p_localhistory.h"

#include "doomstat.h"
#include "p_local.h"

#include <algorithm>

LocalSectorHistory& LocalSectorHistory::getInstance()
{
	static LocalSectorHistory instance;
	return instance;
}

bool LocalSectorHistory::enabled()
{
	return clientside and not serverside;
}

void LocalSectorHistory::watch(sector_t* sector)
{
	if (not sector or not enabled())
		return;

	for (const auto& record : m_records)
	{
		if (record.sector == sector)
			return;
	}

	PlaneRecord& record = m_records.emplace_back();
	record.sector = sector;
	std::ranges::fill(record.tics, -1);
}

bool LocalSectorHistory::watching(const sector_t* sector) const
{
	if (not sector)
		return false;

	return std::ranges::any_of(m_records,
	                           [sector](const PlaneRecord& record)
	                           { return record.sector == sector; });
}

void LocalSectorHistory::clear()
{
	m_records.clear();
	m_replaying = false;
}

void LocalSectorHistory::record(int tic)
{
	// A replay installs old heights to check mispredictions --
	// they must never be mistaken for what actually happened
	// on this tic.
	if (m_replaying or tic < 0)
		return;

	const size_t slot = static_cast<size_t>(tic) % MAX_HISTORY_TICS;

	for (auto& record : m_records)
	{
		record.tics[slot] = tic;
		record.floorheights[slot] = P_FloorHeight(record.sector);
		record.ceilingheights[slot] = P_CeilingHeight(record.sector);
	}
}

void LocalSectorHistory::beginReplay()
{
	if (m_records.empty())
		return;

	for (auto& record : m_records)
	{
		record.liveFloor = P_FloorHeight(record.sector);
		record.liveCeiling = P_CeilingHeight(record.sector);
	}

	m_replaying = true;
}

void LocalSectorHistory::restore(int tic)
{
	if (not m_replaying or tic < 0)
		return;

	const size_t slot = static_cast<size_t>(tic) % MAX_HISTORY_TICS;

	for (auto& record : m_records)
	{
		// Nothing was recorded for that tic, so leave the plane alone rather than
		// move it to wherever a much older tic happened to leave it.
		if (record.tics[slot] != tic)
			continue;

		P_SetFloorHeight(record.sector, record.floorheights[slot]);
		P_SetCeilingHeight(record.sector, record.ceilingheights[slot]);

		// Moving the plane is not enough: a thing caches floorz, and only
		// P_CheckPosition refreshes it - which P_ZMovement is never reached.
		// A player standing still would keep the stale value and the replay would
		// not follow the plane at all.
		// No crunch, just refresh the sector planes.
		P_ChangeSector(record.sector, false);
	}
}

void LocalSectorHistory::endReplay()
{
	if (not m_replaying)
		return;

	for (auto& record : m_records)
	{
		P_SetFloorHeight(record.sector, record.liveFloor);
		P_SetCeilingHeight(record.sector, record.liveCeiling);
		P_ChangeSector(record.sector, false);
	}

	m_replaying = false;
}
