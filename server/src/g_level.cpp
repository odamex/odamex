// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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


#include <string>

#include "d_main.h"
#include "m_alloc.h"
#include "g_level.h"
#include "g_game.h"
#include "s_sound.h"
#include "d_event.h"
#include "m_random.h"
#include "doomstat.h"
#include "r_data.h"
#include "w_wad.h"
#include "c_dispatch.h"
#include "z_zone.h"
#include "i_system.h"
#include "p_setup.h"
#include "p_local.h"
#include "r_sky.h"
#include "c_console.h"
#include "dstrings.h"
#include "v_video.h"
#include "p_saveg.h"
#include "d_protocol.h"
#include "v_text.h"
#include "d_main.h"
#include "p_mobj.h"
#include "m_fileio.h"
#include "m_misc.h"

#include "gi.h"
#include "minilzo.h"

#include "sv_main.h"
#include "sv_ctf.h"

#define lioffset(x)		myoffsetof(level_pwad_info_t,x)
#define cioffset(x)		myoffsetof(cluster_info_t,x)

extern int nextupdate;
extern int shotclock;

EXTERN_CVAR (endmapscript)
EXTERN_CVAR (startmapscript)
EXTERN_CVAR (curmap)
EXTERN_CVAR (nextmap)
EXTERN_CVAR (loopepisode)

static level_info_t *FindDefLevelInfo (char *mapname);
static cluster_info_t *FindDefClusterInfo (int cluster);

extern int timingdemo;

extern int mapchange;

// Start time for timing demos
int starttime;

BOOL firstmapinit = true; // Nes - Avoid drawing same init text during every rebirth in single-player servers.

extern BOOL netdemo;
BOOL savegamerestore;

extern int mousex, mousey, joyxmove, joyymove, Impulse;
extern BOOL sendpause, sendsave, sendcenterview;

level_locals_t level;			// info about current level

static level_pwad_info_t *wadlevelinfos;
static cluster_info_t *wadclusterinfos;
static size_t numwadlevelinfos = 0;
static size_t numwadclusterinfos = 0;

bool isFast = false;

enum
{
	MITL_MAP,
	MITL_DEFAULTMAP,
	MITL_CLUSTERDEF
};

enum EMIType
{
	MITYPE_IGNORE,
	MITYPE_EATNEXT,
	MITYPE_INT,
	MITYPE_COLOR,
	MITYPE_MAPNAME,
	MITYPE_LUMPNAME,
	MITYPE_SKY,
	MITYPE_SETFLAG,
	MITYPE_SCFLAGS,
	MITYPE_CLUSTER,
	MITYPE_STRING,
	MITYPE_CSTRING
};

struct MapInfoHandler
{
	EMIType type;
	DWORD data1, data2;
}
MapHandlers[] =
{
	{ MITYPE_INT,		lioffset(levelnum), 0 }, // denis - fixme - lioffset, offsetof will generate warnings unless given a POD struct - but "level_pwad_info_s : public level_info_s" isn't a POD!
	{ MITYPE_MAPNAME,	lioffset(nextmap), 0 },
	{ MITYPE_MAPNAME,	lioffset(secretmap), 0 },
	{ MITYPE_CLUSTER,	lioffset(cluster), 0 },
	{ MITYPE_SKY,		lioffset(skypic), 0 },				//[ML] 5/11/06 - Remove sky scrolling
	{ MITYPE_COLOR,		lioffset(fadeto), 0 },
	{ MITYPE_COLOR,		lioffset(outsidefog), 0 },
	{ MITYPE_LUMPNAME,	lioffset(pname), 0 },
	{ MITYPE_INT,		lioffset(partime), 0 },
	{ MITYPE_LUMPNAME,	lioffset(music), 0 },
	{ MITYPE_SETFLAG,	LEVEL_NOINTERMISSION, 0 },
	{ MITYPE_SETFLAG,	LEVEL_DOUBLESKY, 0 },
	{ MITYPE_SETFLAG,	LEVEL_NOSOUNDCLIPPING, 0 },
	{ MITYPE_SETFLAG,	LEVEL_MONSTERSTELEFRAG, 0 },
	{ MITYPE_SETFLAG,	LEVEL_MAP07SPECIAL, 0 },
	{ MITYPE_SETFLAG,	LEVEL_BRUISERSPECIAL, 0 },
	{ MITYPE_SETFLAG,	LEVEL_CYBORGSPECIAL, 0 },
	{ MITYPE_SETFLAG,	LEVEL_SPIDERSPECIAL, 0 },
	{ MITYPE_SCFLAGS,	0, ~LEVEL_SPECACTIONSMASK },
	{ MITYPE_SCFLAGS,	LEVEL_SPECOPENDOOR, ~LEVEL_SPECACTIONSMASK },
	{ MITYPE_SCFLAGS,	LEVEL_SPECLOWERFLOOR, ~LEVEL_SPECACTIONSMASK },
	{ MITYPE_IGNORE,	0, 0 },		// lightning
	{ MITYPE_LUMPNAME,	lioffset(fadetable), 0 },
	{ MITYPE_SETFLAG,	LEVEL_EVENLIGHTING, 0 },
	{ MITYPE_SETFLAG,	LEVEL_SNDSEQTOTALCTRL, 0 },
	{ MITYPE_SETFLAG,	LEVEL_FORCENOSKYSTRETCH, 0 },
	{ MITYPE_SCFLAGS,	LEVEL_FREELOOK_YES, ~LEVEL_FREELOOK_NO },
	{ MITYPE_SCFLAGS,	LEVEL_FREELOOK_NO, ~LEVEL_FREELOOK_YES },
	{ MITYPE_SCFLAGS,	LEVEL_JUMP_YES, ~LEVEL_JUMP_NO },
	{ MITYPE_SCFLAGS,	LEVEL_JUMP_NO, ~LEVEL_JUMP_YES },
	{ MITYPE_EATNEXT,	0, 0 },
	{ MITYPE_EATNEXT,	0, 0 },
	{ MITYPE_EATNEXT,	0, 0 },
	{ MITYPE_EATNEXT,	0, 0 },
	{ MITYPE_EATNEXT,	0, 0 },
	{ MITYPE_EATNEXT,	0, 0 },
	{ MITYPE_EATNEXT,	0, 0 },
	{ MITYPE_EATNEXT,	0, 0 }
};

static int FindWadLevelInfo (char *name)
{
	for (size_t i = 0; i < numwadlevelinfos; i++)
		if (!strnicmp (name, wadlevelinfos[i].mapname, 8))
			return i;

	return -1;
}

static int FindWadClusterInfo (int cluster)
{
	for (size_t i = 0; i < numwadclusterinfos; i++)
		if (wadclusterinfos[i].cluster == cluster)
			return i;

	return -1;
}

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

	// nextmap cvar may be overridden by a script
	nextmap.ForceSet(d_mapname);
}


BEGIN_COMMAND (map)
{
	if (argc > 1)
	{
		// [Dash|RD] -- We can make a safe assumption that the user might not specify
		//              the whole lumpname for the level, and might opt for just the
		//              number. This makes sense, so why isn't there any code for it?
		if (W_CheckNumForName (argv[1]) == -1)
		{ // The map name isn't valid, so lets try to make some assumptions for the user.
			char mapname[32];

			// If argc is 2, we assume Doom 2/Final Doom. If it's 3, Ultimate Doom.
			if ( argc == 2 )
			{
				sprintf( mapname, "MAP%02i", atoi( argv[1] ) );
			}
			else if ( argc == 3 )
			{
				sprintf( mapname, "E%iM%i", atoi( argv[1] ), atoi( argv[2] ) );
			}

			if (W_CheckNumForName (mapname) == -1)
			{ // Still no luck, oh well.
				Printf (PRINT_HIGH, "Map %s not found.\n", argv[1]);
			}
			else
			{ // Success
				unnatural_level_progression = true;
				G_DeferedInitNew (mapname);
			}

		}
		else
		{
			unnatural_level_progression = true;
			G_DeferedInitNew (argv[1]);
		}
	}
	else
	{
		Printf (PRINT_HIGH, "The current map is %s: \"%s\"\n", level.mapname, level.level_name);
	}
}
END_COMMAND (map)


const char* GetBase(const char* in)
{
	const char* out = &in[strlen(in) - 1];
	
	while (out > in && *(out-1) != '/' && *(out-1) != '\\')
		out--;
	
	return out;
}

BEGIN_COMMAND (wad) // denis - changes wads
{
	std::vector<std::string> wads, patch_files, hashes;
	bool AddedIWAD = false;
	bool Reboot = false;
	QWORD i, j;
	
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
    
    // add our iwad if it is one
	if (W_IsIWAD(argv[1]))
	{
		wads.push_back(argv[1]);
		AddedIWAD = true;
	}

    // check whether they are wads or patch files
	for (i = 1; i < argc; i++)
	{
		std::string ext;
		
		if (M_ExtractFileExtension(argv[i], ext))
		{
		    // don't allow subsequent iwads to be loaded
		    if ((ext == "wad") && !W_IsIWAD(argv[i]))
                wads.push_back(argv[i]);
            else if (ext == "deh" || ext == "bex")
                patch_files.push_back(argv[i]);
		}
	}
	
	// GhostlyDeath <August 14, 2008> -- Check our environment, if the same WADs are used, ignore this command
	if (AddedIWAD)
	{
		if (stricmp(GetBase(wads[0].c_str()), GetBase(wadfiles[1].c_str())) != 0)
			Reboot = true;
	}
	
	// IWAD, odamex.wad, ...
	if (!Reboot)
	{
		Reboot = true;
		
		for (i = 2, j = (AddedIWAD ? 1 : 0); i < wadfiles.size() && j < wads.size(); i++, j++)
		{
			if (stricmp(GetBase(wads[j].c_str()), GetBase(wadfiles[i].c_str())) == 0)
				Reboot = false;
			else if (Reboot)
			{
				Reboot = true;
				break;
			}
		}
		
		// May be more wads...
		if ((j == wads.size() && i < wadfiles.size()) ||
			(j < wads.size() && i == wadfiles.size()))
			Reboot = true;
	}
	
	if (Reboot)
	{
		D_DoomWadReboot(wads, patch_files);

		unnatural_level_progression = true;
		G_DeferedInitNew (startmap);
	}
}
END_COMMAND (wad)


// Handle map cycling.
struct maplist_s
{
	char *MapName;
	char *WadCmds;

	struct maplist_s *Next;
};

struct maplist_s *MapListBegin = NULL;
struct maplist_s *MapListEnd = NULL;
struct maplist_s *MapListPointer = NULL;

// GhostlyDeath <August 14, 2008> -- Random Map List
std::vector<maplist_s*> RandomMaps;
size_t RandomMapPos = 0;

void G_ClearRandomMaps(void)
{
	RandomMaps.clear();
	RandomMapPos = 0;
}

void G_GenerateRandomMaps(void)
{
	bool* Used = NULL;
	size_t Count = 0;
	maplist_s* Rover = NULL;
	size_t i, j;
	std::vector<maplist_s*> Ptrs;
	
	// Clear old map list
	G_ClearRandomMaps();
	
	if (!MapListBegin)
		return;
	
	// First count the number of entries in the map list
	Rover = MapListBegin;
	
	while (Rover)
	{
		Count++;
		Ptrs.push_back(Rover);
		Rover = Rover->Next;
		
		if (Rover == MapListBegin)
			break;
	}
	
	if (Count <= 0)
		return;
	
	// Allocate our bool array
	Used = new bool[Count];
	
	for (i = 0; i < Count; i++)
		Used[i] = 0;
	
	// Now populate the list
	for (i = 0; i < Count; i++)
	{
		j = (M_Random() + M_Random()) % Count;
		
		// Move forward if j is used
		while (Used[j])
		{
			j++;
			
			if (j == Count)
				j = 0;
		}
		
		// Add it...
		RandomMaps.push_back(Ptrs[j]);
		
		// Marked used
		Used[j] = true;
	}
	
	delete [] Used;
	
	RandomMapPos = 0;
}

CVAR_FUNC_IMPL (shufflemaplist)
{
	// Create random list
	if (var)
		G_GenerateRandomMaps();
	// Erase random list...
	else
		G_ClearRandomMaps();
}

BEGIN_COMMAND (addmap)
{
	if (argc > 1)
	{
        struct maplist_s *NewMap;
        struct maplist_s *OldMap = NULL;

		// Initalize the structure
        NewMap = (struct maplist_s *) Malloc(sizeof(struct maplist_s));
        NewMap->WadCmds = NULL;

        // Add it to our linked list
        if ( MapListBegin == NULL )
        { // This is the first entry
            MapListEnd = MapListBegin = MapListPointer = NewMap->Next = NewMap;
            OldMap = NULL;
        }
        else
        { // Tag it on to the end.
        	OldMap = MapListEnd;
            MapListEnd->Next = NewMap;
            MapListEnd = NewMap;
            NewMap->Next = MapListBegin;
        }

        // Fill in MapName
        NewMap->MapName = (char *) Malloc(strlen(argv[1])+1);
        NewMap->MapName[strlen(argv[1])] = '\0';
        strcpy(NewMap->MapName, argv[1]);

        // Any more arguments are passed to the wad ccmd
        if ( argc > 2 )
        {
            std::string arglist = "wad ";
            
            for (size_t i = 2; i < argc; ++i)
            {
                arglist += argv[i];
                arglist += ' ';
            }
				
            NewMap->WadCmds = (char *) Malloc(strlen(arglist.c_str())+1);
            NewMap->WadCmds[strlen(arglist.c_str())] = '\0';
            strcpy(NewMap->WadCmds, arglist.c_str());
        }
        else// if ( NewMap == MapListBegin )
        {
			// GhostlyDeath <August 14, 2008> -- Changed logic, remember WAD
			if (OldMap)
			{
				NewMap->WadCmds = (char *) Malloc(strlen(OldMap->WadCmds)+1);
				NewMap->WadCmds[strlen(OldMap->WadCmds)] = '\0';
				strcpy(NewMap->WadCmds, OldMap->WadCmds);
			}
			else
			{
				NewMap->WadCmds = (char *) Malloc(2);
				NewMap->WadCmds[0] = '-';
				NewMap->WadCmds[1] = '\0';
			}
        }
        
        // GhostlyDeath <August 14, 2008> -- Regenerate New Map List
        if (shufflemaplist)
	        G_GenerateRandomMaps();
	}
}
END_COMMAND (addmap)

BEGIN_COMMAND (maplist)
{
	if ( MapListBegin == NULL )
	{
		Printf( PRINT_HIGH, "Map list is empty.\n" );
		return;
	}

	struct maplist_s *Iterator = MapListBegin;

	while ( 1 )
	{
		if ( Iterator->WadCmds )
			Printf( PRINT_HIGH, "-> Wad: %s\n", Iterator->WadCmds);
		Printf( PRINT_HIGH, " ^ Map: %s\n", Iterator->MapName);

		Iterator = Iterator->Next;
		if ( Iterator == MapListBegin ) // Looped back to the beginning.
			break;
	}
}
END_COMMAND (maplist)

BEGIN_COMMAND (clearmaplist)
{
	if ( MapListBegin == NULL )
	{
		Printf( PRINT_HIGH, "Map list is empty.\n" );
		return;
	}

	MapListPointer = MapListBegin;

	// Rip the ends off the linked list.
	MapListEnd->Next = NULL;
	MapListEnd = NULL;
	MapListBegin = NULL;

	struct maplist_s *NextPointer = MapListPointer->Next;

	// Crawl through the linked list zapping entries.
	while ( 1 )
	{
		M_Free( MapListPointer->MapName );
		M_Free( MapListPointer->WadCmds );

		M_Free( MapListPointer );

		if ( NextPointer == NULL )
			break; // The linked list is dead.

		MapListPointer = NextPointer;
		NextPointer = NextPointer->Next;
	}

	MapListPointer = NULL; // make sure
	
	G_ClearRandomMaps();
}
END_COMMAND (clearmaplist)

BEGIN_COMMAND (nextmap)
{
	if (!MapListPointer)
	{
		Printf( PRINT_HIGH, "Map list is empty.\n" );
		return;
	}

	G_ExitLevel (0, 1);
}
END_COMMAND (nextmap)

BEGIN_COMMAND (forcenextmap)
{
	if (!MapListPointer)
	{
		Printf( PRINT_HIGH, "Map list is empty.\n" );
		return;
	}

	G_ChangeMap ();
}
END_COMMAND (forcenextmap)

BOOL 			secretexit;
static int		startpos;	// [RH] Support for multiple starts per level

void G_ChangeMap (void)
{
	unnatural_level_progression = false;

	if (!MapListPointer)
	{
		char *next = level.nextmap;

        // for latched "deathmatch 0" cvar
        if (gamestate == GS_STARTUP)
        {
            next = level.mapname;
        }

		// if deathmatch, stay on same level
		if(gametype != GM_COOP)
			next = level.mapname;
		else
			if(secretexit && W_CheckNumForName (level.secretmap) != -1)
				next = level.secretmap;

		if (!strncmp (next, "EndGame", 7))
		{
			// NES - exiting a Doom 1 episode moves to the next episode, rather than always going back to E1M1
			if (gameinfo.flags & GI_MAPxx || gamemode == shareware || (!loopepisode &&
				((gamemode == registered && level.cluster == 3) || (gamemode == retail && level.cluster == 4))))
					next = CalcMapName(1, 1);
				else if (loopepisode)
					next = CalcMapName(level.cluster, 1);
				else
					next = CalcMapName(level.cluster+1, 1);
		}

		G_DeferedInitNew(next);
	}
	else
	{
		if (shufflemaplist && RandomMaps.size())
		{
			// Change the map
			if (RandomMaps[RandomMapPos]->WadCmds)
			{
				if (strcmp(RandomMaps[RandomMapPos]->WadCmds, "-" ) != 0)
					AddCommandString(RandomMaps[RandomMapPos]->WadCmds);
			}
			G_DeferedInitNew(RandomMaps[RandomMapPos]->MapName);
			
			// Increment position
			RandomMapPos++;
			
			// If our counter has reached it's end, regenerate the map
			if (RandomMapPos >= RandomMaps.size())
				G_GenerateRandomMaps();
		}
		else
		{
			if ( MapListPointer->WadCmds )
			{
				if ( strcmp( MapListPointer->WadCmds, "-" ) != 0 )
					AddCommandString(MapListPointer->WadCmds);
			}
            
            char *next = MapListPointer->MapName;
            
            // for latched "deathmatch 0" cvar
            if (gamestate == GS_STARTUP)
            {
                next = level.mapname;
            }
                       
			G_DeferedInitNew(next);
			MapListPointer = MapListPointer->Next;
		}
	}

	// run script at the end of each map
	if(strlen(endmapscript.cstring()))
		AddCommandString(endmapscript.cstring(), true);
}

void SV_ClientFullUpdate(player_t &pl);
void SV_CheckTeam(player_t &pl);
void G_DoReborn(player_t &playernum);
void SV_SendServerSettings(client_t *cl);

//
// G_DoNewGame
//
// denis - rewritten so that it does not force client reconnects
//
void G_DoNewGame (void)
{
	size_t i;

	for(i = 0; i < players.size(); i++)
	{
		if(!players[i].ingame())
			continue;

		client_t *cl = &clients[i];

		MSG_WriteMarker   (&cl->reliablebuf, svc_loadmap);
		MSG_WriteString (&cl->reliablebuf, d_mapname);
	}

	curmap.ForceSet(d_mapname);

	G_InitNew (d_mapname);
	gameaction = ga_nothing;

	// run script at the start of each map
	if(strlen(startmapscript.cstring()))
		AddCommandString(startmapscript.cstring(), true);

	for(i = 0; i < players.size(); i++)
	{
		if(!players[i].ingame())
			continue;

		if (gametype == GM_TEAMDM || gametype == GM_CTF)
			SV_CheckTeam(players[i]);
		else
			players[i].userinfo.color = players[i].prefcolor;
		
		SV_ClientFullUpdate (players[i]);
	}
}

EXTERN_CVAR (skill)
EXTERN_CVAR (monstersrespawn)
EXTERN_CVAR (fastmonsters)
EXTERN_CVAR (maxplayers)

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
		for (i = 0; i < numwadlevelinfos; i++)
			wadlevelinfos[i].flags &= ~LEVEL_VISITED;

		for (i = 0; LevelInfos[i].mapname[0]; i++)
			LevelInfos[i].flags &= ~LEVEL_VISITED;
	}

	int old_gametype = gametype;

	cvar_t::UnlatchCVars ();

	if(old_gametype != gametype || gametype != GM_COOP) {
		unnatural_level_progression = true;
		
		// Nes - Force all players to be spectators when the gametype is not now or previously co-op.
		for (i = 0; i < players.size(); i++) {
			for (size_t j = 0; j < players.size(); j++) {
				MSG_WriteMarker (&(players[j].client.reliablebuf), svc_spectate);
				MSG_WriteByte (&(players[j].client.reliablebuf), players[i].id);
				MSG_WriteByte (&(players[j].client.reliablebuf), true);
			}			
			players[i].spectator = true;
			players[i].playerstate = PST_LIVE;
			players[i].joinafterspectatortime = -(TICRATE*5);
		}
	}

	SV_ServerSettingChange();

	if (paused)
		paused = false;

	// [RH] If this map doesn't exist, bomb out
	if (W_CheckNumForName (mapname) == -1)
	{
		I_Error ("Could not find map %s\n", mapname);
	}

	if (skill == sk_nightmare || monstersrespawn)
		respawnmonsters = true;
	else
		respawnmonsters = false;

	bool wantFast = fastmonsters || (skill == sk_nightmare);
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

	if (!savegamerestore)
	{
		M_ClearRandom ();
		level.time = 0;

		// force players to be initialized upon first level load
		for (size_t i = 0; i < players.size(); i++)
		{
			if(!players[i].ingame())
				continue;

			// denis - dead players should have their stuff looted, otherwise they'd take their ammo into their afterlife!
			if(players[i].playerstate == PST_DEAD)
				G_PlayerReborn(players[i]);

			players[i].playerstate = PST_REBORN;
			
			players[i].joinafterspectatortime = -(TICRATE*5);
		}
	}

	// if only one player allowed, then this is a single player server
	if(maxplayers == 1)
		multiplayer = false;
	else
		multiplayer = true;

	usergame = true;				// will be set false if a demo
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

	gamestate = GS_INTERMISSION;
	shotclock = 0;
	mapchange = TICRATE*10;  // wait 10 seconds

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

	gamestate = GS_INTERMISSION;
	shotclock = 0;
	mapchange = TICRATE*10;  // wait 10 seconds

	// IF NO WOLF3D LEVELS, NO SECRET EXIT!
	if ( (gamemode == commercial)
	&& (W_CheckNumForName("map31")<0))
		secretexit = false;
	else
		secretexit = true;

    gameaction = ga_completed;

	// denis - this will skip wi_stuff and allow some time for finale text
	//G_WorldDone();
}

void G_DoCompleted (void)
{
	size_t i;

	gameaction = ga_nothing;

	for(i = 0; i < players.size(); i++)
		if(players[i].ingame())
			G_PlayerFinishLevel(players[i]);
}

//
// G_DoLoadLevel
//
extern gamestate_t 	wipegamestate;
extern float BaseBlendA;

void G_DoLoadLevel (int position)
{
	static int lastposition = 0;
	size_t i;
	
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

//	if (demoplayback || oldgs == GS_STARTUP)
//		C_HideConsole ();

	// Set the sky map.
	// First thing, we have a dummy sky texture name,
	//	a flat. The data is in the WAD only because
	//	we look for an actual index, instead of simply
	//	setting one.
	skyflatnum = R_FlatNumForName ( SKYFLATNAME );

	// DOOM determines the sky texture to be used
	// depending on the current episode, and the game version.
	// [RH] Fetch sky parameters from level_locals_t.
	skytexture = R_TextureNumForName (level.skypic);

	for (i = 0; i < players.size(); i++)
	{
		if (players[i].ingame() && players[i].playerstate == PST_DEAD)
			players[i].playerstate = PST_REBORN;

		players[i].fragcount = 0;
		players[i].deathcount = 0; // [Toke - Scores - deaths]
		players[i].killcount = 0; // [deathz0r] Coop kills
		players[i].points = 0;
	}

	// [deathz0r] It's a smart idea to reset the team points
	if (gametype == GM_TEAMDM || gametype == GM_CTF)
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
	for (i = 0; i < players.size(); i++)
		players[i].joinafterspectatortime -= level.time;

	flagdata *tempflag;

	// Nes - CTF Pre flag setup
	if (gametype == GM_CTF) {
		tempflag = &CTFdata[it_blueflag];
		tempflag->flaglocated = false;
		
		tempflag = &CTFdata[it_redflag];
		tempflag->flaglocated = false;
	}

	P_SetupLevel (level.mapname, position);
	
	// Nes - CTF Post flag setup
	if (gametype == GM_CTF) {
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

	// clear cmd building stuff // denis - todo - could we get rid of this?
	Impulse = 0;
	for (i = 0; i < NUM_ACTIONS; i++)
		if (i != ACTION_MLOOK && i != ACTION_KLOOK)
			Actions[i] = 0;

	joyxmove = joyymove = 0;
	mousex = mousey = 0;
	sendpause = sendsave = paused = sendcenterview = false;

	if (timingdemo) {
		static BOOL firstTime = true;

		if (firstTime) {
			starttime = I_GetTimePolled ();
			firstTime = false;
		}
	}

	level.starttime = I_GetTime ();
	G_UnSnapshotLevel (!savegamerestore);	// [RH] Restore the state of the level.
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
	if (!strncmp (level.nextmap, "EndGame", 7)) {
//		F_StartFinale (thiscluster->messagemusic, thiscluster->finaleflat, thiscluster->exittext); // denis - fixme - what should happen on the server?
		finaletext = thiscluster->exittext;
	} else {
		if (!secretexit)
			nextcluster = FindClusterInfo (FindLevelInfo (level.nextmap)->cluster);
		else
			nextcluster = FindClusterInfo (FindLevelInfo (level.secretmap)->cluster);

		if (nextcluster->cluster != level.cluster && gametype == GM_COOP) {
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

void G_DoWorldDone (void)
{
	gamestate = GS_LEVEL;
	if (wminfo.next[0] == 0) {
		// Don't die if no next map is given,
		// just repeat the current one.
		Printf (PRINT_HIGH, "No next map specified.\n");
	} else {
		strncpy (level.mapname, wminfo.next, 8);
	}
	G_DoLoadLevel (startpos);
	startpos = 0;
	gameaction = ga_nothing;
	viewactive = true;
}


extern dyncolormap_t NormalLight;

void G_InitLevelLocals ()
{
//	unsigned long oldfade = level.fadeto;
	level_info_t *info;
	int i;

	NormalLight.maps = realcolormaps;

	if ((i = FindWadLevelInfo (level.mapname)) > -1) {
		level_pwad_info_t *pinfo = wadlevelinfos + i;

		// [ML] 5/11/06 - Remove sky scrolling and sky2
		level.info = (level_info_t *)pinfo;
		info = (level_info_t *)pinfo;
		level.fadeto = pinfo->fadeto;
		if (level.fadeto) {
//			NormalLight.maps = DefaultPalette->maps.colormaps;
		} else {
//			R_SetDefaultColormap (pinfo->fadetable);
		}
		level.outsidefog = pinfo->outsidefog;
		level.flags |= LEVEL_DEFINEDINMAPINFO;
	} else {
		info = FindDefLevelInfo (level.mapname);
		level.info = info;
		level.fadeto = 0;
		level.outsidefog = 0xff000000;	// 0xff000000 signals not to handle it special
		R_SetDefaultColormap ("COLORMAP");
	}

	if (info->level_name) {
		level.partime = info->partime;
		level.cluster = info->cluster;
		level.flags = info->flags;
		level.levelnum = info->levelnum;

		strncpy (level.level_name, info->level_name, 63);
		strncpy (level.nextmap, info->nextmap, 8);
		strncpy (level.secretmap, info->secretmap, 8);
		strncpy (level.music, info->music, 8);
		strncpy (level.skypic, info->skypic, 8);
	} else {
		level.partime = level.cluster = 0;
		strcpy (level.level_name, "Unnamed");
		level.nextmap[0] =
			level.secretmap[0] =
			level.music[0] = 0;
			strncpy (level.skypic, "SKY1", 8);
		level.flags = 0;
		level.levelnum = 1;
	}
//  [deathz0r] Doesn't appear to affect client
//	if (oldfade != level.fadeto)
//		RefreshPalettes ();
}

char *CalcMapName (int episode, int level)
{
	static char lumpname[9];

	if (gameinfo.flags & GI_MAPxx)
	{
		sprintf (lumpname, "MAP%02d", level);
	}
	else
	{
		lumpname[0] = 'E';
		lumpname[1] = '0' + episode;
		lumpname[2] = 'M';
		lumpname[3] = '0' + level;
		lumpname[4] = 0;
	}
	return lumpname;
}

static level_info_t *FindDefLevelInfo (char *mapname)
{
	level_info_t *i;

	i = LevelInfos;
	while (i->level_name) {
		if (!strnicmp (i->mapname, mapname, 8))
			break;
		i++;
	}
	return i;
}

level_info_t *FindLevelInfo (char *mapname)
{
	int i;

	if ((i = FindWadLevelInfo (mapname)) > -1)
		return (level_info_t *)(wadlevelinfos + i);
	else
		return FindDefLevelInfo (mapname);
}

level_info_t *FindLevelByNum (int num)
{
	{
		for (size_t i = 0; i < numwadlevelinfos; i++)
			if (wadlevelinfos[i].levelnum == num)
				return (level_info_t *)(wadlevelinfos + i);
	}
	{
		level_info_t *i = LevelInfos;
		while (i->level_name) {
			if (i->levelnum == num)
				return i;
			i++;
		}
		return NULL;
	}
}

static cluster_info_t *FindDefClusterInfo (int cluster)
{
	cluster_info_t *i;

	i = ClusterInfos;
	while (i->cluster && i->cluster != cluster)
		i++;

	return i;
}

cluster_info_t *FindClusterInfo (int cluster)
{
	int i;

	if ((i = FindWadClusterInfo (cluster)) > -1)
		return wadclusterinfos + i;
	else
		return FindDefClusterInfo (cluster);
}

void G_SetLevelStrings (void)
{
	char temp[8];
	const char *namepart;
	int i, start;

	temp[0] = '0';
	temp[1] = ':';
	temp[2] = 0;
	for (i = 65; i < 101; i++) {		// HUSTR_E1M1 .. HUSTR_E4M9
		if (temp[0] < '9')
			temp[0]++;
		else
			temp[0] = '1';

		if ( (namepart = strstr (Strings[i].string, temp)) ) {
			namepart += 2;
			while (*namepart && *namepart <= ' ')
				namepart++;
		} else {
			namepart = Strings[i].string;
		}

		ReplaceString ((const char **)&LevelInfos[i-65].level_name, namepart);
	}

	for (i = 0; i < 4; i++)
		ReplaceString ((const char **)&ClusterInfos[i].exittext, Strings[221+i].string);

	if (gamemission == pack_plut)
		start = 133;		// PHUSTR_1
	else if (gamemission == pack_tnt)
		start = 165;		// THUSTR_1
	else
		start = 101;		// HUSTR_1

	for (i = 0; i < 32; i++) {
		sprintf (temp, "%d:", i + 1);
		if ( (namepart = strstr (Strings[i+start].string, temp)) ) {
			namepart += strlen (temp);
			while (*namepart && *namepart <= ' ')
				namepart++;
		} else {
			namepart = Strings[i+start].string;
		}
		ReplaceString ((const char **)&LevelInfos[36+i].level_name, namepart);
	}

	if (gamemission == pack_plut)
		start = 231;		// P1TEXT
	else if (gamemission == pack_tnt)
		start = 237;		// T1TEXT
	else
		start = 225;		// C1TEXT

	for (i = 0; i < 4; i++)
		ReplaceString ((const char **)&ClusterInfos[4 + i].exittext, Strings[start+i].string);
	for (; i < 6; i++)
		ReplaceString ((const char **)&ClusterInfos[4 + i].entertext, Strings[start+i].string);

	if (level.info)
		strncpy (level.level_name, level.info->level_name, 63);
}


void G_SerializeLevel (FArchive &arc, bool hubLoad)
{
	if (arc.IsStoring ())
	{
		arc << level.flags
			<< level.fadeto
			<< level.found_secrets
			<< level.found_items
			<< level.killed_monsters;

//		for (i = 0; i < NUM_MAPVARS; i++)
//			arc << level.vars[i];
	}
	else
	{
		arc >> level.flags
			>> level.fadeto
			>> level.found_secrets
			>> level.found_items
			>> level.killed_monsters;

//		for (i = 0; i < NUM_MAPVARS; i++)
//			arc >> level.vars[i];
	}
	P_SerializeThinkers (arc, hubLoad);
	P_SerializeWorld (arc);
	P_SerializeSounds (arc);
	if (!hubLoad)
		P_SerializePlayers (arc);
}

// Archives the current level
void G_SnapshotLevel ()
{
	delete level.info->snapshot;

	level.info->snapshot = new FLZOMemFile;
	level.info->snapshot->Open ();

	FArchive arc (*level.info->snapshot);

	G_SerializeLevel (arc, false);
}

// Unarchives the current level based on its snapshot
// The level should have already been loaded and setup.
void G_UnSnapshotLevel (bool hubLoad)
{
	if (level.info->snapshot == NULL)
		return;

	level.info->snapshot->Reopen ();
	FArchive arc (*level.info->snapshot);
	if (hubLoad)
		arc.SetHubTravel ();
	G_SerializeLevel (arc, hubLoad);
	arc.Close ();
	// No reason to keep the snapshot around once the level's been entered.
	delete level.info->snapshot;
	level.info->snapshot = NULL;
}

void G_ClearSnapshots (void)
{
	size_t i;

	for (i = 0; i < numwadlevelinfos; i++)
		if (wadlevelinfos[i].snapshot)
		{
			delete wadlevelinfos[i].snapshot;
			wadlevelinfos[i].snapshot = NULL;
		}

	for (i = 0; LevelInfos[i].level_name; i++)
		if (LevelInfos[i].snapshot)
		{
			delete LevelInfos[i].snapshot;
			LevelInfos[i].snapshot = NULL;
		}
}

static void writeSnapShot (FArchive &arc, level_info_t *i)
{
	arc.Write (i->mapname, 8);
	i->snapshot->Serialize (arc);
}

void G_SerializeSnapshots (FArchive &arc)
{
	if (arc.IsStoring ())
	{
		size_t i;

		for (i = 0; i < numwadlevelinfos; i++)
			if (wadlevelinfos[i].snapshot)
				writeSnapShot (arc, (level_info_s *)&wadlevelinfos[i]);

		for (i = 0; LevelInfos[i].level_name; i++)
			if (LevelInfos[i].snapshot)
				writeSnapShot (arc, &LevelInfos[i]);

		// Signal end of snapshots
		arc << (char)0;
	}
	else
	{
		char mapname[8];

		G_ClearSnapshots ();

		arc >> mapname[0];
		while (mapname[0])
		{
			arc.Read (&mapname[1], 7);
			level_info_t *i = FindLevelInfo (mapname);
			i->snapshot = new FLZOMemFile;
			i->snapshot->Serialize (arc);
			arc >> mapname[0];
		}
	}
}

// Static level info from original game
// The level names and cluster messages get filled in
// by G_SetLevelStrings().

level_info_t LevelInfos[] = {
	// Registered/Retail Episode 1
	{
		"E1M1",
		1,
		NULL,
		"WILV00",
		"E1M2",
		"E1M9",
		30,
		"SKY1",
		"D_E1M1",
		0,
		1,
		0
	},
	{
		"E1M2",
		2,
		NULL,
		"WILV01",
		"E1M3",
		"E1M9",
		75,
		"SKY1",
		"D_E1M2",
		0,
		1,
		0
	},
	{
		"E1M3",
		3,
		NULL,
		"WILV02",
		"E1M4",
		"E1M9",
		120,
		"SKY1",
		"D_E1M3",
		0,
		1,
		0
	},
	{
		"E1M4",
		4,
		NULL,
		"WILV03",
		"E1M5",
		"E1M9",
		90,
		"SKY1",
		"D_E1M4",
		0,
		1,
		0
	},
	{
		"E1M5",
		5,
		NULL,
		"WILV04",
		"E1M6",
		"E1M9",
		165,
		"SKY1",
		"D_E1M5",
		0,
		1,
		0
	},
	{
		"E1M6",
		6,
		NULL,
		"WILV05",
		"E1M7",
		"E1M9",
		180,
		"SKY1",
		"D_E1M6",
		0,
		1,
		0
	},
	{
		"E1M7",
		7,
		NULL,
		"WILV06",
		"E1M8",
		"E1M9",
		180,
		"SKY1",
		"D_E1M7",
		0,
		1,
		0
	},
	{
		"E1M8",
		8,
		NULL,
		"WILV07",
		"EndGame1",
//		{ 'E','n','d','G','a','m','e','1' },
		"E1M9",
		30,
		"SKY1",
		"D_E1M8",
		LEVEL_NOINTERMISSION|LEVEL_NOSOUNDCLIPPING|LEVEL_BRUISERSPECIAL|LEVEL_SPECLOWERFLOOR,
		1,
		0
	},
	{
		"E1M9",
		9,
		NULL,
		"WILV08",
		"E1M4",
		"E1M4",
		165,
		"SKY1",
		"D_E1M9",
		0,
		1,
		0
	},

	// Registered/Retail Episode 2
	{
		"E2M1",
		11,
		NULL,
		"WILV10",
		"E2M2",
		"E2M9",
		90,
		"SKY2",
		"D_E2M1",
		0,
		2,
		0
	},

	{
		"E2M2",
		12,
		NULL,
		"WILV11",
		"E2M3",
		"E2M9",
		90,
		"SKY2",
		"D_E2M2",
		0,
		2,
		0
	},
	{
		"E2M3",
		13,
		NULL,
		"WILV12",
		"E2M4",
		"E2M9",
		90,
		"SKY2",
		"D_E2M3",
		0,
		2,
		0
	},
	{
		"E2M4",
		14,
		NULL,
		"WILV13",
		"E2M5",
		"E2M9",
		120,
		"SKY2",
		"D_E2M4",
		0,
		2,
		0
	},
	{
		"E2M5",
		15,
		NULL,
		"WILV14",
		"E2M6",
		"E2M9",
		90,
		"SKY2",
		"D_E2M5",
		0,
		2,
		0
	},
	{
		"E2M6",
		16,
		NULL,
		"WILV15",
		"E2M7",
		"E2M9",
		360,
		"SKY2",
		"D_E2M6",
		0,
		2,
		0
	},
	{
		"E2M7",
		17,
		NULL,
		"WILV16",
		"E2M8",
		"E2M9",
		240,
		"SKY2",
		"D_E2M7",
		0,
		2,
		0
	},
	{
		"E2M8",
		18,
		NULL,
		"WILV17",
		"EndGame2",
//		{ 'E','n','d','G','a','m','e','2' },
		"E2M9",
		30,
		"SKY2",
		"D_E2M8",
		LEVEL_NOINTERMISSION|LEVEL_NOSOUNDCLIPPING|LEVEL_CYBORGSPECIAL,
		2,
		0
	},
	{
		"E2M9",
		19,
		NULL,
		"WILV18",
		"E2M6",
		"E2M6",
		170,
		"SKY2",
		"D_E2M9",
		0,
		2,
		0
	},

	// Registered/Retail Episode 3

	{
		"E3M1",
		21,
		NULL,
		"WILV20",
		"E3M2",
		"E3M9",
		90,
		"SKY3",
		"D_E3M1",
		0,
		3,
		0
	},
	{
		"E3M2",
		22,
		NULL,
		"WILV21",
		"E3M3",
		"E3M9",
		45,
		"SKY3",
		"D_E3M2",
		0,
		3,
		0
	},
	{
		"E3M3",
		23,
		NULL,
		"WILV22",
		"E3M4",
		"E3M9",
		90,
		"SKY3",
		"D_E3M3",
		0,
		3,
		0
	},
	{
		"E3M4",
		24,
		NULL,
		"WILV23",
		"E3M5",
		"E3M9",
		150,
		"SKY3",
		"D_E3M4",
		0,
		3,
		0
	},
	{
		"E3M5",
		25,
		NULL,
		"WILV24",
		"E3M6",
		"E3M9",
		90,
		"SKY3",
		"D_E3M5",
		0,
		3,
		0
	},
	{
		"E3M6",
		26,
		NULL,
		"WILV25",
		"E3M7",
		"E3M9",
		90,
		"SKY3",
		"D_E3M6",
		0,
		3,
		0
	},
	{
		"E3M7",
		27,
		NULL,
		"WILV26",
		"E3M8",
		"E3M9",
		165,
		"SKY3",
		"D_E3M7",
		0,
		3,
		0
	},
	{
		"E3M8",
		28,
		NULL,
		"WILV27",
		"EndGame3",
//		{ 'E','n','d','G','a','m','e','3' },
		"E3M9",
		30,
		"SKY3",
		"D_E3M8",
		LEVEL_NOINTERMISSION|LEVEL_NOSOUNDCLIPPING|LEVEL_SPIDERSPECIAL,
		3,
		0
	},
	{
		"E3M9",
		29,
		NULL,
		"WILV28",
		"E3M7",
		"E3M7",
		135,
		"SKY3",
		"D_E3M9",
		0,
		3,
		0
	},

	// Retail Episode 4
	{
		"E4M1",
		31,
		NULL,
		"WILV30",
		"E4M2",
		"E4M9",
		0,
		"SKY4",
		"D_E3M4",
		0,
		4,
		0
	},
	{
		"E4M2",
		32,
		NULL,
		"WILV31",
		"E4M3",
		"E4M9",
		0,
		"SKY4",
		"D_E3M2",
		0,
		4,
		0
	},
	{
		"E4M3",
		33,
		NULL,
		"WILV32",
		"E4M4",
		"E4M9",
		0,
		"SKY4",
		"D_E3M3",
		0,
		4,
		0
	},
	{
		"E4M4",
		34,
		NULL,
		"WILV33",
		"E4M5",
		"E4M9",
		0,
		"SKY4",
		"D_E1M5",
		0,
		4,
		0
	},
	{
		"E4M5",
		35,
		NULL,
		"WILV34",
		"E4M6",
		"E4M9",
		0,
		"SKY4",
		"D_E2M7",
		0,
		4,
		0
	},
	{
		"E4M6",
		36,
		NULL,
		"WILV35",
		"E4M7",
		"E4M9",
		0,
		"SKY4",
		"D_E2M4",
		LEVEL_CYBORGSPECIAL|LEVEL_SPECOPENDOOR,
		4,
		0
	},
	{
		"E4M7",
		37,
		NULL,
		"WILV36",
		"E4M8",
		"E4M9",
		0,
		"SKY4",
		"D_E2M6",
		0,
		4,
		0
	},
	{
		"E4M8",
		38,
		NULL,
		"WILV37",
		"EndGame4",
//		{ 'E','n','d','G','a','m','e','4' },
		"E4M9",
		0,
		"SKY4",
		"D_E2M5",
		LEVEL_NOINTERMISSION|LEVEL_NOSOUNDCLIPPING|LEVEL_SPIDERSPECIAL|LEVEL_SPECLOWERFLOOR,
		4,
		0
	},
	{
		"E4M9",
		39,
		NULL,
		"WILV38",
		"E4M3",
		"E4M3",
		0,
		"SKY4",
		"D_E1M9",
		0,
		4,
		0
	},

	// DOOM 2 Levels

	{
		"MAP01",
		1,
		NULL,
		"CWILV00",
		"MAP02",
		"MAP02",
		30,
		"SKY1",
        "D_RUNNIN",
//		{ 'D','_','R','U','N','N','I','N' },
		0,
		5,
		0
	},
	{
		"MAP02",
		2,
		NULL,
		"CWILV01",
		"MAP03",
		"MAP03",
		90,
		"SKY1",
		"D_STALKS",
//		{ 'D','_','S','T','A','L','K','S' },
		0,
		5,
		0
	},
	{
		"MAP03",
		3,
		NULL,
		"CWILV02",
		"MAP04",
		"MAP04",
		120,
		"SKY1",
        "D_COUNTD",
//		{ 'D','_','C','O','U','N','T','D' },
		0,
		5,
		0
	},
	{
		"MAP04",
		4,
		NULL,
		"CWILV03",
		"MAP05",
		"MAP05",
		120,
		"SKY1",
		"D_BETWEE",
//		{ 'D','_','B','E','T','W','E','E' },
		0,
		5,
		0
	},
	{
		"MAP05",
		5,
		NULL,
		"CWILV04",
		"MAP06",
		"MAP06",
		90,
		"SKY1",
		"D_DOOM",
		0,
		5,
		0
	},
	{
		"MAP06",
		6,
		NULL,
		"CWILV05",
		"MAP07",
		"MAP07",
		150,
		"SKY1",
		"D_THE_DA",
//		{ 'D','_','T','H','E','_','D','A' },
		0,
		5,
		0
	},
	{
		"MAP07",
		7,
		NULL,
		"CWILV06",
		"MAP08",
		"MAP08",
		120,
		"SKY1",
		"D_SHAWN",
		LEVEL_MAP07SPECIAL,
		6,
		0
	},
	{
		"MAP08",
		8,
		NULL,
		"CWILV07",
		"MAP09",
		"MAP09",
		120,
		"SKY1",
		"D_DDTBLU",
//		{ 'D','_','D','D','T','B','L','U' },
		0,
		6,
		0
	},
	{
		"MAP09",
		9,
		NULL,
		"CWILV08",
		"MAP10",
		"MAP10",
		270,
		"SKY1",
        "D_IN_CIT",
//		{ 'D','_','I','N','_','C','I','T' },
		0,
		6,
		0
	},
	{
		"MAP10",
		10,
		NULL,
		"CWILV09",
		"MAP11",
		"MAP11",
		90,
		"SKY1",
		"D_DEAD",
		0,
		6,
		0
	},
	{
		"MAP11",
		11,
		NULL,
		"CWILV10",
		"MAP12",
		"MAP12",
		210,
		"SKY1",
        "D_STLKS2",
//		{ 'D','_','S','T','L','K','S','2' },
		0,
		6,
		0
	},
	{
		"MAP12",
		12,
		NULL,
		"CWILV11",
		"MAP13",
		"MAP13",
		150,
		"SKY2",
		"D_THEDA2",
//		{ 'D','_','T','H','E','D','A','2' },
		0,
		7,
		0
	},
	{
		"MAP13",
		13,
		NULL,
		"CWILV12",
		"MAP14",
		"MAP14",
		150,
		"SKY2",
		"D_DOOM2",
		0,
		7,
		0
	},
	{
		"MAP14",
		14,
		NULL,
		"CWILV13",
		"MAP15",
		"MAP15",
		150,
		"SKY2",
		"D_DDTBL2",
//		{ 'D','_','D','D','T','B','L','2' },
		0,
		7,
		0
	},
	{
		"MAP15",
		15,
		NULL,
		"CWILV14",
		"MAP16",
		"MAP31",
		210,
		"SKY2",
		"D_RUNNI2",
//		{ 'D','_','R','U','N','N','I','2' },
		0,
		7,
		0
	},
	{
		"MAP16",
		16,
		NULL,
		"CWILV15",
		"MAP17",
		"MAP17",
		150,
		"SKY2",
		"D_DEAD2",
		0,
		7,
		0
	},
	{
		"MAP17",
		17,
		NULL,
		"CWILV16",
		"MAP18",
		"MAP18",
		420,
		"SKY2",
		"D_STLKS3",
//		{ 'D','_','S','T','L','K','S','3' },
		0,
		7,
		0
	},
	{
		"MAP18",
		18,
		NULL,
		"CWILV17",
		"MAP19",
		"MAP19",
		150,
		"SKY2",
		"D_ROMERO",
//		{ 'D','_','R','O','M','E','R','O' },
		0,
		7,
		0
	},
	{
		"MAP19",
		19,
		NULL,
		"CWILV18",
		"MAP20",
		"MAP20",
		210,
		"SKY2",
		"D_SHAWN2",
//		{ 'D','_','S','H','A','W','N','2' },
		0,
		7,
		0
	},
	{
		"MAP20",
		20,
		NULL,
		"CWILV19",
		"MAP21",
		"MAP21",
		150,
		"SKY2",
		"D_MESSAG",
//		{ 'D','_','M','E','S','S','A','G' },
		0,
		7,
		0
	},
	{
		"MAP21",
		21,
		NULL,
		"CWILV20",
		"MAP22",
		"MAP22",
		240,
		"SKY3",
		"D_COUNT2",
//		{ 'D','_','C','O','U','N','T','2' },
		0,
		8,
		0
	},
	{
		"MAP22",
		22,
		NULL,
		"CWILV21",
		"MAP23",
		"MAP23",
		150,
		"SKY3",
		"D_DDTBL3",
//		{ 'D','_','D','D','T','B','L','3' },
		0,
		8,
		0
	},
	{
		"MAP23",
		23,
		NULL,
		"CWILV22",
		"MAP24",
		"MAP24",
		180,
		"SKY3",
		"D_AMPIE",
		0,
		8,
		0
	},
	{
		"MAP24",
		24,
		NULL,
		"CWILV23",
		"MAP25",
		"MAP25",
		150,
		"SKY3",
		"D_THEDA3",
//		{ 'D','_','T','H','E','D','A','3' },
		0,
		8,
		0
	},
	{
		"MAP25",
		25,
		NULL,
		"CWILV24",
		"MAP26",
		"MAP26",
		150,
		"SKY3",
		"D_ADRIAN",
//		{ 'D','_','A','D','R','I','A','N' },
		0,
		8,
		0
	},
	{
		"MAP26",
		26,
		NULL,
		"CWILV25",
		"MAP27",
		"MAP27",
		300,
		"SKY3",
		"D_MESSG2",
//		{ 'D','_','M','E','S','S','G','2' },
		0,
		8,
		0
	},
	{
		"MAP27",
		27,
		NULL,
		"CWILV26",
		"MAP28",
		"MAP28",
		330,
		"SKY3",
        "D_ROMER2",
//		{ 'D','_','R','O','M','E','R','2' },
		0,
		8,
		0
	},
	{
		"MAP28",
		28,
		NULL,
		"CWILV27",
		"MAP29",
		"MAP29",
		420,
		"SKY3",
		"D_TENSE",
		0,
		8,
		0
	},
	{
		"MAP29",
		29,
		NULL,
		"CWILV28",
		"MAP30",
		"MAP30",
		300,
		"SKY3",
		"D_SHAWN3",
//		{ 'D','_','S','H','A','W','N','3' },
		0,
		8,
		0
	},
	{
		"MAP30",
		30,
		NULL,
		"CWILV29",
        "EndGameC",
        "EndGameC",
//		{ 'E','n','d','G','a','m','e','C' },
//		{ 'E','n','d','G','a','m','e','C' },
		180,
		"SKY3",
		"D_OPENIN",
//		{ 'D','_','O','P','E','N','I','N' },
		LEVEL_MONSTERSTELEFRAG,
		8,
		0
	},
	{
		"MAP31",
		31,
		NULL,
		"CWILV30",
		"MAP16",
		"MAP32",
		120,
		"SKY3",
		"D_EVIL",
		0,
		9,
		0
	},
	{
		"MAP32",
		32,
		NULL,
		"CWILV31",
		"MAP16",
		"MAP16",
		30,
		"SKY3",
		"D_ULTIMA",
//		{ 'D','_','U','L','T','I','M','A' },
		0,
		10,
		0
	},
	{
		"",
		0,
		NULL,
		"",
		"",
		"",
		0,
		"",
		"",
		0,
		0,
		0
	}
};

// Episode/Cluster information
cluster_info_t ClusterInfos[] = {
	{
		1,		// DOOM Episode 1
		"D_VICTOR",
		"FLOOR4_8",
//		{ 'F','L','O','O','R','4','_','8' }, // questionable
		NULL,
		NULL,
		0
	},
	{
		2,		// DOOM Episode 2
		"D_VICTOR",
		"SFLR6_1",
		NULL,
		NULL,
		0
	},
	{
		3,		// DOOM Episode 3
		"D_VICTOR",
		"MFLR8_4",
		NULL,
		NULL,
		0
	},
	{
		4,		// DOOM Episode 4
		"D_VICTOR",
		"MFLR8_3",
		NULL,
		NULL,
		0
	},
	{
		5,		// DOOM II first cluster (up thru level 6)
		"D_READ_M",
		"SLIME16",
		NULL,
		NULL,
		0
	},
	{
		6,		// DOOM II second cluster (up thru level 11)
		"D_READ_M",
		"RROCK14",
		NULL,
		NULL,
		0
	},
	{
		7,		// DOOM II third cluster (up thru level 20)
		"D_READ_M",
		"RROCK07",
		NULL,
		NULL,
		0
	},
	{
		8,		// DOOM II fourth cluster (up thru level 30)
		"D_READ_M",
		"RROCK17",
		NULL,
		NULL,
		0
	},
	{
		9,		// DOOM II fifth cluster (level 31)
		"D_READ_M",
		"RROCK13",
		NULL,
		NULL,
		0
	},
	{
		10,		// DOOM II sixth cluster (level 32)
		"D_READ_M",
		"RROCK19",
		NULL,
		NULL,
		0
	},
	{
		0,
		"",
		"",
		NULL,
		NULL,
		0		// End-of-clusters marker
	}
};

VERSION_CONTROL (g_level_cpp, "$Id$")


