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
//   Bookkeeping of where players last died on the current level.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "g_deathspot.h"

#include <algorithm>
#include <array>
#include <vector>

#include "c_cvars.h"
#include "d_player.h"
#include "g_level.h"
#include "p_local.h"
#include "p_lnspec.h"
#include "p_mapformat.h"

EXTERN_CVAR(g_spawnatdeathspot)
EXTERN_CVAR(sv_friendlyfire)
EXTERN_CVAR(sv_unblockplayers)
EXTERN_CVAR(sv_unblockfriendly)

DeathSpotManager& DeathSpotManager::getInstance()
{
	static DeathSpotManager instance;
	return instance;
}

DeathSpotManager::DeathSpotManager()
{
	emptySpot = DeathSpot_s();
}

DeathSpotManager::~DeathSpotManager()
{
	reset();
}

void DeathSpotManager::reset()
{
	deathSpotPlayerDict.clear();
}

void DeathSpotManager::setDeathSpot(const int playerid, const fixed_t x, const fixed_t y,
                                    const fixed_t z, const angle_t angle)
{
	deathSpotPlayerDict[playerid] = DeathSpot_s(x, y, z, angle);
}

bool DeathSpotManager::hasDeathSpot(const int playerid) const
{
	return deathSpotPlayerDict.find(playerid) != deathSpotPlayerDict.end();
}

const DeathSpot_s& DeathSpotManager::getDeathSpot(const int playerid) const
{
	const auto it = deathSpotPlayerDict.find(playerid);

	if (it == deathSpotPlayerDict.end())
	{
		return emptySpot;
	}

	return it->second;
}

void DeathSpotManager::eraseDeathSpot(const int playerid)
{
	deathSpotPlayerDict.erase(playerid);
}

void DeathSpotManager::clearDeathSpots()
{
	deathSpotPlayerDict.clear();
}

blockerAction_t G_ClassifyDeathSpotBlocker(const AActor& thing,
                                          const player_t& player)
{
	// Only solid things stand in the way. This is also what keeps the corpse
	// we are about to leave behind from blocking its own spot.
	if (!(thing.flags & MF_SOLID))
		return BLOCKER_IGNORE;

	// Spectators are not really there.
	if (thing.player && thing.player->spectator)
		return BLOCKER_IGNORE;

	// Never trip over ourselves.
	if (thing.player == &player || &thing == player.mo)
		return BLOCKER_IGNORE;

	if (thing.player)
	{
		// Friendly fire only has a say over an actual teammate.
		if (sv_friendlyfire || !P_AreTeammates(*thing.player, player))
			return BLOCKER_STOMP;

		// sv_unblockplayers only lets us share the spot with a teammate - it
		// is not a licence to kill.
		return sv_unblockplayers ? BLOCKER_IGNORE : BLOCKER_BLOCKS;
	}

	// An avatar cannot be stomped
	if (thing.type == MT_AVATAR)
		return BLOCKER_BLOCKS;

	// Anything that cannot think cannot be told to move, and decorations and
	// barrels never wander off on their own.
	if (!sentient(&thing))
		return BLOCKER_BLOCKS;

	// Monsters get stomped, friendly or not. Only the unblock cvar spares them.
	if (thing.flags & MF_FRIEND && P_IsFriendlyThing(&thing, player.mo) &&
	    sv_unblockfriendly)
		return BLOCKER_IGNORE;

	return BLOCKER_STOMP;
}

bool G_IsInstantDeathSector(const sector_t& sec)
{
	// Boom's extended sector types carry the kill in the special itself
	if (sec.special & DEATH_MASK)
		return true;

	// ZDoom maps can either get it thru the sector special or damageamount
	// (set with line specials or scripts)
	if (map_format.getZDoom() && sec.special == Damage_InstantDeath)
		return true;

	return sec.damageamount >= 999;
}

/// <summary>
/// Whether a player sized body fits at a spot, taking into account every line
/// its footprint crosses.
///
/// This is pretty much PIT_CheckLine without any side effects.
/// </summary>
bool SpotHasRoom(const DeathSpot_s& spot, const sector_t& sec, const fixed_t radius,
                 const fixed_t height)
{
	fixed_t floorz = P_FloorHeight(spot.x, spot.y, &sec);
	fixed_t ceilingz = P_CeilingHeight(spot.x, spot.y, &sec);

	std::array<fixed_t, 4> bbox{};
	bbox[BOXTOP] = spot.y + radius;
	bbox[BOXBOTTOM] = spot.y - radius;
	bbox[BOXRIGHT] = spot.x + radius;
	bbox[BOXLEFT] = spot.x - radius;

	const int xl = (bbox[BOXLEFT] - blockmap.originx()) >> MAPBLOCKSHIFT;
	const int xh = (bbox[BOXRIGHT] - blockmap.originx()) >> MAPBLOCKSHIFT;
	const int yl = (bbox[BOXBOTTOM] - blockmap.originy()) >> MAPBLOCKSHIFT;
	const int yh = (bbox[BOXTOP] - blockmap.originy()) >> MAPBLOCKSHIFT;

	for (int bx = xl; bx <= xh; bx++)
	{
		for (int by = yl; by <= yh; by++)
		{
			if (!blockmap.containsCoordinate(bx, by))
				continue;

			for (const int idx : blockmap.list(bx, by))
			{
				const line_t& ld = R_GetLines()[idx];

				if (bbox[BOXRIGHT] <= ld.bbox[BOXLEFT] ||
				    bbox[BOXLEFT] >= ld.bbox[BOXRIGHT] ||
				    bbox[BOXTOP] <= ld.bbox[BOXBOTTOM] ||
				    bbox[BOXBOTTOM] >= ld.bbox[BOXTOP])
				{
					continue;
				}

				if (P_BoxOnLineSide(bbox, &ld) != -1)
					continue;

				// A wall, or a line that turns players away, inside the
				// footprint - there is no standing here at any height.
				if (!ld.backsector ||
				    (ld.flags & (ML_BLOCKING | ML_BLOCKEVERYTHING | ML_BLOCKPLAYERS)))
				{
					return false;
				}

				// A closed door leaves an opening of nothing, which the
				// headroom test below then rejects.
				P_LineOpening(&ld, spot.x, spot.y, spot.x, spot.y);

				if (opentop < ceilingz)
					ceilingz = opentop;

				if (openbottom > floorz)
					floorz = openbottom;
			}
		}
	}

	return ceilingz - floorz >= height;
}

/// <summary>
/// Walks everything overlapping a death spot and classifies it.
/// </summary>
/// <param name="player">Player who wants their spot back.</param>
/// <param name="spot">Spot to look at.</param>
/// <param name="victims"> Filled with the things we are allowed to stomp,
/// deduplicated.
/// Pass nullptr for a dry run/report only.
/// </param>
/// <returns>Whether the spot is usable or blocked.</returns>
deathSpotBlock_t ScanDeathSpot(const player_t& player, const DeathSpot_s& spot,
                               std::vector<AActor*>* victims)
{
	// The body we are testing for is the one about to be spawned, not the
	// corpse, so use the stock player size.
	const fixed_t radius = mobjinfo[MT_PLAYER].radius;
	const fixed_t height = mobjinfo[MT_PLAYER].height;

	// Nothing sane to say about a spot that is not on the map - P_CheckPosition
	// used to be the one guarding against this.
	const subsector_t* subsec = P_PointInSubsector(spot.x, spot.y);

	if (!subsec || !subsec->sector)
		return DEATHSPOT_BLOCKED_NOROOM;

	const sector_t& sec = *subsec->sector;

	// Putting someone back into a floor that kills on contact would only start
	// the same death over again, so it is never worth offering.
	if (G_IsInstantDeathSector(sec))
		return DEATHSPOT_BLOCKED_DEADLY;

	// A crusher, a closing door or a rising floor can leave less room than a
	// player takes up, and a door that shut across the footprint walls it off
	// sideways. Both are moving targets, so the answer is only good for the tic
	// it is asked in.
	if (!SpotHasRoom(spot, sec, radius, height))
		return DEATHSPOT_BLOCKED_NOROOM;

	// Matches the height G_CheckSpot and P_SpawnPlayer will settle on.
	const fixed_t z = (level.flags & LEVEL_USEPLAYERSTARTZ)
	                      ? spot.z
	                      : P_FloorHeight(spot.x, spot.y);

	deathSpotBlock_t blocked = DEATHSPOT_CLEAR;

	auto visit = [&](AActor& thing) -> bool {
		const fixed_t blockdist = thing.radius + radius;

		if (abs(thing.x - spot.x) >= blockdist || abs(thing.y - spot.y) >= blockdist)
			return true; // didn't hit it

		if (P_AllowPassover())
		{
			if (z > thing.z + thing.height)
				return true; // overhead
			if (z + height < thing.z)
				return true; // underneath
		}

		switch (G_ClassifyDeathSpotBlocker(thing, player))
		{
		case BLOCKER_BLOCKS:
			// Something that will never move outranks someone who might, so
			// the prompt does not send the player off waiting for a barrel.
			if (thing.player && blocked != DEATHSPOT_BLOCKED_OBSTACLE)
				blocked = DEATHSPOT_BLOCKED_PLAYER;
			else if (!thing.player)
				blocked = DEATHSPOT_BLOCKED_OBSTACLE;
			break;
		case BLOCKER_STOMP:
			// A thing can be linked into several blockmap cells, so make sure
			// we only line it up for a stomping once.
			if (victims && std::find(victims->begin(), victims->end(), &thing) ==
			                   victims->end())
			{
				victims->push_back(&thing);
			}
			break;
		default:
			break;
		}

		// Keep going -- we want every blocker, not just the first.
		return true;
	};

	const int xl = (spot.x - radius - blockmap.originx() - MAXRADIUS) >> MAPBLOCKSHIFT;
	const int xh = (spot.x + radius - blockmap.originx() + MAXRADIUS) >> MAPBLOCKSHIFT;
	const int yl = (spot.y - radius - blockmap.originy() - MAXRADIUS) >> MAPBLOCKSHIFT;
	const int yh = (spot.y + radius - blockmap.originy() + MAXRADIUS) >> MAPBLOCKSHIFT;

	for (int bx = xl; bx <= xh; bx++)
	{
		for (int by = yl; by <= yh; by++)
		{
			P_BlockThingsIterator(bx, by, visit, nullptr);
		}
	}

	if (G_IsDeathSpotBlocked(blocked))
	{
		// Nothing is going to be stomped after all.
		if (victims)
			victims->clear();

		return blocked;
	}

	return DEATHSPOT_CLEAR;
}

deathSpotBlock_t G_CheckDeathSpot(const player_t& player)
{
	if (!g_spawnatdeathspot)
		return DEATHSPOT_NOSPOT;

	const DeathSpotManager& spots = DeathSpotManager::getInstance();

	if (!spots.hasDeathSpot(player.id))
		return DEATHSPOT_NOSPOT;

	return ScanDeathSpot(player, spots.getDeathSpot(player.id), nullptr);
}

void G_StompDeathSpot(player_t& player, const DeathSpot_s& spot)
{
	if (!player.mo)
		return;

	std::vector<AActor*> victims;

	if (ScanDeathSpot(player, spot, &victims) != DEATHSPOT_CLEAR)
		return;

	// Damage is dealt after the walk, not during it - killing a thing can
	// unlink it from the blockmap we would still be iterating.
	static constexpr int telefrag_damage = 10000;

	for (AActor* victim : victims)
	{
		P_DamageMobj(victim, player.mo, player.mo, telefrag_damage, MOD_TELEFRAG);
	}
}

VERSION_CONTROL (g_deathspot_cpp, "$Id$")
