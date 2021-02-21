//-----------------------------------------------------------------------------
//
// Copyright (C) 2017 Christoph Oelckers
// Copyright (C) 2006-2020 by The Odamex Team.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "cmdlib.h"
#include "umapinfo.h"
#include "oscanner.h"

#include "m_misc.h"
#include "g_game.h"
#include "doomdef.h"
#include "doomstat.h"
#include "g_episode.h"
#include "i_system.h"
#include "p_setup.h"
#include "w_wad.h"
#include "z_zone.h"


//==========================================================================
//
// The Doom actors in their original order
// Names are the same as in ZDoom.
//
//==========================================================================

static const char * const ActorNames[] =
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


// -----------------------------------------------
//
// Parses a set of string and concatenates them
//
// -----------------------------------------------


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

	// return token as bool
	bool GetTokenAsBool(OScanner &os)
	{
		return os.compareToken("true");
	}

	void MustGetString(OScanner& os)
	{
		if (!os.scan())
		{
			I_Error("Missing string (unexpected end of file).");
		}
	}

	void MustGetStringName(OScanner& os, const char* name)
	{
		MustGetString(os);
		if (os.compareToken(name) == false)
		{
			// TODO: was previously SC_ScriptError, less information is printed now
			I_Error("Expected '%s', got '%s'.", name, os.getToken());
		}
	}

	void MustGetNumber(OScanner& os)
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

	void MustGetBool(OScanner& os)
	{
		MustGetString(os);
		if (!os.compareToken("true") && !os.compareToken("false"))
		{
			I_Error("Missing boolean (unexpected end of file).");
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
		char* p;
		for (p = str; *p; p++)
			*p = toupper(*p);
		return str;
	}

	bool UpperCompareToken(OScanner &os, const char *str)
	{
		char p[64];
		char q[64];

		uppercopy(p, os.getToken().c_str());
		uppercopy(q, str);

		bool result = (strcmp(p, q) != 0) ? false : true;

		return result;
	}
}

// TODO: remove everything above

// used for munching the strings in UMAPINFO
static char *ParseMultiString(OScanner &os, int error)
{
	char *build = NULL;

	os.scan();
	// TODO: properly identify identifiers so clear can be separated from regular strings
	//if (IsIdentifier(os))
	{
		if (os.compareToken("clear"))
		{
			return strdup("-");	// this was explicitly deleted to override the default.
		}

		//I_Error("Either 'clear' or string constant expected");
	}
	os.unScan();
	
	do
	{
		MustGetString(os);
		std::string test1 = os.getToken();
		
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
		std::string test2 = os.getToken();
	} while (os.compareToken(","));
	os.unScan();
	
	return build;
}

// -----------------------------------------------
//
// Parses a lump name. The buffer must be at least 9 characters.
//
// -----------------------------------------------

static int ParseLumpName(OScanner& os, char *buffer)
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

// -----------------------------------------------
//
// Parses a standard property that is already known
// These do not get stored in the property list
// but in dedicated struct member variables.
//
// -----------------------------------------------

namespace
{
	int G_ValidateMapName(const char *mapname, int *pEpi, int *pMap)
	{
		// Check if the given map name can be expressed as a gameepisode/gamemap pair and be reconstructed from it.
		char lumpname[9], mapuname[9];
		int epi = -1, map = -1;

		if (strlen(mapname) > 8) return 0;
		strncpy(mapuname, mapname, 8);
		mapuname[8] = 0;
		M_Strupr(mapuname);

		if (gamemode != commercial)
		{
			if (sscanf(mapuname, "E%dM%d", &epi, &map) != 2) return 0;
			snprintf(lumpname, 9, "E%dM%d", epi, map);
		}
		else
		{
			if (sscanf(mapuname, "MAP%d", &map) != 1) return 0;
			snprintf(lumpname, 9, "MAP%02d", map);
			epi = 1;
		}
		if (pEpi) *pEpi = epi;
		if (pMap) *pMap = map;
		return !strcmp(mapuname, lumpname);
	}
}

// TODO: Remove this function and replace it with g_level equivalent when merging files
static void MapNameToLevelNum(level_pwad_info_t *info)
{
	if (info->mapname[0] == 'E' && info->mapname[2] == 'M')
	{
		// Convert a char into its equivalent integer.
		int e = info->mapname[1] - '0';
		int m = info->mapname[3] - '0';
		if (e >= 0 && e <= 9 && m >= 0 && m <= 9)
		{
			// Copypasted from the ZDoom wiki.
			info->levelnum = (e - 1) * 10 + m;
		}
	}
	else if (strnicmp(info->mapname, "MAP", 3) == 0)
	{
		// Try and turn the trailing digits after the "MAP" into a
		// level number.
		int mapnum = atoi(info->mapname + 3);
		if (mapnum >= 0 && mapnum <= 99)
		{
			info->levelnum = mapnum;
		}
	}
}

static int ParseStandardProperty(OScanner& os, level_pwad_info_t *mape)
{
	// find the next line with content.
	// this line is no property.

	if (!IsIdentifier(os))
	{
		I_Error("Expected identifier");
	}
	char *pname = strdup(os.getToken().c_str());
	MustGetStringName(os, "=");

	if (!stricmp(pname, "levelname"))
	{
		MustGetString(os);
		mape->level_name = strdup(os.getToken().c_str());
	}
	else if (!stricmp(pname, "next"))
	{
		ParseLumpName(os, mape->nextmap);
		if (!G_ValidateMapName(mape->nextmap, NULL, NULL))
		{
			I_Error("Invalid map name %s.", mape->nextmap);
			return 0;
		}
	}
	else if (!stricmp(pname, "nextsecret"))
	{
		ParseLumpName(os, mape->secretmap);
		if (!G_ValidateMapName(mape->secretmap, NULL, NULL))
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
		ParseLumpName(os, mape->endpic);
		strncpy(mape->nextmap, "EndGame1", 8);
		mape->nextmap[8] = '\0';
	}
	else if (!stricmp(pname, "endcast"))
	{
		MustGetBool(os);
		if (GetTokenAsBool(os))
			strncpy(mape->nextmap, "EndGameC", 8);
		else
			strcpy(mape->endpic, "\0");
	}
	else if (!stricmp(pname, "endbunny"))
	{
		MustGetBool(os);
		if (GetTokenAsBool(os))
			strncpy(mape->nextmap, "EndGame3", 8);
		else
			strcpy(mape->endpic, "\0");
	}
	else if (!stricmp(pname, "endgame"))
	{
		MustGetBool(os);
		if (GetTokenAsBool(os))
		{
			strcpy(mape->endpic, "!");
		}
		else
		{
			strcpy(mape->endpic, "\0");
		}
	}
	else if (!stricmp(pname, "exitpic"))
	{
		ParseLumpName(os, mape->exitpic);
	}
	else if (!stricmp(pname, "enterpic"))
	{
		ParseLumpName(os, mape->enterpic);
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
		MustGetNumber(os);
		mape->partime = TICRATE * GetTokenAsInt(os);
	}
	else if (!stricmp(pname, "intertext"))
	{
		char *lname = ParseMultiString(os, 1);
		if (!lname)
			return 0;
		mape->intertext = lname;
	}
	else if (!stricmp(pname, "intertextsecret"))
	{
		char *lname = ParseMultiString(os, 1);
		if (!lname)
			return 0;
		mape->intertextsecret = lname;
	}
	else if (!stricmp(pname, "interbackdrop"))
	{
		ParseLumpName(os, mape->interbackdrop);
	}
	else if (!stricmp(pname, "intermusic"))
	{
		ParseLumpName(os, mape->intermusic);
	}
	else if (!stricmp(pname, "episode"))
	{
		if (!episodes_modified && gamemode == commercial)
		{
			episodenum = 0;
			episodes_modified = true;
		}
		
		char *lname = ParseMultiString(os, 1);
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

			strncpy(EpisodeMaps[episodenum], mape->mapname, 8);
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
			MustGetNumber(os);
			const int special = GetTokenAsInt(os);
			//MustGetStringName(os, ",");
			MustGetNumber(os);
			const int tag = GetTokenAsInt(os);
			// allow no 0-tag specials here, unless a level exit.
			if (tag != 0 || special == 11 || special == 51 || special == 52 || special == 124)
			{
				if (mape->bossactions_donothing == true)
					mape->bossactions_donothing = false;

				BossAction new_bossaction;

				new_bossaction.type = i;
				new_bossaction.special = special;
				new_bossaction.tag = tag;

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

// -----------------------------------------------
//
// Parses a complete UMAPINFO lump
//
// -----------------------------------------------

// TODO: remove when merging
static void SetLevelDefaults(level_pwad_info_t *levelinfo)
{
	memset(levelinfo, 0, sizeof(*levelinfo));
	levelinfo->snapshot = NULL;
	levelinfo->outsidefog_color[0] = 255;
	levelinfo->outsidefog_color[1] = 0;
	levelinfo->outsidefog_color[2] = 0;
	levelinfo->outsidefog_color[3] = 0;
	strncpy(levelinfo->fadetable, "COLORMAP", 8);
}

void ParseUMapInfoLump(int lump, const char* lumpname)
{
	LevelInfos& levels = getLevelInfos();

	level_pwad_info_t defaultinfo;
	SetLevelDefaults(&defaultinfo);
	
	const char *buffer = (char *)W_CacheLumpNum(lump, PU_STATIC);

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
		if (!G_ValidateMapName(os.getToken().c_str(), NULL, NULL))
		{
			// TODO: should display line number of error
			I_Error("Invalid map name %s", os.getToken().c_str());
		}

		// Find the level.
		level_pwad_info_t &info = (levels.findByName(os.getToken()).exists()) ?
			levels.findByName(os.getToken()) :
			levels.create();
		
		// Free the level name string before we pave over it.
		free(info.level_name);

		info = defaultinfo;
		uppercopy(info.mapname, os.getToken().c_str());

		MapNameToLevelNum(&info);

		MustGetStringName(os, "{");
		os.scan();
		while (!os.compareToken("}"))
		{
			ParseStandardProperty(os, &info);
		}

		// Set default level progression here to simplify the checks elsewhere. Doing this lets us skip all normal code for this if nothing has been defined.
		if (!info.nextmap[0] && !info.endpic[0])
		{
			if (!stricmp(info.mapname, "MAP30"))
			{
				strcpy(info.endpic, "$CAST");
				strncpy(info.nextmap, "EndGameC", 8);
			}
			else if (!stricmp(info.mapname, "E1M8"))
			{
				strcpy(info.endpic, gamemode == retail ? "CREDIT" : "HELP2");
				strncpy(info.nextmap, "EndGameC", 8);
			}
			else if (!stricmp(info.mapname, "E2M8"))
			{
				strcpy(info.endpic, "VICTORY");
				strncpy(info.nextmap, "EndGame2", 8);
			}
			else if (!stricmp(info.mapname, "E3M8"))
			{
				strcpy(info.endpic, "$BUNNY");
				strncpy(info.nextmap, "EndGame3", 8);
			}
			else if (!stricmp(info.mapname, "E4M8"))
			{
				strcpy(info.endpic, "ENDPIC");
				strncpy(info.nextmap, "EndGame4", 8);
			}
			else if (gamemission == chex && !stricmp(info.mapname, "E1M5"))
			{
				strcpy(info.endpic, "CREDIT");
				strncpy(info.nextmap, "EndGame1", 8);
			}
			else
			{
				int ep, map;
				G_ValidateMapName(info.mapname, &ep, &map);
				map++;
				if (gamemode == commercial)
					sprintf(info.nextmap, "MAP%02d", map);
				else
					sprintf(info.nextmap, "E%dM%d", ep, map);
			}
		}
	}
}
