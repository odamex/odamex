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
//	Common level routines
//
//-----------------------------------------------------------------------------

#include "g_level.h"

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
#include "v_video.h"
#include "w_wad.h"
#include "w_ident.h"
#include "z_zone.h"

#define lioffset(x)		offsetof(level_pwad_info_t,x)
#define cioffset(x)		offsetof(cluster_info_t,x)

level_locals_t level;			// info about current level

std::vector<level_pwad_info_t> wadlevelinfos;
std::vector<cluster_info_t> wadclusterinfos;

BOOL HexenHack;

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
	MITL_CLUSTERDEF,
};

static const char *MapInfoMapLevel[] =
{
	"levelnum",
	"next",
	"secretnext",
	"cluster",
	"sky1",
	"sky2",
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
	"gravity",
	"aircontrol",
	"islobby",					// Support for lobbies
	"lobby",					// Alias for "islobby"
	NULL
};

enum EMIType
{
	MITYPE_IGNORE,
	MITYPE_EATNEXT,
	MITYPE_INT,
	MITYPE_FLOAT,
	MITYPE_COLOR,
	MITYPE_MAPNAME,
	MITYPE_LUMPNAME,
	MITYPE_SKY,
	MITYPE_SETFLAG,
	MITYPE_SCFLAGS,
	MITYPE_CLUSTER,
	MITYPE_STRING,
	MITYPE_CSTRING,
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
	{ MITYPE_SKY,		lioffset(skypic), 0 },
	{ MITYPE_SKY,		lioffset(skypic2), 0 },
	{ MITYPE_COLOR,		lioffset(fadeto_color), 0 },
	{ MITYPE_COLOR,		lioffset(outsidefog_color), 0 },
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
	{ MITYPE_EATNEXT,	0, 0 },
	{ MITYPE_FLOAT,		lioffset(gravity), 0 },
	{ MITYPE_FLOAT,		lioffset(aircontrol), 0 },
	{ MITYPE_SETFLAG,	LEVEL_LOBBYSPECIAL, 0},
	{ MITYPE_SETFLAG,	LEVEL_LOBBYSPECIAL, 0},
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

int FindWadLevelInfo (char *name)
{
	for (size_t i = 0; i < wadlevelinfos.size(); i++)
		if (!strnicmp (name, wadlevelinfos[i].mapname, 8))
			return i;

	return -1;
}

int FindWadClusterInfo (int cluster)
{
	for (size_t i = 0; i < wadclusterinfos.size(); i++)
		if (wadclusterinfos[i].cluster == cluster)
			return i;

	return -1;
}

static void SetLevelDefaults (level_pwad_info_t *levelinfo)
{
	memset (levelinfo, 0, sizeof(*levelinfo));
	levelinfo->snapshot = NULL;
	levelinfo->outsidefog_color[0] = 255; 
	levelinfo->outsidefog_color[1] = 0; 
	levelinfo->outsidefog_color[2] = 0; 
	levelinfo->outsidefog_color[3] = 0; 
	strncpy(levelinfo->fadetable, "COLORMAP", 8);
}

//
// G_ParseMapInfo
// Parses the MAPINFO lumps of all loaded WADs and generates
// data for wadlevelinfos and wadclusterinfos.
//
void G_ParseMapInfo (void)
{
	level_pwad_info_t defaultinfo;
	level_pwad_info_t *levelinfo;
	int levelindex;
	cluster_info_t *clusterinfo;
	int clusterindex;
	DWORD levelflags;

	int lump = -1;
	while ((lump = wads.FindLump("MAPINFO", lump)) != -1)
	{
		SetLevelDefaults (&defaultinfo);
		sc.OpenLumpNum (lump, "MAPINFO");

		while (sc.GetString ())
		{
			switch (sc.MustMatchString (MapInfoTopLevel))
			{
			case MITL_DEFAULTMAP:
				SetLevelDefaults (&defaultinfo);
				ParseMapInfoLower (MapHandlers, MapInfoMapLevel, &defaultinfo, NULL, 0);
				break;

			case MITL_MAP:		// map <MAPNAME> <Nice Name>
				levelflags = defaultinfo.flags;
				sc.MustGetString ();
				if (IsNum (sc.String))
				{	// MAPNAME is a number, assume a Hexen wad
					int map = atoi (sc.String);
					sprintf (sc.String, "MAP%02d", map);
					SKYFLATNAME[5] = 0;
					HexenHack = true;
					// Hexen levels are automatically nointermission
					// and even lighting and no auto sound sequences
					levelflags |= LEVEL_NOINTERMISSION
								| LEVEL_EVENLIGHTING
								| LEVEL_SNDSEQTOTALCTRL;
				}
				levelindex = FindWadLevelInfo (sc.String);
				if (levelindex == -1)
				{
					wadlevelinfos.push_back(level_pwad_info_t());
					levelindex = wadlevelinfos.size() - 1;
				}
				levelinfo = &wadlevelinfos[levelindex];
				memcpy (levelinfo, &defaultinfo, sizeof(level_pwad_info_t));
				uppercopy (levelinfo->mapname, sc.String);
				sc.MustGetString ();
				ReplaceString (&levelinfo->level_name, sc.String);
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
				sc.MustGetNumber ();
				clusterindex = FindWadClusterInfo (sc.Number);
				if (clusterindex == -1)
				{
					wadclusterinfos.push_back(cluster_info_t());
					clusterindex = wadclusterinfos.size() - 1;
					memset(&wadclusterinfos[clusterindex], 0, sizeof(cluster_info_t));
				}
				clusterinfo = &wadclusterinfos[clusterindex];
				clusterinfo->cluster = sc.Number;
				ParseMapInfoLower (ClusterHandlers, MapInfoClusterLevel, NULL, clusterinfo, 0);
				break;
			}
		}
		sc.Close ();
	}
}

static void ParseMapInfoLower (MapInfoHandler *handlers,
							   const char *strings[],
							   level_pwad_info_t *levelinfo,
							   cluster_info_t *clusterinfo,
							   DWORD flags)
{
	MapInfoHandler *handler;

	byte* info = levelinfo ? (byte*)levelinfo : (byte*)clusterinfo;

	while (sc.GetString())
	{
		if (sc.MatchString(MapInfoTopLevel) != -1)
		{
			sc.UnGet();
			break;
		}

		int entry = sc.MustMatchString(strings);
		if (entry == -1)
			continue;
    
		handler = handlers + entry;

		switch (handler->type)
		{
		case MITYPE_IGNORE:
			break;

		case MITYPE_EATNEXT:
			sc.MustGetString();
			break;

		case MITYPE_INT:
			sc.MustGetNumber();
			*((int*)(info + handler->data1)) = sc.Number;
			break;

		case MITYPE_FLOAT:
			sc.MustGetFloat();
			*((float*)(info + handler->data1)) = sc.Float;
			break;

		case MITYPE_COLOR:
			{
			sc.MustGetString();

				argb_t color(V_GetColorFromString(sc.String));
				uint8_t* ptr = (uint8_t*)(info + handler->data1);
				ptr[0] = color.geta(); ptr[1] = color.getr(); ptr[2] = color.getg(); ptr[3] = color.getb();
			}
			break;

		case MITYPE_MAPNAME:
			sc.MustGetString();
			if (IsNum(sc.String))
			{
				int map = atoi(sc.String);
				sprintf(sc.String, "MAP%02d", map);
			}
			strncpy((char*)(info + handler->data1), sc.String, 8);
			break;

		case MITYPE_LUMPNAME:
			sc.MustGetString();
			uppercopy((char*)(info + handler->data1), sc.String);
			break;

		case MITYPE_SKY:
			sc.MustGetString();	// get texture name;
			uppercopy((char*)(info + handler->data1), sc.String);
			sc.MustGetFloat();		// get scroll speed
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
			sc.MustGetNumber();
			*((int*)(info + handler->data1)) = sc.Number;
			if (HexenHack)
			{
				cluster_info_t* clusterH = FindClusterInfo(sc.Number);
				if (clusterH)
					clusterH->flags |= CLUSTER_HUB;
			}
			break;

		case MITYPE_STRING:
			sc.MustGetString();
			ReplaceString((const char**)(info + handler->data1), sc.String);
			break;

		case MITYPE_CSTRING:
			sc.MustGetString();
			strncpy((char*)(info + handler->data1), sc.String, handler->data2);
			*((char*)(info + handler->data1 + handler->data2)) = '\0';
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
	for (i = 0; i < wadlevelinfos.size(); i++)
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

// [ML] Not sure where to put this for now...
// 	G_ParseMusInfo
void G_ParseMusInfo(void)
{
	// Nothing yet...
}

//
// G_LoadWad
//
// Determines if the vectors of wad & patch filenames differs from the currently
// loaded ones and calls D_DoomWadReboot if so.
//
bool G_LoadWad(	const std::vector<std::string> &newwadfiles,
				const std::vector<std::string> &newpatchfiles,
				const std::vector<std::string> &newwadhashes,
				const std::vector<std::string> &newpatchhashes,
				const std::string &mapname)
{
	bool AddedIWAD = false;
	bool Reboot = false;
	size_t i, j;

	// Did we pass an IWAD?
	if (!newwadfiles.empty() && W_IsIWAD(newwadfiles[0]))
		AddedIWAD = true;

	// Check our environment, if the same WADs are used, ignore this command.

	// Did we switch IWAD files?
	if (AddedIWAD && !wadfiles.empty())
	{
		if (!iequals(M_ExtractFileName(newwadfiles[0]), M_ExtractFileName(wadfiles[1])))
			Reboot = true;
	}

	// Do the sizes of the WAD lists not match up?
	if (!Reboot)
	{
		if (wadfiles.size() - 2 != newwadfiles.size() - (AddedIWAD ? 1 : 0))
			Reboot = true;
	}

	// Do our WAD lists match up exactly?
	if (!Reboot)
	{
		for (i = 2, j = (AddedIWAD ? 1 : 0); i < wadfiles.size() && j < newwadfiles.size(); i++, j++)
		{
			if (!iequals(M_ExtractFileName(newwadfiles[j]), M_ExtractFileName(wadfiles[i])))
			{
				Reboot = true;
				break;
			}
		}
	}

	// Do the sizes of the patch lists not match up?
	if (!Reboot)
	{
		if (patchfiles.size() != newpatchfiles.size())
			Reboot = true;
	}

	// Do our patchfile lists match up exactly?
	if (!Reboot)
	{
		for (i = 0, j = 0; i < patchfiles.size() && j < newpatchfiles.size(); i++, j++)
		{
			if (!iequals(M_ExtractFileName(newpatchfiles[j]), M_ExtractFileName(patchfiles[i])))
			{
				Reboot = true;
				break;
			}
		}
	}

	if (Reboot)
	{
		unnatural_level_progression = true;

		// [SL] Stop any playing/recording demos before D_DoomWadReboot wipes out
		// the zone memory heap and takes the demo data with it.
#ifdef CLIENT_APP
		{
			G_CheckDemoStatus();
		}
#endif
		D_DoomWadReboot(newwadfiles, newpatchfiles, newwadhashes, newpatchhashes);
		if (!missingfiles.empty())
		{
			G_DeferedInitNew(startmap);
			return false;
		}
	}

	if (mapname.length())
	{
		if (wads.CheckNumForName(mapname.c_str()) != -1)
            G_DeferedInitNew((char *)mapname.c_str());
        else
        {
            Printf(PRINT_HIGH, "map %s not found, loading start map instead", mapname.c_str());
            G_DeferedInitNew(startmap);
        }
	}
	else
		G_DeferedInitNew(startmap);

	return true;
}

const char *ParseString2(const char *data);

//
// G_LoadWad
//
// Takes a space-separated string list of wad and patch names, which is parsed
// into a vector of wad filenames and patch filenames and then calls
// D_DoomWadReboot.
//
bool G_LoadWad(const std::string &str, const std::string &mapname)
{
	std::vector<std::string> newwadfiles;
	std::vector<std::string> newpatchfiles;
	std::vector<std::string> nohashes;	// intentionally empty

	const char *data = str.c_str();

	for (size_t argv = 0; (data = ParseString2(data)); argv++)
	{
		std::string ext;

		if (argv == 0 && W_IsIWAD(com_token))
		{
			// Add an IWAD
			std::string iwad_name(com_token);

			// The first argument in the string can be the name of an IWAD
			// with the WAD extension omitted
			M_AppendExtension(iwad_name, ".wad");

			newwadfiles.push_back(iwad_name);
		}
		else if (M_ExtractFileExtension(com_token, ext))
		{
			if (iequals(ext, "wad") && !W_IsIWAD(com_token))
				newwadfiles.push_back(com_token);
			else if (iequals(ext, "deh") || iequals(ext, "bex"))
				newpatchfiles.push_back(com_token);		// Patch file
		}
	}

	return G_LoadWad(newwadfiles, newpatchfiles, nohashes, nohashes, mapname);
}


BEGIN_COMMAND (map)
{
	if (argc > 1)
	{
		char mapname[32];

		// [Dash|RD] -- We can make a safe assumption that the user might not specify
		//              the whole lumpname for the level, and might opt for just the
		//              number. This makes sense, so why isn't there any code for it?
		if (wads.CheckNumForName (argv[1]) == -1 && isdigit(argv[1][0]))
		{ // The map name isn't valid, so lets try to make some assumptions for the user.

			// If argc is 2, we assume Doom 2/Final Doom. If it's 3, Ultimate Doom.
            // [Russell] - gamemode is always the better option compared to above
			if ( argc == 2 )
			{
				if ((gameinfo.flags & GI_MAPxx))
                    sprintf( mapname, "MAP%02i", atoi( argv[1] ) );
                else
                    sprintf( mapname, "E%cM%c", argv[1][0], argv[1][1]);

			}

			if (wads.CheckNumForName (mapname) == -1)
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
			// Ch0wW - Map was still not found, so don't bother trying loading the map.
			if (wads.CheckNumForName (argv[1]) == -1)
			{
				Printf (PRINT_HIGH, "Map %s not found.\n", argv[1]);
			}
			else
			{
				unnatural_level_progression = true;
				uppercopy(mapname, argv[1]); // uppercase the mapname
				G_DeferedInitNew (mapname);
			}
		}
	}
	else
	{
		Printf (PRINT_HIGH, "The current map is %s: \"%s\"\n", level.mapname, level.level_name);
	}
}
END_COMMAND (map)

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

level_info_t *FindDefLevelInfo (char *mapname)
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
		return (level_info_t *)(&wadlevelinfos[i]);
	else
		return FindDefLevelInfo (mapname);
}

level_info_t *FindLevelByNum (int num)
{
	{
		for (size_t i = 0; i < wadlevelinfos.size(); i++)
			if (wadlevelinfos[i].levelnum == num)
				return (level_info_t *)(&wadlevelinfos[i]);
	}
	{
		level_info_t *i = LevelInfos;
		while (i->level_name) {
			if (i->levelnum == num && wads.CheckNumForName (i->mapname) != -1)
				return i;
			i++;
		}
		return NULL;
	}
}

cluster_info_t *FindDefClusterInfo (int cluster)
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
		return &wadclusterinfos[i];
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
	for (i = HUSTR_E1M1; i <= HUSTR_E4M9; ++i)
	{
		if (temp[0] < '9')
			temp[0]++;
		else
			temp[0] = '1';

		if ( (namepart = strstr (GStrings(i), temp)) )
		{
			namepart += 2;
			while (*namepart && *namepart <= ' ')
				namepart++;
		}
		else
		{
			namepart = GStrings(i);
		}

		ReplaceString (&LevelInfos[i-HUSTR_E1M1].level_name, namepart);
		//ReplaceString (&LevelInfos[i-HUSTR_E1M1].music, Musics1[i-HUSTR_E1M1]);
	}

	for (i = 0; i < 4; i++)
		ReplaceString (&ClusterInfos[i].exittext, GStrings(E1TEXT+i));

	if (gamemission == pack_plut)
		start = PHUSTR_1;
	else if (gamemission == pack_tnt)
		start = THUSTR_1;
	else
		start = HUSTR_1;

 	for (i = 0; i < 32; i++) {
 		sprintf (temp, "%d:", i + 1);
		if ( (namepart = strstr (GStrings(i+start), temp)) ) {
 			namepart += strlen (temp);
 			while (*namepart && *namepart <= ' ')
 				namepart++;
 		} else {
			namepart = GStrings(i+start);
 		}
 		ReplaceString (&LevelInfos[36+i].level_name, namepart);
 	}

	if (gamemission == pack_plut)
		start = P1TEXT;		// P1TEXT
	else if (gamemission == pack_tnt)
		start = T1TEXT;		// T1TEXT
	else
		start = C1TEXT;		// C1TEXT

	for (i = 0; i < 4; i++)
		ReplaceString (&ClusterInfos[4 + i].exittext, GStrings(start+i));
	for (; i < 6; i++)
		ReplaceString (&ClusterInfos[4 + i].entertext, GStrings(start+i));

	//for (i = 0; i < 15; i++)
	//	ReplaceString (&ClusterInfos[i].messagemusic, Musics4[i]);

	if (level.info)
		strncpy (level.level_name, level.info->level_name, 63);
}


void G_AirControlChanged ()
{
	if (level.aircontrol <= 256)
	{
		level.airfriction = FRACUNIT;
	}
	else
	{
		// Friction is inversely proportional to the amount of control
		float fric = ((float)level.aircontrol/65536.f) * -0.0941f + 1.0004f;
		level.airfriction = (fixed_t)(fric * 65536.f);
	}
}

// Serialize or unserialize the state of the level depending on the state of
// the first parameter.  Second parameter is true if you need to deal with hub
// playerstate.  Third parameter is true if you want to handle playerstate
// yourself (map resets), just make sure you set it the same for both
// serialization and unserialization.
void G_SerializeLevel(FArchive &arc, bool hubLoad, bool noStorePlayers)
{
	if (arc.IsStoring ())
	{
		unsigned int playernum = players.size();
		arc << level.flags
			<< level.fadeto_color[0] << level.fadeto_color[1] << level.fadeto_color[2] << level.fadeto_color[3]
			<< level.found_secrets
			<< level.found_items
			<< level.killed_monsters
			<< level.gravity
			<< level.aircontrol;

		G_AirControlChanged();

		for (int i = 0; i < NUM_MAPVARS; i++)
			arc << level.vars[i];

		if (!noStorePlayers)
			arc << playernum;
	}
	else
	{
		unsigned int playernum;
		arc >> level.flags
			>> level.fadeto_color[0] >> level.fadeto_color[1] >> level.fadeto_color[2] >> level.fadeto_color[3]
			>> level.found_secrets
			>> level.found_items
			>> level.killed_monsters
			>> level.gravity
			>> level.aircontrol;

		G_AirControlChanged();

		for (int i = 0; i < NUM_MAPVARS; i++)
			arc >> level.vars[i];

		if (!noStorePlayers)
		{
			arc >> playernum;
			players.resize(playernum);
		}
	}

	if (!hubLoad && !noStorePlayers)
		P_SerializePlayers(arc);

	P_SerializeThinkers(arc, hubLoad, noStorePlayers);
	P_SerializeWorld(arc);
	P_SerializePolyobjs(arc);
	P_SerializeSounds(arc);
}

// Archives the current level
void G_SnapshotLevel ()
{
	delete level.info->snapshot;

	level.info->snapshot = new FLZOMemFile;
	level.info->snapshot->Open ();

	FArchive arc (*level.info->snapshot);

	G_SerializeLevel (arc, false, false);
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
	G_SerializeLevel (arc, hubLoad, false);
	arc.Close ();
	// No reason to keep the snapshot around once the level's been entered.
	delete level.info->snapshot;
	level.info->snapshot = NULL;
}

void G_ClearSnapshots (void)
{
	size_t i;

	for (i = 0; i < wadlevelinfos.size(); i++)
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
	if (arc.IsStoring())
	{
		for (size_t i = 0; i < wadlevelinfos.size(); i++)
			if (wadlevelinfos[i].snapshot)
				writeSnapShot(arc, (level_info_t*)&wadlevelinfos[i]);

		for (size_t i = 0; LevelInfos[i].level_name; i++)
			if (LevelInfos[i].snapshot)
				writeSnapShot(arc, &LevelInfos[i]);

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
			arc.Read(&mapname[1], 7);
			level_info_t* i = FindLevelInfo(mapname);
			i->snapshot = new FLZOMemFile;
			i->snapshot->Serialize(arc);
			arc >> mapname[0];
		}
	}
}

static void writeDefereds (FArchive &arc, level_info_t *i)
{
	arc.Write (i->mapname, 8);
	arc << i->defered;
}

void P_SerializeACSDefereds(FArchive &arc)
{
	if (arc.IsStoring())
	{
		for (size_t i = 0; i < wadlevelinfos.size(); i++)
			if (wadlevelinfos[i].defered)
				writeDefereds(arc, (level_info_t*)&wadlevelinfos[i]);

		for (size_t i = 0; LevelInfos[i].level_name; i++)
			if (LevelInfos[i].defered)
				writeDefereds(arc, &LevelInfos[i]);

		// Signal end of defereds
		arc << (byte)0;
	}
	else
	{
		char mapname[8];

		P_RemoveDefereds();

		arc >> mapname[0];
		while (mapname[0])
		{
			arc.Read(&mapname[1], 7);
			level_info_t* i = FindLevelInfo(mapname);
			if (i == NULL)
			{
				char name[9];

				strncpy(name, mapname, 8);
				name[8] = 0;
				I_Error("Unknown map '%s' in savegame", name);
			}
			arc >> i->defered;
			arc >> mapname[0];
		}
	}
}

static int		startpos;	// [RH] Support for multiple starts per level

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

EXTERN_CVAR (sv_gravity)
EXTERN_CVAR (sv_aircontrol)
EXTERN_CVAR (sv_allowjump)
EXTERN_CVAR (sv_freelook)

void G_InitLevelLocals()
{
	byte old_fadeto_color[4];
	memcpy(old_fadeto_color, level.fadeto_color, 4);

	level_info_t *info;
	int i;

	R_ExitLevel();

	NormalLight.maps = shaderef_t(&realcolormaps, 0);
	//NormalLight.maps = shaderef_t(&DefaultPalette->maps, 0);

	level.gravity = sv_gravity;
	level.aircontrol = (fixed_t)(sv_aircontrol * 65536.f);
	G_AirControlChanged();

	// clear all ACS variables
	memset(level.vars, 0, sizeof(level.vars));

	if ((i = FindWadLevelInfo(level.mapname)) > -1)
	{
		level_pwad_info_t* pinfo = &wadlevelinfos[i];

		// [ML] 5/11/06 - Remove sky scrolling and sky2
		// [SL] 2012-03-19 - Add sky2 back
		level.info = (level_info_t*)pinfo;
		info = (level_info_t*)pinfo;
		strncpy (level.skypic2, pinfo->skypic2, 8);

		memcpy(level.fadeto_color, pinfo->fadeto_color, 4);
	
		if (level.fadeto_color[0] || level.fadeto_color[1] || level.fadeto_color[2] || level.fadeto_color[3])
			NormalLight.maps = shaderef_t(&V_GetDefaultPalette()->maps, 0);
		else
			R_ForceDefaultColormap(pinfo->fadetable);

		memcpy(level.outsidefog_color, pinfo->outsidefog_color, 4);

		level.flags |= LEVEL_DEFINEDINMAPINFO;
		if (pinfo->gravity != 0.f)
			level.gravity = pinfo->gravity;
		if (pinfo->aircontrol != 0.f)
			level.aircontrol = (fixed_t)(pinfo->aircontrol * 65536.f);
	}
	else
	{
		info = FindDefLevelInfo(level.mapname);
		level.info = info;
		level.skypic2[0] = 0;

		level.fadeto_color[0] = 0;
		level.fadeto_color[1] = 0;
		level.fadeto_color[2] = 0;
		level.fadeto_color[3] = 0;
		level.outsidefog_color[0] = 255;	// 0xFF000000 == special token signaling to not handle it specially
		level.outsidefog_color[1] = 0;
		level.outsidefog_color[2] = 0;
		level.outsidefog_color[3] = 0;
		R_ForceDefaultColormap ("COLORMAP");
	}

	if (info->level_name)
	{
		level.partime = info->partime;
		level.cluster = info->cluster;
		level.flags = info->flags;
		level.levelnum = info->levelnum;

		strncpy(level.level_name, info->level_name, 63);
		strncpy(level.nextmap, info->nextmap, 8);
		strncpy(level.secretmap, info->secretmap, 8);
		strncpy(level.music, info->music, 8);
		strncpy(level.skypic, info->skypic, 8);
		if (!level.skypic2[0])
			strncpy(level.skypic2, level.skypic, 8);
	}
	else
	{
		level.partime = level.cluster = 0;
		strcpy(level.level_name, "Unnamed");
		level.nextmap[0] = level.secretmap[0] = level.music[0] = 0;
		strncpy(level.skypic, "SKY1", 8);
		strncpy(level.skypic2, "SKY1", 8);
		level.flags = 0;
		level.levelnum = 1;
	}
	
	if (level.flags & LEVEL_JUMP_YES)
		sv_allowjump = 1;
	if (level.flags & LEVEL_JUMP_NO)
		sv_allowjump = 0.0;
	if (level.flags & LEVEL_FREELOOK_YES)
		sv_freelook = 1;
	if (level.flags & LEVEL_FREELOOK_NO)
		sv_freelook = 0.0;

//	memset (level.vars, 0, sizeof(level.vars));

	if (memcmp(level.fadeto_color, old_fadeto_color, 4) != 0)
		V_RefreshColormaps();

	movingsectors.clear();
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
