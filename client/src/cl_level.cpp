// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	G_LEVEL
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <set>

#include "am_map.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "cl_main.h"
#include "d_event.h"
#include "d_main.h"
#include "f_finale.h"
#include "g_game.h"
#include "g_levelstate.h"
#include "gi.h"
#include "g_skill.h"
#include "i_system.h"
#include "i_music.h"
#include "minilzo.h"
#include "m_random.h"
#include "p_acs.h"
#include "p_local.h"
#include "p_saveg.h"
#include "p_setup.h"
#include "r_data.h"
#include "r_sky.h"
#include "r_interp.h"
#include "s_sound.h"
#include "s_sndseq.h"
#include "st_stuff.h"
#include "v_video.h"
#include "w_wad.h"
#include "wi_stuff.h"
#include "z_zone.h"
#include "m_wdlstats.h"


#define lioffset(x)		offsetof(level_pwad_info_t,x)
#define cioffset(x)		offsetof(cluster_info_t,x)

void CL_ClearSectorSnapshots();
bool G_CheckSpot (player_t &player, mapthing2_t *mthing);
void P_SpawnPlayer (player_t &player, mapthing2_t *mthing);

EXTERN_CVAR(sv_fastmonsters)
EXTERN_CVAR(sv_monstersrespawn)
EXTERN_CVAR(sv_gravity)
EXTERN_CVAR(sv_aircontrol)
EXTERN_CVAR(g_resetinvonexit)

// Start time for timing demos
dtime_t starttime;

// ACS variables with world scope
int ACS_WorldVars[NUM_WORLDVARS];
ACSWorldGlobalArray ACS_WorldArrays[NUM_WORLDVARS];

// ACS variables with global scope
int ACS_GlobalVars[NUM_GLOBALVARS];
ACSWorldGlobalArray ACS_GlobalArrays[NUM_GLOBALVARS];

// [AM] Stores the reset snapshot
FLZOMemFile	*reset_snapshot = NULL;

extern bool r_underwater;
BOOL savegamerestore;

extern int mousex, mousey, joyforward, joystrafe, joyturn, joylook, Impulse;
extern BOOL sendpause, sendsave, sendcenterview;


bool isFast = false;

//
// G_InitNew
// Can be called by the startup code or the menu task,
// consoleplayer, displayplayer, should be set.
//
static OLumpName d_mapname;

void G_DeferedInitNew (const char *mapname)
{
	G_CleanupDemo();
	d_mapname = mapname;
	gameaction = ga_newgame;
}

void G_DeferedFullReset() {

}

void G_DeferedReset() {

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

	if (paused)
	{
		paused = false;
		S_ResumeSound ();
	}

	C_HideConsole();

	std::string wadstr = C_EscapeWadList(VectorArgs(argc, argv));
	G_LoadWadString(wadstr);

	D_StartTitle ();
	CL_QuitNetGame(NQ_SILENT);
	S_StopMusic();
	currentmusic = gameinfo.titleMusic.c_str();

	S_StartMusic(currentmusic.c_str());
}
END_COMMAND (wad)

EXTERN_CVAR(sv_allowexit)
EXTERN_CVAR(sv_nomonsters)
EXTERN_CVAR(sv_freelook)
EXTERN_CVAR(sv_allowjump)


//
// G_DoNewGame
// Is called whenever a new Singleplayer game will be started.
//
void G_DoNewGame (void)
{
	if (demoplayback)
	{
		cvar_t::C_RestoreCVars ();
		demoplayback = false;
		D_SetupUserInfo ();
	}

	CL_QuitNetGame(NQ_SILENT);

	multiplayer = false;

	// denis - single player warp (like in d_main)
	serverside = true;

	players.clear();
	players.push_back(player_t());
	players.front().doreborn = true;
	consoleplayer_id = displayplayer_id = players.back().id = 1;

	G_InitNew(d_mapname);
	gameaction = ga_nothing;
}

void G_InitNew (const char *mapname)
{
	size_t i;

	// [RH] Remove all particles
	R_ClearParticles ();

	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		it->mo = AActor::AActorPtr();
		it->camera = AActor::AActorPtr();
		it->attacker = AActor::AActorPtr();
	}

	if (!savegamerestore)
		G_ClearSnapshots ();

	// [RH] Mark all levels as not visited
	if (!savegamerestore)
	{
		LevelInfos& levels = getLevelInfos();
		for (size_t i = 0; i < levels.size(); i++)
		{
			level_pwad_info_t& level = levels.at(i);
			level.flags &= ~LEVEL_VISITED;
		}
	}

	cvar_t::UnlatchCVars ();

	if (paused)
	{
		paused = false;
		S_ResumeSound ();
	}

	// If were in chasecam mode, clear out // [Toke - fix]
	if ((consoleplayer().cheats & CF_CHASECAM))
	{
		consoleplayer().cheats &= ~CF_CHASECAM;
	}

	// [RH] If this map doesn't exist, bomb out
	if (W_CheckNumForName (mapname) == -1)
	{
		I_Error ("Could not find map %s\n", mapname);
	}

	const bool wantFast = sv_fastmonsters || G_GetCurrentSkill().fast_monsters;
	if (wantFast != isFast)
	{
		if (wantFast)
		{
			for (i = 0; i < NUMSTATES; i++)
			{
				if (states[i].flags & STATEF_SKILL5FAST &&
				    (states[i].tics != 1 || demoplayback))
					states[i].tics >>= 1; // don't change 1->0 since it causes cycles
			}

			for (i = 0; i < NUMMOBJTYPES; ++i)
			{
				if (mobjinfo[i].altspeed != NO_ALTSPEED)
				{
					int swap = mobjinfo[i].speed;
					mobjinfo[i].speed = mobjinfo[i].altspeed;
					mobjinfo[i].altspeed = swap;
				}
			}
		}
		else
		{
			for (i = 0; i < NUMSTATES; i++)
			{
				if (states[i].flags & STATEF_SKILL5FAST)
					states[i].tics <<= 1; // don't change 1->0 since it causes cycles
			}

			for (i = 0; i < NUMMOBJTYPES; ++i)
			{
				if (mobjinfo[i].altspeed != NO_ALTSPEED)
				{
					int swap = mobjinfo[i].altspeed;
					mobjinfo[i].altspeed = mobjinfo[i].speed;
					mobjinfo[i].speed = swap;
				}
			}
		}
		isFast = wantFast;
	}

	if (!savegamerestore)
	{
		M_ClearRandom ();
		memset (ACS_WorldVars, 0, sizeof(ACS_WorldVars));
		memset (ACS_GlobalVars, 0, sizeof(ACS_GlobalVars));
		for (int i = 0; i < NUM_GLOBALVARS; i++)
			ACS_GlobalArrays[i].clear();
		for (int i = 0; i < NUM_WORLDVARS; i++)
			ACS_WorldArrays[i].clear();
		level.time = 0;
		level.inttimeleft = 0;
	}

	AM_Stop();

	usergame = true;				// will be set false if a demo
	paused = false;
	viewactive = true;

	D_SetupUserInfo();

	level.mapname = mapname;

	// [AM}] WDL stats (for testing purposes)
	M_StartWDLLog(true);

	G_DoLoadLevel (0);
}

//
// G_DoCompleted
//
BOOL 			secretexit;
static int		startpos;	// [RH] Support for multiple starts per level
extern BOOL		NoWipe;		// [RH] Don't wipe when travelling in hubs

// [RH] The position parameter to these next three functions should
//		match the first parameter of the single player start spots
//		that should appear in the next map.
static void goOn(int position)
{
	LevelInfos& levels = getLevelInfos();
	ClusterInfos& clusters = getClusterInfos();
	cluster_info_t& thiscluster = clusters.findByCluster(level.cluster);
	cluster_info_t& nextcluster = clusters.findByCluster(levels.findByName(level.nextmap).cluster);

	startpos = position;
	gameaction = ga_completed;

	if (thiscluster.cluster != 0 && (thiscluster.flags & CLUSTER_HUB))
	{
		if ((level.flags & LEVEL_NOINTERMISSION) || (nextcluster.cluster == thiscluster.cluster))
		{
			NoWipe = 4;
		}
		D_DrawIcon = "TELEICON";
	}
}

void G_ExitLevel (int position, int drawscores, bool resetinv)
{
	if (resetinv)
	{
		for (Players::iterator it = players.begin();it != players.end();++it)
		{
			if (it->ingame())
			{
				it->doreborn = true;
			}
		}
	}

	secretexit = false;

	goOn (position);

	//gameaction = ga_completed;
}

// Here's for the german edition.
void G_SecretExitLevel (int position, int drawscores, bool resetinv)
{
	if (resetinv)
	{
		for (Players::iterator it = players.begin();it != players.end();++it)
		{
			if (it->ingame())
			{
				it->doreborn = true;
			}
		}
	}

	// IF NO WOLF3D LEVELS, NO SECRET EXIT!
	if ( (gameinfo.flags & GI_MAPxx)
		 && (W_CheckNumForName("map31")<0))
		secretexit = false;
	else
		secretexit = true;

    goOn (position);
	//gameaction = ga_completed;
}

void G_DoCompleted (void)
{
	gameaction = ga_nothing;

	for (Players::iterator it = players.begin();it != players.end();++it)
		if (it->ingame())
			G_PlayerFinishLevel(*it);

	V_RestoreScreenPalette();
	R_ExitLevel();

	// [RH] Mark this level as having been visited
	if (!(level.flags & LEVEL_CHANGEMAPCHEAT))
	{
		getLevelInfos().findByName(level.mapname).flags |= LEVEL_VISITED;
	}

	AM_Stop();

	wminfo.epsd = level.cluster - 1;		// Only used for DOOM I.
	strncpy (wminfo.lname0, level.info->pname.c_str(), 8);
	strncpy (wminfo.current, level.mapname.c_str(), 8);

	if (sv_gametype != GM_COOP && !(level.flags & LEVEL_CHANGEMAPCHEAT))
	{
		strncpy (wminfo.next, level.mapname.c_str(), 8);
		strncpy (wminfo.lname1, level.info->pname.c_str(), 8);
	}
	else
	{
		wminfo.next[0] = 0;

		if (!level.endpic.empty() && level.flags & LEVEL_NOINTERMISSION)
		{
			gameaction = ga_victory;
			return;
		}
		if (secretexit)
		{
			if (W_CheckNumForName (level.secretmap.c_str()) != -1)
			{
				strncpy(wminfo.next, level.secretmap.c_str(), 8);
				strncpy(wminfo.lname1, getLevelInfos().findByName(level.secretmap).pname.c_str(), 8);
			}
			else
			{
				secretexit = false;
			}
		}
		if (!wminfo.next[0])
		{
			strncpy(wminfo.next, level.nextmap.c_str(), 8);
			strncpy(wminfo.lname1, getLevelInfos().findByName(level.nextmap).pname.c_str(), 8);
		}
	}

	wminfo.maxkills = (level.total_monsters+level.respawned_monsters);
	wminfo.maxitems = level.total_items;
	wminfo.maxsecret = level.total_secrets;
	wminfo.maxfrags = 0;
	wminfo.partime = TICRATE * level.partime;

	wminfo.plyr.resize(players.size());

	size_t i = 0;
	for (Players::iterator it = players.begin();it != players.end();++it,++i)
	{
		wminfo.plyr[i].in = it->ingame();
		wminfo.plyr[i].skills = it->killcount;
		wminfo.plyr[i].sitems = it->itemcount;
		wminfo.plyr[i].ssecret = it->secretcount;
		wminfo.plyr[i].stime = level.time;
		//memcpy (wminfo.plyr[i].frags, players[i].frags
		//		, sizeof(wminfo.plyr[i].frags));
		wminfo.plyr[i].fragcount = it->fragcount;

		if(&*it == &consoleplayer())
			wminfo.pnum = static_cast<unsigned int>(i);
	}

	// [RH] If we're in a hub and staying within that hub, take a snapshot
	//		of the level. If we're traveling to a new hub, take stuff from
	//		the player and clear the world vars. If this is just an
	//		ordinary cluster (not a hub), take stuff from the player, but
	//		leave the world vars alone.
	LevelInfos& levels= getLevelInfos();
	ClusterInfos& clusters = getClusterInfos();
	{
		cluster_info_t& thiscluster = clusters.findByCluster(::level.cluster);
		cluster_info_t& nextcluster = clusters.findByCluster(levels.findByName(::level.nextmap).cluster);

		if (&thiscluster != &nextcluster || sv_gametype == GM_DM || !(thiscluster.flags & CLUSTER_HUB))
		{
			for (Players::iterator it = players.begin();it != players.end();++it)
				if (it->ingame())
					G_PlayerFinishLevel(*it); // take away cards and stuff

			if (nextcluster.flags & CLUSTER_HUB) {
				memset (ACS_WorldVars, 0, sizeof(ACS_WorldVars));
				for (int i = 0; i < NUM_WORLDVARS; i++)
					ACS_WorldArrays[i].clear();
				P_RemoveDefereds ();
				G_ClearSnapshots ();
			}
		}
		else
		{
			G_SnapshotLevel ();
		}

		if (!(nextcluster.flags & CLUSTER_HUB) || !(thiscluster.flags & CLUSTER_HUB))
		{
			level.time = 0;	// Reset time to zero if not entering/staying in a hub
			//level.inttimeleft = 0;
		}

		if (sv_gametype == GM_COOP)
		{
			if (level.flags & LEVEL_NOINTERMISSION && strnicmp(level.nextmap.c_str(), "EndGame", 7) == 0)
			{
				if (!multiplayer || demoplayback)
				{
					// Normal progression
					G_WorldDone();
					return;
				}
			}
			else if (&nextcluster == &thiscluster && thiscluster.flags & CLUSTER_HUB)
			{
				// Cluster progression
				G_WorldDone();
				return;
			}
		}
	}

	gamestate = GS_INTERMISSION;
	viewactive = false;

	WI_Start (&wminfo);
}

//
// G_DoLoadLevel
//
extern gamestate_t 	wipegamestate;


void G_DoLoadLevel (int position)
{
	static int lastposition = 0;
	size_t i;

	if (position == -1)
		position = lastposition;
	else
		lastposition = position;

	cvar_t::UnlatchCVars();

	G_InitLevelLocals ();

    Printf_Bold ("\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36"
                 "\36\36\36\36\36\36\36\36\36\36\36\36\37\n"
                 "%s: \"%s\"\n\n", level.mapname.c_str(), level.level_name);

	if (wipegamestate == GS_LEVEL)
		wipegamestate = GS_FORCEWIPE;

	const bool demoscreen = (gamestate == GS_DEMOSCREEN);

	gamestate = GS_LEVEL;

	// [SL] Hide the console unless this is just part of the demo loop
	// It's annoying to have the console close every time a new demo starts...
	if (!demoscreen)
		C_HideConsole();

	// [SL] clear the saved sector data from the last level
	OInterpolation::getInstance().resetGameInterpolation();

	// Set the sky map.
	// First thing, we have a dummy sky texture name,
	//	a flat. The data is in the WAD only because
	//	we look for an actual index, instead of simply
	//	setting one.
	skyflatnum = R_FlatNumForName(SKYFLATNAME);

	// DOOM determines the sky texture to be used
	// depending on the current episode, and the game version.
	// [RH] Fetch sky parameters from level_locals_t.
	// [ML] 5/11/06 - remove sky2 remenants
	// [SL] 2012-03-19 - Add sky2 back
	sky1texture = R_TextureNumForName (level.skypic.c_str());
	sky1scrolldelta = level.sky1ScrollDelta;
	sky1columnoffset = 0;
	sky2columnoffset = 0;
	if (!level.skypic2.empty())
	{
		sky2texture = R_TextureNumForName(level.skypic2.c_str());
		sky2scrolldelta = level.sky2ScrollDelta;
	}
	else
	{
		sky2texture = 0;
		sky2scrolldelta = 0;
	}

	// [RH] Set up details about sky rendering
	R_InitSkyMap ();

	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		if (it->ingame())
		{
			if (::g_resetinvonexit || it->playerstate == PST_DEAD ||
			    it->playerstate == PST_REBORN)
			{
				it->doreborn = true;
			}
			it->playerstate = PST_ENTER;
		}

		// Properly reset Cards, Powerups, and scores.
		P_ClearPlayerCards(*it);
		P_ClearPlayerPowerups(*it);
		P_ClearPlayerScores(*it, SCORES_CLEAR_ALL);
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
			actor->touching_sectorlist = NULL;
	}

 	SN_StopAllSequences (); // denis - todo - equivalent?
	P_SetupLevel (level.mapname.c_str(), position);

	// [AM] Prevent holding onto stale snapshots.
	CL_ClearSectorSnapshots();

	// [SL] 2011-09-18 - Find an alternative start if the single player start
	// point is not availible.
	if (!multiplayer && !consoleplayer().mo && consoleplayer().ingame())
	{
		// Check for a co-op start point
		for (size_t n = 0; n < playerstarts.size() && !consoleplayer().mo; n++)
		{
			if (G_CheckSpot(consoleplayer(), &playerstarts[n]))
				P_SpawnPlayer(consoleplayer(), &playerstarts[n]);
		}

		// Check for a free deathmatch start point
		for (size_t n = 0; n < DeathMatchStarts.size() && !consoleplayer().mo; n++)
		{
			if (G_CheckSpot(consoleplayer(), &DeathMatchStarts[n]))
				P_SpawnPlayer(consoleplayer(), &DeathMatchStarts[n]);
		}

		for (int iTeam = 0; iTeam < NUMTEAMS; iTeam++)
		{
			TeamInfo* teamInfo = GetTeamInfo((team_t)iTeam);
			for (size_t n = 0; n < teamInfo->Starts.size(); n++)
			{
				if (G_CheckSpot(consoleplayer(), &teamInfo->Starts[n]))
					P_SpawnPlayer(consoleplayer(), &teamInfo->Starts[n]);
			}
		}
	}

	displayplayer_id = consoleplayer_id;				// view the guy you are playing
	ST_Start();		// [RH] Make sure status bar knows who we are
	gameaction = ga_nothing;

	// clear cmd building stuff // denis - todo - could we get rid of this?
	Impulse = 0;
	for (i = 0; i < NUM_ACTIONS; i++)
		if (i != ACTION_MLOOK && i != ACTION_KLOOK)
			Actions[i] = 0;
	joyforward = joystrafe = joyturn = joylook = 0;
	mousex = mousey = 0;
	sendpause = sendsave = paused = sendcenterview = false;

	if (timingdemo)
	{
		static bool firstTime = true;

		if (firstTime)
		{
			starttime = I_MSTime();
			firstTime = false;
		}
	}

	level.starttime = I_MSTime() * TICRATE / 1000;
	G_UnSnapshotLevel (!savegamerestore);	// [RH] Restore the state of the level.
    P_DoDeferedScripts ();	// [RH] Do script actions that were triggered on another map.

	::levelstate.reset();

	C_FlushDisplay ();
}

//
// G_WorldDone
//
void G_WorldDone()
{
	LevelInfos& levels = getLevelInfos();
	ClusterInfos& clusters = getClusterInfos();

	gameaction = ga_worlddone;

	R_ExitLevel();

	if (level.flags & LEVEL_CHANGEMAPCHEAT)
		return;

	cluster_info_t& thiscluster = clusters.findByCluster(level.cluster);

	// Sort out default options to pass to F_StartFinale
	finale_options_t options = { 0, 0, 0, 0 };
	options.music = !level.intermusic.empty() ? level.intermusic.c_str() : thiscluster.messagemusic.c_str();

	if (!level.interbackdrop.empty())
	{
		options.flat = level.interbackdrop.c_str();
	}
	else if (!thiscluster.finalepic.empty())
	{
		options.pic = &thiscluster.finalepic[0];
	}
	else
	{
		options.flat = &thiscluster.finaleflat[0];
	}

	if (secretexit)
	{
		options.text = (!level.intertextsecret.empty()) ? level.intertextsecret.c_str() : thiscluster.exittext;
	}
	else
	{
		options.text = (!level.intertext.empty()) ? level.intertext.c_str() : thiscluster.exittext;
	}

	if (!strnicmp(level.nextmap.c_str(), "EndGame", 7))
	{
		AM_Stop();
		if (thiscluster.flags & CLUSTER_EXITTEXTISLUMP)
		{
			options.text = static_cast<const char*>(W_CacheLumpName(thiscluster.exittext, PU_STATIC));
		}
		F_StartFinale(options);
	}
	else
	{
		// Figure out if we took a secret exit.
		cluster_info_t& nextcluster = (secretexit) ?
			clusters.findByCluster(levels.findByName(::level.secretmap).cluster) :
			clusters.findByCluster(levels.findByName(::level.nextmap).cluster);

		if (nextcluster.cluster != level.cluster && sv_gametype == GM_COOP) {
			// Only start the finale if the next level's cluster is different
			// than the current one and we're not in deathmatch.
			if (nextcluster.entertext)
			{
				// All of our options need to be from the next cluster.
				options.music = nextcluster.messagemusic.c_str();
				if (!nextcluster.finalepic.empty())
				{
					options.pic = &nextcluster.finalepic[0];
				}
				else
				{
					options.flat = &nextcluster.finaleflat[0];
				}
				options.text = nextcluster.entertext;

				AM_Stop();
				F_StartFinale(options);
			}
			else if (thiscluster.exittext)
			{
				AM_Stop();
				if (thiscluster.flags & CLUSTER_EXITTEXTISLUMP)
				{
					options.text = static_cast<const char*>(W_CacheLumpName(thiscluster.exittext, PU_STATIC));
				}
				F_StartFinale(options);
			}
		}
	}
}

VERSION_CONTROL (g_level_cpp, "$Id$")
