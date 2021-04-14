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
//	Moving object handling. Spawn functions.
//
//-----------------------------------------------------------------------------

#include "m_random.h"
#include "doomdef.h"
#include "p_local.h"
#include "s_sound.h"
#include "doomstat.h"
#include "doomtype.h"
#include "v_video.h"
#include "c_cvars.h"
#include "c_effect.h"
#include "m_vectors.h"
#include "p_mobj.h"
#include "st_stuff.h"
#include "p_acs.h"
#include "p_ctf.h"
#include "g_gametype.h"
#include "g_spawninv.h"

EXTERN_CVAR(sv_nomonsters)
EXTERN_CVAR(cl_showspawns)
EXTERN_CVAR(chasedemo)

void G_PlayerReborn(player_t &player);

//

//
// P_SpawnPlayer
// Called when a player is spawned on the level.
// Most of the player structure stays unchanged
//	between levels.
//
void P_SpawnPlayer(player_t& player, mapthing2_t* mthing)
{
	// denis - clients should not control spawning
	if (!serverside)
		return;

	// [RH] Things 4001-? are also multiplayer starts. Just like 1-4.
	//		To make things simpler, figure out which player is being
	//		spawned here.

	// not playing?
	if (!player.ingame())
		return;

	byte playerstate = player.playerstate;

	if (player.doreborn) {
		G_PlayerReborn(player);
		player.doreborn = false;
	}

	AActor* mobj;
//	if (player.deadspectator && player.mo)
//		mobj = new AActor(player.mo->x, player.mo->y, ONFLOORZ, MT_PLAYER);
//	else
//		mobj = new AActor(mthing->x << FRACBITS, mthing->y << FRACBITS, ONFLOORZ, MT_PLAYER);
	mobj = new AActor(mthing->x << FRACBITS, mthing->y << FRACBITS, ONFLOORZ, MT_PLAYER);

	// set color translations for player sprites
	// [RH] Different now: MF_TRANSLATION is not used.
	mobj->translation = translationref_t(translationtables + 256 * player.id, player.id);
	if (player.id == consoleplayer_id)
	{
		// NOTE(jsd): Copy the player setup menu's translation to the player_id's:
		// [SL] don't screw with vanilla demo player colors
		if (!demoplayback)
			R_CopyTranslationRGB(0, player.id);
	}

//	if (player.deadspectator && player.mo)
//	{
//		mobj->angle = player.mo->angle;
//		mobj->pitch = player.mo->pitch;
//	}
//	else
//	{
//		mobj->angle = ANG45 * (mthing->angle/45);
//		mobj->pitch = 0;
//	}
	mobj->angle = ANG45 * (mthing->angle/45);
	mobj->pitch = 0;

	mobj->player = &player;
	mobj->health = player.health;

	player.fov = 90.0f;
	player.mo = player.camera = mobj->ptr();
	player.playerstate = PST_LIVE;
	player.refire = 0;
	player.damagecount = 0;
	player.bonuscount = 0;
	player.extralight = 0;
	player.fixedcolormap = 0;
	player.viewheight = VIEWHEIGHT;
	player.xviewshift = 0;
	player.attacker = AActor::AActorPtr();

	consoleplayer().camera = displayplayer().mo;

	// Set up some special spectator stuff
	if (player.spectator)
	{
		mobj->translucency = 0;
		player.mo->flags |= MF_SPECTATOR;
		player.mo->flags2 |= MF2_FLY;
		player.mo->flags &= ~MF_SOLID;
	}

	// [RH] Allow chasecam for demo watching
	if (demoplayback && chasedemo)
		player.cheats = CF_CHASECAM;

	// setup gun psprite
	P_SetupPsprites(&player);

	// give all cards in death match mode
	if (sv_gametype != GM_COOP)
	{
		for (int i = 0; i < NUMCARDS; i++)
			player.cards[i] = true;
	}

	// Give any other between-level inventory.
	if (!player.spectator)
		G_GiveBetweenInventory(player);

	if (consoleplayer().camera == player.mo)
		ST_Start();	// wake up the status bar

	// [RH] If someone is in the way, kill them
	P_TeleportMove(mobj, mobj->x, mobj->y, mobj->z, true);

	// [BC] Do script stuff
	if (serverside && level.behavior)
	{
		if (playerstate == PST_ENTER)
			level.behavior->StartTypedScripts(SCRIPT_Enter, player.mo);
		else if (playerstate == PST_REBORN)
			level.behavior->StartTypedScripts(SCRIPT_Respawn, player.mo);
	}
}

std::vector<AActor*> spawnfountains;

/**
 * Show spawn points as particle fountains
 * ToDo: Make an independant spawning loop to handle these.
 */
void P_ShowSpawns(mapthing2_t* mthing)
{
	// Ch0wW: DO NOT add new spawns to a DOOM2 demo !
	// It'll immediately desync in DM!
	if (demoplayback)
		return;

	if (clientside && cl_showspawns)
	{
		AActor* spawn = 0;

		if (sv_gametype == GM_DM && mthing->type == 11)
		{
			spawn = new AActor(mthing->x << FRACBITS, mthing->y << FRACBITS, mthing->z << FRACBITS, MT_FOUNTAIN);
			spawn->args[0] = 7; // White
		}

		if (G_IsTeamGame())
		{
			for (int iTeam = 0; iTeam < NUMTEAMS; iTeam++)
			{
				TeamInfo* teamInfo = GetTeamInfo((team_t)iTeam);
				if (teamInfo->TeamSpawnThingNum == mthing->type)
				{
					spawn = new AActor(mthing->x << FRACBITS, mthing->y << FRACBITS, mthing->z << FRACBITS, MT_FOUNTAIN);
					spawn->args[0] = teamInfo->FountainColorArg;
					break;
				}
			}
		}

		if (spawn) {
			spawn->effects = spawn->args[0] << FX_FOUNTAINSHIFT;
		}
	}
}

VERSION_CONTROL (cl_mobj_cpp, "$Id$")
