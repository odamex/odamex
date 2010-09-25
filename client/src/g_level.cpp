// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	G_LEVEL
//
//-----------------------------------------------------------------------------

#include "d_main.h"
#include "m_alloc.h"
#include "g_level.h"
#include "g_game.h"
#include "s_sound.h"
#include "d_event.h"
#include "m_random.h"
#include "doomstat.h"
#include "wi_stuff.h"
#include "r_data.h"
#include "w_wad.h"
#include "am_map.h"
#include "c_dispatch.h"
#include "z_zone.h"
#include "i_system.h"
#include "p_setup.h"
#include "p_local.h"
#include "r_sky.h"
#include "c_console.h"
#include "f_finale.h"
#include "dstrings.h"
#include "v_video.h"
#include "st_stuff.h"
#include "hu_stuff.h"
#include "p_saveg.h"
#include "p_acs.h"
#include "d_protocol.h"
#include "v_text.h"
#include "sc_man.h"
#include "cl_main.h"
#include "m_fileio.h"
#include "m_misc.h"

#include "gi.h"
#include "minilzo.h"

#define lioffset(x)		myoffsetof(level_pwad_info_t,x)
#define cioffset(x)		myoffsetof(cluster_info_t,x)

static level_info_t *FindDefLevelInfo (char *mapname);
static cluster_info_t *FindDefClusterInfo (int cluster);

static const char Musics1[48][9] =
{
	"D_E1M1",		"D_E1M2",		"D_E1M3",		"D_E1M4",		"D_E1M5",
	"D_E1M6",		"D_E1M7",		"D_E1M8",		"D_E1M9",		"D_E2M1",
	"D_E2M2",		"D_E2M3",		"D_E2M4",		"D_E2M5",		"D_E2M6",
	"D_E2M7",		"D_E2M8",		"D_E2M9",		"D_E3M1",		"D_E3M2",
	"D_E3M3",		"D_E3M4",		"D_E3M5",		"D_E3M6",		"D_E3M7",
	"D_E3M8",		"D_E3M9",		"D_E3M4",		"D_E3M2",		"D_E3M3",
	"D_E1M5",		"D_E2M7",		"D_E2M4",		"D_E2M6",		"D_E2M5",
	"D_E1M9",		"MUS_E2M1",		"MUS_E2M2",		"MUS_E2M3",		"MUS_E2M4",
	"MUS_E1M4",		"MUS_E2M6",		"MUS_E2M7",		"MUS_E2M8",		"MUS_E2M9",
	"MUS_E3M2",		"MUS_E3M3",		"MUS_E1M6"
};

static const char Musics2[36][9] =
{
	"MUS_E1M1",		"MUS_E1M2",		"MUS_E1M3",		"MUS_E1M4",		"MUS_E1M5",
	"MUS_E1M6",		"MUS_E1M7",		"MUS_E1M8",		"MUS_E1M9",		"MUS_E2M1",
	"MUS_E2M2",		"MUS_E2M3",		"MUS_E2M4",		"MUS_E1M4",		"MUS_E2M6",
	"MUS_E2M7",		"MUS_E2M8",		"MUS_E2M9",		"MUS_E1M1",		"MUS_E3M2",
	"MUS_E3M3",		"MUS_E1M6",		"MUS_E1M3",		"MUS_E1M2",		"MUS_E1M5",
	"MUS_E1M9",		"MUS_E2M6",		"MUS_E1M6",		"MUS_E1M2",		"MUS_E1M3",
	"MUS_E1M4",		"MUS_E1M5",		"MUS_E1M1",		"MUS_E1M7",		"MUS_E1M8",
	"MUS_E1M9"
};

static const char Musics3[32][9] =
{
	"D_RUNNIN",		"D_STALKS",		"D_COUNTD",		"D_BETWEE",		"D_DOOM",
	"D_THE_DA",		"D_SHAWN",		"D_DDTBLU",		"D_IN_CIT",		"D_DEAD",
	"D_STLKS2",		"D_THEDA2",		"D_DOOM2",		"D_DDTBL2",		"D_RUNNI2",
	"D_DEAD2",		"D_STLKS3",		"D_ROMERO",		"D_SHAWN2",		"D_MESSAG",
	"D_COUNT2",		"D_DDTBL3",		"D_AMPIE",		"D_THEDA3",		"D_ADRIAN",
	"D_MESSG2",		"D_ROMER2",		"D_TENSE",		"D_SHAWN3",		"D_OPENIN",
	"D_EVIL",		"D_ULTIMA"
};

static const char Musics4[15][9] =
{
	"D_VICTOR",		"D_VICTOR",		"D_VICTOR",		"D_VICTOR",		"D_READ_M",
	"D_READ_M",		"D_READ_M",		"D_READ_M",		"D_READ_M",		"D_READ_M",
	"MUS_CPTD",		"MUS_CPTD",		"MUS_CPTD",		"MUS_CPTD",		"MUS_CPTD"
};

extern int timingdemo;

EXTERN_CVAR(sv_fastmonsters)
EXTERN_CVAR(sv_monstersrespawn)

// Start time for timing demos
int starttime;

// ACS variables with world scope
int WorldVars[NUM_WORLDVARS];

extern BOOL netdemo;
BOOL savegamerestore;

extern int mousex, mousey;

extern int joyxmove, joyymove, Impulse;
extern BOOL sendpause, sendsave, sendcenterview;

level_locals_t level;			// info about current level

level_pwad_info_t *wadlevelinfos;
cluster_info_t *wadclusterinfos;
size_t numwadlevelinfos = 0;
size_t numwadclusterinfos = 0;

BOOL HexenHack;

bool isFast = false;

static const char *MapInfoTopLevel[] =
{
	"map",
	"defaultmap",
	"clusterdef",
	NULL
};

enum
{
	MITL_MAP,
	MITL_DEFAULTMAP,
	MITL_CLUSTERDEF
};

static const char *MapInfoMapLevel[] =
{
	"levelnum",
	"next",
	"secretnext",
	"cluster",
	"sky1",
	"fade",
	"outsidefog",
	"titlepatch",
	"par",
	"music",
	"nointermission",
	"doublesky",
	"nosoundclipping",
	"allowmonstertelefrags",
	"map07special",
	"baronspecial",
	"cyberdemonspecial",
	"spidermastermindspecial",
	"specialaction_exitlevel",
	"specialaction_opendoor",
	"specialaction_lowerfloor",
	"lightning",
	"fadetable",
	"evenlighting",
	"noautosequences",
	"forcenoskystretch",
	"allowfreelook",
	"nofreelook",
	"allowjump",
	"nojump",
	"cdtrack",
	"cd_start_track",
	"cd_end1_track",
	"cd_end2_track",
	"cd_end3_track",
	"cd_intermission_track",
	"cd_title_track",
	"warptrans",
	NULL
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
	{ MITYPE_INT,		lioffset(levelnum), 0 },
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

static const char *MapInfoClusterLevel[] =
{
	"entertext",
	"exittext",
	"music",
	"flat",
	"hub",
	NULL
};

MapInfoHandler ClusterHandlers[] =
{
	{ MITYPE_STRING,	cioffset(entertext), 0 },
	{ MITYPE_STRING,	cioffset(exittext), 0 },
	{ MITYPE_CSTRING,	cioffset(messagemusic), 8 },
	{ MITYPE_LUMPNAME,	cioffset(finaleflat), 0 },
	{ MITYPE_SETFLAG,	CLUSTER_HUB, 0 }
};

static void ParseMapInfoLower (MapInfoHandler *handlers,
							   const char *strings[],
							   level_pwad_info_t *levelinfo,
							   cluster_info_t *clusterinfo,
							   DWORD levelflags);

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

static void SetLevelDefaults (level_pwad_info_t *levelinfo)
{
	memset (levelinfo, 0, sizeof(*levelinfo));
	levelinfo->snapshot = NULL;
	levelinfo->outsidefog = 0xff000000;
	strncpy (levelinfo->fadetable, "COLORMAP", 8);
}

//
// G_ParseMapInfo
// Parses the MAPINFO lumps of all loaded WADs and generates
// data for wadlevelinfos and wadclusterinfos.
//
void G_ParseMapInfo (void)
{
	int lump, lastlump = 0;
	level_pwad_info_t defaultinfo;
	level_pwad_info_t *levelinfo;
	int levelindex;
	cluster_info_t *clusterinfo;
	int clusterindex;
	DWORD levelflags;

	while ((lump = W_FindLump ("MAPINFO", &lastlump)) != -1)
	{
		SetLevelDefaults (&defaultinfo);
		SC_OpenLumpNum (lump, "MAPINFO");

		while (SC_GetString ())
		{
			switch (SC_MustMatchString (MapInfoTopLevel))
			{
			case MITL_DEFAULTMAP:
				SetLevelDefaults (&defaultinfo);
				ParseMapInfoLower (MapHandlers, MapInfoMapLevel, &defaultinfo, NULL, 0);
				break;

			case MITL_MAP:		// map <MAPNAME> <Nice Name>
				levelflags = defaultinfo.flags;
				SC_MustGetString ();
				if (IsNum (sc_String))
				{	// MAPNAME is a number, assume a Hexen wad
					int map = atoi (sc_String);
					sprintf (sc_String, "MAP%02d", map);
					SKYFLATNAME[5] = 0;
					HexenHack = true;
					// Hexen levels are automatically nointermission
					// and even lighting and no auto sound sequences
					levelflags |= LEVEL_NOINTERMISSION
								| LEVEL_EVENLIGHTING
								| LEVEL_SNDSEQTOTALCTRL;
				}
				levelindex = FindWadLevelInfo (sc_String);
				if (levelindex == -1)
				{
					levelindex = numwadlevelinfos++;
					wadlevelinfos = (level_pwad_info_t *)Realloc (wadlevelinfos, sizeof(level_pwad_info_t)*numwadlevelinfos);
				}
				levelinfo = wadlevelinfos + levelindex;
				memcpy (levelinfo, &defaultinfo, sizeof(*levelinfo));
				uppercopy (levelinfo->mapname, sc_String);
				SC_MustGetString ();
				ReplaceString ((char **)&levelinfo->level_name, sc_String);
				// Set up levelnum now so that the Teleport_NewMap specials
				// in hexen.wad work without modification.
				if (!strnicmp (levelinfo->mapname, "MAP", 3) && levelinfo->mapname[5] == 0)
				{
					int mapnum = atoi (levelinfo->mapname + 3);

					if (mapnum >= 1 && mapnum <= 99)
						levelinfo->levelnum = mapnum;
				}
				ParseMapInfoLower (MapHandlers, MapInfoMapLevel, levelinfo, NULL, levelflags);
				break;

			case MITL_CLUSTERDEF:	// clusterdef <clusternum>
				SC_MustGetNumber ();
				clusterindex = FindWadClusterInfo (sc_Number);
				if (clusterindex == -1)
				{
					clusterindex = numwadclusterinfos++;
					wadclusterinfos = (cluster_info_t *)Realloc (wadclusterinfos, sizeof(cluster_info_t)*numwadclusterinfos);
					memset (wadclusterinfos + clusterindex, 0, sizeof(cluster_info_t));
				}
				clusterinfo = wadclusterinfos + clusterindex;
				clusterinfo->cluster = sc_Number;
				ParseMapInfoLower (ClusterHandlers, MapInfoClusterLevel, NULL, clusterinfo, 0);
				break;
			}
		}
		SC_Close ();
	}
}

static void ParseMapInfoLower (MapInfoHandler *handlers,
							   const char *strings[],
							   level_pwad_info_t *levelinfo,
							   cluster_info_t *clusterinfo,
							   DWORD flags)
{
	int entry;
	MapInfoHandler *handler;
	byte *info;

	info = levelinfo ? (byte *)levelinfo : (byte *)clusterinfo;

	while (SC_GetString ())
	{
		if (SC_MatchString (MapInfoTopLevel) != -1)
		{
			SC_UnGet ();
			break;
		}
		entry = SC_MustMatchString (strings);
		handler = handlers + entry;
		switch (handler->type)
		{
		case MITYPE_IGNORE:
			break;

		case MITYPE_EATNEXT:
			SC_MustGetString ();
			break;

		case MITYPE_INT:
			SC_MustGetNumber ();
			*((int *)(info + handler->data1)) = sc_Number;
			break;

		case MITYPE_COLOR:
			{
				SC_MustGetString ();
				std::string string = V_GetColorStringByName (sc_String);
				if (string.length())
				{
					*((DWORD *)(info + handler->data1)) =
						V_GetColorFromString (NULL, string.c_str());
				}
				else
				{
					*((DWORD *)(info + handler->data1)) =
										V_GetColorFromString (NULL, sc_String);
				}
			}
			break;

		case MITYPE_MAPNAME:
			SC_MustGetString ();
			if (IsNum (sc_String))
			{
				int map = atoi (sc_String);
				sprintf (sc_String, "MAP%02d", map);
			}
			strncpy ((char *)(info + handler->data1), sc_String, 8);
			break;

		case MITYPE_LUMPNAME:
			SC_MustGetString ();
			uppercopy ((char *)(info + handler->data1), sc_String);
			break;

		case MITYPE_SKY:
			SC_MustGetString ();	// get texture name;
			uppercopy ((char *)(info + handler->data1), sc_String);
			SC_MustGetFloat ();		// get scroll speed
			//if (HexenHack)
			//{
			//	*((fixed_t *)(info + handler->data2)) = sc_Number << 8;
			//}
			//else
			//{
			//	*((fixed_t *)(info + handler->data2)) = (fixed_t)(sc_Float * 65536.0f);
			//}
			break;

		case MITYPE_SETFLAG:
			flags |= handler->data1;
			break;

		case MITYPE_SCFLAGS:
			flags = (flags & handler->data2) | handler->data1;
			break;

		case MITYPE_CLUSTER:
			SC_MustGetNumber ();
			*((int *)(info + handler->data1)) = sc_Number;
			if (HexenHack)
			{
				cluster_info_t *clusterH = FindClusterInfo (sc_Number);
				if (clusterH)
					clusterH->flags |= CLUSTER_HUB;
			}
			break;

		case MITYPE_STRING:
			SC_MustGetString ();
			ReplaceString ((char **)(info + handler->data1), sc_String);
			break;

		case MITYPE_CSTRING:
			SC_MustGetString ();
			strncpy ((char *)(info + handler->data1), (const char *)sc_String, handler->data2);
			*((char *)(info + handler->data1 + handler->data2)) = '\0';
			break;
		}
	}
	if (levelinfo)
		levelinfo->flags = flags;
	else
		clusterinfo->flags = flags;
}

static void zapDefereds (acsdefered_t *def)
{
	while (def) {
		acsdefered_t *next = def->next;
		delete def;
		def = next;
	}
}

void P_RemoveDefereds (void)
{
	unsigned int i;

	// Remove any existing defereds
	for (i = 0; i < numwadlevelinfos; i++)
		if (wadlevelinfos[i].defered) {
			zapDefereds (wadlevelinfos[i].defered);
			wadlevelinfos[i].defered = NULL;
		}

	for (i = 0; LevelInfos[i].level_name; i++)
		if (LevelInfos[i].defered) {
			zapDefereds (LevelInfos[i].defered);
			LevelInfos[i].defered = NULL;
		}
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
            // [Russell] - gamemode is always the better option compared to above
			if ( argc == 2 )
			{
				if (gamemode == commercial)
                    sprintf( mapname, "MAP%02i", atoi( argv[1] ) );
                else
                    sprintf( mapname, "E%cM%c", argv[1][0], argv[1][1]);

			}

			if (W_CheckNumForName (mapname) == -1)
			{ // Still no luck, oh well.
				Printf (PRINT_HIGH, "Map %s not found.\n", argv[1]);
			}
			else
			{ // Success
				G_DeferedInitNew (mapname);
			}

		}
		else
		{
			G_DeferedInitNew (argv[1]);
		}
	}
}
END_COMMAND (map)

BEGIN_COMMAND (wad) // denis - changes wads
{
	std::vector<std::string> wads, patch_files, hashes;

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

    // add our iwad if it is one
    // [ML] 7/26/2010: otherwise reload the currently-loaded iwad
    if (W_IsIWAD(argv[1]))
        wads.push_back(argv[1]);
    else
        wads.push_back(wadfiles[1].c_str());

    // check whether they are wads or patch files
	for (QWORD i = 1; i < argc; i++)
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

    hashes.resize(wads.size());

	D_DoomWadReboot(wads, patch_files, hashes);

	D_StartTitle ();
	CL_QuitNetGame();
	S_StopMusic();
	S_StartMusic(gameinfo.titleMusic);
}
END_COMMAND (wad)

EXTERN_CVAR(sv_allowexit)
EXTERN_CVAR(sv_nomonsters)
EXTERN_CVAR(sv_freelook)
EXTERN_CVAR(sv_allowjump)

void G_DoNewGame (void)
{
	if (demoplayback)
	{
		cvar_t::C_RestoreCVars ();
		demoplayback = false;
		D_SetupUserInfo ();
	}

	CL_QuitNetGame();

	netdemo = false;
	netgame = false;
	multiplayer = false;

	// denis - single player warp (like in d_main)
	serverside = true;
	sv_allowexit = "1";
	sv_nomonsters = "0";
	sv_freelook = "1";
	sv_allowjump = "1";
	sv_gametype = GM_COOP;

	players.clear();
	players.push_back(player_t());
	players.back().playerstate = PST_REBORN;
	consoleplayer_id = displayplayer_id = players.back().id = 1;

	G_InitNew (d_mapname);
	gameaction = ga_nothing;
}

void G_InitNew (const char *mapname)
{
	size_t i = 0, j = 0;

	for (i = 0; i < players.size(); i++)
	{
		players[i].mo = AActor::AActorPtr();
		players[i].camera = AActor::AActorPtr();
		players[i].attacker = AActor::AActorPtr();
	}

	if (!savegamerestore)
		G_ClearSnapshots ();

	// [RH] Mark all levels as not visited
	if (!savegamerestore)
	{
		for (j = 0; j < numwadlevelinfos; j++)
			wadlevelinfos[j].flags &= ~LEVEL_VISITED;

		for (i = 0; LevelInfos[i].mapname[0]; i++)
			LevelInfos[i].flags &= ~LEVEL_VISITED;
	}

	cvar_t::UnlatchCVars ();

	if (sv_skill > sk_nightmare)
		sv_skill.Set (sk_nightmare);
	else if (sv_skill < sk_baby)
		sv_skill.Set (sk_baby);

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

	if (!savegamerestore)
	{
		M_ClearRandom ();
		level.time = 0;

		// force players to be initialized upon first level load
		for (i = 0; i < players.size(); i++)
			players[i].playerstate = PST_REBORN;
	}

	usergame = true;				// will be set false if a demo
	paused = false;
	demoplayback = false;
	automapactive = false;
	viewactive = true;

	strncpy (level.mapname, mapname, 8);
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
static void goOn (int position)
{
	cluster_info_t *thiscluster = FindClusterInfo (level.cluster);
	cluster_info_t *nextcluster = FindClusterInfo (FindLevelInfo (level.nextmap)->cluster);

	startpos = position;
	gameaction = ga_completed;

	if (thiscluster && (thiscluster->flags & CLUSTER_HUB))
	{
		if ((level.flags & LEVEL_NOINTERMISSION) || (nextcluster == thiscluster))
			NoWipe = 4;
		D_DrawIcon = "TELEICON";
	}
}

void G_ExitLevel (int position, int drawscores)
{
    //if (multiplayer && drawscores && interscoredraw)
    //    AddCommandString("displayscores");
    // Never called.

	secretexit = false;
	goOn (position);

	//gameaction = ga_completed;
	
}

// Here's for the german edition.
void G_SecretExitLevel (int position, int drawscores)
{
    //if (multiplayer && drawscores && interscoredraw)
    //    AddCommandString("displayscores");
    // Never called.

	// IF NO WOLF3D LEVELS, NO SECRET EXIT!
	if ( (gamemode == commercial)
		 && (W_CheckNumForName("map31")<0))
		secretexit = false;
	else
		secretexit = true;
    
    goOn (position);
	//gameaction = ga_completed;
}

void G_DoCompleted (void)
{
	size_t i;

	gameaction = ga_nothing;

	for(i = 0; i < players.size(); i++)
		if(players[i].ingame())
			G_PlayerFinishLevel(players[i]);

	V_RestoreScreenPalette();

	// [RH] Mark this level as having been visited
	if (!(level.flags & LEVEL_CHANGEMAPCHEAT))
		FindLevelInfo (level.mapname)->flags |= LEVEL_VISITED;

	if (automapactive)
		AM_Stop ();

	// [ML] Chex mode: they didn't even show the intermission screen
	// after the fifth level - I checked.
	if (gamemode == retail_chex && !strncmp(level.mapname,"E1M5",4)) {
		G_WorldDone();
		return;
	}

	wminfo.epsd = level.cluster - 1;		// Only used for DOOM I.
	strncpy (wminfo.lname0, level.info->pname, 8);
	strncpy (wminfo.current, level.mapname, 8);

	if (sv_gametype != GM_COOP &&
		!(level.flags & LEVEL_CHANGEMAPCHEAT)) {
		strncpy (wminfo.next, level.mapname, 8);
		strncpy (wminfo.lname1, level.info->pname, 8);
	} else {
		wminfo.next[0] = 0;
		if (secretexit) {
			if (W_CheckNumForName (level.secretmap) != -1) {
				strncpy (wminfo.next, level.secretmap, 8);
				strncpy (wminfo.lname1, FindLevelInfo (level.secretmap)->pname, 8);
			} else {
				secretexit = false;
			}
		}
		if (!wminfo.next[0]) {
			strncpy (wminfo.next, level.nextmap, 8);
			strncpy (wminfo.lname1, FindLevelInfo (level.nextmap)->pname, 8);
		}
	}

	wminfo.maxkills = level.total_monsters;
	wminfo.maxitems = level.total_items;
	wminfo.maxsecret = level.total_secrets;
	wminfo.maxfrags = 0;
	wminfo.partime = TICRATE * level.partime;

	wminfo.plyr.resize(players.size());

	for (i=0 ; i < players.size(); i++)
	{
		wminfo.plyr[i].in = players[i].ingame();
		wminfo.plyr[i].skills = players[i].killcount;
		wminfo.plyr[i].stime = level.time;
		wminfo.plyr[i].fragcount = players[i].fragcount;

		if(&players[i] == &consoleplayer())
			wminfo.pnum = i;
	}

	// [RH] If we're in a hub and staying within that hub, take a snapshot
	//		of the level. If we're traveling to a new hub, take stuff from
	//		the player and clear the world vars. If this is just an
	//		ordinary cluster (not a hub), take stuff from the player, but
	//		leave the world vars alone.
	{
		cluster_info_t *thiscluster = FindClusterInfo (level.cluster);
		cluster_info_t *nextcluster = FindClusterInfo (FindLevelInfo (level.nextmap)->cluster);

		if (thiscluster != nextcluster ||
			sv_gametype == GM_DM ||
			!(thiscluster->flags & CLUSTER_HUB)) {
			for (i=0 ; i<players.size(); i++)
				if (players[i].ingame())
					G_PlayerFinishLevel (players[i]);	// take away cards and stuff

				if (nextcluster->flags & CLUSTER_HUB) {
					memset (WorldVars, 0, sizeof(WorldVars));
					P_RemoveDefereds ();
					G_ClearSnapshots ();
				}
		} else {
			G_SnapshotLevel ();
		}
		if (!(nextcluster->flags & CLUSTER_HUB) || !(thiscluster->flags & CLUSTER_HUB))
			level.time = 0;	// Reset time to zero if not entering/staying in a hub

		if (!(sv_gametype == GM_DM) &&
			((level.flags & LEVEL_NOINTERMISSION) ||
			((nextcluster == thiscluster) && (thiscluster->flags & CLUSTER_HUB)))) {
			G_WorldDone ();
			return;
		}
	}
	
	gamestate = GS_INTERMISSION;
	viewactive = false;
	automapactive = false;

	WI_Start (&wminfo);
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

	if (position == -1)
		position = lastposition;
	else
		lastposition = position;

	G_InitLevelLocals ();

    Printf_Bold ("\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36"
                 "\36\36\36\36\36\36\36\36\36\36\36\36\37\n"
                 "%s: \"%s\"\n\n", level.mapname, level.level_name);

	if (wipegamestate == GS_LEVEL)
		wipegamestate = GS_FORCEWIPE;

	gamestate = GS_LEVEL;

	if(ConsoleState == c_down)
		C_HideConsole();

	// Set the sky map.
	// First thing, we have a dummy sky texture name,
	//	a flat. The data is in the WAD only because
	//	we look for an actual index, instead of simply
	//	setting one.
	skyflatnum = R_FlatNumForName ( SKYFLATNAME );

	// DOOM determines the sky texture to be used
	// depending on the current episode, and the game version.
	// [RH] Fetch sky parameters from level_locals_t.
	sky1texture = R_TextureNumForName (level.skypic);

	// [RH] Set up details about sky rendering
	R_InitSkyMap ();

	for (i = 0; i < players.size(); i++)
	{
		if (players[i].ingame() && players[i].playerstate == PST_DEAD)
			players[i].playerstate = PST_REBORN;

		players[i].fragcount = 0;
		players[i].deathcount = 0; // [Toke - Scores - deaths]
		players[i].killcount = 0; // [deathz0r] Coop kills
		players[i].points = 0;
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

// 	SN_StopAllSequences (); // denis - todo - equivalent?
	P_SetupLevel (level.mapname, position);
	displayplayer_id = consoleplayer_id;				// view the guy you are playing
	//ST_Start();		// [RH] Make sure status bar knows who we are
	gameaction = ga_nothing;
	Z_CheckHeap ();

	// clear cmd building stuff
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
    P_DoDeferedScripts ();	// [RH] Do script actions that were triggered on another map.
    
	C_FlushDisplay ();
}


//
// G_WorldDone
//
void G_WorldDone (void)
{
	cluster_info_t *nextcluster;
	cluster_info_t *thiscluster;

	gameaction = ga_worlddone;

	if (level.flags & LEVEL_CHANGEMAPCHEAT)
		return;

	thiscluster = FindClusterInfo (level.cluster);
	if (!strncmp (level.nextmap, "EndGame", 7) || (gamemode == retail_chex && !strncmp (level.nextmap, "E1M6", 4))) {
		automapactive = false;
		F_StartFinale (thiscluster->messagemusic, thiscluster->finaleflat, thiscluster->exittext);
	} else {
		if (!secretexit)
			nextcluster = FindClusterInfo (FindLevelInfo (level.nextmap)->cluster);
		else
			nextcluster = FindClusterInfo (FindLevelInfo (level.secretmap)->cluster);

		if (nextcluster->cluster != level.cluster && sv_gametype == GM_COOP) {
			// Only start the finale if the next level's cluster is different
			// than the current one and we're not in deathmatch.
			if (nextcluster->entertext) {
				automapactive = false;
				F_StartFinale (nextcluster->messagemusic, nextcluster->finaleflat, nextcluster->entertext);
			} else if (thiscluster->exittext) {
				automapactive = false;
				F_StartFinale (thiscluster->messagemusic, thiscluster->finaleflat, thiscluster->exittext);
			}
		}
	}
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
	unsigned long oldfade = level.fadeto;
	level_info_t *info;
	int i;

	BaseBlendA = 0.0f;		// Remove underwater blend effect, if any
	NormalLight.maps = realcolormaps;

	if ((i = FindWadLevelInfo (level.mapname)) > -1)
	{
		level_pwad_info_t *pinfo = wadlevelinfos + i;

		// [ML] 5/11/06 - Remove sky scrolling and sky2
		level.info = (level_info_t *)pinfo;
		info = (level_info_t *)pinfo;
		level.fadeto = pinfo->fadeto;
		
		if (level.fadeto)
		{
			NormalLight.maps = DefaultPalette->maps.colormaps;
		}
		else
		{
			R_SetDefaultColormap (pinfo->fadetable);
		}
		
		level.outsidefog = pinfo->outsidefog;
		level.flags |= LEVEL_DEFINEDINMAPINFO;
	}
	else
	{
		info = FindDefLevelInfo (level.mapname);
		level.info = info;
		level.fadeto = 0;
		level.outsidefog = 0xff000000;	// 0xff000000 signals not to handle it special
		R_SetDefaultColormap ("COLORMAP");
	}

	if (info->level_name)
	{
		level.partime = info->partime;
		level.cluster = info->cluster;
		level.flags = info->flags;
		level.levelnum = info->levelnum;
		level.music = info->music;

		strncpy (level.level_name, info->level_name, 63);
		strncpy (level.nextmap, info->nextmap, 8);
		strncpy (level.secretmap, info->secretmap, 8);
		strncpy (level.skypic, info->skypic, 8);
	}
	else
	{
		level.partime = level.cluster = 0;
		strcpy (level.level_name, "Unnamed");
		level.nextmap[0] =
			level.secretmap[0] = 0;
		level.music = '\0';
		strncpy (level.skypic, "SKY1", 8);
		level.flags = 0;
		level.levelnum = 1;
	}

	memset (level.vars, 0, sizeof(level.vars));
    
	if (oldfade != level.fadeto)
		RefreshPalettes ();
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
	char *namepart;
	int i, start;

	temp[0] = '0';
	temp[1] = ':';
	temp[2] = 0;

	for (i = 65; i < 101; i++) 		// HUSTR_E1M1 .. HUSTR_E4M9
	{
		if (temp[0] < '9')
			temp[0]++;
		else
			temp[0] = '1';

		if ( (namepart = strstr (Strings[i].string, temp)) ) 
		{
			namepart += 2;
			while (*namepart && *namepart <= ' ')
				namepart++;
		}
		else
		{
			namepart = Strings[i].string;
		}

		if (gameinfo.gametype != GAME_Heretic)
		{
			ReplaceString (&LevelInfos[i-65].level_name, namepart);
			ReplaceString (&LevelInfos[i-65].music, Musics1[i-65]);
		}
	}
		
	if (gameinfo.gametype == GAME_Heretic)
	{
		for (i = 0; i < 4*9; i++)
		{
			ReplaceString (&LevelInfos[i].level_name, Strings[355+i].string);
			ReplaceString (&LevelInfos[i].music, Musics2[i]);
			LevelInfos[i].cluster = 11 + (i / 9);
			LevelInfos[i].pname[0] = 0;
		}
		for (i = 0; i < 9; i++)
		{
			strcpy (LevelInfos[i+3*9].skypic, "SKY1");
		}
		LevelInfos[7].flags = LEVEL_NOINTERMISSION|LEVEL_NOSOUNDCLIPPING|LEVEL_SPECLOWERFLOOR|LEVEL_HEADSPECIAL;
		LevelInfos[16].flags = LEVEL_NOINTERMISSION|LEVEL_NOSOUNDCLIPPING|LEVEL_SPECLOWERFLOOR|LEVEL_MINOTAURSPECIAL;
		LevelInfos[25].flags = LEVEL_NOINTERMISSION|LEVEL_NOSOUNDCLIPPING|LEVEL_SPECLOWERFLOOR|LEVEL_SORCERER2SPECIAL;
		LevelInfos[34].flags = LEVEL_NOINTERMISSION|LEVEL_NOSOUNDCLIPPING|LEVEL_SPECLOWERFLOOR|LEVEL_HEADSPECIAL;
	
		//SetEndSequence (LevelInfos[16].nextmap, END_Underwater);
		//SetEndSequence (LevelInfos[25].nextmap, END_Demon);

		LevelInfos[8].nextmap[3] = LevelInfos[8].secretmap[3] = '7';
		LevelInfos[17].nextmap[3] = LevelInfos[17].secretmap[3] = '5';
		LevelInfos[26].nextmap[3] = LevelInfos[26].secretmap[3] = '5';
		LevelInfos[35].nextmap[3] = LevelInfos[35].secretmap[3] = '5';
	}
	else if (gameinfo.gametype == GAME_Doom)
	{
		//SetEndSequence (LevelInfos[16].nextmap, END_Pic2);
		//SetEndSequence (LevelInfos[25].nextmap, END_Bunny);
	}

	//SetEndSequence (LevelInfos[7].nextmap, END_Pic1);
	//SetEndSequence (LevelInfos[34].nextmap, END_Pic3);
	//SetEndSequence (LevelInfos[43].nextmap, END_Pic3);
	//SetEndSequence (LevelInfos[77].nextmap, END_Cast);

	for (i = 0; i < 12; i++)
	{
		if (i < 9)
			ReplaceString (&LevelInfos[i+36].level_name, Strings[391+i].string);
		ReplaceString (&LevelInfos[i+36].music, Musics1[i+36]);
	}

	for (i = 0; i < 4; i++)
		ReplaceString (&ClusterInfos[i].exittext, Strings[221+i].string);

	if (gamemission == pack_plut)
		start = 133;
	else if (gamemission == pack_tnt)
		start = 165;
	else
		start = 101;

	for (i = 0; i < 32; i++)
	{
		sprintf (temp, "%d:", i + 1);
		if ( (namepart = strstr (Strings[i+start].string, temp)) )
		{
			namepart += strlen (temp);
			while (*namepart && *namepart <= ' ')
				namepart++;
		}
		else
		{
			namepart = Strings[i+start].string;
		}
		ReplaceString (&LevelInfos[48+i].level_name, namepart);
		ReplaceString (&LevelInfos[48+i].music, Musics3[i]);
	}

	if (gamemission == pack_plut)
		start = 231;		// P1TEXT
	else if (gamemission == pack_tnt)
		start = 237;		// T1TEXT
	else
		start = 225;		// C1TEXT

	for (i = 0; i < 4; i++)
		ReplaceString (&ClusterInfos[4 + i].exittext, Strings[start+i].string);
	for (; i < 6; i++)
		ReplaceString (&ClusterInfos[4 + i].entertext, Strings[start+i].string);
	for (i = 350; i <= 354; i++)
		ReplaceString (&ClusterInfos[10 + i - 350].exittext, Strings[i].string);
	for (i = 0; i < 15; i++)
		ReplaceString (&ClusterInfos[i].messagemusic, Musics4[i]);

	if (level.info && level.info->level_name)
		strncpy (level.level_name, level.info->level_name, 63);
}

void G_SerializeLevel (FArchive &arc, bool hubLoad)
{
    int i;
    
	if (arc.IsStoring ())
	{
		unsigned int playernum = players.size();
		arc << level.flags
			<< level.fadeto
			<< level.found_secrets
			<< level.found_items
			<< level.killed_monsters;
			//<< playernum;

		for (i = 0; i < NUM_MAPVARS; i++)
			arc << level.vars[i];
			
        arc << playernum;
	}
	else
	{
		unsigned int playernum;
		arc >> level.flags
			>> level.fadeto
			>> level.found_secrets
			>> level.found_items
			>> level.killed_monsters;
			//>> playernum;
			
		for (i = 0; i < NUM_MAPVARS; i++)
			arc >> level.vars[i];
        
        arc >> playernum;

		players.resize(playernum);
	}

	P_SerializeThinkers (arc, hubLoad);
    P_SerializeWorld (arc);
    P_SerializePolyobjs (arc);
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
		arc.SetHubTravel (); // denis - hexen?
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

static void writeDefereds (FArchive &arc, level_info_t *i)
{
	arc.Write (i->mapname, 8);
	arc << i->defered;
}

void P_SerializeACSDefereds (FArchive &arc)
{
	if (arc.IsStoring ())
	{
		unsigned int i;

		for (i = 0; i < numwadlevelinfos; i++)
			if (wadlevelinfos[i].defered)
				writeDefereds (arc, (level_info_s *)&wadlevelinfos[i]);

		for (i = 0; LevelInfos[i].level_name; i++)
			if (LevelInfos[i].defered)
				writeDefereds (arc, &LevelInfos[i]);

		// Signal end of defereds
		arc << (char)0;
	}
	else
	{
		char mapname[8];

		P_RemoveDefereds ();

		arc >> mapname[0];
		while (mapname[0])
		{
			arc.Read (&mapname[1], 7);
			level_info_t *i = FindLevelInfo (mapname);
			if (i == NULL)
			{
				char name[9];

				strncpy (name, mapname, 8);
				name[8] = 0;
				I_Error ("Unknown map '%s' in savegame", name);
			}
			arc >> i->defered;
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
		"WILV00",
		"E1M2",
		"E1M9",
		"SKY1",
		1,
		30,
	},
	{
		"E1M2",
		2,
		"WILV01",
		"E1M3",
		"E1M9",
		"SKY1",
		1,
		75,
	},
	{
		"E1M3",
		3,
		"WILV02",
		"E1M4",
		"E1M9",
		"SKY1",
		1,
		120,
	},
	{
		"E1M4",
		4,
		"WILV03",
		"E1M5",
		"E1M9",
		"SKY1",
		1,
		90,
	},
	{
		"E1M5",
		5,
		"WILV04",
		"E1M6",
		"E1M9",
		"SKY1",
		1,
		165,
	},
	{
		"E1M6",
		6,
		"WILV05",
		"E1M7",
		"E1M9",
		"SKY1",
		1,
		180,
	},
	{
		"E1M7",
		7,
		"WILV06",
		"E1M8",
		"E1M9",
		"SKY1",
		1,
		180,
	},
	{
		"E1M8",
		8,
		"WILV07",
		"",
		"E1M9",
		"SKY1",
		1,
		30,
		LEVEL_NOINTERMISSION|LEVEL_NOSOUNDCLIPPING|LEVEL_BRUISERSPECIAL|LEVEL_SPECLOWERFLOOR,
	},
	{
		"E1M9",
		9,
		"WILV08",
		"E1M4",
		"E1M4",
		"SKY1",
		1,
		165,
	},

	// Registered/Retail Episode 2
	{
		"E2M1",
		11,
		"WILV10",
		"E2M2",
		"E2M9",
		"SKY2",
		2,
		90,
	},

	{
		"E2M2",
		12,
		"WILV11",
		"E2M3",
		"E2M9",
		"SKY2",
		2,
		90,
	},
	{
		"E2M3",
		13,
		"WILV12",
		"E2M4",
		"E2M9",
		"SKY2",
		2,
		90,
	},
	{
		"E2M4",
		14,
		"WILV13",
		"E2M5",
		"E2M9",
		"SKY2",
		2,
		120,
	},
	{
		"E2M5",
		15,
		"WILV14",
		"E2M6",
		"E2M9",
		"SKY2",
		2,
		90,
	},
	{
		"E2M6",
		16,
		"WILV15",
		"E2M7",
		"E2M9",
		"SKY2",
		2,
		360,
	},
	{
		"E2M7",
		17,
		"WILV16",
		"E2M8",
		"E2M9",
		"SKY2",
		2,
		240,
	},
	{
		"E2M8",
		18,
		"WILV17",
		"",
		"E2M9",
		"SKY2",
		2,
		30,
		LEVEL_NOINTERMISSION|LEVEL_NOSOUNDCLIPPING|LEVEL_CYBORGSPECIAL,
	},
	{
		"E2M9",
		19,
		"WILV18",
		"E2M6",
		"E2M6",
		"SKY2",
		2,
		170,
	},

	// Registered/Retail Episode 3

	{
		"E3M1",
		21,
		"WILV20",
		"E3M2",
		"E3M9",
		"SKY3",
		3,
		90,
	},
	{
		"E3M2",
		22,
		"WILV21",
		"E3M3",
		"E3M9",
		"SKY3",
		3,
		45,
	},
	{
		"E3M3",
		23,
		"WILV22",
		"E3M4",
		"E3M9",
		"SKY3",
		3,
		90,
	},
	{
		"E3M4",
		24,
		"WILV23",
		"E3M5",
		"E3M9",
		"SKY3",
		3,
		150,
	},
	{
		"E3M5",
		25,
		"WILV24",
		"E3M6",
		"E3M9",
		"SKY3",
		3,
		90,
	},
	{
		"E3M6",
		26,
		"WILV25",
		"E3M7",
		"E3M9",
		"SKY3",
		3,
		90,
	},
	{
		"E3M7",
		27,
		"WILV26",
		"E3M8",
		"E3M9",
		"SKY3",
		3,
		165,
	},
	{
		"E3M8",
		28,
		"WILV27",
		"",
		"E3M9",
		"SKY3",
		3,
		30,
		LEVEL_NOINTERMISSION|LEVEL_NOSOUNDCLIPPING|LEVEL_SPIDERSPECIAL,
	},
	{
		"E3M9",
		29,
		"WILV28",
		"E3M7",
		"E3M7",
		"SKY3",
		3,
		135,
	},

	// Retail Episode 4
	{
		"E4M1",
		31,
		"WILV30",
		"E4M2",
		"E4M9",
		"SKY4",
		4,
	},
	{
		"E4M2",
		32,
		"WILV31",
		"E4M3",
		"E4M9",
		"SKY4",
		4,
	},
	{
		"E4M3",
		33,
		"WILV32",
		"E4M4",
		"E4M9",
		"SKY4",
		4,
	},
	{
		"E4M4",
		34,
		"WILV33",
		"E4M5",
		"E4M9",
		"SKY4",
		4,
	},
	{
		"E4M5",
		35,
		"WILV34",
		"E4M6",
		"E4M9",
		"SKY4",
		4,
	},
	{
		"E4M6",
		36,
		"WILV35",
		"E4M7",
		"E4M9",
		"SKY4",
		4,
		0,
		LEVEL_CYBORGSPECIAL|LEVEL_SPECOPENDOOR,
	},
	{
		"E4M7",
		37,
		"WILV36",
		"E4M8",
		"E4M9",
		"SKY4",
		4,
	},
	{
		"E4M8",
		38,
		"WILV37",
		"",
		"E4M9",
		"SKY4",
		4,
		0,
		LEVEL_NOINTERMISSION|LEVEL_NOSOUNDCLIPPING|LEVEL_SPIDERSPECIAL|LEVEL_SPECLOWERFLOOR,
	},
	{
		"E4M9",
		39,
		"WILV38",
		"E4M3",
		"E4M3",
		"SKY4",
		4,
	},

	// Heretic Episode 5
	{
		"E5M1",
		41,
		{0},
		"E5M2",
		"E5M9",
		"SKY3",
		15,
	},
	{
		"E5M2",
		42,
		{0},
		"E5M3",
		"E5M9",
		"SKY3",
		15,
	},
	{
		"E5M3",
		43,
		{0},
		"E5M4",
		"E5M9",
		"SKY3",
		15,
	},
	{
		"E5M4",
		44,
		{0},
		"E5M5",
		"E5M9",
		"SKY3",
		15,
	},
	{
		"E5M5",
		45,
		{0},
		"E5M6",
		"E5M9",
		"SKY3",
		15,
	},
	{
		"E5M6",
		46,
		{0},
		"E5M7",
		"E5M9",
		"SKY3",
		15,
	},
	{
		"E5M7",
		47,
		{0},
		"E5M8",
		"E5M9",
		"SKY3",
		15,
	},
	{
		"E5M8",
		48,
		{0},
		"",
		"E5M9",
		"SKY3",
		15,
		0,
		LEVEL_NOINTERMISSION|LEVEL_NOSOUNDCLIPPING|LEVEL_SPECLOWERFLOOR|LEVEL_MINOTAURSPECIAL
	},
	{
		"E5M9",
		49,
		{0},
		"E5M4",
		"E5M4",
		"SKY3",
		15,
	},

	// Heretic Episode 6
	{
		"E6M1",
		51,
		{0},
		"E6M2",
		"E6M2",
		"SKY1",
		16,
		0,
		0,
		NULL,
		"Untitled"
	},
	{
		"E6M2",
		52,
		{0},
		"E6M3",
		"E6M3",
		"SKY1",
		16,
		0,
		0,
		NULL,
		"Untitled"
	},
	{
		"E6M3",
		53,
		{0},
		"E6M1",
		"E6M1",
		"SKY1",
		16,
		0,
		0,
		NULL,
		"Untitled"
	},

	// DOOM 2 Levels

	{
		"MAP01",
		1,
		"CWILV00",
		"MAP02",
		"MAP02",
		"SKY1",
		5,
		30,
		0,
	},
	{
		"MAP02",
		2,
		"CWILV01",
		"MAP03",
		"MAP03",
		"SKY1",
		5,
		90,
		0,
	},
	{
		"MAP03",
		3,
		"CWILV02",
		"MAP04",
		"MAP04",
		"SKY1",
		5,
		120,
		0,
	},
	{
		"MAP04",
		4,
		"CWILV03",
		"MAP05",
		"MAP05",
		"SKY1",
		5,
		120,
		0,
	},
	{
		"MAP05",
		5,
		"CWILV04",
		"MAP06",
		"MAP06",
		"SKY1",
		5,
		90,
		0,
	},
	{
		"MAP06",
		6,
		"CWILV05",
		"MAP07",
		"MAP07",
		"SKY1",
		5,
		150,
		0,
	},
	{
		"MAP07",
		7,
		"CWILV06",
		"MAP08",
		"MAP08",
		"SKY1",
		6,
		120,
		LEVEL_MAP07SPECIAL,
	},
	{
		"MAP08",
		8,
		"CWILV07",
		"MAP09",
		"MAP09",
		"SKY1",
		6,
		120,
		0,
	},
	{
		"MAP09",
		9,
		"CWILV08",
		"MAP10",
		"MAP10",
		"SKY1",
		6,
		270,
		0,
	},
	{
		"MAP10",
		10,
		"CWILV09",
		"MAP11",
		"MAP11",
		"SKY1",
		6,
		90,
		0,
	},
	{
		"MAP11",
		11,
		"CWILV10",
		"MAP12",
		"MAP12",
		"SKY1",
		6,
		210,
		0,
	},
	{
		"MAP12",
		12,
		"CWILV11",
		"MAP13",
		"MAP13",
		"SKY2",
		7,
		150,
		0,
	},
	{
		"MAP13",
		13,
		"CWILV12",
		"MAP14",
		"MAP14",
		"SKY2",
		7,
		150,
		0,
	},
	{
		"MAP14",
		14,
		"CWILV13",
		"MAP15",
		"MAP15",
		"SKY2",
		7,
		150,
		0,
	},
	{
		"MAP15",
		15,
		"CWILV14",
		"MAP16",
		"MAP31",
		"SKY2",
		7,
		210,
		0,
	},
	{
		"MAP16",
		16,
		"CWILV15",
		"MAP17",
		"MAP17",
		"SKY2",
		7,
		150,
		0,
	},
	{
		"MAP17",
		17,
		"CWILV16",
		"MAP18",
		"MAP18",
		"SKY2",
		7,
		420,
		0,
	},
	{
		"MAP18",
		18,
		"CWILV17",
		"MAP19",
		"MAP19",
		"SKY2",
		7,
		150,
		0,
	},
	{
		"MAP19",
		19,
		"CWILV18",
		"MAP20",
		"MAP20",
		"SKY2",
		7,
		210,
		0,
	},
	{
		"MAP20",
		20,
		"CWILV19",
		"MAP21",
		"MAP21",
		"SKY2",
		7,
		150,
		0,
	},
	{
		"MAP21",
		21,
		"CWILV20",
		"MAP22",
		"MAP22",
		"SKY3",
		8,
		240,
		0,
	},
	{
		"MAP22",
		22,
		"CWILV21",
		"MAP23",
		"MAP23",
		"SKY3",
		8,
		150,
		0,
	},
	{
		"MAP23",
		23,
		"CWILV22",
		"MAP24",
		"MAP24",
		"SKY3",
		8,
		180,
		0,
	},
	{
		"MAP24",
		24,
		"CWILV23",
		"MAP25",
		"MAP25",
		"SKY3",
		8,
		150,
		0,
	},
	{
		"MAP25",
		25,
		"CWILV24",
		"MAP26",
		"MAP26",
		"SKY3",
		8,
		150,
		0,
	},
	{
		"MAP26",
		26,
		"CWILV25",
		"MAP27",
		"MAP27",
		"SKY3",
		8,
		300,
		0,
	},
	{
		"MAP27",
		27,
		"CWILV26",
		"MAP28",
		"MAP28",
		"SKY3",
		8,
		330,
		0,
	},
	{
		"MAP28",
		28,
		"CWILV27",
		"MAP29",
		"MAP29",
		"SKY3",
		8,
		420,
		0,
	},
	{
		"MAP29",
		29,
		"CWILV28",
		"MAP30",
		"MAP30",
		"SKY3",
		8,
		300,
		0,
	},
	{
		"MAP30",
		30,
		"CWILV29",
		"",
		"",
		"SKY3",
		8,
		180,
		LEVEL_MONSTERSTELEFRAG,
	},
	{
		"MAP31",
		31,
		"CWILV30",
		"MAP16",
		"MAP32",
		"SKY3",
		9,
		120,
		0,
	},
	{
		"MAP32",
		32,
		"CWILV31",
		"MAP16",
		"MAP16",
		"SKY3",
		10,
		30,
		0,
	},

	// End-of-list marker
	{
		"",
	}
};

// Episode/Cluster information
cluster_info_t ClusterInfos[] = {
	{		// DOOM Episode 1
		1,	"FLOOR4_8",	NULL,	NULL,	"D_VICTOR",	0
	},
	{		// DOOM Episode 2
		2,	"SFLR6_1",	NULL,	NULL,	"D_VICTOR",	0
	},
	{		// DOOM Episode 3
		3,	"MFLR8_4",	NULL,	NULL,	"D_VICTOR", 0
	},
	{		// DOOM Episode 4
		4,	"MFLR8_3",	NULL,	NULL,	"D_VICTOR", 0
	},
	{		// DOOM II first cluster (up thru level 6)
		5,	"SLIME16",	NULL,	NULL,	"D_READ_M",	0
	},
	{		// DOOM II second cluster (up thru level 11)
		6,	"RROCK14",	NULL,	NULL,	"D_READ_M",	0
	},
	{		// DOOM II third cluster (up thru level 20)
		7,	"RROCK07",	NULL,	NULL,	"D_READ_M",	0
	},
	{		// DOOM II fourth cluster (up thru level 30)
		8,	"RROCK17",	NULL,	NULL,	"D_READ_M",	0
	},
	{		// DOOM II fifth cluster (level 31)
		9,	"RROCK13",	NULL,	NULL,	"D_READ_M",	0
	},
	{		// DOOM II sixth cluster (level 32)
		10,	"RROCK19",	NULL,	NULL,	"D_READ_M",	0
	},
    {		// Heretic episode 1
		11,	"FLOOR25",	NULL,	NULL,	"",	0
	},
	{		// Heretic episode 2
		12,	"FLATHUH1",	NULL,	NULL,	"",	0
	},
	{		// Heretic episode 3
		13,	"FLTWAWA2",	NULL,	NULL,	"",	0
	},
	{		// Heretic episode 4
		14,	"FLOOR28",	NULL,	NULL,	"",	0
	},
	{		// Heretic episode 5
		15,	"FLOOR08",	NULL,	NULL,	"",	0
	},
	{		// Heretic episode 6
		16, "FLOOR25",	NULL,	NULL,	"",	0
	},
	{		// End-of-clusters marker
		0,	"",	NULL,	NULL,	"",	0
	}	
};

VERSION_CONTROL (g_level_cpp, "$Id$")
