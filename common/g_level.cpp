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
#include "g_episode.h"
#include "g_game.h"
#include "gstrings.h"
#include "gi.h"
#include "i_system.h"
#include "m_fileio.h"
#include "oscanner.h"
#include "p_acs.h"
#include "p_local.h"
#include "p_saveg.h"
#include "p_setup.h"
#include "p_unlag.h"
#include "r_data.h"
#include "r_sky.h"
#include "s_sound.h"
#include "stringenums.h"
#include "v_video.h"
#include "w_wad.h"
#include "w_ident.h"
#include "z_zone.h"

#define lioffset(x)		offsetof(level_pwad_info_t,x)
#define cioffset(x)		offsetof(cluster_info_t,x)

#define G_NOMATCH (-1)			// used for MatchString

level_locals_t level;			// info about current level

EXTERN_CVAR(co_allowdropoff)
EXTERN_CVAR(co_realactorheight)

//
// LevelInfos methods
//

// Construct from array of levelinfos, ending with an "empty" level
LevelInfos::LevelInfos(const level_info_t* levels) :
	_defaultInfos(levels)
{
	//addDefaults();
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
	clearSnapshots();
	zapDeferreds();
	_infos.clear();
}

// Clear all stored snapshots
void LevelInfos::clearSnapshots()
{
	for (_LevelInfoArray::iterator it = _infos.begin(); it != _infos.end(); ++it)
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
level_pwad_info_t& LevelInfos::findByName(const char* mapname)
{
	for (_LevelInfoArray::iterator it = _infos.begin(); it != _infos.end(); ++it)
	{
		if (it->mapname == mapname)
		{
			return *it;
		}
	}
	return _empty;
}

level_pwad_info_t& LevelInfos::findByName(const std::string &mapname)
{
	for (_LevelInfoArray::iterator it = _infos.begin(); it != _infos.end(); ++it)
	{
		if (it->mapname == mapname)
		{
			return *it;
		}
	}
	return _empty;
}

// Find a levelinfo by mapnum
level_pwad_info_t& LevelInfos::findByNum(int levelnum)
{
	for (_LevelInfoArray::iterator it = _infos.begin(); it != _infos.end(); ++it)
	{
		if (it->levelnum == levelnum && W_CheckNumForName(it->mapname.c_str()) != -1)
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
	for (_LevelInfoArray::iterator it = _infos.begin(); it != _infos.end(); ++it)
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
	"",   // level_name
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
    "",	  // exitpic
	"",	  // enterpic
	"",	  // endpic
    "",   // intertext
    "",   // intertextsecret
    "",   // interbackdrop
    "",   // intermusic
	};

//
// ClusterInfos methods
//

// Construct from array of clusterinfos, ending with an "empty" cluster.
ClusterInfos::ClusterInfos(const cluster_info_t* clusters) :
	_defaultInfos(clusters)
{
	//addDefaults();
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
	for (_ClusterInfoArray::iterator it = _infos.begin(); it != _infos.end(); ++it)
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
	for (_ClusterInfoArray::iterator it = _infos.begin();it != _infos.end();++it)
	{
		if (it->cluster == i)
		{
			return *it;
		}
	}
	return _empty;
}

// Number of info entries.
size_t ClusterInfos::size() const
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
	MITYPE_SETCOMPATFLAG,
};

struct MapInfoHandler
{
    EMIType type;
    DWORD data1, data2;
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
    "enterpic",
    "exitpic",
    "interpic"
	"translator",
	"compat_shorttex",
	"compat_limitpain",
    "compat_dropoff",
	"compat_trace",
	"compat_boomscroll",
	"compat_sectorsounds",
	"compat_nopassover",
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
	// enterpic <$pic>
    { MITYPE_EATNEXT, 0, 0 },
	// exitpic <$pic>
    { MITYPE_EATNEXT, 0, 0 },
	// interpic <$pic>
    { MITYPE_EATNEXT, 0, 0 },
	// translator <value>
	{ MITYPE_EATNEXT, 0, 0 },
	// compat_shorttex <value>
    {MITYPE_EATNEXT, 0, 0},
    // compat_limitpain <value>
    {MITYPE_EATNEXT, 0, 0},
    // compat_dropoff <value>
    {MITYPE_SETCOMPATFLAG, LEVEL_COMPAT_DROPOFF, 0},
    // compat_trace <value>
    {MITYPE_EATNEXT, 0, 0},
    // compat_boomscroll <value>
    {MITYPE_EATNEXT, 0, 0},
    // compat_sectorsounds <value>
    {MITYPE_EATNEXT, 0, 0},
    // compat_nopassover <value>
    {MITYPE_SETFLAG, LEVEL_COMPAT_NOPASSOVER, 0},
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

// [DE] used for UMAPINFO's boss actions
static const char* const ActorNames[] =
{
	"DoomPlayer",
	"ZombieMan",
	"ShotgunGuy",
	"Archvile",
	"ArchvileFire",
	"Revenant",
	"RevenantTracer",
	"RevenantTracerSmoke",
	"Fatso",
	"FatShot",
	"ChaingunGuy",
	"DoomImp",
	"Demon",
	"Spectre",
	"Cacodemon",
	"BaronOfHell",
	"BaronBall",
	"HellKnight",
	"LostSoul",
	"SpiderMastermind",
	"Arachnotron",
	"Cyberdemon",
	"PainElemental",
	"WolfensteinSS",
	"CommanderKeen",
	"BossBrain",
	"BossEye",
	"BossTarget",
	"SpawnShot",
	"SpawnFire",
	"ExplosiveBarrel",
	"DoomImpBall",
	"CacodemonBall",
	"Rocket",
	"PlasmaBall",
	"BFGBall",
	"ArachnotronPlasma",
	"BulletPuff",
	"Blood",
	"TeleportFog",
	"ItemFog",
	"TeleportDest",
	"BFGExtra",
	"GreenArmor",
	"BlueArmor",
	"HealthBonus",
	"ArmorBonus",
	"BlueCard",
	"RedCard",
	"YellowCard",
	"YellowSkull",
	"RedSkull",
	"BlueSkull",
	"Stimpack",
	"Medikit",
	"Soulsphere",
	"InvulnerabilitySphere",
	"Berserk",
	"BlurSphere",
	"RadSuit",
	"Allmap",
	"Infrared",
	"Megasphere",
	"Clip",
	"ClipBox",
	"RocketAmmo",
	"RocketBox",
	"Cell",
	"CellPack",
	"Shell",
	"ShellBox",
	"Backpack",
	"BFG9000",
	"Chaingun",
	"Chainsaw",
	"RocketLauncher",
	"PlasmaRifle",
	"Shotgun",
	"SuperShotgun",
	"TechLamp",
	"TechLamp2",
	"Column",
	"TallGreenColumn",
	"ShortGreenColumn",
	"TallRedColumn",
	"ShortRedColumn",
	"SkullColumn",
	"HeartColumn",
	"EvilEye",
	"FloatingSkull",
	"TorchTree",
	"BlueTorch",
	"GreenTorch",
	"RedTorch",
	"ShortBlueTorch",
	"ShortGreenTorch",
	"ShortRedTorch",
	"Stalagtite",
	"TechPillar",
	"CandleStick",
	"Candelabra",
	"BloodyTwitch",
	"Meat2",
	"Meat3",
	"Meat4",
	"Meat5",
	"NonsolidMeat2",
	"NonsolidMeat4",
	"NonsolidMeat3",
	"NonsolidMeat5",
	"NonsolidTwitch",
	"DeadCacodemon",
	"DeadMarine",
	"DeadZombieMan",
	"DeadDemon",
	"DeadLostSoul",
	"DeadDoomImp",
	"DeadShotgunGuy",
	"GibbedMarine",
	"GibbedMarineExtra",
	"HeadsOnAStick",
	"Gibs",
	"HeadOnAStick",
	"HeadCandles",
	"DeadStick",
	"LiveStick",
	"BigTree",
	"BurningBarrel",
	"HangNoGuts",
	"HangBNoBrain",
	"HangTLookingDown",
	"HangTSkull",
	"HangTLookingUp",
	"HangTNoBrain",
	"ColonGibs",
	"SmallBloodPool",
	"BrainStem",
	//Boom/MBF additions
	"PointPusher",
	"PointPuller",
	"MBFHelperDog",
	"PlasmaBall1",
	"PlasmaBall2",
	"EvilSceptre",
	"UnholyBible",
	NULL
};

static void SetLevelDefaults(level_pwad_info_t *levelinfo)
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
static void SkipUnknownParams(OScanner &os)
{
	// Every loop, try to burn a comma.
	while (os.scan())
	{
		if (!os.compareToken(","))
		{
			os.unScan();
			return;
		}

		// Burn the parameter.
		os.scan();
	}
}

//
// Assumes that you have already munched the unknown type name, and just need
// to much parameters, if any.
//
static void SkipUnknownType(OScanner &os)
{
	os.scan();
	if (!os.compareToken("="))
	{
		os.unScan();
		return;
	}

	os.scan(); // Get the first parameter
	SkipUnknownParams(os);
}

//
// Assumes you have already munched the first opening brace.
//
// This function does not work with old-school ZDoom MAPINFO.
//
static void SkipUnknownBlock(OScanner &os)
{
	int stack = 0;

	while (os.scan())
	{
		if (os.compareToken("{"))
		{
			// Found another block
			stack++;
		}
		else if (os.compareToken("}"))
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

// [DE] Below are helper functions for UMAPINFO and Z/MAPINFO parsing, partially to stand in for
// the way sc_man did things before
namespace
{
	// return token as int
	int GetTokenAsInt(OScanner& os)
	{
		// fix for parser reading in commas
		std::string str = os.getToken();

		if (str[str.length() - 1] == ',')
		{
			str[str.length() - 1] = '\0';
		}

		char* stopper;

		//if (os.compareToken("MAXINT"))
		if (str == "MAXINT")
		{
			return MAXINT;
		}

		const int num = strtol(str.c_str(), &stopper, 0);

		if (*stopper != 0)
		{
			I_Error("Bad numeric constant \"%s\".", str.c_str());
		}

		return num;
	}

	// return token as float
	float GetTokenAsFloat(OScanner& os)
	{
		// fix for parser reading in commas
		std::string str = os.getToken();

		if (str[str.length() - 1] == ',')
		{
			str[str.length() - 1] = '\0';
		}

		char* stopper;

		const double num = strtod(str.c_str(), &stopper);

		if (*stopper != 0)
		{
			I_Error("Bad numeric constant \"%s\".", str.c_str());
		}

		return static_cast<float>(num);
	}

	void MustGetString(OScanner& os)
	{
		if (!os.scan())
		{
			I_Error("Missing string (unexpected end of file).");
		}
    }

	void MustGetInt(OScanner& os)
	{
		MustGetString(os);

		// fix for parser reading in commas
		std::string str = os.getToken();

		if (str[str.length() - 1] == ',')
		{
			str[str.length() - 1] = '\0';
		}

		if (IsNum(str.c_str()) == false)
		{
			I_Error("Missing integer (unexpected end of file).");
		}
	}

	void MustGetFloat(OScanner& os)
	{
		MustGetString(os);

		// fix for parser reading in commas
		std::string str = os.getToken();

		if (str[str.length() - 1] == ',')
		{
			str[str.length() - 1] = '\0';
		}

		if (IsRealNum(str.c_str()) == false)
		{
			I_Error("Missing floating-point number (unexpected end of file).");
		}
	}

	bool IsIdentifier(OScanner& os)
	{
		char ch = os.getToken()[0];

		return (ch == '_' || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'));
	}

	void MustGetIdentifier(OScanner& os)
	{
		MustGetString(os);
		if (!IsIdentifier(os))
		{
			I_Error("Expected identifier (unexpected end of file).");
		}
	}

	char* M_Strupr(char* str)
	{
		for (char* p = str; *p; p++)
			*p = toupper(*p);
		return str;
	}

	bool UpperCompareToken(OScanner& os, const char* str)
	{
		return stricmp(os.getToken().c_str(), str) == 0;
	}

	// [DE] lazy copy from sc_man
    int MatchString(OScanner& os, const char** strings)
    {
	    if (strings == NULL)
	    {
		    return G_NOMATCH;
	    }

	    for (int i = 0; *strings != NULL; i++)
	    {
		    if (UpperCompareToken(os, *strings++))
		    {
			    return i;
		    }
	    }

	    return G_NOMATCH;
    }

	bool ContainsMapInfoTopLevel(OScanner& os)
    {
	    return UpperCompareToken(os, "map") ||
			   UpperCompareToken(os, "defaultmap") ||
	           UpperCompareToken(os, "cluster") ||
			   UpperCompareToken(os, "clusterdef") ||
	           UpperCompareToken(os, "episode") ||
	           UpperCompareToken(os, "clearepisodes") ||
			   UpperCompareToken(os, "skill") ||
	           UpperCompareToken(os, "clearskills") ||
	           UpperCompareToken(os, "gameinfo") ||
	           UpperCompareToken(os, "intermission") ||
			   UpperCompareToken(os, "automap");
    }

	// return token as bool
    bool GetTokenAsBool(OScanner& os)
    {
	    return UpperCompareToken(os, "true");
    }

    void MustGetBool(OScanner& os)
    {
	    MustGetString(os);
	    if (!UpperCompareToken(os, "true") && !UpperCompareToken(os, "false"))
	    {
		    I_Error("Missing boolean (unexpected end of file).");
	    }
    }

    void MustGetStringName(OScanner& os, const char* name)
    {
	    MustGetString(os);
	    if (UpperCompareToken(os, name) == false)
	    {
		    // TODO: was previously SC_ScriptError, less information is printed now
		    I_Error("Expected '%s', got '%s'.", name, os.getToken().c_str());
	    }
    }

	// used for munching the strings in UMAPINFO
	char* ParseMultiString(OScanner& os, int error)
	{
		char* build = NULL;

		os.scan();
		// TODO: properly identify identifiers so clear can be separated from regular strings
		//if (IsIdentifier(os))
		{
		    if (UpperCompareToken(os, "clear"))
			{
				return strdup("-");	// this was explicitly deleted to override the default.
			}

			//I_Error("Either 'clear' or string constant expected");
		}
		os.unScan();

		do
		{
			MustGetString(os);

			if (build == NULL)
				build = strdup(os.getToken().c_str());
			else
			{
				size_t newlen = strlen(build) + os.getToken().length() + 2; // strlen for both the existing text and the new line, plus room for one \n and one \0
				build = (char*)realloc(build, newlen); // Prepare the destination memory for the below strcats
				strcat(build, "\n"); // Replace the existing text's \0 terminator with a \n
				strcat(build, os.getToken().c_str()); // Concatenate the new line onto the existing text
			}
			os.scan();
		} while (os.compareToken(","));
		os.unScan();

		return build;
	}

	int ParseLumpName(OScanner& os, char* buffer)
	{
		MustGetString(os);
		if (strlen(os.getToken().c_str()) > 8)
		{
			I_Error("String too long. Maximum size is 8 characters.");
			return 0;
		}
		strncpy(buffer, os.getToken().c_str(), 8);
		buffer[8] = 0;
		M_Strupr(buffer);
		return 1;
	}

	int ParseOLumpName(OScanner& os, OLumpName& buffer)
    {
	    MustGetString(os);
	    if (strlen(os.getToken().c_str()) > 8)
	    {
		    I_Error("String too long. Maximum size is 8 characters.");
		    return 0;
	    }
	    buffer = os.getToken();
	    return 1;
    }

	int ValidateMapName(const char* mapname, int* pEpi = NULL, int* pMap = NULL)
	{
		// Check if the given map name can be expressed as a gameepisode/gamemap pair and be reconstructed from it.
	    char lumpname[9];
		int epi = -1, map = -1;
		
		const OLumpName mapuname = mapname;

		if (gamemode != commercial)
		{
			if (sscanf(mapuname.c_str(), "E%dM%d", &epi, &map) != 2)
				return 0;
			snprintf(lumpname, 9, "E%dM%d", epi, map);
		}
		else
		{
			if (sscanf(mapuname.c_str(), "MAP%d", &map) != 1)
				return 0;
			snprintf(lumpname, 9, "MAP%02d", map);
			epi = 1;
		}
		if (pEpi)
			*pEpi = epi;
		if (pMap)
			*pMap = map;
		return !strcmp(mapuname.c_str(), lumpname);
	}

	int ParseStandardUmapInfoProperty(OScanner& os, level_pwad_info_t* mape)
	{
		// find the next line with content.
		// this line is no property.

		if (!IsIdentifier(os))
		{
			I_Error("Expected identifier");
		}
		char* pname = strdup(os.getToken().c_str());
		MustGetStringName(os, "=");

		if (!stricmp(pname, "levelname"))
		{
			MustGetString(os);
			mape->level_name = strdup(os.getToken().c_str());
		}
		else if (!stricmp(pname, "next"))
		{
			ParseLumpName(os, mape->nextmap);
			if (!ValidateMapName(mape->nextmap))
			{
				I_Error("Invalid map name %s.", mape->nextmap);
				return 0;
			}
		}
		else if (!stricmp(pname, "nextsecret"))
		{
			ParseLumpName(os, mape->secretmap);
			if (!ValidateMapName(mape->secretmap))
			{
				I_Error("Invalid map name %s", mape->nextmap);
				return 0;
			}
		}
		else if (!stricmp(pname, "levelpic"))
		{
			ParseLumpName(os, mape->pname);
		}
		else if (!stricmp(pname, "skytexture"))
		{
			ParseLumpName(os, mape->skypic);
		}
		else if (!stricmp(pname, "music"))
		{
			ParseLumpName(os, mape->music);
		}
		else if (!stricmp(pname, "endpic"))
		{
			ParseOLumpName(os, mape->endpic);
			strncpy(mape->nextmap, "EndGame1", 8);
			mape->nextmap[8] = '\0';
		}
		else if (!stricmp(pname, "endcast"))
		{
			MustGetBool(os);
			if (GetTokenAsBool(os))
				strncpy(mape->nextmap, "EndGameC", 8);
			else
				mape->endpic.clear();
		}
		else if (!stricmp(pname, "endbunny"))
		{
			MustGetBool(os);
			if (GetTokenAsBool(os))
				strncpy(mape->nextmap, "EndGame3", 8);
			else
			    mape->endpic.clear();
		}
		else if (!stricmp(pname, "endgame"))
		{
			MustGetBool(os);
			if (GetTokenAsBool(os))
			{
			    mape->endpic = "!";
			}
			else
			{
			    mape->endpic.clear();
			}
		}
		else if (!stricmp(pname, "exitpic"))
		{
			ParseOLumpName(os, mape->exitpic);
		}
		else if (!stricmp(pname, "enterpic"))
		{
			ParseOLumpName(os, mape->enterpic);
		}
		else if (!stricmp(pname, "nointermission"))
		{
			MustGetBool(os);
			if (GetTokenAsBool(os))
			{
				mape->flags |= LEVEL_NOINTERMISSION;
			}
		}
		else if (!stricmp(pname, "partime"))
		{
			MustGetInt(os);
			mape->partime = TICRATE * GetTokenAsInt(os);
		}
		else if (!stricmp(pname, "intertext"))
		{
			char* lname = ParseMultiString(os, 1);
			if (!lname)
				return 0;
			mape->intertext = lname;
		}
		else if (!stricmp(pname, "intertextsecret"))
		{
			char* lname = ParseMultiString(os, 1);
			if (!lname)
				return 0;
			mape->intertextsecret = lname;
		}
		else if (!stricmp(pname, "interbackdrop"))
		{
			ParseOLumpName(os, mape->interbackdrop);
		}
		else if (!stricmp(pname, "intermusic"))
		{
			ParseOLumpName(os, mape->intermusic);
		}
		else if (!stricmp(pname, "episode"))
		{
			if (!episodes_modified && gamemode == commercial)
			{
				episodenum = 0;
				episodes_modified = true;
			}

			char* lname = ParseMultiString(os, 1);
			if (!lname)
				return 0;

			if (*lname == '-') // means "clear"
			{
				episodenum = 0;
			}
			else
			{
				const char* gfx = std::strtok(lname, "\n");
				const char* txt = std::strtok(NULL, "\n");
				const char* alpha = std::strtok(NULL, "\n");

				if (episodenum >= 8)
				{
					return 0;
				}

				strncpy(EpisodeMaps[episodenum], mape->mapname.c_str(), 8);
				EpisodeInfos[episodenum].name = gfx;
				EpisodeInfos[episodenum].fulltext = false;
				EpisodeInfos[episodenum].noskillmenu = false;
				EpisodeInfos[episodenum].key = alpha ? *alpha : 0;
				++episodenum;
			}
		}
		else if (!stricmp(pname, "bossaction"))
		{
			MustGetIdentifier(os);

			if (!stricmp(os.getToken().c_str(), "clear"))
			{
				// mark level free of boss actions
				mape->bossactions.clear();
				mape->bossactions_donothing = true;
			}
			else
			{
				int i;

				// remove comma from token
				std::string actor_name = os.getToken();
				actor_name[actor_name.length() - 1] = '\0';

				for (i = 0; ActorNames[i]; i++)
				{
					if (!stricmp(actor_name.c_str(), ActorNames[i]))
						break;
				}
				if (ActorNames[i] == NULL)
				{
					I_Error("Unknown thing type %s", os.getToken().c_str());
					return 0;
				}

				// skip comma token
				//MustGetStringName(os, ",");
				MustGetInt(os);
				const int special = GetTokenAsInt(os);
				//MustGetStringName(os, ",");
				MustGetInt(os);
				const int tag = GetTokenAsInt(os);
				// allow no 0-tag specials here, unless a level exit.
				if (tag != 0 || special == 11 || special == 51 || special == 52 || special == 124)
				{
					if (mape->bossactions_donothing == true)
						mape->bossactions_donothing = false;

					OBossAction new_bossaction;

					maplinedef_t mld;
				    mld.special = static_cast<short>(special);
				    mld.tag = static_cast<short>(tag);
				    P_TranslateLineDef(&new_bossaction.ld, &mld);
					
					new_bossaction.type = i;

					mape->bossactions.push_back(new_bossaction);
				}

			}
		}
		else
		{
			do
			{
				if (!IsRealNum(os.getToken().c_str()))
					os.scan();

			} while (os.compareToken(","));
		}
		free(pname);
		os.scan();

		return 1;
	}

	void MapNameToLevelNum(level_pwad_info_t& info)
	{
		if (info.mapname[0] == 'E' && info.mapname[2] == 'M')
		{
			// Convert a char into its equivalent integer.
			const int e = info.mapname[1] - '0';
			const int m = info.mapname[3] - '0';
			if (e >= 0 && e <= 9 && m >= 0 && m <= 9)
			{
				// Copypasted from the ZDoom wiki.
				info.levelnum = (e - 1) * 10 + m;
			}
		}
		else if (strnicmp(info.mapname.c_str(), "MAP", 3) == 0)
		{
			// Try and turn the trailing digits after the "MAP" into a
			// level number.
			int mapnum = std::atoi(info.mapname.c_str() + 3);
			if (mapnum >= 0 && mapnum <= 99)
			{
				info.levelnum = mapnum;
			}
		}
	}

	void ParseUMapInfoLump(int lump, const char* lumpname)
	{
		LevelInfos& levels = getLevelInfos();

		level_pwad_info_t defaultinfo;
		SetLevelDefaults(&defaultinfo);

		const char* buffer = (char*)W_CacheLumpNum(lump, PU_STATIC);

		OScannerConfig config = {
			lumpname, // lumpName
			false,      // semiComments
			true,       // cComments
		};
		OScanner os = OScanner::openBuffer(config, buffer, buffer + W_LumpLength(lump));

		while (os.scan())
		{
			if (!UpperCompareToken(os, "map"))
			{
				I_Error("Expected map definition, got %s", os.getToken().c_str());
			}

			MustGetIdentifier(os);
		    if (os.getToken().length() > 8 || !ValidateMapName(os.getToken().c_str()))
		    {
			    I_Error("Invalid map name %s", os.getToken().c_str());
		    }

			// Find the level.
			level_pwad_info_t& info = (levels.findByName(os.getToken()).exists()) ?
				levels.findByName(os.getToken()) :
				levels.create();

			// Free the level name string before we pave over it.
			info.level_name.clear();

			info = defaultinfo;
		    if (os.getToken().length() > 8)
		    {
			    I_Error("Invalid map name %s", os.getToken().c_str());
		    }
			info.mapname = os.getToken();

			MapNameToLevelNum(info);

			MustGetStringName(os, "{");
			os.scan();
			while (!os.compareToken("}"))
			{
				ParseStandardUmapInfoProperty(os, &info);
			}

			// Set default level progression here to simplify the checks elsewhere.
			// Doing this lets us skip all normal code for this if nothing has been defined.
			if (!info.nextmap[0] && !info.endpic[0])
			{
				if (info.mapname == "MAP30")
				{
					info.endpic = "$CAST";
					strncpy(info.nextmap, "EndGameC", 8);
				}
				else if (info.mapname == "E1M8")
				{
					info.endpic = gamemode == retail ? "CREDIT" : "HELP2";
					strncpy(info.nextmap, "EndGameC", 8);
				}
				else if (info.mapname == "E2M8")
				{
					info.endpic = "VICTORY";
					strncpy(info.nextmap, "EndGame2", 8);
				}
				else if (info.mapname == "E3M8")
				{
					info.endpic = "$BUNNY";
					strncpy(info.nextmap, "EndGame3", 8);
				}
				else if (info.mapname == "E4M8")
				{
					info.endpic = "ENDPIC";
					strncpy(info.nextmap, "EndGame4", 8);
				}
				else if (gamemission == chex && info.mapname == "E1M5")
				{
					info.endpic = "CREDIT";
					strncpy(info.nextmap, "EndGame1", 8);
				}
				else
				{
					int ep, map;
					ValidateMapName(info.mapname.c_str(), &ep, &map);
					map++;
					if (gamemode == commercial)
						sprintf(info.nextmap, "MAP%02d", map);
					else
						sprintf(info.nextmap, "E%dM%d", ep, map);
				}
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
static void ParseMapInfoLower
(
	MapInfoHandler* handlers, const char** strings, tagged_info_t* tinfo, DWORD flags, OScanner &os
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

	while (os.scan())
	{
		if (os.compareToken("{"))
		{
			// Detected new-style MAPINFO
			newMapinfoStack++;
			continue;
		}
		if (os.compareToken("}"))
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
			ContainsMapInfoTopLevel(os) &&
			// "cluster" is a valid map block type and is also
			// a valid top-level type.
		    !UpperCompareToken(os, "cluster")
		)
		{
			// Old-style MAPINFO is done
			os.unScan();
			break;
		}

		const int entry = MatchString(os, strings);
		if (entry == G_NOMATCH)
		{
			if (newMapinfoStack <= 0)
			{
				// Old MAPINFO is up a creek, we need to be
				// able to parse all types even if we can't
				// do anything with them.
				//
				// TODO: was previously SC_ScriptError, less information is printed now
				I_Error("Unknown MAPINFO token \"%s\"", os.getToken().c_str());
			}

			// New MAPINFO is capable of skipping past unknown
			// types.
			SkipUnknownType(os);
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
				MustGetStringName(os, "=");
			}

			MustGetString(os);
			break;

		case MITYPE_INT:
			if (newMapinfoStack > 0)
			{
				MustGetStringName(os, "=");
			}

			MustGetInt(os);
			*((int*)(info + handler->data1)) = GetTokenAsInt(os);
			break;

		case MITYPE_FLOAT:
			if (newMapinfoStack > 0)
			{
				MustGetStringName(os, "=");
			}

			MustGetFloat(os);
			*((float*)(info + handler->data1)) = GetTokenAsFloat(os);
			break;

		case MITYPE_COLOR:
		{
			if (newMapinfoStack > 0)
			{
				MustGetStringName(os, "=");
			}

			MustGetString(os);
			argb_t color(V_GetColorFromString(os.getToken()));
			uint8_t* ptr = (uint8_t*)(info + handler->data1);
			ptr[0] = color.geta(); ptr[1] = color.getr(); ptr[2] = color.getg(); ptr[3] = color.getb();
			break;
		}
		case MITYPE_MAPNAME:
			if (newMapinfoStack > 0)
			{
				MustGetStringName(os, "=");
			}

			MustGetString(os);
			
			char map_name[9];
			strncpy(map_name, os.getToken().c_str(), 8);
			
			if (IsNum(map_name))
			{
				int map = std::atoi(map_name);
				sprintf(map_name, "MAP%02d", map);
			}
			strncpy((char*)(info + handler->data1), map_name, 8);
			break;

		case MITYPE_LUMPNAME:
			if (newMapinfoStack > 0)
			{
				MustGetStringName(os, "=");
			}

			MustGetString(os);
			uppercopy((char*)(info + handler->data1), os.getToken().c_str());
			break;


		case MITYPE_$LUMPNAME:
			if (newMapinfoStack > 0)
			{
				MustGetStringName(os, "=");
			}

			MustGetString(os);
			if (os.getToken()[0] == '$')
			{
				// It is possible to pass a DeHackEd string
				// prefixed by a $.
				const OString& s = GStrings(os.getToken().c_str() + 1);
				if (s.empty())
				{
					I_Error("Unknown lookup string \"%s\"", os.getToken().c_str());
				}
				uppercopy((char*)(info + handler->data1), s.c_str());
			}
			else
			{
				uppercopy((char*)(info + handler->data1), os.getToken().c_str());
			}
			break;

		case MITYPE_MUSICLUMPNAME:
		{
			if (newMapinfoStack > 0)
			{
				MustGetStringName(os, "=");
			}

			MustGetString(os);
			if (os.getToken()[0] == '$')
			{
				// It is possible to pass a DeHackEd string
				// prefixed by a $.
				const OString& s = GStrings(os.getToken().c_str() + 1);
				if (s.empty())
				{
					I_Error("Unknown lookup string \"%s\"", s.c_str());
				}

				// Music lumps in the stringtable do not begin
				// with a D_, so we must add it.
				char lumpname[9];
				snprintf(lumpname, ARRAY_LENGTH(lumpname), "D_%s", s.c_str());
				uppercopy((char*)(info + handler->data1), lumpname);
			}
			else
			{
				uppercopy((char*)(info + handler->data1), os.getToken().c_str());
			}
			break;
		}
		case MITYPE_SKY:
			if (newMapinfoStack > 0)
			{
				MustGetStringName(os, "=");
				MustGetString(os); // Texture name
				uppercopy((char*)(info + handler->data1), os.getToken().c_str());
				SkipUnknownParams(os);
			}
			else
			{
				MustGetString(os);	// get texture name;
				uppercopy((char*)(info + handler->data1), os.getToken().c_str());
				MustGetFloat(os);		// get scroll speed
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
				MustGetStringName(os, "=");
			}

			MustGetInt(os);
			*((int*)(info + handler->data1)) = GetTokenAsInt(os);
			if (HexenHack)
			{
				ClusterInfos& clusters = getClusterInfos();
				cluster_info_t& clusterH = clusters.findByCluster(GetTokenAsInt(os));
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
				MustGetStringName(os, "=");
			}

			MustGetString(os);
			free(*text);
			*text = strdup(os.getToken().c_str());
			break;
		}

		case MITYPE_CSTRING:
			if (newMapinfoStack > 0)
			{
				MustGetStringName(os, "=");
			}

			MustGetString(os);
			strncpy((char*)(info + handler->data1), os.getToken().c_str(), handler->data2);
			*((char*)(info + handler->data1 + handler->data2)) = '\0';
			break;
		case MITYPE_CLUSTERSTRING:
		{
			char** text = (char**)(info + handler->data1);

			if (newMapinfoStack > 0)
			{
				MustGetStringName(os, "=");
				MustGetString(os);
				if (UpperCompareToken(os, "lookup,"))
				{
					MustGetString(os);
					const OString& s = GStrings(os.getToken());
					if (s.empty())
					{
						I_Error("Unknown lookup string \"%s\"", os.getToken().c_str());
					}
					free(*text);
					*text = strdup(s.c_str());
				}
				else
				{
					// One line per string.
					std::string ctext;
					os.unScan();
					do
					{
						MustGetString(os);
						ctext += os.getToken();
						ctext += "\n";
						os.scan();
					} while (os.compareToken(","));
					os.unScan();

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
				MustGetString(os);
				if (UpperCompareToken(os, "lookup"))
				{
					MustGetString(os);
					const OString& s = GStrings(os.getToken());
					if (s.empty())
					{
						I_Error("Unknown lookup string \"%s\"", os.getToken().c_str());
					}

					free(*text);
					*text = strdup(s.c_str());
				}
				else
				{
					free(*text);
					*text = strdup(os.getToken().c_str());
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

static void ParseEpisodeInfo(OScanner &os)
{
	bool new_mapinfo = 0;
	OLumpName map;
	std::string pic;
	bool picisgfx = false;
	bool remove = false;
	char key = 0;
	bool noskillmenu = false;
	bool optional = false;
	bool extended = false;
	
	MustGetString(os); // Map lump
	map = os.getToken();
	
	MustGetString(os);
	if (UpperCompareToken(os, "teaser"))
	{
		// Teaser lump
		MustGetString(os);
		if (gameinfo.flags & GI_SHAREWARE)
		{
			map = os.getToken();
		}
		MustGetString(os);
	}
	else
	{
		os.unScan();
	}

	if (os.compareToken("{"))
	{
		// Detected new-style MAPINFO
		new_mapinfo = true;
	}
	
	MustGetString(os);
	while (os.scan())
	{
		if (os.compareToken("{"))
		{
			// Detected new-style MAPINFO
			I_Error("Detected incorrectly placed curly brace in MAPINFO episode definiton");
		}
		else if (os.compareToken("}"))
		{
			if (new_mapinfo == false)
			{
				I_Error("Detected incorrectly placed curly brace in MAPINFO episode definiton");
			}
			else
			{
				break;
			}
		}
		else if (UpperCompareToken(os, "name"))
		{
			if (new_mapinfo == true)
			{
				MustGetStringName(os, "=");
			}
			MustGetString(os);
			if (picisgfx == false)
			{
				pic = os.getToken();
			}
		}
		else if (UpperCompareToken(os, "lookup"))
		{
			if (new_mapinfo == true)
			{
				MustGetStringName(os, "=");
			}
			MustGetString(os);
			
			// Not implemented
		}
		else if (UpperCompareToken(os, "picname"))
		{
			if (new_mapinfo == true)
			{
				MustGetStringName(os, "=");
			}
			MustGetString(os);
			pic = os.getToken();
			picisgfx = true;
		}
		else if (UpperCompareToken(os, "key"))
		{
			if (new_mapinfo == true)
			{
				MustGetStringName(os, "=");
			}
			MustGetString(os);
			key = os.getToken()[0];
		}
		else if (UpperCompareToken(os, "remove"))
		{
			remove = true;
		}
		else if (UpperCompareToken(os, "noskillmenu"))
		{
			noskillmenu = true;
		}
		else if (UpperCompareToken(os, "optional"))
		{
			optional = true;
		}
		else if (UpperCompareToken(os, "extended"))
		{
			extended = true;
		}
		else
		{
			os.unScan();
			break;
		}
	}

	int i;
	for (i = 0; i < episodenum; ++i)
	{
		if (map == EpisodeMaps[i])
			break;
	}

	if (remove || (optional && W_CheckNumForName(map.c_str()) == -1) || (extended && W_CheckNumForName("EXTENDED") == -1))
	{
		// If the remove property is given for an episode, remove it.
		if (i < episodenum)
		{
			if (i + 1 < episodenum)
			{
				memmove(&EpisodeMaps[i], &EpisodeMaps[i + 1],
					sizeof(EpisodeMaps[0]) * (episodenum - i - 1));
				memmove(&EpisodeInfos[i], &EpisodeInfos[i + 1],
					sizeof(EpisodeInfos[0]) * (episodenum - i - 1));
			}
			episodenum--;
		}
	}
	else
	{
		if (pic.empty())
		{
			pic = map.c_str();
			picisgfx = false;
		}

		if (i == episodenum)
		{
			if (episodenum == MAX_EPISODES)
			{
				i = episodenum - 1;
			}
			else
			{
				i = episodenum++;
			}
		}

		EpisodeInfos[i].name = pic;
		EpisodeInfos[i].key = tolower(key);
		EpisodeInfos[i].fulltext = !picisgfx;
		EpisodeInfos[i].noskillmenu = noskillmenu;
		strncpy(EpisodeMaps[i], map.c_str(), 8);
	}
}

static void ParseGameInfo(OScanner &os)
{
	MustGetStringName(os, "{");

	while (os.scan())
	{
		if (UpperCompareToken(os, "{"))
		{
			// Detected new-style MAPINFO
			I_Error("Detected incorrectly placed curly brace in MAPINFO episode definiton");
		}
		else if (UpperCompareToken(os, "}"))
		{
			break;
		}
		else if (UpperCompareToken(os, "advisorytime"))
		{
			MustGetStringName(os, "=");
			os.scan();

			gameinfo.advisoryTime = GetTokenAsFloat(os);
		}
		else if (UpperCompareToken(os, "chatsound"))
		{
			MustGetStringName(os, "=");
			os.scan();

			strncpy(gameinfo.chatSound, os.getToken().c_str(), 16);
		}
		else if (UpperCompareToken(os, "pagetime"))
		{
			MustGetStringName(os, "=");
			os.scan();

			gameinfo.pageTime = GetTokenAsFloat(os);
		}
		else if (UpperCompareToken(os, "finaleflat"))
		{
			MustGetStringName(os, "=");
			os.scan();

			strncpy(gameinfo.finaleFlat, os.getToken().c_str(), 8);
		}
		else if (UpperCompareToken(os, "finalemusic"))
		{
			MustGetStringName(os, "=");
			os.scan();

			strncpy(gameinfo.finaleMusic, os.getToken().c_str(), 8);
		}
		else if (UpperCompareToken(os, "titlemusic"))
		{
			MustGetStringName(os, "=");
			os.scan();

			strncpy(gameinfo.titleMusic, os.getToken().c_str(), 8);
		}
		else if (UpperCompareToken(os, "titlepage"))
		{
			MustGetStringName(os, "=");
			os.scan();

			strncpy(gameinfo.titlePage, os.getToken().c_str(), 8);
		}
		else if (UpperCompareToken(os, "titletime"))
		{
			MustGetStringName(os, "=");
			os.scan();

			gameinfo.titleTime = GetTokenAsFloat(os);
		}
		else
		{
			// Game info property is not implemented
			MustGetStringName(os, "=");
			os.scan();
		}
	}
}

static void ParseMapInfoLump(int lump, const char* lumpname)
{
	LevelInfos& levels = getLevelInfos();
	ClusterInfos& clusters = getClusterInfos();

	level_pwad_info_t defaultinfo;
	SetLevelDefaults (&defaultinfo);

	const char *buffer = (char *)W_CacheLumpNum(lump, PU_STATIC);
	
	OScannerConfig config = {
		lumpname, // lumpName
		false,      // semiComments
		true,       // cComments
	};
	OScanner os = OScanner::openBuffer(config, buffer, buffer + W_LumpLength(lump));

	while (os.scan())
	{
		if (UpperCompareToken(os,"defaultmap"))
		{
			SetLevelDefaults(&defaultinfo);
			tagged_info_t tinfo;
			tinfo.tag = tagged_info_t::LEVEL;
			tinfo.level = &defaultinfo;
			ParseMapInfoLower(MapHandlers, MapInfoMapLevel, &tinfo, 0, os);
		}
		else if (UpperCompareToken(os, "map"))
		{
			DWORD levelflags = defaultinfo.flags;
			MustGetString(os);

			char map_name[9];
			strncpy(map_name, os.getToken().c_str(), 8);
			
			if (IsNum(map_name))
			{
				// MAPNAME is a number, assume a Hexen wad
				int map = std::atoi(map_name);
				
				sprintf(map_name, "MAP%02d", map);
				SKYFLATNAME[5] = 0;
				HexenHack = true;
				// Hexen levels are automatically nointermission
				// and even lighting and no auto sound sequences
				levelflags |= LEVEL_NOINTERMISSION
					| LEVEL_EVENLIGHTING
					| LEVEL_SNDSEQTOTALCTRL;
			}

			// Find the level.
			level_pwad_info_t& info = (levels.findByName(map_name).exists()) ?
				levels.findByName(map_name) :
				levels.create();

			info = defaultinfo;
			info.mapname = map_name;

			// Map name.
			MustGetString(os);
			if (UpperCompareToken(os, "lookup"))
			{
				MustGetString(os);
				const OString& s = GStrings(os.getToken());
				if (s.empty())
				{
					I_Error("Unknown lookup string \"%s\"", os.getToken().c_str());
				}
				info.level_name = strdup(s.c_str());
			}
			else
			{
				info.level_name = strdup(os.getToken().c_str());
			}

			tagged_info_t tinfo;
			tinfo.tag = tagged_info_t::LEVEL;
			tinfo.level = &info;
			ParseMapInfoLower(MapHandlers, MapInfoMapLevel, &tinfo, levelflags, os);

			// If the level info was parsed and no levelnum was applied,
			// try and synthesize one from the level name.
			if (info.levelnum == 0)
			{
				MapNameToLevelNum(info);
			}
		}
		else if (UpperCompareToken(os, "cluster") || UpperCompareToken(os, "clusterdef"))
		{
			MustGetInt(os);

			// Find the cluster.
			cluster_info_t& info = (clusters.findByCluster(GetTokenAsInt(os)).cluster != 0) ?
				clusters.findByCluster(GetTokenAsInt(os)) :
				clusters.create();

			info.cluster = GetTokenAsInt(os);
			tagged_info_t tinfo;
			tinfo.tag = tagged_info_t::CLUSTER;
			tinfo.cluster = &info;
			ParseMapInfoLower(ClusterHandlers, MapInfoClusterLevel, &tinfo, 0, os);
		}
		else if (UpperCompareToken(os, "episode"))
		{
			ParseEpisodeInfo(os);
		}
		else if (UpperCompareToken(os, "clearepisodes"))
		{
			episodenum = 0;
			// Set this for UMAPINFOs sake (UMAPINFO doesn't consider Doom 2's episode a real episode)
			episodes_modified = false;
		}
		else if (UpperCompareToken(os, "skill"))
		{
			// Not implemented
			MustGetString(os); // Name
			ParseMapInfoLower(NULL, NULL, NULL, 0, os);
		}
		else if (UpperCompareToken(os, "clearskills"))
		{
			// Not implemented
		}
		else if (UpperCompareToken(os, "gameinfo"))
		{
			ParseGameInfo(os);
		}
		else if (UpperCompareToken(os, "intermission"))
		{
			// Not implemented
			MustGetString(os); // Name
			ParseMapInfoLower(NULL, NULL, NULL, 0, os);
		}
		else if (UpperCompareToken(os, "automap"))
		{
			// Not implemented
			ParseMapInfoLower(NULL, NULL, NULL, 0, os);
		}
		else
		{
			I_Error("Unimplemented top-level type \"%s\"", os.getToken().c_str());
		}
	}
}

//
// G_ParseMapInfo
// Parses the MAPINFO lumps of all loaded WADs and generates
// data for wadlevelinfos and wadclusterinfos.
//
void G_ParseMapInfo()
{
	const char* baseinfoname = NULL;
	int lump;

	switch (gamemission)
	{
	case doom:
		baseinfoname = "_D1NFO";
		break;
	case doom2:
		baseinfoname = "_D2NFO";
		if (gamemode == commercial_bfg)
		{
			lump = W_GetNumForName(baseinfoname);
			ParseMapInfoLump(lump, baseinfoname);
			baseinfoname = "_BFGNFO";
		}
		break;
	case pack_tnt:
		baseinfoname = "_TNTNFO";
		break;
	case pack_plut:
		baseinfoname = "_PLUTNFO";
		break;
	case chex:
		baseinfoname = "_CHEXNFO";
		break;
	default:
		break;
	}

	lump = W_GetNumForName(baseinfoname);
	ParseMapInfoLump(lump, baseinfoname);
	
	bool found_mapinfo = false;
	lump = -1;
	
	while ((lump = W_FindLump("ZMAPINFO", lump)) != -1)
	{
		found_mapinfo = true;
		ParseMapInfoLump(lump, "ZMAPINFO");
	}

	// If ZMAPINFO exists, we don't parse a normal MAPINFO
	if (found_mapinfo == true)
	{
		return;
	}

	lump = -1;
	while ((lump = W_FindLump("UMAPINFO", lump)) != -1)
	{
		ParseUMapInfoLump(lump, "UMAPINFO");
	}

	// If UMAPINFO exists, we don't parse a normal MAPINFO
	if (found_mapinfo == true)
	{
		return;
	}

	lump = -1;
	while ((lump = W_FindLump("MAPINFO", lump)) != -1)
	{
		ParseMapInfoLump(lump, "MAPINFO");
	}

	if (episodenum == 0)
	{
		I_FatalError("You cannot use clearepisodes in a MAPINFO if you do not define any new episodes after it.");
	}
}

void P_RemoveDefereds()
{
	::getLevelInfos().zapDeferreds();
}

// [ML] Not sure where to put this for now...
// 	G_ParseMusInfo
void G_ParseMusInfo()
{
	// Nothing yet...
}

//
// G_LoadWad
//
// Determines if the vectors of wad & patch filenames differs from the currently
// loaded ones and calls D_DoomWadReboot if so.
//
bool G_LoadWad(const OWantFiles& newwadfiles, const OWantFiles& newpatchfiles,
               const std::string& mapname)
{
	bool AddedIWAD = false;
	bool Reboot = false;

	// Did we pass an IWAD?
	if (!newwadfiles.empty() && W_IsKnownIWAD(newwadfiles[0]))
	{
		AddedIWAD = true;
	}

	// Check our environment, if the same WADs are used, ignore this command.

	// Did we switch IWAD files?
	if (AddedIWAD && !::wadfiles.empty())
	{
		if (newwadfiles.at(0).getBasename() != wadfiles.at(1).getBasename())
		{
			Reboot = true;
		}
	}

	// Do the sizes of the WAD lists not match up?
	if (!Reboot)
	{
		if (::wadfiles.size() - 2 != newwadfiles.size() - (AddedIWAD ? 1 : 0))
		{
			Reboot = true;
		}
	}

	// Do our WAD lists match up exactly?
	if (!Reboot)
	{
		for (size_t i = 2, j = (AddedIWAD ? 1 : 0);
		     i < ::wadfiles.size() && j < newwadfiles.size(); i++, j++)
		{
			if (!(newwadfiles.at(j).getBasename() == ::wadfiles.at(i).getBasename()))
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
		{
			Reboot = true;
		}
	}

	// Do our patchfile lists match up exactly?
	if (!Reboot)
	{
		for (size_t i = 0, j = 0; i < ::patchfiles.size() && j < newpatchfiles.size();
		     i++, j++)
		{
			if (!(newpatchfiles.at(j).getBasename() == ::patchfiles.at(i).getBasename()))
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
		D_DoomWadReboot(newwadfiles, newpatchfiles);
		if (!missingfiles.empty())
		{
			G_DeferedInitNew(startmap);
			return false;
		}
	}

	if (mapname.length())
	{
		if (W_CheckNumForName(mapname.c_str()) != -1)
		{
			G_DeferedInitNew(mapname.c_str());
		}
        else
        {
            Printf_Bold("map %s not found, loading start map instead", mapname.c_str());
            G_DeferedInitNew(startmap);
        }
	}
	else
		G_DeferedInitNew(startmap);

	return true;
}

const char *ParseString2(const char *data);

//
// G_LoadWadString
//
// Takes a string of random wads and patches, which is sorted through and
// trampolined to the implementation of G_LoadWad.
//
bool G_LoadWadString(const std::string& str, const std::string& mapname)
{
	const std::vector<std::string>& wad_exts = M_FileTypeExts(OFILE_WAD);
	const std::vector<std::string>& deh_exts = M_FileTypeExts(OFILE_DEH);

	OWantFiles newwadfiles;
	OWantFiles newpatchfiles;

	const char* data = str.c_str();
	for (size_t argv = 0; (data = ParseString2(data)); argv++)
	{
		OWantFile file;
		if (!OWantFile::make(file, ::com_token, OFILE_UNKNOWN))
		{
			Printf(PRINT_WARNING, "Could not parse \"%s\" into file, skipping...\n",
			       ::com_token);
			continue;
		}

		// Does this look like a DeHackEd patch?
		bool is_deh =
		    std::find(deh_exts.begin(), deh_exts.end(), file.getExt()) != deh_exts.end();
		if (is_deh)
		{
			if (!OWantFile::make(file, ::com_token, OFILE_DEH))
			{
				Printf(PRINT_WARNING,
				       "Could not parse \"%s\" into patch file, skipping...\n",
				       ::com_token);
				continue;
			}

			newpatchfiles.push_back(file);
			continue;
		}

		// Does this look like a WAD file?
		bool is_wad =
		    std::find(wad_exts.begin(), wad_exts.end(), file.getExt()) != wad_exts.end();
		if (is_wad)
		{
			if (!OWantFile::make(file, ::com_token, OFILE_WAD))
			{
				Printf(PRINT_WARNING,
				       "Could not parse \"%s\" into WAD file, skipping...\n",
				       ::com_token);
				continue;
			}

			newwadfiles.push_back(file);
			continue;
		}

		// Just push the unknown file into the resource list.
		newwadfiles.push_back(file);
		continue;
	}

	return G_LoadWad(newwadfiles, newpatchfiles, mapname);
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
				Printf (PRINT_WARNING, "Map %s not found.\n", argv[1]);
			}
			else
			{ // Success
				unnatural_level_progression = true;
				G_DeferedInitNew(mapname);
			}

		}
		else
		{
			// Ch0wW - Map was still not found, so don't bother trying loading the map.
			if (W_CheckNumForName (argv[1]) == -1)
			{
				Printf (PRINT_WARNING, "Map %s not found.\n", argv[1]);
			}
			else
			{
				unnatural_level_progression = true;
				uppercopy(mapname, argv[1]); // uppercase the mapname
				G_DeferedInitNew(mapname);
			}
		}
	}
	else
	{
		Printf (PRINT_HIGH, "The current map is %s: \"%s\"\n", level.mapname.c_str(), level.level_name);
	}
}
END_COMMAND (map)

char *CalcMapName(int episode, int level)
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

void G_AirControlChanged()
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
void G_SerializeLevel(FArchive &arc, bool hubLoad)
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

		if (!arc.IsReset())
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

		if (!arc.IsReset())
		{
			arc >> playernum;
			players.resize(playernum);
		}
	}

	if (!hubLoad && !arc.IsReset())
		P_SerializePlayers(arc);

	P_SerializeThinkers(arc, hubLoad);
	P_SerializeWorld(arc);
	P_SerializePolyobjs(arc);
	P_SerializeSounds(arc);
}

// Archives the current level
void G_SnapshotLevel()
{
	delete level.info->snapshot;

	level.info->snapshot = new FLZOMemFile;
	level.info->snapshot->Open();

	FArchive arc(*level.info->snapshot);

	G_SerializeLevel(arc, false);
}

// Unarchives the current level based on its snapshot
// The level should have already been loaded and setup.
void G_UnSnapshotLevel(bool hubLoad)
{
	if (level.info->snapshot == NULL)
		return;

	level.info->snapshot->Reopen ();
	FArchive arc (*level.info->snapshot);
	if (hubLoad)
		arc.SetHubTravel (); // denis - hexen?
	G_SerializeLevel(arc, hubLoad);
	arc.Close ();
	// No reason to keep the snapshot around once the level's been entered.
	delete level.info->snapshot;
	level.info->snapshot = NULL;
}

void G_ClearSnapshots()
{
	getLevelInfos().clearSnapshots();
}

static void writeSnapShot(FArchive &arc, level_info_t *i)
{
	arc.Write (i->mapname.c_str(), 8);
	i->snapshot->Serialize (arc);
}

void G_SerializeSnapshots(FArchive &arc)
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
		arc << static_cast<char>(0);
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

static void writeDefereds(FArchive &arc, level_info_t *i)
{
	arc.Write (i->mapname.c_str(), 8);
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

static int startpos;	// [RH] Support for multiple starts per level

void G_DoWorldDone()
{
	gamestate = GS_LEVEL;
	if (wminfo.next[0] == 0)
	{
		// Don't die if no next map is given,
		// just repeat the current one.
		Printf (PRINT_WARNING, "No next map specified.\n");
	}
	else
	{
		level.mapname = wminfo.next;
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
	level.aircontrol = static_cast<fixed_t>(sv_aircontrol * 65536.f);
	G_AirControlChanged();

	// clear all ACS variables
	memset(level.vars, 0, sizeof(level.vars));

	// Get our canonical level data.
	level_pwad_info_t& info = getLevelInfos().findByName(::level.mapname.c_str());

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
	
	if (!info.level_name.empty())
	{
		// Get rid of initial lump name or level number.
		std::string begin;
		if (info.mapname[0] == 'E' && info.mapname[2] == 'M')
		{
			std::string search;
			StrFormat(search, "E%cM%c: ", info.mapname[1], info.mapname[3]);

			const std::size_t pos = info.level_name.find(search);

			if (pos != std::string::npos)
				begin = info.level_name.substr(pos + search.length());
			else
				begin = info.level_name;
		}
		else if (strstr(info.mapname.c_str(), "MAP") == &info.mapname[0])
		{
			std::string search;
			StrFormat(search, "%u: ", info.levelnum);
			
			const std::size_t pos = info.level_name.find(search);

			if (pos != std::string::npos)
				begin = info.level_name.substr(pos + search.length());
			else
				begin = info.level_name;
		}		
		if (!begin.empty())
		{
			std::string level_name(begin);
			TrimString(level_name);
			strncpy(::level.level_name, level_name.c_str(),
			        ARRAY_LENGTH(::level.level_name) - 1);
		}
		else
		{
			strncpy(::level.level_name, "Untitled Level",
			        ARRAY_LENGTH(::level.level_name) - 1);
		}
	}
	else
	{
		strncpy(::level.level_name, "Untitled Level",
		        ARRAY_LENGTH(::level.level_name) - 1);
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

	::level.exitpic = info.exitpic;
	::level.enterpic = info.enterpic;
	::level.endpic = info.endpic;

	::level.intertext = info.intertext;
	::level.intertextsecret = info.intertextsecret;
	::level.interbackdrop = info.interbackdrop;
	::level.intermusic = info.intermusic;
	
	::level.bossactions = &info.bossactions;
	::level.bossactions_donothing = info.bossactions_donothing;
	
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
		Printf(PRINT_HIGH, "%" PRIuSIZE " maps found\n", levels.size());
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

	Printf(PRINT_HIGH, "Map Name: %s\n", info.mapname.c_str());
	Printf(PRINT_HIGH, "Level Number: %d\n", info.levelnum);
	Printf(PRINT_HIGH, "Level Name: %s\n", info.level_name.c_str());
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
	flags += (info.flags & LEVEL_COMPAT_DROPOFF ? "COMPAT_DROPOFF" : "");

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

// Get global canonical levelinfo
LevelInfos& getLevelInfos()
{
	static LevelInfos li(NULL);
	return li;
}

// Get global canonical clusterinfo
ClusterInfos& getClusterInfos()
{
	static ClusterInfos ci(NULL);
	return ci;
}

// P_AllowDropOff()
bool P_AllowDropOff()
{
	return level.flags & LEVEL_COMPAT_DROPOFF || co_allowdropoff;
}

bool P_AllowPassover()
{
	if (level.flags & LEVEL_COMPAT_NOPASSOVER)
		return false;

	return co_realactorheight;
}

VERSION_CONTROL (g_level_cpp, "$Id$")
