// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2010 by The Odamex Team.
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
#include "cl_main.h"
#include "p_ctf.h"
#include "st_stuff.h"
#include "hu_stuff.h"

extern BOOL demonew;

EXTERN_CVAR(sv_nomonsters)
EXTERN_CVAR(chasedemo)

void G_PlayerReborn(player_t &player);

void AActor::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	if (arc.IsStoring ())
	{
		int playerid = player ? player->id : 0;
		arc << x
			<< y
			<< z
			<< pitch
			<< angle
			<< roll
			<< (int)sprite
			<< frame
			<< effects
			<< floorz
			<< ceilingz
			<< radius
			<< height
			<< momx
			<< momy
			<< momz
			<< (int)type
			<< tics
			<< state
			<< flags
			<< flags2
			<< special1
			<< special2			
			<< health
			<< movedir
			<< visdir
			<< movecount
			/*<< target ? target->netid : 0*/
			/*<< lastenemy ? lastenemy->netid : 0*/
			<< reactiontime
			<< threshold
			<< playerid
			<< lastlook
			/*<< tracer ? tracer->netid : 0*/
			<< tid
            << special
			<< args[0]
			<< args[1]
			<< args[2]
			<< args[3]
			<< args[4]
			/*<< goal ? goal->netid : 0*/
			<< (unsigned)0
			<< translucency
			<< waterlevel;

		if (translation)
			arc << (DWORD)(translation - translationtables);
		else
			arc << (DWORD)0xffffffff;
		spawnpoint.Serialize (arc);
	}
	else
	{
		unsigned dummy;
		unsigned playerid;
		arc >> x
			>> y
			>> z
			>> pitch
			>> angle
			>> roll
			>> (int&)sprite
			>> frame
			>> effects
			>> floorz
			>> ceilingz
			>> radius
			>> height
			>> momx
			>> momy
			>> momz
			>> (int&)type
			>> tics
			>> state
			>> flags
			>> flags2
			>> special1
			>> special2			
			>> health
			>> movedir
			>> visdir
			>> movecount
			/*>> target->netid*/
			/*>> lastenemy->netid*/
			>> reactiontime
			>> threshold
			>> playerid
			>> lastlook
			/*>> tracer->netid*/
			>> tid
			>> special
			>> args[0]
			>> args[1]
			>> args[2]
			>> args[3]
			>> args[4]			
			/*>> goal->netid*/
			>> dummy
			>> translucency
			>> waterlevel;

		DWORD trans;
		arc >> trans;
		if (trans == (DWORD)0xffffffff)
			translation = NULL;
		else
			translation = translationtables + trans;
		spawnpoint.Serialize (arc);
		if(type >= NUMMOBJTYPES)
			I_Error("Unknown object type in saved game");
		if(sprite >= NUMSPRITES)
			I_Error("Unknown sprite in saved game");
		info = &mobjinfo[type];
		touching_sectorlist = NULL;
		LinkToWorld ();
		AddToHash ();
		if(playerid && validplayer(idplayer(playerid)))
		{
			player = &idplayer(playerid);
			player->mo = ptr();
			player->camera = player->mo;
		}
	}
}

//

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

	if (p->playerstate == PST_REBORN)
		G_PlayerReborn (*p);

	AActor *mobj = new AActor (mthing->x << FRACBITS, mthing->y << FRACBITS, ONFLOORZ, MT_PLAYER);

	// set color translations for player sprites
	// [RH] Different now: MF_TRANSLATION is not used.
	mobj->translation = translationtables + 256*p->id;

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
	p->attacker = AActor::AActorPtr();

	consoleplayer().camera = displayplayer().mo;
	
	// Set up some special spectator stuff
	if (p->spectator)
	{
		mobj->translucency = 0;
		p->mo->flags |= MF_SPECTATOR;
		p->mo->flags2 |= MF2_FLY;
	}

	// [RH] Allow chasecam for demo watching
	if ((demoplayback || demonew) && chasedemo)
		p->cheats = CF_CHASECAM;

	// setup gun psprite
	P_SetupPsprites (p);

	// give all cards in death match mode
	if (sv_gametype != GM_COOP)
	{
		for (int i = 0; i < NUMCARDS; i++)
			p->cards[i] = true;
	}

	if (consoleplayer().camera == p->mo)
	{
		// wake up the status bar
		ST_Start ();
	}

	// [RH] If someone is in the way, kill them
	P_TeleportMove (mobj, mobj->x, mobj->y, mobj->z, true);
}

VERSION_CONTROL (cl_mobj_cpp, "$Id$")

