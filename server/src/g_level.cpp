// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2015 by The Odamex Team.
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
//	G_LEVEL
//
//-----------------------------------------------------------------------------


#include <sstream>
#include <string>
#include <algorithm>
#include <vector>
#include <set>

#include "c_console.h"
#include "c_dispatch.h"
#include "d_event.h"
#include "d_main.h"
#include "doomstat.h"
#include "g_level.h"
#include "g_game.h"
#include "gstrings.h"
#include "gi.h"

#include "i_system.h"
#include "m_alloc.h"
#include "m_fileio.h"
#include "m_misc.h"
#include "minilzo.h"
#include "m_random.h"
#include "p_acs.h"
#include "p_ctf.h"
#include "p_local.h"
#include "p_mobj.h"
#include "p_saveg.h"
#include "p_setup.h"
#include "p_unlag.h"
#include "r_data.h"
#include "r_sky.h"
#include "s_sound.h"
#include "s_sndseq.h"
#include "sc_man.h"
#include "sv_main.h"
#include "sv_maplist.h"
#include "sv_vote.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"
#include "g_warmup.h"


// FIXME: Remove this as soon as the JoinString is gone from G_ChangeMap()
#include "cmdlib.h"

#define lioffset(x)		myoffsetof(level_pwad_info_t,x)
#define cioffset(x)		myoffsetof(cluster_info_t,x)

extern int nextupdate;

EXTERN_CVAR (sv_endmapscript)
EXTERN_CVAR (sv_startmapscript)
EXTERN_CVAR (sv_curmap)
EXTERN_CVAR (sv_nextmap)
EXTERN_CVAR (sv_loopepisode)
EXTERN_CVAR (sv_intermissionlimit)
EXTERN_CVAR (sv_warmup)
EXTERN_CVAR (sv_timelimit)

extern int mapchange;
extern int shotclock;

// ACS variables with world scope
int ACS_WorldVars[NUM_WORLDVARS];

// ACS variables with global scope
int ACS_GlobalVars[NUM_GLOBALVARS];

// [AM] Stores the reset snapshot
FLZOMemFile	*reset_snapshot = NULL;

BOOL firstmapinit = true; // Nes - Avoid drawing same init text during every rebirth in single-player servers.

BOOL savegamerestore;

extern BOOL sendpause;


bool isFast = false;

//
// G_InitNew
// Can be called by the startup code or the menu task,
// consoleplayer, displayplayer, should be set.
//
static char d_mapname[9];

void G_DeferedInitNew (char *mapname)
{
	strncpy (d_mapname, mapname, 8);
	gameaction = ga_newgame;

	// sv_nextmap cvar may be overridden by a script
	sv_nextmap.ForceSet(d_mapname);
}

void G_DeferedFullReset()
{
	gameaction = ga_fullresetlevel;
}

void G_DeferedReset()
{
	gameaction = ga_resetlevel;
}

const char* GetBase(const char* in)
{
	const char* out = &in[strlen(in) - 1];

	while (out > in && *(out-1) != PATHSEPCHAR)
		out--;

	return out;
}

BEGIN_COMMAND (wad) // denis - changes wads
{
	// [Russell] print out some useful info
	if (argc == 1)
	{
	    Printf(PRINT_HIGH, "Usage: wad pwad [...] [deh/bex [...]]\n");
	    Printf(PRINT_HIGH, "       wad iwad [pwad [...]] [deh/bex [...]]\n");
	    Printf(PRINT_HIGH, "\n");
	    Printf(PRINT_HIGH, "Load a wad file on the fly, pwads/dehs/bexs require extension\n");
	    Printf(PRINT_HIGH, "eg: wad doom\n");

	    return;
	}

	std::string str = JoinStrings(VectorArgs(argc, argv), " ");
	G_LoadWad(str);
}
END_COMMAND (wad)

BOOL 			secretexit;

EXTERN_CVAR(sv_shufflemaplist)

// Returns the next map, assuming there is no maplist.
std::string G_NextMap(void) {
	std::string next = level.nextmap;

	if (gamestate == GS_STARTUP || sv_gametype != GM_COOP || !strlen(next.c_str())) {
		// if not coop, stay on same level
		// [ML] 1/25/10: OR if next is empty
		next = level.mapname;
	} else if (secretexit && W_CheckNumForName(level.secretmap) != -1) {
		// if we hit a secret exit switch, go there instead.
		next = level.secretmap;
	}

	// NES - exiting a Doom 1 episode moves to the next episode,
	// rather than always going back to E1M1
	if (!strncmp(next.c_str(), "EndGame", 7) ||
		(gamemode == retail_chex && !strncmp (level.nextmap, "E1M6", 4))) {
		if (gameinfo.flags & GI_MAPxx || gamemode == shareware ||
			(!sv_loopepisode && ((gamemode == registered && level.cluster == 3) || ((gameinfo.flags & GI_MENUHACK_RETAIL) && level.cluster == 4)))) {
			next = CalcMapName(1, 1);
		} else if (sv_loopepisode) {
			next = CalcMapName(level.cluster, 1);
		} else {
			next = CalcMapName(level.cluster + 1, 1);
		}
	}
	return next;
}

// Determine the "next map" and change to it.
void G_ChangeMap() {
	unnatural_level_progression = false;

	// Skip the maplist to go to the desired level in case of a lobby map.
	if (level.flags & LEVEL_LOBBYSPECIAL && level.nextmap[0])
	{
		G_DeferedInitNew(level.nextmap);
	}
	else
	{
		size_t next_index;
		if (!Maplist::instance().get_next_index(next_index)) {
			// We don't have a maplist, so grab the next 'natural' map lump.
			std::string next = G_NextMap();
			G_DeferedInitNew((char *)next.c_str());
		}
		else {
			maplist_entry_t maplist_entry;
			Maplist::instance().get_map_by_index(next_index, maplist_entry);

			G_LoadWad(JoinStrings(maplist_entry.wads, " "), maplist_entry.map);

			// Set the new map as the current map
			Maplist::instance().set_index(next_index);
		}

		// run script at the end of each map
		// [ML] 8/22/2010: There are examples in the wiki that outright don't work
		// when onlcvars (addcommandstring's second param) is true.  Is there a
		// reason why the mapscripts ahve to be safe mode?
		if (strlen(sv_endmapscript.cstring()))
			AddCommandString(sv_endmapscript.cstring()/*, true*/);
	}
}

// Change to a map based on a maplist index.
void G_ChangeMap(size_t index) {
	maplist_entry_t maplist_entry;
	if (!Maplist::instance().get_map_by_index(index, maplist_entry)) {
		// That maplist index doesn't actually exist
		Printf(PRINT_HIGH, "%s\n", Maplist::instance().get_error().c_str());
		return;
	}

	G_LoadWad(JoinStrings(maplist_entry.wads, " "), maplist_entry.map);

	// Set the new map as the current map
	Maplist::instance().set_index(index);

	// run script at the end of each map
	// [ML] 8/22/2010: There are examples in the wiki that outright don't work
	// when onlcvars (addcommandstring's second param) is true.  Is there a
	// reason why the mapscripts ahve to be safe mode?
	if(strlen(sv_endmapscript.cstring()))
		AddCommandString(sv_endmapscript.cstring()/*, true*/);
}

// Restart the current map.
void G_RestartMap() {
	// Restart the current map.
	G_DeferedInitNew(level.mapname);

	// run script at the end of each map
	// [ML] 8/22/2010: There are examples in the wiki that outright don't work
	// when onlcvars (addcommandstring's second param) is true.  Is there a
	// reason why the mapscripts ahve to be safe mode?
	if(strlen(sv_endmapscript.cstring()))
		AddCommandString(sv_endmapscript.cstring()/*, true*/);
}

BEGIN_COMMAND (nextmap) {
	G_ExitLevel(0, 1);
} END_COMMAND (nextmap)

BEGIN_COMMAND (forcenextmap) {
	G_ChangeMap();
} END_COMMAND (forcenextmap)

BEGIN_COMMAND (restart) {
	warmup.restart();
} END_COMMAND (restart)

void SV_ClientFullUpdate(player_t &pl);
void SV_CheckTeam(player_t &pl);

//
// G_DoNewGame
//
// denis - rewritten so that it does not force client reconnects
//
void G_DoNewGame (void)
{
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		if(!(it->ingame()))
			continue;

		SV_SendLoadMap(wadfiles, patchfiles, d_mapname, &*it);
	}

	sv_curmap.ForceSet(d_mapname);

	G_InitNew (d_mapname);
	gameaction = ga_nothing;

	// run script at the start of each map
	// [ML] 8/22/2010: There are examples in the wiki that outright don't work
	// when onlcvars (addcommandstring's second param) is true.  Is there a
	// reason why the mapscripts ahve to be safe mode?
	if (strlen(sv_startmapscript.cstring()))
		AddCommandString(sv_startmapscript.cstring()/*,true*/);

	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		if (!(it->ingame()))
			continue;

		if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
			SV_CheckTeam(*it);
		else
			memcpy(it->userinfo.color, it->prefcolor, 4);

		SV_ClientFullUpdate(*it);
	}
}

EXTERN_CVAR (sv_skill)
EXTERN_CVAR (sv_monstersrespawn)
EXTERN_CVAR (sv_fastmonsters)
EXTERN_CVAR (sv_maxplayers)

void G_PlayerReborn (player_t &player);
void SV_ServerSettingChange();

void G_InitNew (const char *mapname)
{
	size_t i;

	if (!savegamerestore)
		G_ClearSnapshots ();

	// [RH] Mark all levels as not visited
	if (!savegamerestore)
	{
		for (i = 0; i < wadlevelinfos.size(); i++)
			wadlevelinfos[i].flags &= ~LEVEL_VISITED;

		for (i = 0; LevelInfos[i].mapname[0]; i++)
			LevelInfos[i].flags &= ~LEVEL_VISITED;
	}

	int old_gametype = sv_gametype.asInt();

	cvar_t::UnlatchCVars ();

	if (old_gametype != sv_gametype || sv_gametype != GM_COOP) {
		unnatural_level_progression = true;

		// Nes - Force all players to be spectators when the sv_gametype is not now or previously co-op.
		for (Players::iterator it = players.begin();it != players.end();++it)
		{
			// [SL] 2011-07-30 - Don't force downloading players to become spectators
			// it stops their downloading
			if (!(it->ingame()))
				continue;

			for (Players::iterator pit = players.begin();pit != players.end();++pit)
			{
				if (!(pit->ingame()))
					continue;
				MSG_WriteMarker (&(pit->client.reliablebuf), svc_spectate);
				MSG_WriteByte (&(pit->client.reliablebuf), it->id);
				MSG_WriteByte (&(pit->client.reliablebuf), true);
			}
			it->spectator = true;
			it->playerstate = PST_LIVE;
			it->joinafterspectatortime = -(TICRATE * 5);
		}
	}

	// [SL] 2011-09-01 - Change gamestate here so SV_ServerSettingChange will
	// send changed cvars
	gamestate = GS_LEVEL;
	SV_ServerSettingChange();

	if (paused)
	{
		paused = false;
	}

	// [RH] If this map doesn't exist, bomb out
	if (W_CheckNumForName (mapname) == -1)
	{
		I_Error ("Could not find map %s\n", mapname);
	}

	if (sv_skill == sk_nightmare || sv_monstersrespawn)
		respawnmonsters = true;
	else
		respawnmonsters = false;

	bool wantFast = sv_fastmonsters || (sv_skill == sk_nightmare);
	if (wantFast != isFast)
	{
		if (wantFast)
		{
			for (i=S_SARG_RUN1 ; i<=S_SARG_PAIN2 ; i++)
				states[i].tics >>= 1;
			mobjinfo[MT_BRUISERSHOT].speed = 20*FRACUNIT;
			mobjinfo[MT_HEADSHOT].speed = 20*FRACUNIT;
			mobjinfo[MT_TROOPSHOT].speed = 20*FRACUNIT;
		}
		else
		{
			for (i=S_SARG_RUN1 ; i<=S_SARG_PAIN2 ; i++)
				states[i].tics <<= 1;
			mobjinfo[MT_BRUISERSHOT].speed = 15*FRACUNIT;
			mobjinfo[MT_HEADSHOT].speed = 10*FRACUNIT;
			mobjinfo[MT_TROOPSHOT].speed = 10*FRACUNIT;
		}
		isFast = wantFast;
	}

	// [SL] 2011-05-11 - Reset all reconciliation system data for unlagging
	Unlag::getInstance().reset();

	if (!savegamerestore)
	{
		M_ClearRandom ();
		memset (ACS_WorldVars, 0, sizeof(ACS_WorldVars));
		memset (ACS_GlobalVars, 0, sizeof(ACS_GlobalVars));
		level.time = 0;
		level.timeleft = 0;
		level.inttimeleft = 0;

		// force players to be initialized upon first level load
		for (Players::iterator it = players.begin();it != players.end();++it)
		{
			// [SL] 2011-05-11 - Register the players in the reconciliation
			// system for unlagging
			Unlag::getInstance().registerPlayer(it->id);

			if(!(it->ingame()))
				continue;

			// denis - dead players should have their stuff looted, otherwise they'd take their ammo into their afterlife!
			if (it->playerstate == PST_DEAD)
				G_PlayerReborn(*it);

			it->playerstate = PST_ENTER; // [BC]

			it->joinafterspectatortime = -(TICRATE * 5);
		}
	}

	// [SL] 2012-12-08 - Multiplayer is always true for servers
	multiplayer = true;

	paused = false;
	demoplayback = false;
	viewactive = true;
	shotclock = 0;

	strncpy (level.mapname, mapname, 8);
	G_DoLoadLevel (0);

	// denis - hack to fix ctfmode, as it is only known after the map is processed!
	//if(old_ctfmode != ctfmode)
	//	SV_ServerSettingChange();
}

//
// G_DoCompleted
//

void G_ExitLevel (int position, int drawscores)
{
	SV_ExitLevel();

	if (drawscores)
        SV_DrawScores();
	
	int intlimit = (sv_intermissionlimit < 1 || sv_gametype == GM_COOP ? DEFINTSECS : sv_intermissionlimit);

	gamestate = GS_INTERMISSION;
	shotclock = 0;
	mapchange = TICRATE*intlimit;  // wait n seconds, default 10

    secretexit = false;

    gameaction = ga_completed;

	// denis - this will skip wi_stuff and allow some time for finale text
	//G_WorldDone();
}

// Here's for the german edition.
void G_SecretExitLevel (int position, int drawscores)
{
	SV_ExitLevel();

    if (drawscores)
        SV_DrawScores();
        
	int intlimit = (sv_intermissionlimit < 1 || sv_gametype == GM_COOP ? DEFINTSECS : sv_intermissionlimit);

	gamestate = GS_INTERMISSION;
	shotclock = 0;
	mapchange = TICRATE*intlimit;  // wait n seconds, defaults to 10

	// IF NO WOLF3D LEVELS, NO SECRET EXIT!
	if ( (gameinfo.flags & GI_MAPxx)
		 && (W_CheckNumForName("map31")<0))
		secretexit = false;
	else
		secretexit = true;

    gameaction = ga_completed;

	// denis - this will skip wi_stuff and allow some time for finale text
	//G_WorldDone();
}

void G_DoCompleted()
{
	gameaction = ga_nothing;

	for (Players::iterator it = players.begin();it != players.end();++it)
		if (it->ingame())
			G_PlayerFinishLevel(*it);
}

extern void G_SerializeLevel(FArchive &arc, bool hubLoad, bool noStorePlayers);

// [AM] - Save the state of the level that can be reset to
void G_DoSaveResetState()
{
	if (reset_snapshot != NULL)
	{
		// An existing reset snapshot exists.  Kill it and replace it with
		// a new one.
		delete reset_snapshot;
	}
	reset_snapshot = new FLZOMemFile;
	reset_snapshot->Open();
	FArchive arc(*reset_snapshot);
	G_SerializeLevel(arc, false, true);
	arc << level.time;
}

// [AM] - Reset the state of the level.  Second parameter is true if you want
//        to zero-out gamestate as well (i.e. resetting scores, RNG, etc.).
void G_DoResetLevel(bool full_reset)
{
	gameaction = ga_nothing;
	if (reset_snapshot == NULL)
	{
		// No saved state to reload to
		DPrintf("G_DoResetLevel: No saved state to reload.");
		return;
	}

	// Clear CTF state.
	Players::iterator it;
	if (sv_gametype == GM_CTF)
	{
		for (size_t i = 0;i < NUMFLAGS;i++)
		{
			for (it = players.begin();it != players.end();++it)
			{
				it->flags[i] = false;
			}
			CTFdata[i].flagger = 0;
			CTFdata[i].state = flag_home;
		}
	}

	// Clear netids of every non-player actor so we don't spam the
	// destruction message of actors to clients.
	{
		AActor* mo;
		TThinkerIterator<AActor> iterator;
		while ((mo = iterator.Next()))
		{
			if (mo->netid && mo->type != MT_PLAYER)
			{
				ServerNetID.ReleaseNetID(mo->netid);
				mo->netid = 0;
			}
		}
	}

	// Tell clients that a map reset is incoming.
	for (it = players.begin();it != players.end();++it)
	{
		if (!(it->ingame()))
			continue;

		client_t *cl = &(it->client);
		MSG_WriteMarker(&cl->reliablebuf, svc_resetmap);
	}

	// Unserialize saved snapshot
	reset_snapshot->Reopen();
	FArchive arc(*reset_snapshot);
	G_SerializeLevel(arc, false, true);
	int level_time;
	arc >> level_time;
	reset_snapshot->Seek(0, FFile::ESeekSet);

	// Assign new netids to every non-player actor to make sure we don't have
	// any weird destruction of any items post-reset.
	{
		AActor* mo;
		TThinkerIterator<AActor> iterator;
		while ((mo = iterator.Next()))
		{
			if (mo->netid && mo->type != MT_PLAYER)
			{
				mo->netid = ServerNetID.ObtainNetID();
			}
		}
	}

	// Clear the item respawn queue, otherwise all those actors we just
	// destroyed and replaced with the serialized items will start respawning.
	iquehead = iquetail = 0;
	// Potentially clear out gamestate as well.
	if (full_reset)
	{
		// Set time to the initial tic
		level.time = level_time;
		// Clear global goals.
		for (size_t i = 0; i < NUMTEAMS; i++)
			TEAMpoints[i] = 0;
		// Clear player information.
		for (it = players.begin();it != players.end();++it)
		{
			it->fragcount = 0;
			it->itemcount = 0;
			it->secretcount = 0;
			it->deathcount = 0;
			it->killcount = 0;
			it->points = 0;
			it->joinafterspectatortime = level.time;

			// [AM] Only touch ready state if warmup mode is enabled.
			if (sv_warmup)
				it->ready = false;
		}
		// For predictable first spawns.
		M_ClearRandom();
	}

	// [SL] always reset the time (for now at least)
	level.time = 0;
	level.timeleft = sv_timelimit * TICRATE * 60;
	level.inttimeleft = mapchange / TICRATE;

	// Send information about the newly reset map.
	for (it = players.begin();it != players.end();++it)
	{
		// Player needs to actually be ingame
		if (!it->ingame())
			continue;

		SV_ClientFullUpdate(*it);
	}
	// Force every ingame player to be reborn.
	for (it = players.begin();it != players.end();++it)
	{
		// Spectators aren't reborn.
		if (!it->ingame() || it->spectator)
			continue;

		// Destroy the attached mobj, otherwise we leave a ghost.
		it->mo->Destroy();

		// Set the respawning machinery in motion
		it->playerstate = full_reset ? PST_ENTER : PST_REBORN;

		// Do this here, otherwise players won't be reborn until next tic.
		// [AM] Also, forgetting to do this will result in ticcmds that rely on
		//      a players subsector to be valid (like use) to crash the server.
		G_DoReborn(*it);
	}
}

//
// G_DoLoadLevel
//
extern gamestate_t 	wipegamestate;
extern float BaseBlendA;

void G_DoLoadLevel (int position)
{
	static int lastposition = 0;

	if (position != -1)
		firstmapinit = true;

	if (position == -1)
		position = lastposition;
	else
		lastposition = position;

	G_InitLevelLocals ();

	if (firstmapinit) {
		Printf (PRINT_HIGH, "--- %s: \"%s\" ---\n", level.mapname, level.level_name);
		firstmapinit = false;
	}

	if (wipegamestate == GS_LEVEL)
		wipegamestate = GS_FORCEWIPE;

	gamestate = GS_LEVEL;

	// Set the sky map.
	// First thing, we have a dummy sky texture name,
	//	a flat. The data is in the WAD only because
	//	we look for an actual index, instead of simply
	//	setting one.
	skyflatnum = R_FlatNumForName ( SKYFLATNAME );

	// DOOM determines the sky texture to be used
	// depending on the current episode, and the game version.
	// [RH] Fetch sky parameters from level_locals_t.
	// [ML] 5/11/06 - remove sky2 remenants
	// [SL] 2012-03-19 - Add sky2 back
	sky1texture = R_TextureNumForName (level.skypic);
	if (strlen(level.skypic2))
		sky2texture = R_TextureNumForName (level.skypic2);
	else
		sky2texture = 0;

	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		if (it->ingame() && it->playerstate == PST_DEAD)
			it->playerstate = PST_REBORN;

		// [AM] If sv_keepkeys is on, players might still be carrying keys, so
		//      make sure they're gone.
		for (size_t j = 0; j < NUMCARDS; j++)
			it->cards[j] = false;

		it->fragcount = 0;
		it->itemcount = 0;
		it->secretcount = 0;
		it->deathcount = 0; // [Toke - Scores - deaths]
		it->killcount = 0; // [deathz0r] Coop kills
		it->points = 0;

		// [AM] Only touch ready state if warmup mode is enabled.
		if (sv_warmup)
		{
			it->ready = false;
			it->timeout_ready = 0;

			// [AM] Make sure the clients are updated on the new ready state
			for (Players::iterator pit = players.begin();pit != players.end();++pit)
			{
				MSG_WriteMarker(&(pit->client.reliablebuf), svc_readystate);
				MSG_WriteByte(&(pit->client.reliablebuf), it->id);
				MSG_WriteBool(&(pit->client.reliablebuf), false);
			}
		}
	}

	// [deathz0r] It's a smart idea to reset the team points
	if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
	{
		for (size_t i = 0; i < NUMTEAMS; i++)
			TEAMpoints[i] = 0;
	}

	// initialize the msecnode_t freelist.					phares 3/25/98
	// any nodes in the freelist are gone by now, cleared
	// by Z_FreeTags() when the previous level ended or player
	// died.

	{
		extern msecnode_t *headsecnode; // phares 3/25/98
		headsecnode = NULL;

		// denis - todo - wtf is this crap?
		// [RH] Need to prevent the AActor destructor from trying to
		//		free the nodes
		AActor *actor;
		TThinkerIterator<AActor> iterator;

		while ( (actor = iterator.Next ()) )
		{
			actor->touching_sectorlist = NULL;

			// denis - clear every actor netid so that they don't announce their destruction to clients
			ServerNetID.ReleaseNetID(actor->netid);
			actor->netid = 0;
		}
	}

	// For single-player servers.
	for (Players::iterator it = players.begin();it != players.end();++it)
		it->joinafterspectatortime -= level.time;

	flagdata *tempflag;

	// Nes - CTF Pre flag setup
	if (sv_gametype == GM_CTF) {
		tempflag = &CTFdata[it_blueflag];
		tempflag->flaglocated = false;

		tempflag = &CTFdata[it_redflag];
		tempflag->flaglocated = false;
	}

	P_SetupLevel (level.mapname, position);

	// Nes - CTF Post flag setup
	if (sv_gametype == GM_CTF) {
		tempflag = &CTFdata[it_blueflag];
		if (!tempflag->flaglocated)
			SV_BroadcastPrintf(PRINT_HIGH, "WARNING: Blue flag pedestal not found! No blue flags in game.\n");

		tempflag = &CTFdata[it_redflag];
		if (!tempflag->flaglocated)
			SV_BroadcastPrintf(PRINT_HIGH, "WARNING: Red flag pedestal not found! No red flags in game.\n");
	}

	displayplayer_id = consoleplayer_id;				// view the guy you are playing

	gameaction = ga_nothing;
	Z_CheckHeap ();

	paused = false;

	level.starttime = I_MSTime() * TICRATE / 1000;
	// [RH] Restore the state of the level.
	G_UnSnapshotLevel (!savegamerestore);
	// [RH] Do script actions that were triggered on another map.
	P_DoDeferedScripts ();
	// [AM] Save the state of the level on the first tic.
	G_DoSaveResetState();
	// [AM] Handle warmup init.
	warmup.reset();
	//	C_FlushDisplay ();
}

//
// G_WorldDone
//
void G_WorldDone (void)
{
	cluster_info_t *nextcluster;
	cluster_info_t *thiscluster;

	//gameaction = ga_worlddone;

	if (level.flags & LEVEL_CHANGEMAPCHEAT)
		return;

	const char *finaletext = NULL;
	thiscluster = FindClusterInfo (level.cluster);
	if (!strncmp (level.nextmap, "EndGame", 7) || (gamemode == retail_chex && !strncmp (level.nextmap, "E1M6", 4))) {
//		F_StartFinale (thiscluster->messagemusic, thiscluster->finaleflat, thiscluster->exittext); // denis - fixme - what should happen on the server?
		finaletext = thiscluster->exittext;
	} else {
		if (!secretexit)
			nextcluster = FindClusterInfo (FindLevelInfo (level.nextmap)->cluster);
		else
			nextcluster = FindClusterInfo (FindLevelInfo (level.secretmap)->cluster);

		if (nextcluster->cluster != level.cluster && sv_gametype == GM_COOP) {
			// Only start the finale if the next level's cluster is different
			// than the current one and we're not in deathmatch.
			if (nextcluster->entertext) {
//				F_StartFinale (nextcluster->messagemusic, nextcluster->finaleflat, nextcluster->entertext); // denis - fixme
				finaletext = nextcluster->entertext;
			} else if (thiscluster->exittext) {
//				F_StartFinale (thiscluster->messagemusic, thiscluster->finaleflat, thiscluster->exittext); // denis - fixme
				finaletext = thiscluster->exittext;
			}
		}
	}

	if(finaletext)
		mapchange += strlen(finaletext)*2;
}


VERSION_CONTROL (g_level_cpp, "$Id$")
