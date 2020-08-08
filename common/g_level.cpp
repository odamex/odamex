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
#include "minilzo.h"
#include "p_acs.h"
#include "p_local.h"
#include "p_saveg.h"
#include "p_unlag.h"
#include "r_data.h"
#include "r_sky.h"
#include "s_sound.h"
#include "sc_man.h"
#include "v_video.h"
#include "w_wad.h"
#include "w_ident.h"
#include "z_zone.h"

#define lioffset(x)		offsetof(level_pwad_info_t,x)
#define cioffset(x)		offsetof(cluster_info_t,x)

level_locals_t level;			// info about current level

//
// LevelInfos methods
//

// Construct from array of levelinfos, ending with an "empty" level
LevelInfos::LevelInfos(const level_info_t* levels) :
	_defaultInfos(levels)
{
	addDefaults();
}

// Destructor frees everything in the class
LevelInfos::~LevelInfos()
{
	clear();
}

// Add default level infos
void LevelInfos::addDefaults()
{
	for (size_t i = 0;; i++)
	{
		const level_info_t& level = _defaultInfos[i];
		if (!level.exists())
			break;

		// Copied, so it can be mutated.
		level_pwad_info_t info = _empty;
		memcpy(&info, &level, sizeof(level));
		_infos.push_back(info);
	}
}

// Get a specific info index
level_pwad_info_t& LevelInfos::at(size_t i)
{
	return _infos.at(i);
}

// Clear all cluster definitions
void LevelInfos::clear()
{
	// Free all strings.
	for (_LevelInfoArray::iterator it = _infos.begin(); it != _infos.end(); it++)
	{
		free(it->level_name);
		it->level_name = NULL;
	}
	clearSnapshots();
	zapDeferreds();
	_infos.clear();
}

// Clear all stored snapshots
void LevelInfos::clearSnapshots()
{
	for (_LevelInfoArray::iterator it = _infos.begin(); it != _infos.end(); it++)
	{
		if (it->snapshot)
		{
			delete it->snapshot;
			it->snapshot = NULL;
		}
	}
}

// Add a new levelinfo and return it by reference
level_pwad_info_t& LevelInfos::create()
{
	_infos.push_back(level_pwad_info_t());
	_infos.back() = _empty;
	return _infos.back();
}

// Find a levelinfo by mapname
level_pwad_info_t& LevelInfos::findByName(char* mapname)
{
	for (_LevelInfoArray::iterator it = _infos.begin(); it != _infos.end(); it++)
	{
		if (stricmp(mapname, it->mapname) == 0)
		{
			return *it;
		}
	}
	return _empty;
}

// Find a levelinfo by mapnum
level_pwad_info_t& LevelInfos::findByNum(int levelnum)
{
	for (_LevelInfoArray::iterator it = _infos.begin(); it != _infos.end(); it++)
	{
		if (it->levelnum == levelnum && W_CheckNumForName(it->mapname) != -1)
		{
			return *it;
		}
	}
	return _empty;
}

// Number of info entries.
size_t LevelInfos::size()
{
	return _infos.size();
}

// Zap all deferred ACS scripts
void LevelInfos::zapDeferreds()
{
	for (_LevelInfoArray::iterator it = _infos.begin(); it != _infos.end(); it++)
	{
		acsdefered_t* def = it->defered;
		while (def) {
			acsdefered_t* next = def->next;
			delete def;
			def = next;
		}
		it->defered = NULL;
	}
}

// Empty levelinfo.
level_pwad_info_t LevelInfos::_empty = {
	"",   // mapname
	0,    // levelnum
	NULL, // level_name
	"",   // pname
	"",   // nextmap
	"",   // secretmap
	0,    // partime
	"",   // skypic
	"",   // music
	0,    // flags
	0,    // cluster
	NULL, // snapshot
	NULL, // defered
	{ 0, 0, 0, 0 },    // fadeto_color
	{ 0xFF, 0, 0, 0 }, // outsidefog_color, 0xFF000000 is special token signaling to not handle it specially
	"COLORMAP",        // fadetable
	"",   // skypic2
	0.0,  // gravity
	0.0,  // aircontrol
};

//
// ClusterInfos methods
//

// Construct from array of clusterinfos, ending with an "empty" cluster.
ClusterInfos::ClusterInfos(const cluster_info_t* clusters) :
	_defaultInfos(clusters)
{
	addDefaults();
}

// Destructor frees everything in the class
ClusterInfos::~ClusterInfos()
{
	clear();
}

// Add default level infos
void ClusterInfos::addDefaults()
{
	for (size_t i = 0;; i++)
	{
		const cluster_info_t& cluster = _defaultInfos[i];
		if (cluster.cluster == 0)
		{
			break;
		}

		// Copied, so it can be mutated.
		_infos.push_back(cluster);
	}
}

// Get a specific info index
cluster_info_t& ClusterInfos::at(size_t i)
{
	return _infos.at(i);
}

// Clear all cluster definitions
void ClusterInfos::clear()
{
	// Free all strings.
	for (_ClusterInfoArray::iterator it = _infos.begin(); it != _infos.end(); it++)
	{
		free(it->exittext);
		it->exittext = NULL;
		free(it->entertext);
		it->entertext = NULL;
	}
	return _infos.clear();
}

// Add a new levelinfo and return it by reference
cluster_info_t& ClusterInfos::create()
{
	_infos.push_back(cluster_info_t());
	_infos.back() = _empty;
	return _infos.back();
}

// Find a clusterinfo by mapname
cluster_info_t& ClusterInfos::findByCluster(int i)
{
	for (_ClusterInfoArray::iterator it = _infos.begin();it != _infos.end();it++)
	{
		if (it->cluster == i)
		{
			return *it;
		}
	}
	return _empty;
}

// Number of info entries.
size_t ClusterInfos::size()
{
	return _infos.size();
}

// Empty clusterinfo.
cluster_info_t ClusterInfos::_empty = {
	0,    // cluster
	"",   // messagemusic
	"",   // finaleflat
	NULL, // exittext
	NULL, // entertext
	0,    // flags
};

// A tagged union that represents all possible infos that we can pass to
// the "lower" MAPINFO parser.
struct tagged_info_t {
	enum tags {
		LEVEL,
		CLUSTER,
		EPISODE,
	};
	tags tag;
	union {
		level_pwad_info_t* level;
		cluster_info_t* cluster;
		void* episode;
	};
};

BOOL HexenHack;

enum EMIType
{
	MITYPE_IGNORE,
	MITYPE_EATNEXT,
	MITYPE_INT,
	MITYPE_FLOAT,
	MITYPE_COLOR,
	MITYPE_MAPNAME,
	MITYPE_LUMPNAME,
	MITYPE_$LUMPNAME,
	MITYPE_MUSICLUMPNAME,
	MITYPE_SKY,
	MITYPE_SETFLAG,
	MITYPE_SCFLAGS,
	MITYPE_CLUSTER,
	MITYPE_STRING,
	MITYPE_CSTRING,
	MITYPE_CLUSTERSTRING,
};

struct MapInfoHandler
{
    EMIType type;
    DWORD data1, data2;
};

static const char *MapInfoTopLevel[] =
{
	"map",
	"defaultmap",
	"cluster",
	"clusterdef",
	"episode",
	"clearepisodes",
	"gameinfo",
	"intermission",
	"automap",
	NULL
};

enum
{
	// map <maplump> <nice name>
	// map <maplump> lookup <keyword>
	MITL_MAP,

	// defaultmap
	MITL_DEFAULTMAP,

	// cluster <value>
	MITL_CLUSTER,

	// clusterdef <value>
	MITL_CLUSTERDEF,

	// episode <maplump>
	// episode <maplump> teaser <maplump> // New MAPINFO only
	MITL_EPISODE,

	// clearepisodes
	MITL_CLEAREPISODES,

	// gameinfo // New MAPINFO only
	MITL_GAMEINFO,

	// intermission // New MAPINFO only
	MITL_INTERMISSION,

	// automap // New MAPINFO only
	MITL_AUTOMAP
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
	"islobby",
	"lobby",
	"nocrouch",
	"intermusic",
	"par",
	"sucktime",
	"translator",
	NULL
};

MapInfoHandler MapHandlers[] =
{
	// levelnum <levelnum>
	{ MITYPE_INT, lioffset(levelnum), 0 },
	// next <maplump>
	{ MITYPE_MAPNAME, lioffset(nextmap), 0 },
	// secretnext <maplump>
	{ MITYPE_MAPNAME, lioffset(secretmap), 0 },
	// cluster <number>
	{ MITYPE_CLUSTER, lioffset(cluster), 0 },
	// sky1 <texture> <scrollspeed>
	{ MITYPE_SKY, lioffset(skypic), 0 },
	// sky2 <texture> <scrollspeed>
	{ MITYPE_SKY, lioffset(skypic2), 0 },
	// fade <color>
	{ MITYPE_COLOR, lioffset(fadeto_color), 0 },
	// outsidefog <color>
	{ MITYPE_COLOR, lioffset(outsidefog_color), 0 },
	// titlepatch <patch>
	{ MITYPE_LUMPNAME, lioffset(pname), 0 },
	// par <partime>
	{ MITYPE_INT, lioffset(partime), 0 },
	// music <musiclump>
	{ MITYPE_MUSICLUMPNAME, lioffset(music), 0 },
	// nointermission
	{ MITYPE_SETFLAG, LEVEL_NOINTERMISSION, 0 },
	// doublesky
	{ MITYPE_SETFLAG, LEVEL_DOUBLESKY, 0 },
	// nosoundclipping
	{ MITYPE_SETFLAG, LEVEL_NOSOUNDCLIPPING, 0 },
	// allowmonstertelefrags
	{ MITYPE_SETFLAG, LEVEL_MONSTERSTELEFRAG, 0 },
	// map07special
	{ MITYPE_SETFLAG, LEVEL_MAP07SPECIAL, 0 },
	// baronspecial
	{ MITYPE_SETFLAG, LEVEL_BRUISERSPECIAL, 0 },
	// cyberdemonspecial
	{ MITYPE_SETFLAG, LEVEL_CYBORGSPECIAL, 0 },
	// spidermastermindspecial
	{ MITYPE_SETFLAG, LEVEL_SPIDERSPECIAL, 0 },
	// specialaction_exitlevel
	{ MITYPE_SCFLAGS, 0, ~LEVEL_SPECACTIONSMASK },
	// specialaction_opendoor
	{ MITYPE_SCFLAGS, LEVEL_SPECOPENDOOR, ~LEVEL_SPECACTIONSMASK },
	// specialaction_lowerfloor
	{ MITYPE_SCFLAGS, LEVEL_SPECLOWERFLOOR, ~LEVEL_SPECACTIONSMASK },
	// lightning
	{ MITYPE_IGNORE, 0, 0 },
	// fadetable <colormap>
	{ MITYPE_LUMPNAME, lioffset(fadetable), 0 },
	// evenlighting
	{ MITYPE_SETFLAG, LEVEL_EVENLIGHTING, 0 },
	// noautosequences
	{ MITYPE_SETFLAG, LEVEL_SNDSEQTOTALCTRL, 0 },
	// forcenoskystretch
	{ MITYPE_SETFLAG, LEVEL_FORCENOSKYSTRETCH, 0 },
	// allowfreelook
	{ MITYPE_SCFLAGS, LEVEL_FREELOOK_YES, ~LEVEL_FREELOOK_NO },
	// nofreelook
	{ MITYPE_SCFLAGS, LEVEL_FREELOOK_NO, ~LEVEL_FREELOOK_YES },
	// allowjump
	{ MITYPE_SCFLAGS, LEVEL_JUMP_YES, ~LEVEL_JUMP_NO },
	// nojump
	{ MITYPE_SCFLAGS, LEVEL_JUMP_NO, ~LEVEL_JUMP_YES },
	// cdtrack <track number>
	{ MITYPE_EATNEXT, 0, 0 },
	// cd_start_track ???
	{ MITYPE_EATNEXT, 0, 0 },
	// cd_end1_track ???
	{ MITYPE_EATNEXT, 0, 0 },
	// cd_end2_track ???
	{ MITYPE_EATNEXT, 0, 0 },
	// cd_end3_track ???
	{ MITYPE_EATNEXT, 0, 0 },
	// cd_intermission_track ???
	{ MITYPE_EATNEXT, 0, 0 },
	// cd_title_track ???
	{ MITYPE_EATNEXT, 0, 0 },
	// warptrans ???
	{ MITYPE_EATNEXT, 0, 0 },
	// gravity <amount>
	{ MITYPE_FLOAT, lioffset(gravity), 0 },
	// aircontrol <amount>
	{ MITYPE_FLOAT, lioffset(aircontrol), 0 },
	// islobby
	{ MITYPE_SETFLAG, LEVEL_LOBBYSPECIAL, 0},
	// lobby
	{ MITYPE_SETFLAG, LEVEL_LOBBYSPECIAL, 0},
	// nocrouch
	{ MITYPE_IGNORE, 0, 0 },
	// intermusic <musicname>
	{ MITYPE_EATNEXT, 0, 0 },
	// par <partime>
	{ MITYPE_EATNEXT, 0, 0 },
	// sucktime <value>
	{ MITYPE_EATNEXT, 0, 0 },
	// translator <value>
	{ MITYPE_EATNEXT, 0, 0 },
};

static const char *MapInfoClusterLevel[] =
{
	"entertext",
	"exittext",
	"exittextislump",
	"music",
	"flat",
	"hub",
	"pic",
	NULL
};

MapInfoHandler ClusterHandlers[] =
{
	// entertext <message>
	{ MITYPE_CLUSTERSTRING, cioffset(entertext), 0 },
	// exittext <message>
	{ MITYPE_CLUSTERSTRING, cioffset(exittext), 0 },
	// exittextislump
	{ MITYPE_SETFLAG, CLUSTER_EXITTEXTISLUMP, 0 },
	// messagemusic <musiclump>
	{ MITYPE_MUSICLUMPNAME, cioffset(messagemusic), 8 },
	// flat <flatlump>
	{ MITYPE_$LUMPNAME, cioffset(finaleflat), 0 },
	// hub
	{ MITYPE_SETFLAG, CLUSTER_HUB, 0 },
	// pic <graphiclump>
	{ MITYPE_$LUMPNAME, cioffset(finalepic), 0 },
};

static const char* MapInfoEpisodeLevel[] =
{
	"name",
	"lookup",
	"picname",
	"key",
	"remove",
	"noskillmenu",
	"optional",
	NULL
};

MapInfoHandler EpisodeHandlers[] =
{
	// name <nice name>
	{ MITYPE_EATNEXT, 0, 0 },
	// lookup <keyword>
	{ MITYPE_EATNEXT, 0, 0 },
	// picname <piclump>
	{ MITYPE_EATNEXT, 0, 0 },
	// remove
	{ MITYPE_IGNORE, 0, 0 },
	// noskillmenu
	{ MITYPE_IGNORE, 0, 0 },
	// optional
	{ MITYPE_IGNORE, 0, 0 }
};

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
// Assumes that you have munched the last parameter you know how to handle,
// but have not yet munched a comma.
//
static void SkipUnknownParams()
{
	// Every loop, try to burn a comma.
	while (SC_GetString())
	{
		if (!SC_Compare(","))
		{
			SC_UnGet();
			return;
		}

		// Burn the parameter.
		SC_GetString();
	}
}

//
// Assumes that you have already munched the unknown type name, and just need
// to much parameters, if any.
//
static void SkipUnknownType()
{
	SC_GetString();
	if (!SC_Compare("="))
	{
		SC_UnGet();
		return;
	}

	SC_GetString(); // Get the first parameter
	SkipUnknownParams();
}

//
// Assumes you have already munched the first opening brace.
//
// This function does not work with old-school ZDoom MAPINFO.
//
static void SkipUnknownBlock()
{
	int stack = 0;

	while (SC_GetString())
	{
		if (SC_Compare("{"))
		{
			// Found another block
			stack++;
			continue;
		}
		else if (SC_Compare("}"))
		{
			stack--;
			if (stack <= 0)
			{
				// Done with all blocks
				break;
			}
		}
	}
}

//
// Parse a MAPINFO block
//
// NULL pointers can be passed if the block is unimplemented.  However, if
// the block you want to stub out is compatible with old MAPINFO, you need
// to parse the block anyway, even if you throw away the values.  This is
// done by passing in a strings pointer, and leaving the others NULL.
//
static void ParseMapInfoLower(
	MapInfoHandler* handlers, const char** strings, tagged_info_t* tinfo, DWORD flags
)
{
	// 0 if old mapinfo, positive number if new MAPINFO, the exact
	// number represents current brace depth.
	int newMapinfoStack = 0;

	byte* info = NULL;
	if (tinfo)
	{
		// The union pointer is always the same, regardless of the tag.
		info = reinterpret_cast<byte*>(tinfo->level);
	}

	while (SC_GetString())
	{
		if (SC_Compare("{"))
		{
			// Detected new-style MAPINFO
			newMapinfoStack++;
			continue;
		}
		else if (SC_Compare("}"))
		{
			newMapinfoStack--;
			if (newMapinfoStack <= 0)
			{
				// MAPINFO block is done
				break;
			}
		}

		if (
			newMapinfoStack <= 0 &&
			SC_MatchString(MapInfoTopLevel) != SC_NOMATCH &&
			// "cluster" is a valid map block type and is also
			// a valid top-level type.
			!SC_Compare("cluster")
		)
		{
			// Old-style MAPINFO is done
			SC_UnGet();
			break;
		}

		int entry = SC_MatchString(strings);
		if (entry == SC_NOMATCH)
		{
			if (newMapinfoStack <= 0)
			{
				// Old MAPINFO is up a creek, we need to be
				// able to parse all types even if we can't
				// do anything with them.
				SC_ScriptError("Unknown MAPINFO token \"%s\"", sc_String);
			}

			// New MAPINFO is capable of skipping past unknown
			// types.
			SkipUnknownType();
			continue;
		}

		MapInfoHandler* handler = handlers + entry;

		switch (handler->type)
		{
		case MITYPE_IGNORE:
			break;

		case MITYPE_EATNEXT:
			if (newMapinfoStack > 0)
			{
				SC_MustGetStringName("=");
			}

			SC_MustGetString();
			break;

		case MITYPE_INT:
			if (newMapinfoStack > 0)
			{
				SC_MustGetStringName("=");
			}

			SC_MustGetNumber();
			*((int*)(info + handler->data1)) = sc_Number;
			break;

		case MITYPE_FLOAT:
			if (newMapinfoStack > 0)
			{
				SC_MustGetStringName("=");
			}

			SC_MustGetFloat();
			*((float*)(info + handler->data1)) = sc_Float;
			break;

		case MITYPE_COLOR:
		{
			if (newMapinfoStack > 0)
			{
				SC_MustGetStringName("=");
			}

			SC_MustGetString();
			argb_t color(V_GetColorFromString(sc_String));
			uint8_t* ptr = (uint8_t*)(info + handler->data1);
			ptr[0] = color.geta(); ptr[1] = color.getr(); ptr[2] = color.getg(); ptr[3] = color.getb();
			break;
		}
		case MITYPE_MAPNAME:
			if (newMapinfoStack > 0)
			{
				SC_MustGetStringName("=");
			}

			SC_MustGetString();
			if (IsNum(sc_String))
			{
				int map = atoi(sc_String);
				sprintf(sc_String, "MAP%02d", map);
			}
			strncpy((char*)(info + handler->data1), sc_String, 8);
			break;

		case MITYPE_LUMPNAME:
			if (newMapinfoStack > 0)
			{
				SC_MustGetStringName("=");
			}

			SC_MustGetString();
			uppercopy((char*)(info + handler->data1), sc_String);
			break;


		case MITYPE_$LUMPNAME:
			if (newMapinfoStack > 0)
			{
				SC_MustGetStringName("=");
			}

			SC_MustGetString();
			if (sc_String[0] == '$')
			{
				// It is possible to pass a DeHackEd string
				// prefixed by a $.
				const OString& s = GStrings(sc_String + 1);
				if (s.empty())
				{
					SC_ScriptError("Unknown lookup string \"%s\"", s.c_str());
				}
				uppercopy((char*)(info + handler->data1), s.c_str());
			}
			else
			{
				uppercopy((char*)(info + handler->data1), sc_String);
			}
			break;

		case MITYPE_MUSICLUMPNAME:
		{
			if (newMapinfoStack > 0)
			{
				SC_MustGetStringName("=");
			}

			SC_MustGetString();
			if (sc_String[0] == '$')
			{
				// It is possible to pass a DeHackEd string
				// prefixed by a $.
				const OString& s = GStrings(sc_String + 1);
				if (s.empty())
				{
					SC_ScriptError("Unknown lookup string \"%s\"", s.c_str());
				}

				// Music lumps in the stringtable do not begin
				// with a D_, so we must add it.
				char lumpname[9];
				snprintf(lumpname, ARRAY_LENGTH(lumpname), "D_%s", s.c_str());
				uppercopy((char*)(info + handler->data1), lumpname);
			}
			else
			{
				uppercopy((char*)(info + handler->data1), sc_String);
			}
			break;
		}
		case MITYPE_SKY:
			if (newMapinfoStack > 0)
			{
				SC_MustGetStringName("=");
				SC_MustGetString(); // Texture name
				uppercopy((char*)(info + handler->data1), sc_String);
				SkipUnknownParams();
			}
			else
			{
				SC_MustGetString();	// get texture name;
				uppercopy((char*)(info + handler->data1), sc_String);
				SC_MustGetFloat();		// get scroll speed
				//if (HexenHack)
				//{
				//	*((fixed_t *)(info + handler->data2)) = sc_Number << 8;
				//}
				//else
				//{
				//	*((fixed_t *)(info + handler->data2)) = (fixed_t)(sc_Float * 65536.0f);
				//}
			}
			break;

		case MITYPE_SETFLAG:
			flags |= handler->data1;
			break;

		case MITYPE_SCFLAGS:
			flags = (flags & handler->data2) | handler->data1;
			break;

		case MITYPE_CLUSTER:
			if (newMapinfoStack > 0)
			{
				SC_MustGetStringName("=");
			}

			SC_MustGetNumber();
			*((int*)(info + handler->data1)) = sc_Number;
			if (HexenHack)
			{
				ClusterInfos& clusters = getClusterInfos();
				cluster_info_t& clusterH = clusters.findByCluster(sc_Number);
				if (clusterH.cluster != 0)
				{
					clusterH.flags |= CLUSTER_HUB;
				}
			}
			break;

		case MITYPE_STRING:
		{
			char** text = (char**)(info + handler->data1);

			if (newMapinfoStack > 0)
			{
				SC_MustGetStringName("=");
			}

			SC_MustGetString();
			free(*text);
			*text = strdup(sc_String);
			break;
		}

		case MITYPE_CSTRING:
			if (newMapinfoStack > 0)
			{
				SC_MustGetStringName("=");
			}

			SC_MustGetString();
			strncpy((char*)(info + handler->data1), sc_String, handler->data2);
			*((char*)(info + handler->data1 + handler->data2)) = '\0';
			break;
		case MITYPE_CLUSTERSTRING:
		{
			char** text = (char**)(info + handler->data1);

			if (newMapinfoStack > 0)
			{
				SC_MustGetStringName("=");
				SC_MustGetString();
				if (SC_Compare("lookup"))
				{
					SC_MustGetStringName(",");
					SC_MustGetString();
					const OString& s = GStrings(sc_String);
					if (s.empty())
					{
						SC_ScriptError("Unknown lookup string \"%s\"", sc_String);
					}
					free(*text);
					*text = strdup(s.c_str());
				}
				else
				{
					// One line per string.
					std::string ctext;
					SC_UnGet();
					do
					{
						SC_MustGetString();
						ctext += sc_String;
						ctext += "\n";
						SC_GetString();
					} while (SC_Compare(","));
					SC_UnGet();

					// Trim trailing newline.
					if (ctext.length() > 0)
					{
						ctext.resize(ctext.length() - 1);
					}

					free(*text);
					*text = strdup(ctext.c_str());
				}
			}
			else
			{
				SC_MustGetString();
				if (SC_Compare("lookup"))
				{
					SC_MustGetString();
					const OString& s = GStrings(sc_String);
					if (s.empty())
					{
						SC_ScriptError("Unknown lookup string \"%s\"", sc_String);
					}

					free(*text);
					*text = strdup(s.c_str());
				}
				else
				{
					free(*text);
					*text = strdup(sc_String);
				}
			}
			break;
		}}
	}

	if (tinfo == NULL)
	{
		return;
	}

	switch (tinfo->tag)
	{
	case tagged_info_t::LEVEL:
		tinfo->level->flags = flags;
		break;
	case tagged_info_t::CLUSTER:
		tinfo->cluster->flags = flags;
		break;
	}
}

static void ParseMapInfoLump(int lump, const char* lumpname)
{
	LevelInfos& levels = getLevelInfos();
	ClusterInfos& clusters = getClusterInfos();

	level_pwad_info_t defaultinfo;
	DWORD levelflags;

	SetLevelDefaults (&defaultinfo);
	SC_OpenLumpNum (lump, lumpname);

	while (SC_GetString ())
	{
		switch (SC_MustMatchString (MapInfoTopLevel))
		{
		case MITL_DEFAULTMAP:
		{
			SetLevelDefaults(&defaultinfo);
			tagged_info_t tinfo;
			tinfo.tag = tagged_info_t::LEVEL;
			tinfo.level = &defaultinfo;
			ParseMapInfoLower(MapHandlers, MapInfoMapLevel, &tinfo, 0);
			break;
		}
		case MITL_MAP:
		{
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

			// Find the level.
			level_pwad_info_t& info = (levels.findByName(sc_String).exists()) ?
				levels.findByName(sc_String) :
				levels.create();

			// Free the level name string before we pave over it.
			free(info.level_name);

			info = defaultinfo;
			uppercopy(info.mapname, sc_String);

			// Map name.
			SC_MustGetString();
			if (SC_Compare("lookup"))
			{
				SC_MustGetString();
				const OString& s = GStrings(sc_String);
				if (s.empty())
				{
					SC_ScriptError("Unknown lookup string \"%s\"", sc_String);
				}
				free(info.level_name);
				info.level_name = strdup(s.c_str());
			}
			else
			{
				free(info.level_name);
				info.level_name = strdup(sc_String);
			}

			tagged_info_t tinfo;
			tinfo.tag = tagged_info_t::LEVEL;
			tinfo.level = &info;
			ParseMapInfoLower(MapHandlers, MapInfoMapLevel, &tinfo, levelflags);

			// If the level info was parsed and no levelnum was applied,
			// try and synthesize one from the level name.
			if (info.levelnum == 0)
			{
				if (info.mapname[0] == 'E' && info.mapname[2] == 'M')
				{
					// Convert a char into its equivalent integer.
					int e = info.mapname[1] - '0';
					int m = info.mapname[3] - '0';
					if (e >= 0 && e <= 9 && m >= 0 && m <= 9)
					{
						// Copypasted from the ZDoom wiki.
						info.levelnum = (e - 1) * 10 + m;
					}
				}
				else if (strnicmp(info.mapname, "MAP", 3) == 0)
				{
					// Try and turn the trailing digits after the "MAP" into a
					// level number.
					int mapnum = atoi(info.mapname + 3);
					if (mapnum >= 0 && mapnum <= 99)
					{
						info.levelnum = mapnum;
					}
				}
			}
			break;
		}
		case MITL_CLUSTER:
		case MITL_CLUSTERDEF:
		{
			SC_MustGetNumber();

			// Find the cluster.
			cluster_info_t& info = (clusters.findByCluster(sc_Number).cluster != 0) ?
				clusters.findByCluster(sc_Number) :
				clusters.create();

			info.cluster = sc_Number;
			tagged_info_t tinfo;
			tinfo.tag = tagged_info_t::CLUSTER;
			tinfo.cluster = &info;
			ParseMapInfoLower (ClusterHandlers, MapInfoClusterLevel, &tinfo, 0);
			break;
		}
		case MITL_EPISODE:
		{
			// Not implemented
			SC_MustGetString(); // Map lump
			SC_GetString();
			if (SC_Compare("teaser"))
			{
				SC_MustGetString(); // Teaser lump
			}
			else
			{
				SC_UnGet();
			}

			SC_MustGetString();
			if (SC_Compare("{"))
			{
				// If we encounter an episode block in new MAPINFO, we can just
				// eat the entire block.
				SkipUnknownBlock();
			}
			else
			{
				// If we encounter an episode block in old MAPINFO, we have to
				// parse it and just not do anything with it.
				tagged_info_t tinfo;
				tinfo.tag = tagged_info_t::EPISODE;
				tinfo.episode = NULL;
				ParseMapInfoLower(EpisodeHandlers, MapInfoEpisodeLevel, &tinfo, 0);
			}
			break;
		}
		case MITL_CLEAREPISODES:
			// Not implemented
			break;

		case MITL_GAMEINFO:
			// Not implemented
			ParseMapInfoLower(NULL, NULL, NULL, 0);
			break;

		case MITL_INTERMISSION:
			// Not implemented
			SC_MustGetString(); // Name
			ParseMapInfoLower(NULL, NULL, NULL, 0);
			break;

		case MITL_AUTOMAP:
			// Not implemented
			ParseMapInfoLower(NULL, NULL, NULL, 0);
			break;

		default:
			SC_ScriptError("Unimplemented top-level type \"%s\"", sc_String);
		}
	}
	SC_Close ();
}

//
// G_ParseMapInfo
// Parses the MAPINFO lumps of all loaded WADs and generates
// data for wadlevelinfos and wadclusterinfos.
//
void G_ParseMapInfo (void)
{
	BOOL found_zmapinfo = false;

	int lump = -1;
	while ((lump = W_FindLump("ZMAPINFO", lump)) != -1)
	{
		found_zmapinfo = true;
		ParseMapInfoLump(lump, "ZMAPINFO");
	}

	// If ZMAPINFO exists, we don't parse a normal MAPINFO
	if (found_zmapinfo == true)
	{
		return;
	}

	lump = -1;
	while ((lump = W_FindLump("MAPINFO", lump)) != -1)
	{
		ParseMapInfoLump(lump, "MAPINFO");
	}
}

void P_RemoveDefereds (void)
{
	::getLevelInfos().zapDeferreds();
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
		if (W_CheckNumForName(mapname.c_str()) != -1)
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
		if (W_CheckNumForName (argv[1]) == -1 && isdigit(argv[1][0]))
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
			// Ch0wW - Map was still not found, so don't bother trying loading the map.
			if (W_CheckNumForName (argv[1]) == -1)
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

void G_SetLevelStrings (void)
{
	char temp[9] = "";
	LevelInfos& levels = getLevelInfos();
	ClusterInfos& clusters = getClusterInfos();

	// Plutonia and TNT take the place of Doom 2.
	int hustart, txtstart;
	if (gamemission == pack_plut)
	{
		hustart = GStrings.toIndex(PHUSTR_1);
		txtstart = GStrings.toIndex(P1TEXT);
	}
	else if (gamemission == pack_tnt)
	{
		hustart = GStrings.toIndex(THUSTR_1);
		txtstart = GStrings.toIndex(T1TEXT);
	}
	else
	{
		hustart = GStrings.toIndex(HUSTR_1);
		txtstart = GStrings.toIndex(C1TEXT);
	}

	// Loop through levels and assign any DeHackEd or otherwise dynamic text.
	for (size_t i = 0; i < levels.size(); i++)
	{
		level_pwad_info_t& info = levels.at(i);

		// Level name
		int level_name;
		int muslump;
		if (info.cluster <= 4)
		{
			// Doom 1
			int offset = info.levelnum - 1 - (info.cluster - 1);
			if (offset >= 0 && offset < 36)
			{
				level_name = GStrings.toIndex(HUSTR_E1M1) + offset;
				muslump = GStrings.toIndex(MUSIC_E1M1) + offset;
			}
			else
			{
				level_name = GStrings.toIndex(HUSTR_E1M1);
				muslump = GStrings.toIndex(MUSIC_E1M1);
			}
		}
		else
		{
			// Doom 2 + Expansions
			int offset = (info.levelnum - 1);
			if (offset >= 0 && offset < 32)
			{
				level_name = hustart + offset;
				muslump = GStrings.toIndex(MUSIC_RUNNIN) + offset;
			}
			else
			{
				level_name = GStrings.toIndex(HUSTR_1);
				muslump = GStrings.toIndex(MUSIC_RUNNIN);
			}
		}

		free(info.level_name);
		info.level_name = strdup(GStrings.getIndex(level_name));
	}

	// Loop through clusters and assign any DeHackEd or otherwise dynamic text.
	for (size_t i = 0; i < clusters.size(); i++)
	{
		cluster_info_t& info = clusters.at(i);
		if (info.cluster <= 4)
		{
			// Doom 1

			// Cluster music is all the same.
			snprintf(temp, ARRAY_LENGTH(temp), "D_%s", GStrings(MUSIC_VICTOR));
			uppercopy(info.messagemusic, temp);

			// Exit text at end of episode.
			free(info.exittext);
			info.exittext =
			    strdup(GStrings.getIndex(GStrings.toIndex(E1TEXT) + info.cluster - 1));
		}
		else if (info.cluster <= 8)
		{
			// Doom 2 + Expansions, Normal progression
			snprintf(temp, ARRAY_LENGTH(temp), "D_%s", GStrings(MUSIC_READ_M));
			uppercopy(info.messagemusic, temp);

			// Exit text between clusters.
			free(info.exittext);
			info.exittext = strdup(GStrings.getIndex(txtstart + info.cluster - 5));
		}
		else if (info.cluster <= 10)
		{
			// Doom 2 + Expansions, Secret progression
			snprintf(temp, ARRAY_LENGTH(temp), "D_%s", GStrings(MUSIC_READ_M));
			uppercopy(info.messagemusic, temp);

			// Enter text before secret maps.
			free(info.entertext);
			info.entertext = strdup(GStrings.getIndex(txtstart + info.cluster - 5));
		}

		// Cluster background flat.
		uppercopy(info.finaleflat, GStrings.getIndex(GStrings.toIndex(BGFLATE1) + i));
	}

	if (::level.info)
	{
		strncpy(::level.level_name, ::level.info->level_name, 63);
	}
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
	getLevelInfos().clearSnapshots();
}

static void writeSnapShot (FArchive &arc, level_info_t *i)
{
	arc.Write (i->mapname, 8);
	i->snapshot->Serialize (arc);
}

void G_SerializeSnapshots (FArchive &arc)
{
	LevelInfos& levels = getLevelInfos();

	if (arc.IsStoring())
	{
		for (size_t i = 0; i < levels.size(); i++)
		{
			level_pwad_info_t& level = levels.at(i);
			if (level.snapshot)
			{
				writeSnapShot(arc, reinterpret_cast<level_info_t*>(&level));
			}
		}

		// Signal end of snapshots
		arc << (char)0;
	}
	else
	{
		LevelInfos& levels = getLevelInfos();
		char mapname[8];

		G_ClearSnapshots ();

		arc >> mapname[0];
		while (mapname[0])
		{
			arc.Read(&mapname[1], 7);

			// FIXME: We should really serialize the full levelinfo
			level_info_t narrow = { 0 };
			level_pwad_info_t& info = levels.findByName(mapname);
			memcpy(&narrow, &info, sizeof(narrow));
			info.snapshot = new FLZOMemFile;
			info.snapshot->Serialize(arc);
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
	LevelInfos& levels = getLevelInfos();

	if (arc.IsStoring())
	{
		for (size_t i = 0; i < levels.size(); i++)
		{
			level_pwad_info_t& level = levels.at(i);
			if (level.defered)
			{
				writeDefereds(arc, reinterpret_cast<level_info_t*>(&level));
			}
		}

		// Signal end of defereds
		arc << (byte)0;
	}
	else
	{
		LevelInfos& levels = getLevelInfos();
		char mapname[8];

		P_RemoveDefereds();

		arc >> mapname[0];
		while (mapname[0])
		{
			arc.Read(&mapname[1], 7);
			level_pwad_info_t& info = levels.findByName(mapname);
			if (!info.exists())
			{
				char name[9];
				strncpy(name, mapname, ARRAY_LENGTH(name) - 1);
				name[8] = 0;
				I_Error("Unknown map '%s' in savegame", name);
			}
			arc >> info.defered;
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

	R_ExitLevel();

	NormalLight.maps = shaderef_t(&realcolormaps, 0);
	//NormalLight.maps = shaderef_t(&DefaultPalette->maps, 0);

	level.gravity = sv_gravity;
	level.aircontrol = (fixed_t)(sv_aircontrol * 65536.f);
	G_AirControlChanged();

	// clear all ACS variables
	memset(level.vars, 0, sizeof(level.vars));

	// Get our canonical level data.
	level_pwad_info_t& info = getLevelInfos().findByName(::level.mapname);

	// [ML] 5/11/06 - Remove sky scrolling and sky2
	// [SL] 2012-03-19 - Add sky2 back
	::level.info = (level_info_t*)&info;
	strncpy(::level.skypic2, info.skypic2, 8);
	memcpy(::level.fadeto_color, info.fadeto_color, 4);
	
	if (::level.fadeto_color[0] || ::level.fadeto_color[1] || ::level.fadeto_color[2] || ::level.fadeto_color[3])
	{
		NormalLight.maps = shaderef_t(&V_GetDefaultPalette()->maps, 0);
	}
	else
	{
		R_ForceDefaultColormap(info.fadetable);
	}

	memcpy(::level.outsidefog_color, info.outsidefog_color, 4);

	::level.flags |= LEVEL_DEFINEDINMAPINFO;
	if (info.gravity != 0.f)
	{
		::level.gravity = info.gravity;
	}
	if (info.aircontrol != 0.f)
	{
		::level.aircontrol = (fixed_t)(info.aircontrol * 65536.f);
	}

	::level.partime = info.partime;
	::level.cluster = info.cluster;
	::level.flags = info.flags;
	::level.levelnum = info.levelnum;

	// Only copy the level name if there's a valid level name to be copied.
	if (info.level_name != NULL)
	{
		// Get rid of initial lump name or level number.
		char* begin = NULL;
		if (info.mapname[0] == 'E' && info.mapname[2] == 'M')
		{
			std::string search;
			StrFormat(search, "E%cM%c: ", info.mapname[1], info.mapname[3]);
			begin = strstr(info.level_name, search.c_str());
			if (begin != NULL)
				begin += search.length();
			else
				begin = info.level_name;
		}
		else if (strstr(info.mapname, "MAP") == &info.mapname[0])
		{
			std::string search;
			StrFormat(search, "%u: ", info.levelnum);
			begin = strstr(info.level_name, search.c_str());
			if (begin != NULL)
				begin += search.length();
			else
				begin = info.level_name;
		}
		strncpy(::level.level_name, begin, ARRAY_LENGTH(::level.level_name) - 1);
	}
	else
	{
		strncpy(
			::level.level_name, "Untitled Level",
			ARRAY_LENGTH(::level.level_name) - 1
		);
	}

	strncpy(::level.nextmap, info.nextmap, 8);
	strncpy(::level.secretmap, info.secretmap, 8);
	strncpy(::level.music, info.music, 8);
	strncpy(::level.skypic, info.skypic, 8);
	if (!::level.skypic2[0])
	{
			strncpy(::level.skypic2, ::level.skypic, 8);
	}

	if (::level.flags & LEVEL_JUMP_YES)
	{
		sv_allowjump = 1;
	}
	if (::level.flags & LEVEL_JUMP_NO)
	{
		sv_allowjump = 0.0;
	}
	if (::level.flags & LEVEL_FREELOOK_YES)
	{
		sv_freelook = 1;
	}
	if (::level.flags & LEVEL_FREELOOK_NO)
	{
		sv_freelook = 0.0;
	}

//	memset (level.vars, 0, sizeof(level.vars));

	if (memcmp(::level.fadeto_color, old_fadeto_color, 4) != 0)
	{
		V_RefreshColormaps();
	}

	movingsectors.clear();
}

static void MapinfoHelp()
{
	Printf(PRINT_HIGH,
		"mapinfo - Looks up internal information about levels\n\n"
		"Usage:\n"
		"  ] mapinfo mapname <LUMPNAME>\n"
		"  Looks up a map contained in the lump LUMPNAME.\n\n"
		"  ] mapinfo levelnum <LEVELNUM>\n"
		"  Looks up a map with a levelnum of LEVELNUM.\n\n"
		"  ] mapinfo at <LEVELINFO ID>\n"
		"  Looks up a map based on its placement in the internal level info array.\n\n"
		"  ] mapinfo size\n"
		"  Return the size of the internal level info array.\n");
}

// A debugging tool to examine the state of computed map data.
BEGIN_COMMAND(mapinfo)
{
	if (argc < 2)
	{
		MapinfoHelp();
		return;
	}

	LevelInfos& levels = getLevelInfos();
	if (stricmp(argv[1], "size") == 0)
	{
		Printf(PRINT_HIGH, "%u maps found\n", levels.size());
		return;
	}

	if (argc < 3)
	{
		MapinfoHelp();
		return;
	}

	level_pwad_info_t* infoptr = NULL;
	if (stricmp(argv[1], "mapname") == 0)
	{
		infoptr = &levels.findByName(argv[2]);
		if (!infoptr->exists())
		{
			Printf(PRINT_HIGH, "Map \"%s\" not found\n", argv[2]);
			return;
		}
	}
	else if (stricmp(argv[1], "levelnum") == 0)
	{
		int levelnum = atoi(argv[2]);
		infoptr = &levels.findByNum(levelnum);
		if (!infoptr->exists())
		{
			Printf(PRINT_HIGH, "Map number %d not found\n", levelnum);
			return;
		}
	}
	else if (stricmp(argv[1], "at") == 0)
	{
		// Check ahead of time, otherwise we might crash.
		int id = atoi(argv[2]);
		if (id < 0 || id >= levels.size())
		{
			Printf(PRINT_HIGH, "Map index %d does not exist\n", id);
			return;
		}
		infoptr = &levels.at(id);
	}
	else
	{
		MapinfoHelp();
		return;
	}

	level_pwad_info_t& info = *infoptr;

	Printf(PRINT_HIGH, "Map Name: %s\n", info.mapname);
	Printf(PRINT_HIGH, "Level Number: %d\n", info.levelnum);
	Printf(PRINT_HIGH, "Level Name: %s\n", info.level_name);
	Printf(PRINT_HIGH, "Intermission Graphic: %s\n", info.pname);
	Printf(PRINT_HIGH, "Next Map: %s\n", info.nextmap);
	Printf(PRINT_HIGH, "Secret Map: %s\n", info.secretmap);
	Printf(PRINT_HIGH, "Par Time: %d\n", info.partime);
	Printf(PRINT_HIGH, "Sky: %s\n", info.skypic);
	Printf(PRINT_HIGH, "Music: %s\n", info.music);

	// Stringify the set level flags.
	std::string flags;
	flags += (info.flags & LEVEL_NOINTERMISSION ? " NOINTERMISSION" : "");
	flags += (info.flags & LEVEL_DOUBLESKY ? " DOUBLESKY" : "");
	flags += (info.flags & LEVEL_NOSOUNDCLIPPING ? " NOSOUNDCLIPPING" : "");
	flags += (info.flags & LEVEL_MAP07SPECIAL ? " MAP07SPECIAL" : "");
	flags += (info.flags & LEVEL_BRUISERSPECIAL ? " BRUISERSPECIAL" : "");
	flags += (info.flags & LEVEL_CYBORGSPECIAL ? " CYBORGSPECIAL" : "");
	flags += (info.flags & LEVEL_SPIDERSPECIAL ? " SPIDERSPECIAL" : "");
	flags += (info.flags & LEVEL_SPECLOWERFLOOR ? " SPECLOWERFLOOR" : "");
	flags += (info.flags & LEVEL_SPECOPENDOOR ? " SPECOPENDOOR" : "");
	flags += (info.flags & LEVEL_SPECACTIONSMASK ? " SPECACTIONSMASK" : "");
	flags += (info.flags & LEVEL_MONSTERSTELEFRAG ? " MONSTERSTELEFRAG" : "");
	flags += (info.flags & LEVEL_EVENLIGHTING ? " EVENLIGHTING" : "");
	flags += (info.flags & LEVEL_SNDSEQTOTALCTRL ? " SNDSEQTOTALCTRL" : "");
	flags += (info.flags & LEVEL_FORCENOSKYSTRETCH ? " FORCENOSKYSTRETCH" : "");
	flags += (info.flags & LEVEL_JUMP_NO ? " JUMP_NO" : "");
	flags += (info.flags & LEVEL_JUMP_YES ? " JUMP_YES" : "");
	flags += (info.flags & LEVEL_FREELOOK_NO ? " FREELOOK_NO" : "");
	flags += (info.flags & LEVEL_FREELOOK_YES ? " FREELOOK_YES" : "");
	flags += (info.flags & LEVEL_STARTLIGHTNING ? " STARTLIGHTNING" : "");
	flags += (info.flags & LEVEL_FILTERSTARTS ? " FILTERSTARTS" : "");
	flags += (info.flags & LEVEL_LOBBYSPECIAL ? " LOBBYSPECIAL" : "");
	flags += (info.flags & LEVEL_DEFINEDINMAPINFO ? " DEFINEDINMAPINFO" : "");
	flags += (info.flags & LEVEL_CHANGEMAPCHEAT ? " CHANGEMAPCHEAT" : "");
	flags += (info.flags & LEVEL_VISITED ? " VISITED" : "");

	if (flags.length() > 0)
	{
		Printf(PRINT_HIGH, "Flags:%s\n", flags.c_str());
	}
	else
	{
		Printf(PRINT_HIGH, "Flags: None\n");
	}

	Printf(PRINT_HIGH, "Cluster: %d\n", info.cluster);
	Printf(PRINT_HIGH, "Snapshot? %s\n", info.snapshot ? "Yes" : "No");
	Printf(PRINT_HIGH, "ACS defereds? %s\n", info.defered ? "Yes" : "No");
}
END_COMMAND(mapinfo)

// A debugging tool to examine the state of computed cluster data.
BEGIN_COMMAND(clusterinfo)
{
	if (argc < 2)
	{
		Printf(PRINT_HIGH, "Usage: clusterinfo <cluster id>\n");
		return;
	}

	cluster_info_t& info = getClusterInfos().findByCluster(std::atoi(argv[1]));
	if (info.cluster == 0)
	{
		Printf(PRINT_HIGH, "Cluster %s not found\n", argv[1]);
		return;
	}

	Printf(PRINT_HIGH, "Cluster: %d\n", info.cluster);
	Printf(PRINT_HIGH, "Message Music: %s\n", info.messagemusic);
	Printf(PRINT_HIGH, "Message Flat: %s\n", info.finaleflat);
	if (info.exittext)
	{
		Printf(PRINT_HIGH, "- = Exit Text = -\n%s\n- = = = -\n", info.exittext);
	}
	else
	{
		Printf(PRINT_HIGH, "Exit Text: None\n");
	}
	if (info.entertext)
	{
		Printf(PRINT_HIGH, "- = Enter Text = -\n%s\n- = = = -\n", info.entertext);
	}
	else
	{
		Printf(PRINT_HIGH, "Enter Text: None\n");
	}

	// Stringify the set cluster flags.
	std::string flags;
	flags += (info.flags & CLUSTER_HUB ? " HUB" : "");
	flags += (info.flags & CLUSTER_EXITTEXTISLUMP ? " EXITTEXTISLUMP" : "");

	if (flags.length() > 0)
	{
		Printf(PRINT_HIGH, "Flags:%s\n", flags.c_str());
	}
	else
	{
		Printf(PRINT_HIGH, "Flags: None\n");
	}
}
END_COMMAND(clusterinfo)

// Static level info from original game
// The level names and cluster messages get filled in
// by G_SetLevelStrings().

const level_info_t IWADLevelInfos[] = {
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
const cluster_info_t IWADClusterInfos[] = {
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

// Get global canonical levelinfo
LevelInfos& getLevelInfos()
{
	static LevelInfos li(IWADLevelInfos);
	return li;
}

// Get global canonical clusterinfo
ClusterInfos& getClusterInfos()
{
	static ClusterInfos ci(IWADClusterInfos);
	return ci;
}

VERSION_CONTROL (g_level_cpp, "$Id$")
