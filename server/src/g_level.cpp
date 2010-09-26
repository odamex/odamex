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
//	G_LEVEL
//
//-----------------------------------------------------------------------------


#include <string>
#include <vector>

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
#include "p_acs.h"
#include "d_protocol.h"
#include "d_main.h"
#include "p_mobj.h"
#include "sc_man.h"
#include "m_fileio.h"
#include "m_misc.h"

#include "gi.h"
#include "minilzo.h"

#include "sv_main.h"
#include "p_ctf.h"

#define lioffset(x)		myoffsetof(level_pwad_info_t,x)
#define cioffset(x)		myoffsetof(cluster_info_t,x)

extern int nextupdate;
extern int shotclock;

EXTERN_CVAR (sv_endmapscript)
EXTERN_CVAR (sv_startmapscript)
EXTERN_CVAR (sv_curmap)
EXTERN_CVAR (sv_nextmap)
EXTERN_CVAR (sv_loopepisode)

static level_info_t *FindDefLevelInfo (char *mapname);
static cluster_info_t *FindDefClusterInfo (int cluster);
static int FindEndSequence (int type, const char *picname);
static void SetEndSequence (char *nextmap, int type);

TArray<EndSequence> EndSequences;

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

extern int mapchange;

// Start time for timing demos
int starttime;

// ACS variables with world scope
int WorldVars[NUM_WORLDVARS];

BOOL firstmapinit = true; // Nes - Avoid drawing same init text during every rebirth in single-player servers.

extern BOOL netdemo;
BOOL savegamerestore;

extern int mousex, mousey, joyxmove, joyymove, Impulse;
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
				//std::string string = V_GetColorStringByName (sc_String);
				//if (string.length())
				//{
				//	*((DWORD *)(info + handler->data1)) =
				//		V_GetColorFromString (NULL, string.c_str());
				//}
				//else
				//{
				//	*((DWORD *)(info + handler->data1)) =
				//						V_GetColorFromString (NULL, sc_String);
				//}
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
			strncpy ((char *)(info + handler->data1), sc_String, handler->data2);
			*((char *)(info + handler->data1 + handler->data2)) = '\0';
			break;
		}
	}
	if (levelinfo)
		levelinfo->flags = flags;
	else
		clusterinfo->flags = flags;
}


static int FindEndSequence (int type, const char *picname)
{
	int i, num;

	num = EndSequences.Size ();
	for (i = 0; i < num; i++)
	{
		if (EndSequences[i].EndType == type &&
			(type != END_Pic || stricmp (EndSequences[i].PicName, picname) == 0))
		{
			return i;
		}
	}
	return -1;
}

static void SetEndSequence (char *nextmap, int type)
{
	int seqnum;

	seqnum = FindEndSequence (type, NULL);
	if (seqnum == -1)
	{
		EndSequence newseq;
		newseq.EndType = type;
		memset (newseq.PicName, 0, sizeof(newseq.PicName));
		seqnum = EndSequences.Push (newseq);
	}
	strcpy (nextmap, "enDSeQ");
	*((WORD *)(nextmap + 6)) = (WORD)seqnum;
}

void G_SetForEndGame (char *nextmap)
{
	if (gamemode == commercial)
	{
		SetEndSequence (nextmap, END_Cast);
	}
	else
	{ // The ExMx games actually have different ends based on the episode,
	  // but I want to keep this simple.
		SetEndSequence (nextmap, END_Pic1);
	}
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

	// sv_nextmap cvar may be overridden by a script
	sv_nextmap.ForceSet(d_mapname);
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

std::vector<std::string> dummy_need_hashes;

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
	} else {
        wads.push_back(wadfiles[1].c_str());
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
	size_t i, j, random_seed;
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
		
    srand((unsigned)time(0)); 

	// Now populate the list
	for (i = 0; i < Count; i++)
	{
	    random_seed = rand();
		j = random_seed % Count;

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

CVAR_FUNC_IMPL (sv_shufflemaplist)
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
	    if (argc > 2 && !W_IsIWAD(argv[2]))
        {
            Printf(PRINT_HIGH,"IWAD not specified, map will not be loaded.\n");
            return;
        }
        else
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
            if (sv_shufflemaplist)
                G_GenerateRandomMaps();
        }
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
		if(sv_gametype != GM_COOP)
			next = level.mapname;
		else
			if(secretexit && W_CheckNumForName (level.secretmap) != -1)
				next = level.secretmap;

		if (!strncmp (next, "EndGame", 7) || (gamemode == retail_chex && !strncmp (level.nextmap, "E1M6", 4)))
		{
			// NES - exiting a Doom 1 episode moves to the next episode, rather than always going back to E1M1
			if (gameinfo.flags & GI_MAPxx || gamemode == shareware || (!sv_loopepisode &&
				((gamemode == registered && level.cluster == 3) || (gamemode == retail && level.cluster == 4))))
					next = CalcMapName(1, 1);
				else if (sv_loopepisode)
					next = CalcMapName(level.cluster, 1);
				else
					next = CalcMapName(level.cluster+1, 1);
		}

		G_DeferedInitNew(next);
	}
	else
	{
		if (sv_shufflemaplist && RandomMaps.empty() == false)
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
	// [ML] 8/22/2010: There are examples in the wiki that outright don't work
	// when onlcvars (addcommandstring's second param) is true.  Is there a 
	// reason why the mapscripts ahve to be safe mode?	
	if(strlen(sv_endmapscript.cstring()))
		AddCommandString(sv_endmapscript.cstring()/*, true*/);
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

	sv_curmap.ForceSet(d_mapname);

	G_InitNew (d_mapname);
	gameaction = ga_nothing;

	// run script at the start of each map
	// [ML] 8/22/2010: There are examples in the wiki that outright don't work
	// when onlcvars (addcommandstring's second param) is true.  Is there a 
	// reason why the mapscripts ahve to be safe mode?
	if(strlen(sv_startmapscript.cstring()))
		AddCommandString(sv_startmapscript.cstring()/*,true*/);

	for(i = 0; i < players.size(); i++)
	{
		if(!players[i].ingame())
			continue;

		if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
			SV_CheckTeam(players[i]);
		else
			players[i].userinfo.color = players[i].prefcolor;

		SV_ClientFullUpdate (players[i]);
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
		for (i = 0; i < numwadlevelinfos; i++)
			wadlevelinfos[i].flags &= ~LEVEL_VISITED;

		for (i = 0; LevelInfos[i].mapname[0]; i++)
			LevelInfos[i].flags &= ~LEVEL_VISITED;
	}

	int old_gametype = sv_gametype;

	cvar_t::UnlatchCVars ();

	if(old_gametype != sv_gametype || sv_gametype != GM_COOP) {
		unnatural_level_progression = true;

		// Nes - Force all players to be spectators when the sv_gametype is not now or previously co-op.
		for (i = 0; i < players.size(); i++) {
			for (size_t j = 0; j < players.size(); j++) {
				MSG_WriteMarker (&(players[j].client.reliablebuf), svc_spectate);
				MSG_WriteByte (&(players[j].client.reliablebuf), players[i].id);
				MSG_WriteByte (&(players[j].client.reliablebuf), true);
			}
			players[i].mo->flags |= MF_SPECTATOR;
			players[i].mo->flags2 |= MF2_FLY;
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
	if(sv_maxplayers == 1)
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
	sky1texture = R_TextureNumForName (level.skypic);

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
	P_DoDeferedScripts ();	// [RH] Do script actions that were triggered on another map.
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

	if ((i = FindWadLevelInfo (level.mapname)) > -1)
	{
		level_pwad_info_t *pinfo = wadlevelinfos + i;

		// [ML] 5/11/06 - Remove sky scrolling and sky2
		level.info = (level_info_t *)pinfo;
		info = (level_info_t *)pinfo;
		level.fadeto = pinfo->fadeto;
		
		if (level.fadeto)
		{
//			NormalLight.maps = DefaultPalette->maps.colormaps;
		}
		else
		{
//			R_SetDefaultColormap (pinfo->fadetable);
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
	
		SetEndSequence (LevelInfos[16].nextmap, END_Underwater);
		SetEndSequence (LevelInfos[25].nextmap, END_Demon);

		LevelInfos[8].nextmap[3] = LevelInfos[8].secretmap[3] = '7';
		LevelInfos[17].nextmap[3] = LevelInfos[17].secretmap[3] = '5';
		LevelInfos[26].nextmap[3] = LevelInfos[26].secretmap[3] = '5';
		LevelInfos[35].nextmap[3] = LevelInfos[35].secretmap[3] = '5';
	}
	else if (gameinfo.gametype == GAME_Doom)
	{
		SetEndSequence (LevelInfos[16].nextmap, END_Pic2);
		SetEndSequence (LevelInfos[25].nextmap, END_Bunny);
	}

	SetEndSequence (LevelInfos[7].nextmap, END_Pic1);
	SetEndSequence (LevelInfos[34].nextmap, END_Pic3);
	SetEndSequence (LevelInfos[43].nextmap, END_Pic3);
	SetEndSequence (LevelInfos[77].nextmap, END_Cast);

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

