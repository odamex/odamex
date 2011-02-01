// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: sv_mobj.cpp 1832 2010-09-01 23:59:33Z mike $
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
#include "sv_main.h"
#include "p_ctf.h"
#include "g_game.h"

extern mapthing2_t      itemrespawnque[ITEMQUESIZE];
extern int              itemrespawntime[ITEMQUESIZE];
extern int              iquehead;
extern int              iquetail;

EXTERN_CVAR(sv_itemsrespawn)
EXTERN_CVAR(sv_nomonsters)
EXTERN_CVAR(sv_itemrespawntime)
EXTERN_CVAR(sv_maxplayers)
EXTERN_CVAR(sv_teamspawns)

bool unnatural_level_progression;

void G_PlayerReborn(player_t &player);
void CTF_RememberFlagPos(mapthing2_t *mthing);

void AActor::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	if (arc.IsStoring ())
	{
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
			<< target->netid
			<< lastenemy->netid
			<< reactiontime
			<< threshold
			<< player
			<< lastlook
			<< tracer->netid
			<< tid
			<< special
			<< args[0]
			<< args[1]
			<< args[2]
			<< args[3]
			<< args[4]			
			<< goal->netid
			<< (unsigned)0
			<< translucency
			<< waterlevel;
		spawnpoint.Serialize (arc);
	}
	else
	{
		unsigned dummy;
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
			>> target->netid
			>> lastenemy->netid
			>> reactiontime
			>> threshold
			>> player
			>> lastlook
			>> tracer->netid
			>> tid
			>> special
			>> args[0]
			>> args[1]
			>> args[2]
			>> args[3]
			>> args[4]			
			>> goal->netid
			>> dummy
			>> translucency
			>> waterlevel;

		DWORD trans;
		arc >> trans;
		spawnpoint.Serialize (arc);
		if(type >= NUMMOBJTYPES)
			I_Error("Unknown object type in saved game");
		if(sprite >= NUMSPRITES)
			I_Error("Unknown sprite in saved game");
		info = &mobjinfo[type];
		touching_sectorlist = NULL;
		LinkToWorld ();
		AddToHash ();
	}
}

//
// P_ExplodeMissile
//
void P_ExplodeMissile (AActor* mo)
{
	for (size_t i = 0; i < players.size(); i++)
	{
		client_t *cl = &clients[i];

		if(!SV_IsPlayerAllowedToSee(players[i], mo))
			continue;

		MSG_WriteMarker (&cl->reliablebuf, svc_movemobj);
		MSG_WriteShort (&cl->reliablebuf, mo->netid);
		MSG_WriteByte (&cl->reliablebuf, mo->rndindex);
		MSG_WriteLong (&cl->reliablebuf, mo->x);
		MSG_WriteLong (&cl->reliablebuf, mo->y);
		MSG_WriteLong (&cl->reliablebuf, mo->z);

		MSG_WriteMarker(&cl->reliablebuf, svc_explodemissile);
		MSG_WriteShort(&cl->reliablebuf, mo->netid);
	}


	mo->momx = mo->momy = mo->momz = 0;

	P_SetMobjState (mo, mobjinfo[mo->type].deathstate);
	if (mobjinfo[mo->type].deathstate != S_NULL)
	{
		// [RH] If the object is already translucent, don't change it.
		// Otherwise, make it 66% translucent.
		if (mo->translucency == FRACUNIT)
			mo->translucency = TRANSLUC66;

		mo->tics -= P_Random(mo) & 3;

		if (mo->tics < 1)
			mo->tics = 1;

		mo->flags &= ~MF_MISSILE;

		if (mo->info->deathsound)
			S_Sound (mo, CHAN_VOICE, mo->info->deathsound, 1, ATTN_NORM);

		mo->effects = 0;		// [RH]
	}
}

//
// P_MobjThinker
//
void AActor::RunThink ()
{
    AActor *onmo;
        
	if (type == MT_PLAYER && health <= 0)
		deadtic++;

	// GhostlyDeath -- Was a spectator but now it's nothing!
	if ((this->flags & MF_SPECTATOR ) && !player)
	{
		this->Destroy();
		return;
	}

	// remove dead players but don't tell clients about it
	if (type == MT_PLAYER && !player && deadtic >= REMOVECOPRSESTIC)
	{
		this->Destroy();
		return;
	}

	// [RH] Fade a stealth monster in and out of visibility
	if (visdir > 0)
	{
		translucency += 2*FRACUNIT/TICRATE;
		if (translucency > FRACUNIT)
		{
			translucency = FRACUNIT;
			visdir = 0;
		}
	}
	else if (visdir < 0)
	{
		translucency -= 3*FRACUNIT/TICRATE/2;
		if (translucency < 0)
		{
			translucency = 0;
			visdir = 0;
		}
	}

	// Handle X and Y momemtums
    BlockingMobj = NULL;
	if (momx || momy || (flags & MF_SKULLFLY))
	{
		P_XYMovement (this);

		if (ObjectFlags & OF_MassDestruction)
			return;		// actor was destroyed
	}

	if (flags2 & MF2_FLOATBOB)
	{ // Floating item bobbing motion (special1 is height)
		z = floorz + special1;
	}
	else if ((z != floorz) || momz || BlockingMobj)
	{
	    // Handle Z momentum and gravity
		if (flags2 & MF2_PASSMOBJ)
		{
		    if (!(onmo = P_CheckOnmobj (this)))
			{
				P_ZMovement (this);
				if (player && flags2 & MF2_ONMOBJ)
				{
					flags2 &= ~MF2_ONMOBJ;
				}
			}
			else
			{
			    if (player)
				{
					if (momz < -GRAVITY*8 && !(flags2&MF2_FLY))
					{
						PlayerLandedOnThing (this, onmo);
					}
					
					if (onmo->z + onmo->height - z <= 24 * FRACUNIT)
					{
						//player->viewheight -= z + onmo->height - z;
						//player->deltaviewheight =
						//	(VIEWHEIGHT - player->viewheight)>>3;
						z = onmo->z + onmo->height;
						flags2 |= MF2_ONMOBJ;
						momz = 0;
					}
					else
					{
						// hit the bottom of the blocking mobj
						momz = 0;
					}				    
				}
			}
		}
	    else
	    {
            P_ZMovement (this);        
	    }
	    
        if (ObjectFlags & OF_MassDestruction)
            return;		// actor was destroyed
	}

	if(subsector)
	{
		//byte lastwaterlevel = waterlevel;
		waterlevel = 0;
		if (subsector->sector->waterzone)
			waterlevel = 3;
		sector_t *hsec;
		if ( (hsec = subsector->sector->heightsec) )
		{
			if (hsec->waterzone && !subsector->sector->waterzone)
			{
				if (z < hsec->floorheight)
				{
					waterlevel = 1;
					if (z + height/2 < hsec->floorheight)
					{
						waterlevel = 2;
						if (z + height <= hsec->floorheight)
							waterlevel = 3;
					}
				}
				else if (z + height > hsec->ceilingheight)
				{
					waterlevel = 3;
				}
			}
		}
	}
		
	if (flags2 & MF2_DORMANT)
		return;
		
    // cycle through states,
    // calling action functions at transitions
	if (tics != -1)
	{
		tics--;

		// you can cycle through multiple states in a tic
		if (!tics)
			if (!P_SetMobjState (this, state->nextstate) )
				return; 		// freed itself
	}
	else
	{
		// check for nightmare respawn
		if (!(flags & MF_COUNTKILL) || !respawnmonsters)
			return;

		movecount++;

		if (movecount < 12*TICRATE)
			return;

		if (level.time & 31)
			return;

		if (P_Random (this) > 4)
			return;

		P_NightmareRespawn (this);
	}
}

//

//
// P_RespawnSpecials
//
void P_RespawnSpecials (void)
{
	fixed_t 			x;
	fixed_t 			y;
	fixed_t 			z;

	subsector_t*			ss;
	AActor* 						mo;
	mapthing2_t* 		mthing;

	int 				i;

    // clients do no control respawning of items
	if(!serverside)
		return;

    // allow respawning if we specified it
	if (!sv_itemsrespawn)
		return;

	// nothing left to respawn?
	if (iquehead == iquetail)
		return;

	// wait a certain number of seconds before respawning this special
	if (level.time - itemrespawntime[iquetail] < sv_itemrespawntime*TICRATE)
		return;

	mthing = &itemrespawnque[iquetail];

	x = mthing->x << FRACBITS;
	y = mthing->y << FRACBITS;

	// find which type to spawn
	for (i=0 ; i< NUMMOBJTYPES ; i++)
	{
		if (mthing->type == mobjinfo[i].doomednum)
			break;
	}

	// [Fly] crashes sometimes without it
	if (i >= NUMMOBJTYPES)
	{
		// pull it from the que
		iquetail = (iquetail+1)&(ITEMQUESIZE-1);
		return;
	}

	if (mobjinfo[i].flags & MF_SPAWNCEILING)
		z = ONCEILINGZ;
	else
		z = ONFLOORZ;

	// spawn a teleport fog at the new spot
	ss = R_PointInSubsector (x, y);
	mo = new AActor (x, y, z, MT_IFOG);
	SV_SpawnMobj(mo);
    if (clientside)
        S_Sound (mo, CHAN_VOICE, "misc/spawn", 1, ATTN_IDLE);

	// spawn it
	mo = new AActor (x, y, z, (mobjtype_t)i);
	mo->spawnpoint = *mthing;
	mo->angle = ANG45 * (mthing->angle/45);

	if (z == ONFLOORZ)
		mo->z += mthing->z << FRACBITS;
	else if (z == ONCEILINGZ)
		mo->z -= mthing->z << FRACBITS;

	if (mo->flags2 & MF2_FLOATBOB)
	{ // Seed random starting index for bobbing motion
		mo->health = M_Random();
		mo->special1 = mthing->z << FRACBITS;
	}
	
	mo->special = 0;
	
	// pull it from the que
	iquetail = (iquetail+1)&(ITEMQUESIZE-1);

	SV_SpawnMobj(mo);
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

	if (p->playerstate == PST_REBORN)
		G_PlayerReborn (*p);

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
	p->attacker = AActor::AActorPtr();

	// Set up some special spectator stuff
	if (p->spectator)
	{
		mobj->translucency = 0;
		p->mo->flags |= MF_SPECTATOR;
		p->mo->flags2 |= MF2_FLY;
	}

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

		// send new objects
		SV_SpawnMobj(mobj);
	}
}

//
// P_PreservePlayer
//
void P_PreservePlayer(player_t &player)
{
	if (!serverside || sv_gametype != GM_COOP || !validplayer(player) || !player.ingame())
		return;

	if(!unnatural_level_progression)
		player.playerstate = PST_LIVE; // denis - carry weapons and keys over to next level

	G_DoReborn(player);

	// inform client
	{
		size_t i;
		client_t *cl = &player.client;

		MSG_WriteMarker (&cl->reliablebuf, svc_playerinfo);

		for(i = 0; i < NUMWEAPONS; i++)
			MSG_WriteByte (&cl->reliablebuf, player.weaponowned[i]);

		for(i = 0; i < NUMAMMO; i++)
		{
			MSG_WriteShort (&cl->reliablebuf, player.maxammo[i]);
			MSG_WriteShort (&cl->reliablebuf, player.ammo[i]);
		}

		MSG_WriteByte (&cl->reliablebuf, player.health);
		MSG_WriteByte (&cl->reliablebuf, player.armorpoints);
		MSG_WriteByte (&cl->reliablebuf, player.armortype);
		MSG_WriteByte (&cl->reliablebuf, player.readyweapon);
		MSG_WriteByte (&cl->reliablebuf, player.backpack);
	}
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
	if (mthing->type == 11 || ((mthing->type == 5080 || mthing->type == 5081 || mthing->type == 5082))
		&& !sv_teamspawns)
	{
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

	// [Toke - CTF - starts] CTF starts - count Blue team start positions
	if (mthing->type == 5080 && sv_teamspawns)
	{
		if (blueteam_p == &blueteamstarts[MaxBlueTeamStarts])
		{
			int offset = MaxBlueTeamStarts;
			MaxBlueTeamStarts *= 2;
			blueteamstarts = (mapthing2_t *)Realloc (blueteamstarts, MaxBlueTeamStarts * sizeof(mapthing2_t));
			blueteam_p = &blueteamstarts[offset];
		}
		memcpy (blueteam_p, mthing, sizeof(*mthing));
		blueteam_p++;
		return;
	}

	// [Toke - CTF - starts] CTF starts - count Red team start positions
	if (mthing->type == 5081 && sv_teamspawns)
	{
		if (redteam_p == &redteamstarts[MaxRedTeamStarts])
		{
			int offset = MaxRedTeamStarts;
			MaxRedTeamStarts *= 2;
			redteamstarts = (mapthing2_t *)Realloc (redteamstarts, MaxRedTeamStarts * sizeof(mapthing2_t));
			redteam_p = &redteamstarts[offset];
		}
		memcpy (redteam_p, mthing, sizeof(*mthing));
		redteam_p++;
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

		playerstarts.push_back(*mthing);
		return;
	}

	// [RH] sound sequence overrides
	if (mthing->type >= 1400 && mthing->type < 1410)
	{
		return;
	}
	else if (mthing->type == 1411)
	{
		int type;

		if (mthing->args[0] == 255)
			type = -1;
		else
			type = mthing->args[0];

		if (type > 63)
		{
			Printf (PRINT_HIGH, "Sound sequence %d out of range\n", type);
		}
		else
		{
		}
		return;
	}

	/*if (deathmatch)
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
	}*/

	// GhostlyDeath -- Correctly spawn things
	if (sv_gametype != GM_COOP && !(mthing->flags & MTF_DEATHMATCH))
		return;
	if (sv_gametype == GM_COOP && sv_maxplayers == 1 && !(mthing->flags & MTF_SINGLE))
		return;
	if (sv_gametype == GM_COOP && sv_maxplayers != 1 && !(mthing->flags & MTF_COOPERATIVE))
		return;

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

	if (sv_nomonsters)
		if (i == MT_SKULL || (mobjinfo[i].flags & MF_COUNTKILL) )
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

	SV_SpawnMobj(mobj);

	if (sv_gametype == GM_CTF) {
		// [Toke - CTF] Setup flag sockets
		if (mthing->type == ID_BLUE_FLAG)
		{
			flagdata *data = &CTFdata[it_blueflag];
			if (data->flaglocated)
				return;

			CTF_RememberFlagPos (mthing);
			CTF_SpawnFlag(it_blueflag);
		}

		if (mthing->type == ID_RED_FLAG)
		{
			flagdata *data = &CTFdata[it_redflag];
			if (data->flaglocated)
				return;

			CTF_RememberFlagPos (mthing);
			CTF_SpawnFlag(it_redflag);
		}
	}

	// [RH] Go dormant as needed
	if (mthing->flags & MTF_DORMANT)
		P_DeactivateMobj (mobj);
}

VERSION_CONTROL (sv_mobj_cpp, "$Id: sv_mobj.cpp 1832 2010-09-01 23:59:33Z mike $")

