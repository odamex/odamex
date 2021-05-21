// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2006-2021 by The Odamex Team.
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
//   Functions regarding reading and interpreting MAPINFO lumps.
//
//-----------------------------------------------------------------------------

#include "cmdlib.h"
#include "doomdef.h"
#include "doomtype.h"
#include "doomstat.h"
#include "gi.h"
#include "gstrings.h"
#include "g_episode.h"
#include "g_level.h"
#include "i_system.h"
#include "oscanner.h"
#include "p_setup.h"
#include "r_sky.h"
#include "stringenums.h"
#include "v_video.h"
#include "w_wad.h"

/// Globals
BOOL HexenHack;

namespace
{
	#define lioffset(x) offsetof(level_pwad_info_t, x)
	#define cioffset(x) offsetof(cluster_info_t, x)
	
	#define G_NOMATCH (-1) // used for MatchString

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
	
	enum EMIType
	{
		MITYPE_IGNORE,
		MITYPE_EATNEXT,
		MITYPE_INT,
		MITYPE_FLOAT,
		MITYPE_COLOR,
		MITYPE_MAPNAME,
		MITYPE_OLUMPNAME,
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
	
	const char *MapInfoMapLevel[] =
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
		{ MITYPE_OLUMPNAME, lioffset(pname), 0 },
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
		{ MITYPE_OLUMPNAME, lioffset(fadetable), 0 },
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
	
	const char *MapInfoClusterLevel[] =
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
	const char* const ActorNames[] =
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

	void SetLevelDefaults(level_pwad_info_t* levelinfo)
	{
		memset(levelinfo, 0, sizeof(*levelinfo));
		levelinfo->snapshot = NULL;
		levelinfo->outsidefog_color[0] = 255;
		levelinfo->outsidefog_color[1] = 0;
		levelinfo->outsidefog_color[2] = 0;
		levelinfo->outsidefog_color[3] = 0;
		levelinfo->fadetable = "COLORMAP";
	}
	
	//
	// Assumes that you have munched the last parameter you know how to handle,
	// but have not yet munched a comma.
	//
	void SkipUnknownParams(OScanner& os)
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
	void SkipUnknownType(OScanner& os)
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
	void SkipUnknownBlock(OScanner& os)
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

	// [DE] Below are helper functions for UMAPINFO and Z/MAPINFO parsing, partially to stand
	// in for
	// the way sc_man did things before

	bool UpperCompareToken(OScanner& os, const char* str)
    {
	    return stricmp(os.getToken().c_str(), str) == 0;
    }

	//////////////////////////////////////////////////////////////////////
	/// GetToken

	template <typename T>
    T GetToken(OScanner& os)
	{
	    I_FatalError("Templated function GetToken templated with non-existant specialized type!");
	}

    // return token as int
	template <>
	int GetToken<int>(OScanner& os)
	{
		// fix for parser reading in commas
		std::string str = os.getToken();
	
		if (str[str.length() - 1] == ',')
		{
			str[str.length() - 1] = '\0';
		}
	
		char* stopper;
	
		// if (os.compareToken("MAXINT"))
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
	template <>
	float GetToken<float>(OScanner& os)
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

	// return token as bool
	template <>
	bool GetToken<bool>(OScanner& os)
    {
	    return UpperCompareToken(os, "true");
    }

	// return token as std::string
	template <>
    std::string GetToken<std::string>(OScanner& os)
	{
	    return os.getToken();
	}

	// return token as OLumpName
    template <>
    OLumpName GetToken<OLumpName>(OScanner& os)
    {
	    return os.getToken();
    }

	//////////////////////////////////////////////////////////////////////
    /// MustGet

	// ensure token is string
    void MustGetString(OScanner& os)
    {
	    if (!os.scan())
	    {
		    I_Error("Missing string (unexpected end of file).");
	    }
    }

	template <typename T>
    void MustGet(OScanner &os)
	{
	    I_FatalError("Templated function MustGet templated with non-existant specialized type!");
	}

	// ensure token is int
	template <>
	void MustGet<int>(OScanner& os)
	{
	    if (!os.scan())
	    {
		    I_Error("Missing integer (unexpected end of file).");
	    }
	
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

	// ensure token is float
	template <>
	void MustGet<float>(OScanner& os)
	{
	    if (!os.scan())
	    {
		    I_Error("Missing floating-point number (unexpected end of file).");
	    }
	
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

	// ensure token is bool
	template <>
	void MustGet<bool>(OScanner& os)
    {
	    MustGetString(os);
	    if (!UpperCompareToken(os, "true") && !UpperCompareToken(os, "false"))
	    {
		    I_Error("Missing boolean (unexpected end of file).");
	    }
    }

	// ensure token is std::string
	template <>
    void MustGet<std::string>(OScanner& os)
    {
	    if (!os.scan())
	    {
		    I_Error("Missing string (unexpected end of file).");
	    }
    }

    // ensure token is OLumpName
    template <>
    void MustGet<OLumpName>(OScanner& os)
    {
	    if (!os.scan())
	    {
		    I_Error("Missing lump name (unexpected end of file).");
	    }

		if (os.getToken().length() > 8)
	    {
		    I_Error("Lump name too long. Maximum size is 8 characters.");
	    }
    }

	//////////////////////////////////////////////////////////////////////
    /// Misc

	bool IsIdentifier(OScanner& os)
	{
		const char ch = os.getToken()[0];
	
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
	
	void MustGetStringName(OScanner& os, const char* name)
	{
		MustGetString(os);
		if (UpperCompareToken(os, name) == false)
		{
			I_Error("Expected '%s', got '%s'.", name, os.getToken().c_str());
		}
	}
	
	// used for munching the strings in UMAPINFO
	char* ParseMultiString(OScanner& os, int error)
	{
		char* build = NULL;
	
		os.scan();
		// TODO: properly identify identifiers so clear can be separated from regular strings
		// if (IsIdentifier(os))
		{
			if (UpperCompareToken(os, "clear"))
			{
				return strdup("-"); // this was explicitly deleted to override the default.
			}
	
			// I_Error("Either 'clear' or string constant expected");
		}
		os.unScan();
	
		do
		{
			MustGetString(os);
	
			if (build == NULL)
				build = strdup(os.getToken().c_str());
			else
			{
				size_t newlen = strlen(build) + os.getToken().length() +
				                2; // strlen for both the existing text and the new line, plus
				                   // room for one \n and one \0
				build = (char*)realloc(
				    build, newlen);  // Prepare the destination memory for the below strcats
				strcat(build, "\n"); // Replace the existing text's \0 terminator with a \n
				strcat(
				    build,
				    os.getToken().c_str()); // Concatenate the new line onto the existing text
			}
			os.scan();
		} while (os.compareToken(","));
		os.unScan();
	
		return build;
	}
	
	void ParseOLumpName(OScanner& os, OLumpName& buffer)
	{
		MustGet<OLumpName>(os);
		buffer = os.getToken();
	}
	
	int ValidateMapName(const OLumpName& mapname, int* pEpi = NULL, int* pMap = NULL)
	{
		// Check if the given map name can be expressed as a gameepisode/gamemap pair and be
		// reconstructed from it.
		char lumpname[9];
		int epi = -1, map = -1;
	
		if (gamemode != commercial)
		{
		    if (sscanf(mapname.c_str(), "E%dM%d", &epi, &map) != 2)
				return 0;
			snprintf(lumpname, 9, "E%dM%d", epi, map);
		}
		else
		{
		    if (sscanf(mapname.c_str(), "MAP%d", &map) != 1)
				return 0;
			snprintf(lumpname, 9, "MAP%02d", map);
			epi = 1;
		}
		if (pEpi)
			*pEpi = epi;
		if (pMap)
			*pMap = map;
	    return !strcmp(mapname.c_str(), lumpname);
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
			ParseOLumpName(os, mape->nextmap);
			if (!ValidateMapName(mape->nextmap.c_str()))
			{
				I_Error("Invalid map name %s.", mape->nextmap.c_str());
				return 0;
			}
		}
		else if (!stricmp(pname, "nextsecret"))
		{
			ParseOLumpName(os, mape->secretmap);
			if (!ValidateMapName(mape->secretmap.c_str()))
			{
				I_Error("Invalid map name %s", mape->nextmap.c_str());
				return 0;
			}
		}
		else if (!stricmp(pname, "levelpic"))
		{
			ParseOLumpName(os, mape->pname);
		}
		else if (!stricmp(pname, "skytexture"))
		{
			ParseOLumpName(os, mape->skypic);
		}
		else if (!stricmp(pname, "music"))
		{
			ParseOLumpName(os, mape->music);
		}
		else if (!stricmp(pname, "endpic"))
		{
			ParseOLumpName(os, mape->endpic);
			mape->nextmap = "EndGame1";
		}
		else if (!stricmp(pname, "endcast"))
		{
			MustGet<bool>(os);
		    if (GetToken<bool>(os))
				mape->nextmap = "EndGameC";
			else
				mape->endpic.clear();
		}
		else if (!stricmp(pname, "endbunny"))
		{
			MustGet<bool>(os);
		    if (GetToken<bool>(os))
				mape->nextmap = "EndGame3";
			else
				mape->endpic.clear();
		}
		else if (!stricmp(pname, "endgame"))
		{
			MustGet<bool>(os);
		    if (GetToken<bool>(os))
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
			MustGet<bool>(os);
			if (GetToken<bool>(os))
			{
				mape->flags |= LEVEL_NOINTERMISSION;
			}
		}
		else if (!stricmp(pname, "partime"))
		{
			MustGet<int>(os);
			mape->partime = TICRATE * GetToken<int>(os);
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
				// MustGetStringName(os, ",");
				MustGet<int>(os);
				const int special = GetToken<int>(os);
				// MustGetStringName(os, ",");
				MustGet<int>(os);
				const int tag = GetToken<int>(os);
				// allow no 0-tag specials here, unless a level exit.
				if (tag != 0 || special == 11 || special == 51 || special == 52 ||
				    special == 124)
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
		    false,    // semiComments
		    true,     // cComments
		};
		OScanner os = OScanner::openBuffer(config, buffer, buffer + W_LumpLength(lump));
	
		while (os.scan())
		{
			if (!UpperCompareToken(os, "map"))
			{
				I_Error("Expected map definition, got %s", os.getToken().c_str());
			}
	
			MustGet<OLumpName>(os);
		    OLumpName mapname = GetToken<OLumpName>(os);
			
			if (!ValidateMapName(mapname))
			{
			    I_Error("Invalid map name %s", mapname.c_str());
			}
	
			// Find the level.
		    level_pwad_info_t& info = (levels.findByName(mapname).exists())
		                                  ? levels.findByName(mapname)
			                              : levels.create();
	
			// Free the level name string before we pave over it.
			info.level_name.clear();
	
			info = defaultinfo;
			info.mapname = mapname;
	
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
					info.nextmap = "EndGameC";
				}
				else if (info.mapname == "E1M8")
				{
					info.endpic = gamemode == retail ? "CREDIT" : "HELP2";
					info.nextmap = "EndGameC";
				}
				else if (info.mapname == "E2M8")
				{
					info.endpic = "VICTORY";
					info.nextmap = "EndGame2";
				}
				else if (info.mapname == "E3M8")
				{
					info.endpic = "$BUNNY";
					info.nextmap = "EndGame3";
				}
				else if (info.mapname == "E4M8")
				{
					info.endpic = "ENDPIC";
					info.nextmap = "EndGame4";
				}
				else if (gamemission == chex && info.mapname == "E1M5")
				{
					info.endpic = "CREDIT";
					info.nextmap = "EndGame1";
				}
				else
				{
				    char arr[9] = "";
					int ep, map;
					ValidateMapName(info.mapname.c_str(), &ep, &map);
					map++;
					if (gamemode == commercial)
					{
						sprintf(arr, "MAP%02d", map);
					}
					else
					{
					    sprintf(arr, "E%dM%d", ep, map);
					}
					info.nextmap = arr;
				}
			}
		}
	}

	
	template <typename T>
    void ParseMapinfoHelper(OScanner& os, bool doEquals)
    {
	    if (doEquals)
	    {
		    MustGetStringName(os, "=");
	    }

	    MustGet<T>(os);
    }

	template <>
    void ParseMapinfoHelper<void>(OScanner& os, bool doEquals)
	{
		// do nothing
	}

	///////////////////////////////////////////////////////////
	/// MapInfo type functions

	// Eats the next block and does nothing with the data
	void MIType_EatNext(OScanner& os, bool doEquals)
    {
	    ParseMapinfoHelper<std::string>(os, doEquals);
    }

	// Sets the inputted data as an int
    void MIType_Int(OScanner& os, bool doEquals)
    {
	    ParseMapinfoHelper<int>(os, doEquals);

	    //*((int*)(info + handler->data1)) = GetToken<int>(os);
    }

	// Sets the inputted data as a float
    void MIType_Float(OScanner& os, bool doEquals)
    {
	    ParseMapinfoHelper<float>(os, doEquals);

	    //*((float*)(info + handler->data1)) = GetToken<float>(os);
    }

	// Sets the inputted data as a color
    void MIType_Color(OScanner& os, bool doEquals)
    {
	    ParseMapinfoHelper<std::string>(os, doEquals);

	    /*argb_t color(V_GetColorFromString(os.getToken()));
	    uint8_t* ptr = (uint8_t*)(info + handler->data1);
	    ptr[0] = color.geta();
	    ptr[1] = color.getr();
	    ptr[2] = color.getg();
	    ptr[3] = color.getb();*/
    }

	// Sets the inputted data as an OLumpName map name
    void MIType_MapName(OScanner& os, bool doEquals)
    {
	    ParseMapinfoHelper<OLumpName>(os, doEquals);

	    char map_name[9];
	    strncpy(map_name, os.getToken().c_str(), 8);

	    if (IsNum(map_name))
	    {
		    int map = std::atoi(map_name);
		    sprintf(map_name, "MAP%02d", map);
	    }

	    //*(OLumpName*)(info + handler->data1) = map_name;
    }

	// Sets the inputted data as an OLumpName
    void MIType_LumpName(OScanner& os, bool doEquals)
    {
	    ParseMapinfoHelper<OLumpName>(os, doEquals);

	    //*(OLumpName*)(info + handler->data1) = os.getToken();
    }

	// Sets the inputted data as an OLumpName, checking LANGUAGE for the actual OLumpName
    void MIType_$LumpName(OScanner& os, bool doEquals)
    {
	    ParseMapinfoHelper<std::string>(os, doEquals);

	    OLumpName temp;
	    if (os.getToken()[0] == '$')
	    {
		    // It is possible to pass a DeHackEd string
		    // prefixed by a $.
		    const OString& s = GStrings(os.getToken().c_str() + 1);
		    if (s.empty())
		    {
			    I_Error("Unknown lookup string \"%s\"", os.getToken().c_str());
		    }
		    temp = s;
	    }
	    else
	    {
		    temp = os.getToken();
	    }

	    //*(OLumpName*)(info + handler->data1) = temp;
    }

	// Sets the inputted data as an OLumpName, checking LANGUAGE for the actual OLumpName (Music variant)
    void MIType_MusicLumpName(OScanner& os, bool doEquals)
    {
	    ParseMapinfoHelper<std::string>(os, doEquals);

	    OLumpName temp;
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
		    temp = lumpname;
	    }
	    else
	    {
		    temp = os.getToken();
	    }

	    //*(OLumpName*)(info + handler->data1) = temp;
    }

	// Sets the sky texture with an OLumpName
    void MIType_Sky(OScanner& os, bool doEquals)
    {
	    if (doEquals)
		{
			MustGetStringName(os, "=");
			MustGetString(os); // Texture name
		    //*(OLumpName*)(info + handler->data1) = os.getToken();
			SkipUnknownParams(os);
		}
		else
		{
			MustGetString(os); // get texture name;
		    //*(OLumpName*)(info + handler->data1) = os.getToken();
			MustGet<float>(os); // get scroll speed
			/*if (HexenHack)
			{
				*((fixed_t *)(info + handler->data2)) = sc_Number << 8;
			}
			 else
			{
				*((fixed_t *)(info + handler->data2)) = (fixed_t)(sc_Float * 65536.0f);
			}*/
		}
    }

	// Sets a flag
    void MIType_SetFlag(OScanner& os, bool doEquals)
    {
	    // flags |= handler->data1;
    }

	// Sets an SC flag
    void MIType_SCFlags(OScanner& os, bool doEquals)
    {
	    // flags = (flags & handler->data2) | handler->data1;
    }

	// Sets a cluster
    void MIType_Cluster(OScanner& os, bool doEquals)
    {
	    ParseMapinfoHelper<int>(os, doEquals);

	    //*((int*)(info + handler->data1)) = GetToken<int>(os);
	    if (HexenHack)
	    {
		    ClusterInfos& clusters = getClusterInfos();
		    cluster_info_t& clusterH = clusters.findByCluster(GetToken<int>(os));
		    if (clusterH.cluster != 0)
		    {
			    clusterH.flags |= CLUSTER_HUB;
		    }
	    }
    }

	// Sets a cluster string
    void MIType_ClusterString(OScanner& os, bool doEquals)
    {
	    char** text = NULL; // (char**)(info + handler->data1);
	    return;
		
	    if (doEquals)
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
    }

	// Sets a compatibility flag
    void MIType_SetCompatFlag(OScanner& os, bool doEquals)
    {
	    // todo
    }

	typedef std::list<std::pair<const char*, void(*)(OScanner&, bool)>> MapInfoData;

	MapInfoData MapInfoMapContent =
	{
		{"levelnum",				  MIType_Int},
		{"next",					  MIType_MapName},
        {"secretnext",				  MIType_MapName},
		{"cluster",					  MIType_Cluster},
		{"sky1",					  MIType_Sky},
		{"sky2",					  MIType_Sky},
		{"fade",					  MIType_Color},
		{"outsidefog",				  MIType_Color},
        {"titlepatch",				  MIType_LumpName},
		{"par",						  MIType_Int},
        {"music",					  MIType_MusicLumpName},
		{"nointermission",			  MIType_SetFlag},
		{"doublesky",				  MIType_SetFlag},
		{"nosoundclipping",			  MIType_SetFlag},
		{"allowmonstertelefrags",	  MIType_SetFlag},
		{"map07special",			  MIType_SetFlag},
		{"baronspecial",			  MIType_SetFlag},
		{"cyberdemonspecial",		  MIType_SetFlag},
		{"spidermastermindspecial",	  MIType_SetFlag},
		{"specialaction_exitlevel",	  MIType_SCFlags},
		{"specialaction_opendoor",	  MIType_SCFlags},
		{"specialaction_lowerfloor",  MIType_SCFlags},
		{"lightning",				  NULL},
		{"fadetable",				  MIType_LumpName},
		{"evenlighting",			  MIType_SetFlag},
		{"noautosequences",			  MIType_SetFlag},
		{"forcenoskystretch",		  MIType_SetFlag},
		{"allowfreelook",			  MIType_SCFlags},
		{"nofreelook",				  MIType_SCFlags},
		{"allowjump",				  MIType_SCFlags},
		{"nojump",					  MIType_SCFlags},
		{"cdtrack",					  MIType_EatNext},
		{"cd_start_track",			  MIType_EatNext},
		{"cd_end1_track",			  MIType_EatNext},
		{"cd_end2_track",			  MIType_EatNext},
		{"cd_end3_track",			  MIType_EatNext},
		{"cd_intermission_track",	  MIType_EatNext},
		{"cd_title_track",			  MIType_EatNext},
		{"warptrans",				  MIType_EatNext},
		{"gravity",					  MIType_Float},
		{"aircontrol",				  MIType_Float},
		{"islobby",					  MIType_SetFlag},
		{"lobby",					  MIType_SetFlag},
		{"nocrouch",				  NULL},
		{"intermusic",				  MIType_EatNext},
		{"par",						  MIType_EatNext},
		{"sucktime",				  MIType_EatNext},
	    {"enterpic",				  MIType_EatNext},
	    {"exitpic",					  MIType_EatNext},
	    {"interpic",				  MIType_EatNext},
		{"translator",				  MIType_EatNext},
		{"compat_shorttex",			  MIType_EatNext},
		{"compat_limitpain",		  MIType_EatNext},
	    {"compat_dropoff",			  MIType_SetCompatFlag},
		{"compat_trace",			  MIType_EatNext},
		{"compat_boomscroll",		  MIType_EatNext},
		{"compat_sectorsounds",		  MIType_EatNext},
		{"compat_nopassover", 		  MIType_SetFlag},
	};

	MapInfoHandler MapHandlers2[] =
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
		{ MITYPE_OLUMPNAME, lioffset(pname), 0 },
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
		{ MITYPE_OLUMPNAME, lioffset(fadetable), 0 },
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

	//
    // Parse a MAPINFO block
    //
    // NULL pointers can be passed if the block is unimplemented.  However, if
    // the block you want to stub out is compatible with old MAPINFO, you need
    // to parse the block anyway, even if you throw away the values.  This is
    // done by passing in a strings pointer, and leaving the others NULL.
    //
    void ParseMapInfoLower_New(OScanner& os)
	{
	    // 0 if old mapinfo, positive number if new MAPINFO, the exact
	    // number represents current brace depth.
	    int newMapinfoStack = 0;

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

		    if (newMapinfoStack <= 0 && ContainsMapInfoTopLevel(os) &&
		        // "cluster" is a valid map block type and is also
		        // a valid top-level type.
		        !UpperCompareToken(os, "cluster"))
		    {
			    // Old-style MAPINFO is done
			    os.unScan();
			    break;
		    }

			// find the matching string and use its corresponding function
			MapInfoData::iterator it = MapInfoMapContent.begin();
			for (; it != MapInfoMapContent.end(); ++it)
			{
			    if (UpperCompareToken(os, it->first))
			    {
				    if (it->second)
				    {
					    it->second(os, newMapinfoStack > 0);
				    }
			    }
			}
			
			if (it == MapInfoMapContent.end())
			{
				if (newMapinfoStack <= 0)
				{
					// Old MAPINFO is up a creek, we need to be
					// able to parse all types even if we can't
					// do anything with them.
					//
					I_Error("Unknown MAPINFO token \"%s\"", os.getToken().c_str());
				}
	
				// New MAPINFO is capable of skipping past unknown
				// types.
				SkipUnknownType(os);
			}
	    }

    	// TODO: is this necessary?
    	/*switch (tinfo->tag)
	    {
	    case tagged_info_t::LEVEL:
		    tinfo->level->flags = flags;
		    break;
	    case tagged_info_t::CLUSTER:
		    tinfo->cluster->flags = flags;
		    break;
	    }*/
	}

	//
	// Parse a MAPINFO block
	//
	// NULL pointers can be passed if the block is unimplemented.  However, if
	// the block you want to stub out is compatible with old MAPINFO, you need
	// to parse the block anyway, even if you throw away the values.  This is
	// done by passing in a strings pointer, and leaving the others NULL.
	//
	void ParseMapInfoLower(MapInfoHandler* handlers, const char** strings,
	                              tagged_info_t* tinfo, DWORD flags, OScanner& os)
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
	
			if (newMapinfoStack <= 0 && ContainsMapInfoTopLevel(os) &&
			    // "cluster" is a valid map block type and is also
			    // a valid top-level type.
			    !UpperCompareToken(os, "cluster"))
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
			    ParseMapinfoHelper<std::string>(os, newMapinfoStack > 0);
				
				break;
	
			case MITYPE_INT:
			    ParseMapinfoHelper<int>(os, newMapinfoStack > 0);
				
				*((int*)(info + handler->data1)) = GetToken<int>(os);
				break;
	
			case MITYPE_FLOAT:
			    ParseMapinfoHelper<float>(os, newMapinfoStack > 0);
				
				*((float*)(info + handler->data1)) = GetToken<float>(os);
				break;
	
			case MITYPE_COLOR: {
			    ParseMapinfoHelper<std::string>(os, newMapinfoStack > 0);
				
				argb_t color(V_GetColorFromString(os.getToken()));
				uint8_t* ptr = (uint8_t*)(info + handler->data1);
				ptr[0] = color.geta();
				ptr[1] = color.getr();
				ptr[2] = color.getg();
				ptr[3] = color.getb();
				break;
			}
			case MITYPE_MAPNAME:
			    ParseMapinfoHelper<OLumpName>(os, newMapinfoStack > 0);
	
				char map_name[9];
				strncpy(map_name, os.getToken().c_str(), 8);
	
				if (IsNum(map_name))
				{
					int map = std::atoi(map_name);
					sprintf(map_name, "MAP%02d", map);
				}
				
				*(OLumpName*)(info + handler->data1) = map_name;
				break;
	
			case MITYPE_OLUMPNAME:
			    ParseMapinfoHelper<OLumpName>(os, newMapinfoStack > 0);
				
				*(OLumpName*)(info + handler->data1) = os.getToken();
			    break;
	
			case MITYPE_$LUMPNAME: {
			    ParseMapinfoHelper<std::string>(os, newMapinfoStack > 0);

			    OLumpName temp;
			    if (os.getToken()[0] == '$')
			    {
				    // It is possible to pass a DeHackEd string
				    // prefixed by a $.
				    const OString& s = GStrings(os.getToken().c_str() + 1);
				    if (s.empty())
				    {
					    I_Error("Unknown lookup string \"%s\"", os.getToken().c_str());
				    }
				    temp = s;
			    }
			    else
			    {
				    temp = os.getToken();
			    }

			    *(OLumpName*)(info + handler->data1) = temp;
			    break;
		    }
			case MITYPE_MUSICLUMPNAME: {
			    ParseMapinfoHelper<std::string>(os, newMapinfoStack > 0);

				OLumpName temp;
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
					temp = lumpname;
				}
				else
				{
				    temp = os.getToken();
				}

				*(OLumpName*)(info + handler->data1) = temp;
				break;
			}
			case MITYPE_SKY:
				if (newMapinfoStack > 0)
				{
					MustGetStringName(os, "=");
					MustGetString(os); // Texture name
				    *(OLumpName*)(info + handler->data1) = os.getToken();
					SkipUnknownParams(os);
				}
				else
				{
					MustGetString(os); // get texture name;
				    *(OLumpName*)(info + handler->data1) = os.getToken();
					MustGet<float>(os); // get scroll speed
					// if (HexenHack)
					//{
					//	*((fixed_t *)(info + handler->data2)) = sc_Number << 8;
					//}
					// else
					//{
					//	*((fixed_t *)(info + handler->data2)) = (fixed_t)(sc_Float *
					//65536.0f);
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
			    ParseMapinfoHelper<int>(os, newMapinfoStack > 0);
				
				*((int*)(info + handler->data1)) = GetToken<int>(os);
				if (HexenHack)
				{
					ClusterInfos& clusters = getClusterInfos();
					cluster_info_t& clusterH = clusters.findByCluster(GetToken<int>(os));
					if (clusterH.cluster != 0)
					{
						clusterH.flags |= CLUSTER_HUB;
					}
				}
				break;
	
			case MITYPE_STRING: {
			    ParseMapinfoHelper<std::string>(os, newMapinfoStack > 0);

				char** text = (char**)(info + handler->data1);
				free(*text);
				*text = strdup(os.getToken().c_str());
				break;
			}
	
			case MITYPE_CSTRING:
			    ParseMapinfoHelper<std::string>(os, newMapinfoStack > 0);
				
				strncpy((char*)(info + handler->data1), os.getToken().c_str(),
				        handler->data2);
				*((char*)(info + handler->data1 + handler->data2)) = '\0';
				break;
				
			case MITYPE_CLUSTERSTRING: {
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
			}
			}
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

	void ParseEpisodeInfo(OScanner& os)
	{
		int new_mapinfo = false; // is int instead of bool for template purposes
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
				I_Error(
				    "Detected incorrectly placed curly brace in MAPINFO episode definiton");
			}
			else if (os.compareToken("}"))
			{
				if (new_mapinfo == false)
				{
					I_Error("Detected incorrectly placed curly brace in MAPINFO episode "
					        "definiton");
				}
				else
				{
					break;
				}
			}
			else if (UpperCompareToken(os, "name"))
			{
				ParseMapinfoHelper<std::string>(os, new_mapinfo);
				
				if (picisgfx == false)
				{
					pic = os.getToken();
				}
			}
			else if (UpperCompareToken(os, "lookup"))
			{
			    ParseMapinfoHelper<std::string>(os, new_mapinfo);
	
				// Not implemented
			}
			else if (UpperCompareToken(os, "picname"))
			{
			    ParseMapinfoHelper<std::string>(os, new_mapinfo);
				
				pic = os.getToken();
				picisgfx = true;
			}
			else if (UpperCompareToken(os, "key"))
			{
			    ParseMapinfoHelper<std::string>(os, new_mapinfo);
				
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
	
		if (remove || (optional && W_CheckNumForName(map.c_str()) == -1) ||
		    (extended && W_CheckNumForName("EXTENDED") == -1))
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

	void ParseGameInfo(OScanner& os)
	{
		MustGetStringName(os, "{");
	
		while (os.scan())
		{
			if (UpperCompareToken(os, "{"))
			{
				// Detected new-style MAPINFO
				I_Error(
				    "Detected incorrectly placed curly brace in MAPINFO episode definiton");
			}
			else if (UpperCompareToken(os, "}"))
			{
				break;
			}
			else if (UpperCompareToken(os, "advisorytime"))
			{
				MustGetStringName(os, "=");
				os.scan();
	
				gameinfo.advisoryTime = GetToken<float>(os);
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
	
				gameinfo.pageTime = GetToken<float>(os);
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
	
				gameinfo.titleTime = GetToken<float>(os);
			}
			else
			{
				// Game info property is not implemented
				MustGetStringName(os, "=");
				os.scan();
			}
		}
	}

	void ParseMapInfoLump(int lump, const char* lumpname)
	{
		LevelInfos& levels = getLevelInfos();
		ClusterInfos& clusters = getClusterInfos();
	
		level_pwad_info_t defaultinfo;
		SetLevelDefaults(&defaultinfo);
	
		const char* buffer = (char*)W_CacheLumpNum(lump, PU_STATIC);
	
		OScannerConfig config = {
		    lumpname, // lumpName
		    false,    // semiComments
		    true,     // cComments
		};
		OScanner os = OScanner::openBuffer(config, buffer, buffer + W_LumpLength(lump));
	
		while (os.scan())
		{
			if (UpperCompareToken(os, "defaultmap"))
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
					levelflags |=
					    LEVEL_NOINTERMISSION | LEVEL_EVENLIGHTING | LEVEL_SNDSEQTOTALCTRL;
				}
	
				// Find the level.
				level_pwad_info_t& info = (levels.findByName(map_name).exists())
				                              ? levels.findByName(map_name)
				                              : levels.create();
	
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
				MustGet<int>(os);
	
				// Find the cluster.
				cluster_info_t& info =
				    (clusters.findByCluster(GetToken<int>(os)).cluster != 0)
				        ? clusters.findByCluster(GetToken<int>(os))
				        : clusters.create();
	
				info.cluster = GetToken<int>(os);
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
				// Set this for UMAPINFOs sake (UMAPINFO doesn't consider Doom 2's episode a
				// real episode)
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
} // namespace

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
		I_FatalError("You cannot use clearepisodes in a MAPINFO if you do not define any "
		             "new episodes after it.");
	}
}