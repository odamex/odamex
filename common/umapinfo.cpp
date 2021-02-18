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
#include "scanner.h"

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

static char *ParseMultiString(Scanner &scanner, int error)
{
	char *build = NULL;
	
	if (scanner.CheckToken(TK_Identifier))
	{
		if (!stricmp(scanner.string, "clear"))
		{
			return strdup("-");	// this was explicitly deleted to override the default.
		}

		scanner.ErrorF("Either 'clear' or string constant expected");
	}
	
	do
	{
		scanner.MustGetToken(TK_StringConst);
		if (build == NULL)
			build = strdup(scanner.string);
		else
		{
			size_t newlen = strlen(build) + strlen(scanner.string) + 2; // strlen for both the existing text and the new line, plus room for one \n and one \0
			build = (char*)realloc(build, newlen); // Prepare the destination memory for the below strcats
			strcat(build, "\n"); // Replace the existing text's \0 terminator with a \n
			strcat(build, scanner.string); // Concatenate the new line onto the existing text
		}
	} while (scanner.CheckToken(','));
	return build;
}

// -----------------------------------------------
//
// Parses a lump name. The buffer must be at least 9 characters.
//
// -----------------------------------------------

namespace
{
	char* M_Strupr(char* str)
	{
		char* p;
		for (p = str; *p; p++)
			*p = toupper(*p);
		return str;
	}
}

static int ParseLumpName(Scanner &scanner, char *buffer)
{
	scanner.MustGetToken(TK_StringConst);
	if (strlen(scanner.string) > 8)
	{
		scanner.ErrorF("String too long. Maximum size is 8 characters.");
		return 0;
	}
	strncpy(buffer, scanner.string, 8);
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

static int ParseStandardProperty(Scanner &scanner, level_pwad_info_t *mape)
{
	// find the next line with content.
	// this line is no property.

	scanner.MustGetToken(TK_Identifier);
	char *pname = strdup(scanner.string);
	scanner.MustGetToken('=');

	if (!stricmp(pname, "levelname"))
	{
		scanner.MustGetToken(TK_StringConst);
		mape->level_name = strdup(scanner.string);
	}
	else if (!stricmp(pname, "next"))
	{
		ParseLumpName(scanner, mape->nextmap);
		if (!G_ValidateMapName(mape->nextmap, NULL, NULL))
		{
			scanner.ErrorF("Invalid map name %s.", mape->nextmap);
			return 0;
		}
	}
	else if (!stricmp(pname, "nextsecret"))
	{
		ParseLumpName(scanner, mape->secretmap);
		if (!G_ValidateMapName(mape->secretmap, NULL, NULL))
		{
			scanner.ErrorF("Invalid map name %s", mape->nextmap);
			return 0;
		}
	}
	else if (!stricmp(pname, "levelpic"))
	{
		ParseLumpName(scanner, mape->pname);
	}
	else if (!stricmp(pname, "skytexture"))
	{
		ParseLumpName(scanner, mape->skypic);
	}
	else if (!stricmp(pname, "music"))
	{
		ParseLumpName(scanner, mape->music);
	}
	else if (!stricmp(pname, "endpic"))
	{
		ParseLumpName(scanner, mape->endpic);
		strncpy(mape->nextmap, "EndGame1", 8);
		mape->nextmap[8] = '\0';
	}
	else if (!stricmp(pname, "endcast"))
	{
		scanner.MustGetToken(TK_BoolConst);
		if (scanner.boolean)
			strncpy(mape->nextmap, "EndGameC", 8);
		else
			strcpy(mape->endpic, "\0");
	}
	else if (!stricmp(pname, "endbunny"))
	{
		scanner.MustGetToken(TK_BoolConst);
		if (scanner.boolean)
			strncpy(mape->nextmap, "EndGame3", 8);
		else
			strcpy(mape->endpic, "\0");
	}
	else if (!stricmp(pname, "endgame"))
	{
		scanner.MustGetToken(TK_BoolConst);
		if (scanner.boolean)
		{
			if (gamemission == doom)
			{
				if (mape->mapname[1] == '1')
				{
					strncpy(mape->nextmap, "EndGame1", 8);
				}
				else if (mape->mapname[1] == '2')
				{
					strncpy(mape->nextmap, "EndGame2", 8);
				}
				else if (mape->mapname[1] == '3')
				{
					strncpy(mape->nextmap, "EndGame3", 8);
				}
				else
				{
					strncpy(mape->nextmap, "EndGame4", 8);
				}
			}
			else // Doom 2 cast call
			{
				strncpy(mape->nextmap, "EndGameC", 8);
			}
			strcpy(mape->endpic, "\0");
		}
		else
		{
			strcpy(mape->endpic, "\0");
		}
	}
	else if (!stricmp(pname, "exitpic"))
	{
		ParseLumpName(scanner, mape->exitpic);
	}
	else if (!stricmp(pname, "enterpic"))
	{
		ParseLumpName(scanner, mape->enterpic);
	}
	else if (!stricmp(pname, "nointermission"))
	{
		scanner.MustGetToken(TK_BoolConst);
		if (scanner.boolean)
		{
			mape->flags |= LEVEL_NOINTERMISSION;
		}
	}
	else if (!stricmp(pname, "partime"))
	{
		scanner.MustGetInteger();
		mape->partime = TICRATE * scanner.number;
	}
#if 0
	else if (!stricmp(pname, "intertext"))
	{
		char *lname = ParseMultiString(scanner, 1);
		if (!lname) return 0;
		if (mape->intertext != NULL) free(mape->intertext);
		mape->intertext = lname;
	}
	else if (!stricmp(pname, "intertextsecret"))
	{
		char *lname = ParseMultiString(scanner, 1);
		if (!lname) return 0;
		if (mape->intertextsecret != NULL) free(mape->intertextsecret);
		mape->intertextsecret = lname;
	}
	else if (!stricmp(pname, "interbackdrop"))
	{
		ParseLumpName(scanner, mape->interbackdrop);
	}
	else if (!stricmp(pname, "intermusic"))
	{
		ParseLumpName(scanner, mape->intermusic);
	}
#endif
	else if (!stricmp(pname, "episode"))
	{
		int epi;
		int num;
		
		char *lname = ParseMultiString(scanner, 1);
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

			G_ValidateMapName(scanner.string, &epi, &num);
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
		scanner.MustGetToken(TK_Identifier);
		
		if (!stricmp(scanner.string, "clear"))
		{
			// mark level free of boss actions
			mape->bossactions.clear();
			mape->bossactions_donothing = true;
		}
		else
		{
			int i;

			for (i = 0; ActorNames[i]; i++)
			{
				if (!stricmp(scanner.string, ActorNames[i]))
					break;
			}
			if (ActorNames[i] == NULL)
			{
				scanner.ErrorF("Unknown thing type %s", scanner.string);
				return 0;
			}

			scanner.MustGetToken(',');
			scanner.MustGetInteger();
			const int special = scanner.number;
			scanner.MustGetToken(',');
			scanner.MustGetInteger();
			const int tag = scanner.number;
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
			if (!scanner.CheckFloat())
				scanner.GetNextToken();
			if (scanner.token > TK_BoolConst) 
			{
				scanner.Error(TK_Identifier);
			}
			
		} while (scanner.CheckToken(','));
	}
	free(pname);

	MapNameToLevelNum(mape);
	
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

void ParseUMapInfo(int lump, const char* lumpname)
{
	LevelInfos& levels = getLevelInfos();
	ClusterInfos& clusters = getClusterInfos();

	level_pwad_info_t defaultinfo;
	SetLevelDefaults(&defaultinfo);
	
	const char *buffer = (char *)W_CacheLumpNum(lump, PU_STATIC);
	
	Scanner scanner((const char*)buffer, W_LumpLength(lump));
	Scanner::SetErrorCallback(I_Error);

	while (scanner.TokensLeft())
	{
		scanner.MustGetIdentifier("map");
		scanner.MustGetToken(TK_Identifier);
		if (!G_ValidateMapName(scanner.string, NULL, NULL))
		{
			// TODO: should display line number of error
			I_Error("Invalid map name %s", scanner.string);
		}

		// Find the level.
		level_pwad_info_t &info = (levels.findByName(scanner.string).exists()) ?
			levels.findByName(scanner.string) :
			levels.create();
		
		// Free the level name string before we pave over it.
		free(info.level_name);

		info = defaultinfo;
		uppercopy(info.mapname, scanner.string);

		scanner.MustGetToken('{');
		while (!scanner.CheckToken('}'))
		{
			ParseStandardProperty(scanner, &info);
		}

		// Set default level progression here to simplify the checks elsewhere. Doing this lets us skip all normal code for this if nothing has been defined.
#if 0
		if (info.endpic[0])
		{

			info.nextmap[0] = info.nextsecret[0] = 0;
			if (info.endpic[0] == '!') info.endpic[0] = 0;
		}
		else if (!info.nextmap[0] && !info.endpic[0])
		{
			if (!stricmp(info.mapname, "MAP30")) strcpy(info.endpic, "$CAST");
			else if (!stricmp(info.mapname, "E1M8"))  strcpy(info.endpic, gamemode == retail? "CREDIT" : "HELP2");
			else if (!stricmp(info.mapname, "E2M8"))  strcpy(info.endpic, "VICTORY");
			else if (!stricmp(info.mapname, "E3M8"))  strcpy(info.endpic, "$BUNNY");
			else if (!stricmp(info.mapname, "E4M8"))  strcpy(info.endpic, "ENDPIC");
			else if (gamemission == chex && !stricmp(info.mapname, "E1M5"))  strcpy(info.endpic, "CREDIT");
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
#endif
	}
}
