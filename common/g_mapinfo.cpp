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
#include "doomstat.h"
#include "doomtype.h"
#include "g_episode.h"
#include "g_level.h"
#include "gi.h"
#include "gstrings.h"
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
// [DE] used for UMAPINFO's boss actions
const char* const ActorNames[] = {
    "DoomPlayer", "ZombieMan", "ShotgunGuy", "Archvile", "ArchvileFire", "Revenant",
    "RevenantTracer", "RevenantTracerSmoke", "Fatso", "FatShot", "ChaingunGuy", "DoomImp",
    "Demon", "Spectre", "Cacodemon", "BaronOfHell", "BaronBall", "HellKnight", "LostSoul",
    "SpiderMastermind", "Arachnotron", "Cyberdemon", "PainElemental", "WolfensteinSS",
    "CommanderKeen", "BossBrain", "BossEye", "BossTarget", "SpawnShot", "SpawnFire",
    "ExplosiveBarrel", "DoomImpBall", "CacodemonBall", "Rocket", "PlasmaBall", "BFGBall",
    "ArachnotronPlasma", "BulletPuff", "Blood", "TeleportFog", "ItemFog", "TeleportDest",
    "BFGExtra", "GreenArmor", "BlueArmor", "HealthBonus", "ArmorBonus", "BlueCard",
    "RedCard", "YellowCard", "YellowSkull", "RedSkull", "BlueSkull", "Stimpack",
    "Medikit", "Soulsphere", "InvulnerabilitySphere", "Berserk", "BlurSphere", "RadSuit",
    "Allmap", "Infrared", "Megasphere", "Clip", "ClipBox", "RocketAmmo", "RocketBox",
    "Cell", "CellPack", "Shell", "ShellBox", "Backpack", "BFG9000", "Chaingun",
    "Chainsaw", "RocketLauncher", "PlasmaRifle", "Shotgun", "SuperShotgun", "TechLamp",
    "TechLamp2", "Column", "TallGreenColumn", "ShortGreenColumn", "TallRedColumn",
    "ShortRedColumn", "SkullColumn", "HeartColumn", "EvilEye", "FloatingSkull",
    "TorchTree", "BlueTorch", "GreenTorch", "RedTorch", "ShortBlueTorch",
    "ShortGreenTorch", "ShortRedTorch", "Stalagtite", "TechPillar", "CandleStick",
    "Candelabra", "BloodyTwitch", "Meat2", "Meat3", "Meat4", "Meat5", "NonsolidMeat2",
    "NonsolidMeat4", "NonsolidMeat3", "NonsolidMeat5", "NonsolidTwitch", "DeadCacodemon",
    "DeadMarine", "DeadZombieMan", "DeadDemon", "DeadLostSoul", "DeadDoomImp",
    "DeadShotgunGuy", "GibbedMarine", "GibbedMarineExtra", "HeadsOnAStick", "Gibs",
    "HeadOnAStick", "HeadCandles", "DeadStick", "LiveStick", "BigTree", "BurningBarrel",
    "HangNoGuts", "HangBNoBrain", "HangTLookingDown", "HangTSkull", "HangTLookingUp",
    "HangTNoBrain", "ColonGibs", "SmallBloodPool", "BrainStem",
    // Boom/MBF additions
    "PointPusher", "PointPuller", "MBFHelperDog", "PlasmaBall1", "PlasmaBall2",
    "EvilSceptre", "UnholyBible", NULL};

void SetLevelDefaults(level_pwad_info_t& levelinfo)
{
	levelinfo = level_pwad_info_t();
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
	return iequals(os.getToken(), str);
}

//////////////////////////////////////////////////////////////////////
/// GetToken

template <typename T>
T GetToken(OScanner& os)
{
	I_FatalError(
	    "Templated function GetToken templated with non-existant specialized type!");
}

// return token as int
template <>
int GetToken<int>(OScanner& os)
{
	return os.getTokenInt();
}

// return token as float
template <>
float GetToken<float>(OScanner& os)
{
	return os.getTokenFloat();
}

// return token as bool
template <>
bool GetToken<bool>(OScanner& os)
{
	return os.getTokenBool();
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
void MustGet(OScanner& os)
{
	I_FatalError(
	    "Templated function MustGet templated with non-existant specialized type!");
}

// ensure token is int
template <>
void MustGet<int>(OScanner& os)
{
	os.scanInt();
}

// ensure token is float
template <>
void MustGet<float>(OScanner& os)
{
	os.scanFloat();
}

// ensure token is bool
template <>
void MustGet<bool>(OScanner& os)
{
	os.scanBool();
}

// ensure token is std::string
template <>
void MustGet<std::string>(OScanner& os)
{
	os.scanString();
}

// ensure token is OLumpName
template <>
void MustGet<OLumpName>(OScanner& os)
{
	os.scanString();

	if (os.getToken().length() > 8)
	{
		I_Error("Lump name \"%s\" too long. Maximum size is 8 characters.",
		        os.getToken().c_str());
	}
}

//////////////////////////////////////////////////////////////////////
/// Misc

bool IsIdentifier(OScanner& os)
{
	const char& ch = os.getToken()[0];

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

bool ContainsMapInfoTopLevel(OScanner& os)
{
	return UpperCompareToken(os, "map") || UpperCompareToken(os, "defaultmap") ||
	       UpperCompareToken(os, "cluster") || UpperCompareToken(os, "clusterdef") ||
	       UpperCompareToken(os, "episode") || UpperCompareToken(os, "clearepisodes") ||
	       UpperCompareToken(os, "skill") || UpperCompareToken(os, "clearskills") ||
	       UpperCompareToken(os, "gameinfo") || UpperCompareToken(os, "intermission") ||
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
char* ParseMultiString(OScanner& os)
{
	char* build = NULL;

	os.scan();
	// TODO: properly identify identifiers so clear can be separated from regular strings
	if (!os.isQuotedString())
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
		char* lname = ParseMultiString(os);
		if (!lname)
			return 0;
		mape->intertext = lname;
	}
	else if (!stricmp(pname, "intertextsecret"))
	{
		char* lname = ParseMultiString(os);
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

		char* lname = ParseMultiString(os);
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
	SetLevelDefaults(defaultinfo);

	const char* buffer = static_cast<char*>(W_CacheLumpNum(lump, PU_STATIC));

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
void ParseMapInfoHelper(OScanner& os, bool doEquals)
{
	if (doEquals)
	{
		MustGetStringName(os, "=");
	}

	MustGet<T>(os);
}

template <>
void ParseMapInfoHelper<void>(OScanner& os, bool doEquals)
{
	// do nothing
}

//////////////////////////////////////////////////////////////////////
/// MapInfo type functions

// Eats the next block and does nothing with the data
void MIType_EatNext(OScanner& os, bool doEquals, void* data, unsigned int flags,
                    unsigned int flags2)
{
	ParseMapInfoHelper<std::string>(os, doEquals);
}

// Sets the inputted data as an int
void MIType_Int(OScanner& os, bool doEquals, void* data, unsigned int flags,
                unsigned int flags2)
{
	ParseMapInfoHelper<int>(os, doEquals);

	*static_cast<int*>(data) = GetToken<int>(os);
}

// Sets the inputted data as a float
void MIType_Float(OScanner& os, bool doEquals, void* data, unsigned int flags,
                  unsigned int flags2)
{
	ParseMapInfoHelper<float>(os, doEquals);

	*static_cast<float*>(data) = GetToken<float>(os);
}

// Sets the inputted data as a color
void MIType_Color(OScanner& os, bool doEquals, void* data, unsigned int flags,
                  unsigned int flags2)
{
	ParseMapInfoHelper<std::string>(os, doEquals);

	argb_t color(V_GetColorFromString(os.getToken()));
	uint8_t* ptr = static_cast<uint8_t*>(data);
	ptr[0] = color.geta();
	ptr[1] = color.getr();
	ptr[2] = color.getg();
	ptr[3] = color.getb();
}

// Sets the inputted data as an OLumpName map name
void MIType_MapName(OScanner& os, bool doEquals, void* data, unsigned int flags,
                    unsigned int flags2)
{
	ParseMapInfoHelper<OLumpName>(os, doEquals);

	if (os.isQuotedString())
	{
		char map_name[9];
		strncpy(map_name, os.getToken().c_str(), 8);

		if (IsNum(map_name))
		{
			int map = std::atoi(map_name);
			sprintf(map_name, "MAP%02d", map);
		}
		else if (UpperCompareToken(os, "EndBunny"))
		{
			*static_cast<OLumpName*>(data) = "EndGame3";
			return;
		}

		*static_cast<OLumpName*>(data) = map_name;
	}
	else
	{
		// endgame block
		if (UpperCompareToken(os, "endgame"))
		{
			MustGetStringName(os, "{");

			while (os.scan())
			{
				if (UpperCompareToken(os, "}"))
				{
					break;
				}

				if (UpperCompareToken(os, "pic"))
				{
					ParseMapInfoHelper<OLumpName>(os, doEquals);

					// todo
				}
				else if (UpperCompareToken(os, "hscroll"))
				{
					ParseMapInfoHelper<std::string>(os, doEquals);

					// todo
				}
				else if (UpperCompareToken(os, "vscroll"))
				{
					ParseMapInfoHelper<std::string>(os, doEquals);

					// todo
				}
				else if (UpperCompareToken(os, "cast"))
				{
					*static_cast<OLumpName*>(data) = "EndGameC";
				}
				if (UpperCompareToken(os, "music"))
				{
					ParseMapInfoHelper<OLumpName>(os, doEquals);

					// todo
					os.scan();
					if (UpperCompareToken(os, ","))
					{
						MustGet<float>(os);
						// todo
					}
					else
					{
						os.unScan();
					}
				}
			}
		}
		else if (UpperCompareToken(os, "EndPic"))
		{
			MustGetStringName(os, ",");
			MustGetString(os);

			// todo
		}
		else if (UpperCompareToken(os, "EndSequence"))
		{
			MustGetStringName(os, ",");
			MustGetString(os);

			// todo
		}
	}
}

// Sets the inputted data as an OLumpName
void MIType_LumpName(OScanner& os, bool doEquals, void* data, unsigned int flags,
                     unsigned int flags2)
{
	ParseMapInfoHelper<OLumpName>(os, doEquals);

	*static_cast<OLumpName*>(data) = os.getToken();
}

// Handle lump names that can also be intermission scripts
void MIType_InterLumpName(OScanner& os, bool doEquals, void* data, unsigned int flags,
                          unsigned int flags2)
{
	ParseMapInfoHelper<std::string>(os, doEquals);
	std::string tok = os.getToken();
	if (!tok.empty() && tok.at(0) == '$')
	{
		// Intermission scripts are not supported.
		return;
	}
	*static_cast<OLumpName*>(data) = tok;
}

// Sets the inputted data as an OLumpName, checking LANGUAGE for the actual OLumpName
void MIType_$LumpName(OScanner& os, bool doEquals, void* data, unsigned int flags,
                      unsigned int flags2)
{
	ParseMapInfoHelper<std::string>(os, doEquals);

	if (os.getToken()[0] == '$')
	{
		// It is possible to pass a DeHackEd string
		// prefixed by a $.
		const OString& s = GStrings(os.getToken().c_str() + 1);
		if (s.empty())
		{
			std::string err;
			StrFormat(err, "Unknown lookup string \"%s\".", os.getToken().c_str());
			os.error(err.c_str());
		}
		*static_cast<OLumpName*>(data) = s;
	}
	else
	{
		*static_cast<OLumpName*>(data) = os.getToken();
	}
}

// Sets the inputted data as an OLumpName, checking LANGUAGE for the actual OLumpName
// (Music variant)
void MIType_MusicLumpName(OScanner& os, bool doEquals, void* data, unsigned int flags,
                          unsigned int flags2)
{
	ParseMapInfoHelper<std::string>(os, doEquals);

	if (os.getToken()[0] == '$')
	{
		// It is possible to pass a DeHackEd string
		// prefixed by a $.
		const OString& s = GStrings(os.getToken().c_str() + 1);
		if (s.empty())
		{
			std::string err;
			StrFormat(err, "Unknown lookup string \"%s\".", os.getToken().c_str());
			os.error(err.c_str());
		}

		// Music lumps in the stringtable do not begin
		// with a D_, so we must add it.
		char lumpname[9];
		snprintf(lumpname, ARRAY_LENGTH(lumpname), "D_%s", s.c_str());
		*static_cast<OLumpName*>(data) = lumpname;
	}
	else
	{
		*static_cast<OLumpName*>(data) = os.getToken();
	}
}

// Sets the sky texture with an OLumpName
void MIType_Sky(OScanner& os, bool doEquals, void* data, unsigned int flags,
                unsigned int flags2)
{
	ParseMapInfoHelper<OLumpName>(os, doEquals);

	*static_cast<OLumpName*>(data) = os.getToken();

	// Scroll speed
	if (doEquals)
	{
		os.scan();
		if (!os.compareToken(","))
		{
			os.unScan();
			return;
		}
	}
	MustGet<float>(os);
	/*if (HexenHack)
	{
	    *((fixed_t *)(info + handler->data2)) = sc_Number << 8;
	}
	 else
	{
	    *((fixed_t *)(info + handler->data2)) = (fixed_t)(sc_Float * 65536.0f);
	}*/
}

// Sets a flag
void MIType_SetFlag(OScanner& os, bool doEquals, void* data, unsigned int flags,
                    unsigned int flags2)
{
	*static_cast<DWORD*>(data) |= flags;
}

// Sets an SC flag
void MIType_SCFlags(OScanner& os, bool doEquals, void* data, unsigned int flags,
                    unsigned int flags2)
{
	*static_cast<DWORD*>(data) = (*static_cast<DWORD*>(data) & flags2) | flags;
}

// Sets a cluster
void MIType_Cluster(OScanner& os, bool doEquals, void* data, unsigned int flags,
                    unsigned int flags2)
{
	ParseMapInfoHelper<int>(os, doEquals);

	*static_cast<int*>(data) = GetToken<int>(os);
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
void MIType_ClusterString(OScanner& os, bool doEquals, void* data, unsigned int flags,
                          unsigned int flags2)
{
	ParseMapInfoHelper<std::string>(os, doEquals);

	char** text = static_cast<char**>(data);

	if (doEquals)
	{
		if (UpperCompareToken(os, "lookup"))
		{
			if (doEquals)
			{
				MustGetStringName(os, ",");
			}

			MustGetString(os);
			const OString& s = GStrings(os.getToken());
			if (s.empty())
			{
				std::string err;
				StrFormat(err, "Unknown lookup string \"%s\".", os.getToken().c_str());
				os.error(err.c_str());
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
		if (UpperCompareToken(os, "lookup"))
		{
			MustGetString(os);
			const OString& s = GStrings(os.getToken());
			if (s.empty())
			{
				std::string err;
				StrFormat(err, "Unknown lookup string \"%s\".", os.getToken().c_str());
				os.error(err.c_str());
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

//////////////////////////////////////////////////////////////////////
/// MapInfoData

typedef void (*MITypeFunctionPtr)(OScanner&, bool, void*, unsigned int, unsigned int);

// data structure containing all of the information needed to set a value from mapinfo
struct MapInfoData
{
	const char* name;
	MITypeFunctionPtr fn;
	void* data;
	unsigned int flags;
	unsigned int flags2;

	MapInfoData(const char* _name, MITypeFunctionPtr _fn = NULL, void* _data = NULL,
	            unsigned int _flags = 0, unsigned int _flags2 = 0)
	    : name(_name), fn(_fn), data(_data), flags(_flags), flags2(_flags2)
	{
	}
};

// container holding all MapInfoData types
typedef std::vector<MapInfoData> MapInfoDataContainer;

// base class for MapInfoData; also used when there's no data to process (unimplemented
// blocks)
template <typename T>
struct MapInfoDataSetter
{
	MapInfoDataContainer mapInfoDataContainer;

	MapInfoDataSetter() : mapInfoDataContainer()
	{
	}
};

// macro to make up for lack of initializer lists in C++98
#define ENTRY1(x1) mapInfoDataContainer.push_back(MapInfoData(x1));
#define ENTRY2(x1, x2) mapInfoDataContainer.push_back(MapInfoData(x1, x2));
#define ENTRY3(x1, x2, x3) mapInfoDataContainer.push_back(MapInfoData(x1, x2, x3));
#define ENTRY4(x1, x2, x3, x4) \
	mapInfoDataContainer.push_back(MapInfoData(x1, x2, x3, x4));
#define ENTRY5(x1, x2, x3, x4, x5) \
	mapInfoDataContainer.push_back(MapInfoData(x1, x2, x3, x4, x5));

// level_pwad_info_t
template <>
struct MapInfoDataSetter<level_pwad_info_t>
{
	MapInfoDataContainer mapInfoDataContainer;

	MapInfoDataSetter(level_pwad_info_t& ref)
	{
		mapInfoDataContainer.reserve(
		    70); // [DE] some random number, i'm not counting all these

		ENTRY3("levelnum", &MIType_Int, &ref.levelnum)
		ENTRY3("next", &MIType_MapName, &ref.nextmap)
		ENTRY3("secretnext", &MIType_MapName, &ref.secretmap)
		ENTRY3("secret", &MIType_MapName, &ref.secretmap)
		ENTRY3("cluster", &MIType_Cluster, &ref.cluster)
		ENTRY3("sky1", &MIType_Sky, &ref.skypic)
		ENTRY3("sky2", &MIType_Sky, &ref.skypic2)
		ENTRY3("fade", &MIType_Color, &ref.fadeto_color)
		ENTRY3("outsidefog", &MIType_Color, &ref.outsidefog_color)
		ENTRY3("titlepatch", &MIType_LumpName, &ref.pname)
		ENTRY3("par", &MIType_Int, &ref.partime)
		ENTRY3("music", &MIType_MusicLumpName, &ref.music)
		ENTRY4("nointermission", &MIType_SetFlag, &ref.flags, LEVEL_NOINTERMISSION)
		ENTRY4("doublesky", &MIType_SetFlag, &ref.flags, LEVEL_DOUBLESKY)
		ENTRY4("nosoundclipping", &MIType_SetFlag, &ref.flags, LEVEL_NOSOUNDCLIPPING)
		ENTRY4("allowmonstertelefrags", &MIType_SetFlag, &ref.flags,
		       LEVEL_MONSTERSTELEFRAG)
		ENTRY4("map07special", &MIType_SetFlag, &ref.flags, LEVEL_MAP07SPECIAL)
		ENTRY4("baronspecial", &MIType_SetFlag, &ref.flags, LEVEL_BRUISERSPECIAL)
		ENTRY4("cyberdemonspecial", &MIType_SetFlag, &ref.flags, LEVEL_CYBORGSPECIAL)
		ENTRY4("spidermastermindspecial", &MIType_SetFlag, &ref.flags,
		       LEVEL_SPIDERSPECIAL)
		ENTRY5("specialaction_exitlevel", &MIType_SCFlags, &ref.flags, 0,
		       ~LEVEL_SPECACTIONSMASK)
		ENTRY5("specialaction_opendoor", &MIType_SCFlags, &ref.flags, LEVEL_SPECOPENDOOR,
		       ~LEVEL_SPECACTIONSMASK)
		ENTRY5("specialaction_lowerfloor", &MIType_SCFlags, &ref.flags,
		       LEVEL_SPECLOWERFLOOR, ~LEVEL_SPECACTIONSMASK)
		ENTRY1("lightning")
		ENTRY3("fadetable", &MIType_LumpName, &ref.fadetable)
		ENTRY4("evenlighting", &MIType_SetFlag, &ref.flags, LEVEL_EVENLIGHTING)
		ENTRY4("noautosequences", &MIType_SetFlag, &ref.flags, LEVEL_SNDSEQTOTALCTRL)
		ENTRY4("forcenoskystretch", &MIType_SetFlag, &ref.flags, LEVEL_FORCENOSKYSTRETCH)
		ENTRY5("allowfreelook", &MIType_SCFlags, &ref.flags, LEVEL_FREELOOK_YES,
		       ~LEVEL_FREELOOK_NO)
		ENTRY5("nofreelook", &MIType_SCFlags, &ref.flags, LEVEL_FREELOOK_NO,
		       ~LEVEL_FREELOOK_YES)
		ENTRY5("allowjump", &MIType_SCFlags, &ref.flags, LEVEL_JUMP_YES, ~LEVEL_JUMP_NO)
		ENTRY5("nojump", &MIType_SCFlags, &ref.flags, LEVEL_JUMP_NO, ~LEVEL_JUMP_YES)
		ENTRY2("cdtrack", &MIType_EatNext)
		ENTRY2("cd_start_track", &MIType_EatNext)
		ENTRY2("cd_end1_track", &MIType_EatNext)
		ENTRY2("cd_end2_track", &MIType_EatNext)
		ENTRY2("cd_end3_track", &MIType_EatNext)
		ENTRY2("cd_intermission_track", &MIType_EatNext)
		ENTRY2("cd_title_track", &MIType_EatNext)
		ENTRY2("warptrans", &MIType_EatNext)
		ENTRY3("gravity", &MIType_Float, &ref.gravity)
		ENTRY3("aircontrol", &MIType_Float, &ref.aircontrol)
		ENTRY4("islobby", &MIType_SetFlag, &ref.flags, LEVEL_LOBBYSPECIAL)
		ENTRY4("lobby", &MIType_SetFlag, &ref.flags, LEVEL_LOBBYSPECIAL)
		ENTRY1("nocrouch")
		ENTRY2("intermusic", &MIType_EatNext)
		ENTRY3("par", &MIType_Int, &ref.partime)
		ENTRY2("sucktime", &MIType_EatNext)
		ENTRY3("enterpic", &MIType_InterLumpName,
		       &ref.enterpic) // todo: add intermission script support
		ENTRY3("exitpic", &MIType_InterLumpName,
		       &ref.exitpic) // todo: add intermission script support
		ENTRY2("interpic", &MIType_EatNext)
		ENTRY2("translator", &MIType_EatNext)
		ENTRY2("compat_shorttex", &MIType_EatNext)
		ENTRY2("compat_limitpain", &MIType_EatNext)
		ENTRY4("compat_dropoff", &MIType_SetFlag, &ref.flags, LEVEL_COMPAT_DROPOFF)
		ENTRY2("compat_trace", &MIType_EatNext)
		ENTRY2("compat_boomscroll", &MIType_EatNext)
		ENTRY2("compat_sectorsounds", &MIType_EatNext)
		ENTRY4("compat_nopassover", &MIType_SetFlag, &ref.flags, LEVEL_COMPAT_NOPASSOVER)
	}
};

// cluster_info_t
template <>
struct MapInfoDataSetter<cluster_info_t>
{
	MapInfoDataContainer mapInfoDataContainer;

	MapInfoDataSetter(cluster_info_t& ref)
	{
		mapInfoDataContainer.reserve(7);

		ENTRY3("entertext", &MIType_ClusterString, &ref.entertext)
		ENTRY3("exittext", &MIType_ClusterString, &ref.exittext)
		ENTRY4("exittextislump", &MIType_SetFlag, &ref.flags, CLUSTER_EXITTEXTISLUMP)
		ENTRY3("music", &MIType_MusicLumpName, &ref.messagemusic)
		ENTRY3("flat", &MIType_$LumpName, &ref.finaleflat)
		ENTRY4("hub", &MIType_SetFlag, &ref.flags, CLUSTER_HUB)
		ENTRY3("pic", &MIType_$LumpName, &ref.finalepic)
	}
};

// gameinfo_t
template <>
struct MapInfoDataSetter<gameinfo_t>
{
	MapInfoDataContainer mapInfoDataContainer;

	MapInfoDataSetter()
	{
		mapInfoDataContainer.reserve(7);

		ENTRY3("advisorytime", &MIType_Float, &gameinfo.advisoryTime)
		// ENTRY3("chatsound",			)
		ENTRY3("pagetime", &MIType_Float, &gameinfo.pageTime)
		ENTRY3("finaleflat", &MIType_LumpName, &gameinfo.finaleFlat)
		ENTRY3("finalemusic", &MIType_$LumpName, &gameinfo.finaleMusic)
		ENTRY3("titlemusic", &MIType_$LumpName, &gameinfo.titleMusic)
		ENTRY3("titlepage", &MIType_LumpName, &gameinfo.titlePage)
		ENTRY3("titletime", &MIType_Float, &gameinfo.titleTime)
	}
};

//
// Parse a MAPINFO block
//
// NULL pointers can be passed if the block is unimplemented.  However, if
// the block you want to stub out is compatible with old MAPINFO, you need
// to parse the block anyway, even if you throw away the values.  This is
// done by passing in a strings pointer, and leaving the others NULL.
//
template <typename T>
void ParseMapInfoLower(OScanner& os, MapInfoDataSetter<T>& mapInfoDataSetter)
{
	// 0 if old mapinfo, positive number if new MAPINFO, the exact
	// number represents current brace depth.
	int newMapInfoStack = 0;

	while (os.scan())
	{
		if (os.compareToken("{"))
		{
			// Detected new-style MAPINFO
			newMapInfoStack++;
			continue;
		}
		if (os.compareToken("}"))
		{
			newMapInfoStack--;
			if (newMapInfoStack <= 0)
			{
				// MAPINFO block is done
				break;
			}
		}

		if (newMapInfoStack <= 0 && ContainsMapInfoTopLevel(os) &&
		    // "cluster" is a valid map block type and is also
		    // a valid top-level type.
		    !UpperCompareToken(os, "cluster"))
		{
			// Old-style MAPINFO is done
			os.unScan();
			break;
		}

		MapInfoDataContainer& mapInfoDataContainer =
		    mapInfoDataSetter.mapInfoDataContainer;

		// find the matching string and use its corresponding function
		MapInfoDataContainer::iterator it = mapInfoDataContainer.begin();
		for (; it != mapInfoDataContainer.end(); ++it)
		{
			if (UpperCompareToken(os, it->name))
			{
				if (it->fn)
				{
					it->fn(os, newMapInfoStack > 0, it->data, it->flags, it->flags2);
				}

				// [AM] Some tokens are no-ops, we want to break out either way.
				break;
			}
		}

		if (it == mapInfoDataContainer.end())
		{
			if (newMapInfoStack <= 0)
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
}

// todo: parse episode info like the others? (but how?)
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
			ParseMapInfoHelper<std::string>(os, new_mapinfo);

			if (picisgfx == false)
			{
				pic = os.getToken();
			}
		}
		else if (UpperCompareToken(os, "lookup"))
		{
			ParseMapInfoHelper<std::string>(os, new_mapinfo);

			// Not implemented
		}
		else if (UpperCompareToken(os, "picname"))
		{
			ParseMapInfoHelper<std::string>(os, new_mapinfo);

			pic = os.getToken();
			picisgfx = true;
		}
		else if (UpperCompareToken(os, "key"))
		{
			ParseMapInfoHelper<std::string>(os, new_mapinfo);

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

void ParseMapInfoLump(int lump, const char* lumpname)
{
	LevelInfos& levels = getLevelInfos();
	ClusterInfos& clusters = getClusterInfos();

	level_pwad_info_t defaultinfo;
	SetLevelDefaults(defaultinfo);

	const char* buffer = static_cast<char*>(W_CacheLumpNum(lump, PU_STATIC));

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
			SetLevelDefaults(defaultinfo);

			MapInfoDataSetter<level_pwad_info_t> defaultsetter(defaultinfo);
			ParseMapInfoLower<level_pwad_info_t>(os, defaultsetter);
		}
		else if (UpperCompareToken(os, "map"))
		{
			uint32_t& levelflags = defaultinfo.flags;
			MustGetString(os);

			char map_name[9];
			strncpy(map_name, os.getToken().c_str(), 8);

			if (IsNum(map_name))
			{
				// MAPNAME is a number, assume a Hexen wad
				const int map = std::atoi(map_name);

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
					std::string err;
					StrFormat(err, "Unknown lookup string \"%s\".",
					          os.getToken().c_str());
					os.error(err.c_str());
				}
				info.level_name = strdup(s.c_str());
			}
			else
			{
				info.level_name = strdup(os.getToken().c_str());
			}

			MapInfoDataSetter<level_pwad_info_t> setter(info);
			ParseMapInfoLower<level_pwad_info_t>(os, setter);

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

			MapInfoDataSetter<cluster_info_t> setter(info);
			ParseMapInfoLower<cluster_info_t>(os, setter);
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

			MapInfoDataSetter<void> setter;
			ParseMapInfoLower<void>(os, setter);
		}
		else if (UpperCompareToken(os, "clearskills"))
		{
			// Not implemented
		}
		else if (UpperCompareToken(os, "gameinfo"))
		{
			MapInfoDataSetter<gameinfo_t> setter;
			ParseMapInfoLower<gameinfo_t>(os, setter);
		}
		else if (UpperCompareToken(os, "intermission"))
		{
			// Not implemented
			MustGetString(os); // Name

			MapInfoDataSetter<void> setter;
			ParseMapInfoLower<void>(os, setter);
		}
		else if (UpperCompareToken(os, "automap"))
		{
			// Not implemented
			MapInfoDataSetter<void> setter;
			ParseMapInfoLower<void>(os, setter);
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