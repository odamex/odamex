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

//
// P_SpawnMapThing
// The fields of the mapthing should
// already be in host byte order.
//
// [RH] position is used to weed out unwanted start spots
//
void P_SpawnMapThing (mapthing2_t *mthing, int position)
{
	int i;
	int bit;
	AActor *mobj;
	fixed_t x, y, z;

	if (mthing->type == 0 || mthing->type == -1)
		return;

	// count deathmatch start positions
	if (mthing->type == 11)
	{
		// [Nes] Maximum vanilla demo starts are fixed at 10.
		if (deathmatch_p >= &deathmatchstarts[10] && (demoplayback || demorecording) && democlassic)
			return;

		if (deathmatch_p == &deathmatchstarts[MaxDeathmatchStarts])
		{
			// [RH] Get more deathmatchstarts
			int offset = MaxDeathmatchStarts;
			MaxDeathmatchStarts *= 2;
			deathmatchstarts = (mapthing2_t *)Realloc (deathmatchstarts, MaxDeathmatchStarts * sizeof(mapthing2_t));
			deathmatch_p = &deathmatchstarts[offset];
		}
		memcpy (deathmatch_p, mthing, sizeof(*mthing));
		deathmatch_p++;
		return;
	}
	
	// [RH] Record polyobject-related things
	if (HexenHack)
	{
		switch (mthing->type)
		{
		case PO_HEX_ANCHOR_TYPE:
			mthing->type = PO_ANCHOR_TYPE;
			break;
		case PO_HEX_SPAWN_TYPE:
			mthing->type = PO_SPAWN_TYPE;
			break;
		case PO_HEX_SPAWNCRUSH_TYPE:
			mthing->type = PO_SPAWNCRUSH_TYPE;
			break;
		}
	}

	if (mthing->type == PO_ANCHOR_TYPE ||
		mthing->type == PO_SPAWN_TYPE ||
		mthing->type == PO_SPAWNCRUSH_TYPE)
	{
		polyspawns_t *polyspawn = new polyspawns_t;
		polyspawn->next = polyspawns;
		polyspawn->x = mthing->x << FRACBITS;
		polyspawn->y = mthing->y << FRACBITS;
		polyspawn->angle = mthing->angle;
		polyspawn->type = mthing->type;
		polyspawns = polyspawn;
		if (mthing->type != PO_ANCHOR_TYPE)
			po_NumPolyobjs++;
		return;
	}	

	// check for players specially
	if ((mthing->type <= 4 && mthing->type > 0)
		|| (mthing->type >= 4001 && mthing->type <= 4001 + MAXPLAYERSTARTS - 4))
	{
		// [RH] Only spawn spots that match position.
		if (mthing->args[0] != position)
			return;

		// save spots for respawning in network games
		size_t playernum = mthing->type <= 4 ? mthing->type-1 : (mthing->type - 4001 + 4)%MAXPLAYERSTARTS;
		playerstarts.push_back(*mthing);
		player_t &p = idplayer(playernum+1);

		if (sv_gametype == GM_COOP &&
			(validplayer(p) && p.ingame()))
		{
			P_SpawnPlayer (p, mthing);
			return;
		}

		return;
	}

	// CTF items
	if (mthing->type > 5129 && mthing->type < 5133)
		return;

	else if (mthing->type >= 5080 && mthing->type <= 5082)
		return;

	if (sv_gametype != GM_COOP)
	{
		if (!(mthing->flags & MTF_DEATHMATCH))
			return;
	}
	else if (multiplayer)
	{
		if (!(mthing->flags & MTF_COOPERATIVE))
			return;
	}

	if (!multiplayer)
	{
		if (!(mthing->flags & MTF_SINGLE))
			return;
	}

	// check for apropriate skill level
	if (sv_skill == sk_baby)
		bit = 1;
	else if (sv_skill == sk_nightmare)
		bit = 4;
	else
		bit = 1 << ((int)sv_skill - 2);

	if (!(mthing->flags & bit))
		return;

	// [RH] Determine if it is an old ambient thing, and if so,
	//		map it to MT_AMBIENT with the proper parameter.
	if (mthing->type >= 14001 && mthing->type <= 14064)
	{
		mthing->args[0] = mthing->type - 14000;
		mthing->type = 14065;
		i = MT_AMBIENT;
	}
	// [RH] Check if it's a particle fountain
	else if (mthing->type >= 9027 && mthing->type <= 9033)
	{
		mthing->args[0] = mthing->type - 9026;
		i = MT_FOUNTAIN;
	}
	else
	{
		// find which type to spawn
		for (i = 0; i < NUMMOBJTYPES; i++)
			if (mthing->type == mobjinfo[i].doomednum)
				break;
	}

	if (i >= NUMMOBJTYPES)
	{
		// [RH] Don't die if the map tries to spawn an unknown thing
		Printf (PRINT_HIGH, "Unknown type %i at (%i, %i)\n",
			mthing->type,
			mthing->x, mthing->y);
		i = MT_UNKNOWNTHING;
	}
	// [RH] If the thing's corresponding sprite has no frames, also map
	//		it to the unknown thing.
	else if (sprites[states[mobjinfo[i].spawnstate].sprite].numframes == 0)
	{
		Printf (PRINT_HIGH, "Type %i at (%i, %i) has no frames\n",
				mthing->type, mthing->x, mthing->y);
		i = MT_UNKNOWNTHING;
	}

	// don't spawn keycards and players in deathmatch
	if (sv_gametype != GM_COOP && mobjinfo[i].flags & MF_NOTDMATCH)
		return;

	// don't spawn deathmatch weapons in offline single player mode
	if (!multiplayer)
	{
		switch (i)
		{
			case MT_CHAINGUN:
			case MT_SHOTGUN:
			case MT_SUPERSHOTGUN:
			case MT_MISC25: 		// BFG
			case MT_MISC26: 		// chainsaw
			case MT_MISC27: 		// rocket launcher
			case MT_MISC28: 		// plasma gun
				if ((mthing->flags & (MTF_DEATHMATCH|MTF_SINGLE)) == MTF_DEATHMATCH)
					return;
				break;
			default:
				break;
		}
	}

	// [csDoom] don't spawn any monsters
	if (sv_nomonsters || !serverside)
	{
		if (i == MT_SKULL || (mobjinfo[i].flags & MF_COUNTKILL) )
		{
			return;
		}
	}

    // for client...
	// Type 14 is a teleport exit. We must spawn it here otherwise
	// teleporters won't work well.
	if (!serverside && (mthing->flags & MF_SPECIAL) && (mthing->type != 14))
		return;

	// spawn it
	x = mthing->x << FRACBITS;
	y = mthing->y << FRACBITS;

	if (i == MT_WATERZONE)
	{
		sector_t *sec = R_PointInSubsector (x, y)->sector;
		sec->waterzone = 1;
		return;
	}
	else if (i == MT_SECRETTRIGGER)
	{
		level.total_secrets++;
	}

	if (mobjinfo[i].flags & MF_SPAWNCEILING)
		z = ONCEILINGZ;
	else
		z = ONFLOORZ;

	// only servers control spawning of special items
	if (!serverside && mobjinfo[i].flags & MF_SPECIAL)
		return;

	mobj = new AActor (x, y, z, (mobjtype_t)i);

	if (z == ONFLOORZ)
		mobj->z += mthing->z << FRACBITS;
	else if (z == ONCEILINGZ)
		mobj->z -= mthing->z << FRACBITS;
	mobj->spawnpoint = *mthing;

	if (mobj->flags2 & MF2_FLOATBOB)
	{ // Seed random starting index for bobbing motion
		mobj->health = M_Random();
		mobj->special1 = mthing->z << FRACBITS;
	}
	
	// [RH] Set the thing's special
	mobj->special = mthing->special;
	memcpy (mobj->args, mthing->args, sizeof(mobj->args));

	// [RH] If it's an ambient sound, activate it
	//if (i == MT_AMBIENT)
	//	S_ActivateAmbient (mobj, mobj->args[0]);

	// [RH] If a fountain and not dormant, start it
//	if (i == MT_FOUNTAIN && !(mthing->flags & MTF_DORMANT))
//		mobj->effects = mobj->args[0] << FX_FOUNTAINSHIFT;

	if (mobj->tics > 0)
		mobj->tics = 1 + (P_Random () % mobj->tics);
	if (mobj->flags & MF_COUNTKILL)
		level.total_monsters++;
	if (mobj->flags & MF_COUNTITEM)
		level.total_items++;

	if (i != MT_SPARK)
		mobj->angle = ANG45 * (mthing->angle/45);

	if (mthing->flags & MTF_AMBUSH)
		mobj->flags |= MF_AMBUSH;

	// [RH] Add ThingID to mobj and link it in with the others
	mobj->tid = mthing->thingid;
	mobj->AddToHash ();

	// [RH] Go dormant as needed
	if (mthing->flags & MTF_DORMANT)
		P_DeactivateMobj (mobj);
}

VERSION_CONTROL (cl_mobj_cpp, "$Id$")

