// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: sv_mobj.cpp 1832 2010-09-01 23:59:33Z mike $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2012 by The Odamex Team.
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

#include "m_alloc.h"
#include "i_system.h"
#include "z_zone.h"
#include "m_random.h"
#include "doomdef.h"
#include "p_local.h"
#include "p_lnspec.h"
#include "s_sound.h"
#include "doomstat.h"
#include "doomtype.h"
#include "v_video.h"
#include "c_cvars.h"
#include "vectors.h"
#include "p_mobj.h"
#include "sv_main.h"
#include "p_ctf.h"
#include "g_game.h"
#include "p_acs.h"

EXTERN_CVAR(sv_nomonsters)
EXTERN_CVAR(sv_maxplayers)

void G_PlayerReborn(player_t &player);
void CTF_RememberFlagPos(mapthing2_t *mthing);

void P_SetSpectatorFlags(player_t &player)
{
	player.spectator = true;

	if (player.mo)
	{
		player.mo->flags |= MF_SPECTATOR;
		player.mo->flags &= ~MF_SOLID;
		player.mo->flags2 |= MF2_FLY;
	}
}

//
// P_SpawnPlayer
// Called when a player is spawned on the level.
// Most of the player structure stays unchanged
//	between levels.
//
void P_SpawnPlayer (player_t &player, mapthing2_t *mthing)
{
	// denis - clients should not control spawning
	if(!serverside)
		return;

	// [RH] Things 4001-? are also multiplayer starts. Just like 1-4.
	//		To make things simpler, figure out which player is being
	//		spawned here.
	player_t *p = &player;

	// not playing?
	if(!p->ingame())
		return;

	if (p->playerstate == PST_REBORN || p->playerstate == PST_ENTER)
	{
		G_PlayerReborn (*p);
		// [AM] If we're "reborn" as a spectator, don't touch the keepinventory
		//      flag, but otherwise turn it off.
		if (!p->spectator)
			p->keepinventory = false;
	}

	AActor *mobj = new AActor (mthing->x << FRACBITS, mthing->y << FRACBITS, ONFLOORZ, MT_PLAYER);

	// set color translations for player sprites
	// [RH] Different now: MF_TRANSLATION is not used.
	//		  mobj->translation = translationtables + 256*playernum;

	mobj->angle = ANG45 * (mthing->angle/45);
	mobj->pitch = mobj->roll = 0;
	mobj->player = p;
	mobj->health = p->health;

	// [RH] Set player sprite based on skin
	if(p->userinfo.skin >= numskins)
		p->userinfo.skin = 0;

	mobj->sprite = skins[p->userinfo.skin].sprite;

	p->fov = 90.0f;
	p->mo = p->camera = mobj->ptr();
	p->playerstate = PST_LIVE;
	p->refire = 0;
	p->damagecount = 0;
	p->bonuscount = 0;
	p->extralight = 0;
	p->fixedcolormap = 0;
	p->viewheight = VIEWHEIGHT;
	p->xviewshift = 0;
	p->attacker = AActor::AActorPtr();

	// Set up some special spectator stuff
	if (p->spectator)
		P_SetSpectatorFlags(player);

	// [RH] Allow chasecam for demo watching
	//if ((demoplayback || demonew) && chasedemo)
	//	p->cheats = CF_CHASECAM;

	// setup gun psprite
	P_SetupPsprites (p);

	// give all cards in death match mode
	if (sv_gametype != GM_COOP)
	{
		for (int i = 0; i < NUMCARDS; i++)
			p->cards[i] = true;
	}

	if(serverside)
	{
		// [RH] If someone is in the way, kill them
		P_TeleportMove (mobj, mobj->x, mobj->y, mobj->z, true);

        // [BC] Do script stuff
        if (level.behavior != NULL)
        {
            if (p->playerstate == PST_ENTER)
            {
                level.behavior->StartTypedScripts (SCRIPT_Enter, p->mo);
            }
            else if (p->playerstate == PST_REBORN)
            {
                level.behavior->StartTypedScripts (SCRIPT_Respawn, p->mo);
            }
        }

		// send new objects
		SV_SpawnMobj(mobj);
	}
}

VERSION_CONTROL (sv_mobj_cpp, "$Id: sv_mobj.cpp 1832 2010-09-01 23:59:33Z mike $")

