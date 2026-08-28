// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	G_GAME, serverside.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

BEGIN_DISABLE_WARNING_GNU("-Wold-style-cast")
#include "minilzo.h"
END_DISABLE_WARNING_GNU
#include "d_netinf.h"
#include "z_zone.h"
#include "m_misc.h"
#include "m_random.h"
#include "i_system.h"
#include "p_tick.h"
#include "c_dispatch.h"
#include "gi.h"
#include "p_local.h"
#include "s_sound.h"
#include "r_data.h"
#include "g_game.h"
#include "sv_main.h"
#include "g_spawninv.h"
#include "g_spree.h"
#include "g_multikill.h"
#include "g_deathspot.h"

void	G_PlayerReborn (player_t &player);

void	G_DoNewGame (void);
void	G_DoCompleted (void);
void	G_DoWorldDone (void);

EXTERN_CVAR (sv_maxplayers)
EXTERN_CVAR (sv_timelimit)
EXTERN_CVAR (sv_keepkeys)
EXTERN_CVAR (sv_sharekeys)
EXTERN_CVAR (co_nosilentspawns)
EXTERN_CVAR (sv_fastmonsters)
EXTERN_CVAR (sv_freelook)
EXTERN_CVAR (sv_teamsinplay)

gamestate_t 	gamestate = GS_STARTUP;

bool 			sendpause;				// send a pause event next tic

player_t		nullplayer;				// The null player

int 			gametic;

FILE			*recorddemo_fp;			// Ch0wW : Keeping this for future serverside demo-recording.
int				demostartgametic;		// FIXME : remove this serverside !

wbstartstruct_t wminfo; 				// parms for world map / intermission

player_t		&consoleplayer()
{
	return idplayer(consoleplayer_id);
}

player_t		&displayplayer()
{
	return idplayer(displayplayer_id);
}

BEGIN_COMMAND (pause)
{
	sendpause = true;
}
END_COMMAND (pause)

//
// G_Ticker
// Make ticcmd_ts for the players.
//
int mapchange;

void G_Ticker (void)
{
	// do player reborns if needed
	if (serverside)
	{
		for (auto& player : players)
			if (player.ingame() && (player.playerstate == PST_REBORN || player.playerstate == PST_ENTER))
				G_DoReborn(player);
	}

	// do things to change the game state
	while (gameaction != ga_nothing)
	{
		switch (gameaction)
		{
		// Useless ones from client ? Kick them out.
		case ga_loadgame:
		case ga_savegame:
		case ga_playdemo:
		case ga_screenshot:
		case ga_fullconsole:
		case ga_victory:
			gameaction = ga_nothing;
			break;

		case ga_loadlevel:
			G_DoLoadLevel (-1);
			break;
		case ga_fullresetlevel:
			G_DoResetLevel(true);
			break;
		case ga_resetlevel:
			G_DoResetLevel(false);
			break;
		case ga_newgame:
			G_DoNewGame ();
			break;
		case ga_completed:
			G_DoCompleted ();
			break;
		case ga_worlddone:
			break;
		case ga_nothing:
			break;
		}
	}

	// do main actions
	switch (gamestate)
	{
	case GS_LEVEL:
		P_Ticker ();
		break;

	case GS_INTERMISSION:
	{
		mapchange--; // denis - todo - check if all players are ready, proceed immediately
		if (!mapchange)
		{
			G_ChangeMap();
		}
		// Doom episodes 1-4 end with no intermission, but in
		// multiplayer games we still want to pause on the ending
		// screen.
		else if (level.flags & LEVEL_NOINTERMISSION && strnicmp(level.nextmap.c_str(), "EndGame", 7) != 0)
		{
			G_ChangeMap();
		}
		break;
	}
	break;

	default:
		break;
	}
}


//
// PLAYER STRUCTURE FUNCTIONS
// also see P_SpawnPlayer in P_Mobj
//

//
// G_PlayerFinishLevel
// Call when a player completes a level.
//
void G_PlayerFinishLevel (player_t &player)
{
	player.powers.fill(0);
	player.cards.fill(false);

	SpreeManager::getInstance().erasePoints(player.id);
	MultiKillManager::getInstance().eraseMultiKills(player.id);

	if(player.mo)
		player.mo->flags &= ~MF_SHADOW; 	// cancel invisibility

	player.extralight = 0;					// cancel gun flashes
	player.fixedcolormap = 0;				// cancel ir goggles
	player.damagecount = 0; 				// no palette changes
	player.bonuscount = 0;
}

void SV_SendPlayerInfo(player_t& player);

//
// G_PlayerReborn
// Called after a player dies
// almost everything is cleared and initialized
//
void G_PlayerReborn (player_t &p) // [Toke - todo] clean this function
{
	size_t i;
	for (i = 0; i < NUMAMMO; i++)
	{
		p.maxammo[i] = maxammo[i];
		p.ammo[i] = 0;
	}
	for (i = 0; i < NUMWEAPONS; i++)
		p.weaponowned[i] = false;
	if (!sv_keepkeys && !sv_sharekeys)
	{
		for (i = 0; i < NUMCARDS; i++)
			p.cards[i] = false;
	}

	// That said, if keys are found between a player's death and respawn, resync them.
	if (sv_sharekeys)
	{
		for (i = 0; i < NUMCARDS; i++)
			p.cards[i] = keysfound[i];
	}

	for (i = 0; i < NUMPOWERS; i++)
		p.powers[i] = false;
	for (i = 0; i < NUMTEAMS; i++)
		p.flags[i] = false;
	p.backpack = false;

	G_GiveSpawnInventory(p);

	p.usedown = p.attackdown = true;	// don't do anything immediately
	p.playerstate = PST_LIVE;
	p.doreborn = false;
	p.weaponowned[wp_none] = true;

	if (!p.spectator)
		p.cheats = 0; // Reset cheat flags

	p.death_time = 0;
	DeathSpotManager::getInstance().eraseDeathSpot(p.id);
}

//
// G_SpawnSpotFog
//
// Puts the teleport fog and its sound on a spawn spot.
//
void G_SpawnSpotFog(player_t& player, const fixed_t x, const fixed_t y,
                    const fixed_t z, const angle_t angle, const bool mapthingangle)
{
	unsigned			an;
	fixed_t 			xa,ya;

	// ONLY IF THEY ARE NOT A SPECTATOR
	if (player.spectator)
		return;

	// emulate out-of-bounds access to finecosine / finesine tables
	// which cause west-facing player spawns to have the spawn-fog
	// and its sound located off the map in vanilla Doom.

	// borrowed from Eternity Engine

	// haleyjd: There was a weird bug with this statement:
	//
	// an = (ANG45 * (mthing->angle/45)) >> ANGLETOFINESHIFT;
	//
	// Even though this code stores the result into an unsigned variable, most
	// compilers seem to ignore that fact in the optimizer and use the resulting
	// value directly in a lea instruction. This causes the signed mapthing_t
	// angle value to generate an out-of-bounds access into the fine trig
	// lookups. In vanilla, this accesses the finetangent table and other parts
	// of the finesine table, and the result is what I call the "ninja spawn,"
	// which is missing the fog and sound, as it spawns somewhere out in the
	// far reaches of the void.

	// An arbitrary angle has no vanilla behaviour to preserve.
	if (co_nosilentspawns || !mapthingangle)
	{
		an = angle >> ANGLETOFINESHIFT;
		xa = finecosine[an];
		ya = finesine[an];
	}
	else
	{
		angle_t mtangle = angle / ANG45;

		an = ANG45 * mtangle;

		switch(mtangle)
		{
			case 4: // 180 degrees (0x80000000 >> 19 == -4096)
				xa = finetangent[2048];
				ya = finetangent[0];
				break;
			case 5: // 225 degrees (0xA0000000 >> 19 == -3072)
				xa = finetangent[3072];
				ya = finetangent[1024];
				break;
			case 6: // 270 degrees (0xC0000000 >> 19 == -2048)
				xa = finesine[0];
				ya = finetangent[2048];
				break;
			case 7: // 315 degrees (0xE0000000 >> 19 == -1024)
				xa = finesine[1024];
				ya = finetangent[3072];
				break;
			default: // everything else works properly
				xa = finecosine[an >> ANGLETOFINESHIFT];
				ya = finesine[an >> ANGLETOFINESHIFT];
				break;
		}
	}

	AActor* mo =
	    new AActor(x + 20 * xa, y + 20 * ya, z + INT2FIXED(gameinfo.telefogHeight), MT_TFOG);

	// send new object
	SV_SpawnMobj(mo);
}

//
// G_CheckSpot
// Returns false if the player cannot be respawned
// at the given x/y/z spot
// because something is occupying it
//
void P_SpawnPlayer (player_t &player, const mapthing2_t& mthing);
void P_SpawnPlayer (player_t &player, fixed_t x, fixed_t y, fixed_t startz, angle_t angle);

bool G_CheckSpot (player_t &player, fixed_t x, fixed_t y, fixed_t startz, angle_t angle);
bool G_CheckSpot (player_t &player, const mapthing2_t& mthing)
{
	return G_CheckSpot(player, mthing.x << FRACBITS, mthing.y << FRACBITS,
	                   mthing.z << FRACBITS, ANG45 * (mthing.angle / 45));
}

bool G_CheckSpot (player_t &player, fixed_t x, fixed_t y, fixed_t startz, angle_t angle)
{
	fixed_t z = P_FloorHeight(x, y);

	if (level.flags & LEVEL_USEPLAYERSTARTZ)
		z = startz;

	if (!player.mo)
	{
		// first spawn of level, before corpses
		for (Players::iterator it = players.begin();it != players.end();++it)
		{
			if (&player == &*it)
				continue;

			if (it->mo && it->mo->x == x && it->mo->y == y)
				return false;
		}

		return !P_AvatarBlocksSpot(x, y, z);
	}

	fixed_t oldz = player.mo->z;	// [RH] Need to save corpse's z-height
	player.mo->z = z;		// [RH] Checks are now full 3-D

	// killough 4/2/98: fix bug where P_CheckPosition() uses a non-solid
	// corpse to detect collisions with other players in DM starts
	//
	// Old code:
	// if (!P_CheckPosition (players[playernum].mo, x, y))
	//    return false;

	player.mo->flags |=  MF_SOLID;
	bool valid_position = P_CheckPosition(player.mo, x, y);
	player.mo->flags &= ~MF_SOLID;
	player.mo->z = oldz;	// [RH] Restore corpse's height
	if (!valid_position)
		return false;

	G_SpawnSpotFog(player, x, y, z, angle, true);

	return true;
}


//
// G_DeathMatchSpawnPlayer
// Spawns a player at one of the random death match spots
// called at level load and each death
//

// [RH] Returns the distance of the closest player to the given mapthing2_t.
// denis - todo - should this be used somewhere?
// [Russell] This code is horrible because it does no position checking, even
// zdoom 2.x still has it!
static fixed_t PlayersRangeFromSpot(const mapthing2_t& spot)
{
	fixed_t closest = limits::MAXFIXED;

	for (const auto& player : players)
	{
		if (!player.ingame() || !player.mo || player.health <= 0)
			continue;

		const fixed_t distance = P_AproxDistance (player.mo->x - spot.x * FRACUNIT,
		                         player.mo->y - spot.y * FRACUNIT);

		if (distance < closest)
			closest = distance;
	}

	return closest;
}

// [RH] Select the deathmatch spawn spot farthest from everyone.
static mapthing2_t *SelectFarthestDeathmatchSpot (int selections)
{
	fixed_t bestdistance = 0;
	mapthing2_t* bestspot = nullptr;

	for (int i = 0; i < selections; i++)
	{
		fixed_t distance = PlayersRangeFromSpot(DeathMatchStarts[i]);

		if (distance > bestdistance)
		{
			bestdistance = distance;
			bestspot = &DeathMatchStarts[i];
		}
	}

	return bestspot;
}

// [RH] Select a deathmatch spawn spot at random (original mechanism)
static mapthing2_t *SelectRandomDeathmatchSpot (player_t &player, int selections)
{
	int i = 0, j;

	for (j = 0; j < 20; j++)
	{
		i = P_Random () % selections;
		if (G_CheckSpot (player, DeathMatchStarts[i]) )
		{
			return &DeathMatchStarts[i];
		}
	}

	// [RH] return a spot anyway, since we allow telefragging when a player spawns
	return &DeathMatchStarts[i];
}

static mapthing2_t* SelectTeamSpot(player_t &player, std::vector<mapthing2_t>& starts, int selections)
{
	for (size_t j = 0; j < starts.size(); ++j)
	{
		size_t i = M_Random() % selections;
		if (G_CheckSpot(player, starts[i]))
			return &starts[i];
	}
	return &starts[0];		// could not find a free spot, use spot 0
}

// [Toke] Randomly selects a team spawn point
// [AM] Moved out of CTF gametype and cleaned up.
static mapthing2_t *SelectRandomTeamSpot(player_t &player, int selections)
{
	if (player.userinfo.team < NUMTEAMS)
		return SelectTeamSpot(player, GetTeamInfo(player.userinfo.team)->Starts, selections);

	return SelectRandomDeathmatchSpot(player, selections);
}

void G_TeamSpawnPlayer(player_t &player) // [Toke - CTF - starts] Modified this function to accept teamplay starts
{
	int selections;
	mapthing2_t *spot = NULL;

	selections = 0;

	// [Toke - CTF - starts]
	if (player.userinfo.team < sv_teamsinplay)
		selections = GetTeamInfo(player.userinfo.team)->Starts.size();

	// denis - fall back to deathmatch spawnpoints, if no team ones available
	if (selections < 1)
	{
		selections = DeathMatchStarts.size();

		if (selections)
		{
			spot = SelectRandomDeathmatchSpot(player, selections);
		}
	}
	else
	{
		spot = SelectRandomTeamSpot(player, selections);  // [Toke - Teams]
	}

	if (selections < 1)
		I_Error("No appropriate team starts");

	const mapthing2_t* spawnspot = spot;

	if (!spot && !playerstarts.empty())
		spawnspot = &P_GetPlayerStart(player.id - 1);
	else
	{
		if (player.id < 4)
			spot->type = player.id+1;
		else
			spot->type = player.id+4001-4;
	}

	P_SpawnPlayer(player, *spawnspot);
}

EXTERN_CVAR (sv_dmfarspawn)

void G_DeathMatchSpawnPlayer(player_t &player)
{
	mapthing2_t *spot;

	if(G_UsesCoopSpawns())
		return;

	if(G_IsTeamGame())
	{
		G_TeamSpawnPlayer(player);
		return;
	}

	const int selections = DeathMatchStarts.size();
	// [RH] We can get by with just 1 deathmatch start
	if (selections < 1)
		I_Error("No deathmatch starts");

	// [Toke - dmflags] Old location of DF_SPAWN_FARTHEST
	// [Russell] - Readded, makes modern dm more interesting
	// NOTE - Might also be useful for other game modes
	if ((sv_dmfarspawn) && player.mo)
		spot = SelectFarthestDeathmatchSpot(selections);
	else
		spot = SelectRandomDeathmatchSpot (player, selections);

	const mapthing2_t* spawnspot = spot;

	if (!spot && !playerstarts.empty())
	{
		// no good spot, so the player will probably get stuck
		spawnspot = &P_GetPlayerStart(player.id - 1);
	}
	else
	{
		if (player.id < 4)
			spot->type = player.id+1;
		else
			spot->type = player.id+4001-4;	// [RH] > 4 players
	}

	P_SpawnPlayer (player, *spawnspot);
}

EXTERN_CVAR (g_spawnatdeathspot)

//
// G_DeathSpotSpawnPlayer
//
// Puts the player back on the spot where they fell.
//
// The verdict is passed in because it has to be taken before the corpse is
// disassociated.
//
// Returns false if we determine through rules that the spawn should be blocked.
//
bool G_DeathSpotSpawnPlayer(player_t &player, const deathSpotBlock_t deathspot)
{
	if (!g_spawnatdeathspot)
		return false;

	if (player.playerstate != PST_REBORN)
		return false;

	// Anything standing on the spot that we are not allowed to stomp.
	if (deathspot != DEATHSPOT_CLEAR)
		return false;

	const DeathSpot_s spot = DeathSpotManager::getInstance().getDeathSpot(player.id);
	const fixed_t z = (level.flags & LEVEL_USEPLAYERSTARTZ)
	                      ? spot.z
	                      : P_FloorHeight(spot.x, spot.y);


	G_SpawnSpotFog(player, spot.x, spot.y, z, spot.angle, false);

	P_SpawnPlayer(player, spot.x, spot.y, spot.z, spot.angle);

	G_StompDeathSpot(player, spot);
	return true;
}

//
// G_DoReborn
//
void G_DoReborn (player_t &player)
{
	if(!serverside)
		return;

	const deathSpotBlock_t deathspot = G_CheckDeathSpot(player);

	// respawn at the start
	// first disassociate the corpse
	if (player.mo)
		player.mo->player = NULL;

	// unless they want to respawn where they died
	if (G_DeathSpotSpawnPlayer(player, deathspot))
		return;

	// spawn at random team spot if in team game
	if(G_IsTeamGame())
	{
		G_TeamSpawnPlayer (player);
		return;
	}

	// spawn at random spot if in death match
	if (!G_UsesCoopSpawns())
	{
		G_DeathMatchSpawnPlayer (player);
		return;
	}

	if(playerstarts.empty())
		I_Error("No player starts");

	const mapthing2_t& start = P_GetPlayerStart(player.id - 1);

	if (G_CheckSpot(player, start) )
	{
		P_SpawnPlayer(player, start);
		return;
	}

	// try to spawn at one of the other players' spots
	for (auto& playerstart : playerstarts)
	{
		if (G_CheckSpot(player, playerstart) )
		{
			P_SpawnPlayer(player, playerstart);
			return;
		}
	}

	// he's going to be inside something.  Too bad.
	P_SpawnPlayer(player, start);
}

VERSION_CONTROL (g_game_cpp, "$Id$")
