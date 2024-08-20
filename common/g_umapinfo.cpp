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
//   Functions regarding reading and interpreting UMAPINFO lumps.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "g_episode.h"
#include "oscanner.h"
#include "w_wad.h"
#include "infomap.h"
#include "g_mapinfo.h" // G_MapNameToLevelNum

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

// used for munching the strings in UMAPINFO
std::string ParseMultiString(OScanner& os)
{
	os.scan();

	if (!os.isQuotedString())
	{
		if (os.compareTokenNoCase("clear"))
			return "-"; // this was explicitly deleted to override the default.

		os.error("Either 'clear' or quoted string expected");
	}
	os.unScan();

	std::string build;

	do
	{
		os.mustScan();

		build += os.getToken(); // Concatenate the new line onto the existing text
		build += '\n';          // Add newline

		os.scan();
	} while (os.compareToken(","));
	os.unScan();

	return build;
}

void ParseOLumpName(OScanner& os, OLumpName& buffer)
{
	os.mustScan(8);
	buffer = os.getToken();
}

void MustGetIdentifier(OScanner& os)
{
	os.mustScan();
	if (!os.isIdentifier())
	{
		os.error("Expected identifier (unexpected end of file).");
	}
}

int ParseStandardUmapInfoProperty(OScanner& os, level_pwad_info_t* mape)
{
	// find the next line with content.
	// this line is no property.

	if (!os.isIdentifier())
	{
		os.error("Expected identifier, got \"%s\".", os.getToken().c_str());
	}
	std::string pname = os.getToken();
	os.mustScan();
	os.assertTokenNoCaseIs("=");

	if (!stricmp(pname.c_str(), "levelname"))
	{
		os.mustScan();
		mape->level_name = os.getToken();
		mape->pname.clear();
	}
	else if (!stricmp(pname.c_str(), "label"))
	{
		os.mustScan();
		if (!os.isQuotedString() && os.compareTokenNoCase("clear"))
		{
			mape->clearlabel = true;
		}
		else
		{
			mape->label = os.getToken();
		}
	}
	else if (!stricmp(pname.c_str(), "author"))
	{
		os.mustScan();
		mape->author = os.getToken();
	}
	else if (!stricmp(pname.c_str(), "next"))
	{
		ParseOLumpName(os, mape->nextmap);
		if (!ValidateMapName(mape->nextmap.c_str()))
		{
			os.error("Invalid map name %s.", mape->nextmap.c_str());
			return 0;
		}
	}
	else if (!stricmp(pname.c_str(), "nextsecret"))
	{
		ParseOLumpName(os, mape->secretmap);
		if (!ValidateMapName(mape->secretmap.c_str()))
		{
			os.error("Invalid map name %s", mape->nextmap.c_str());
			return 0;
		}
	}
	else if (!stricmp(pname.c_str(), "levelpic"))
	{
		ParseOLumpName(os, mape->pname);
	}
	else if (!stricmp(pname.c_str(), "skytexture"))
	{
		ParseOLumpName(os, mape->skypic);
	}
	else if (!stricmp(pname.c_str(), "music"))
	{
		os.mustScan(8);
		const std::string musicname = os.getToken();
		if (W_CheckNumForName(musicname.c_str()) != -1)
		{
			mape->music = musicname;
		}
	}
	else if (!stricmp(pname.c_str(), "endpic"))
	{
		ParseOLumpName(os, mape->endpic);
		mape->nextmap = "EndGame1";
	}
	else if (!stricmp(pname.c_str(), "endcast"))
	{
		os.mustScanBool();
		if (os.getTokenBool())
			mape->nextmap = "EndGameC";
		else
			mape->endpic.clear();
	}
	else if (!stricmp(pname.c_str(), "endbunny"))
	{
		os.mustScanBool();
		if (os.getTokenBool())
			mape->nextmap = "EndGame3";
		else
			mape->endpic.clear();
	}
	else if (!stricmp(pname.c_str(), "endgame"))
	{
		os.mustScanBool();
		if (os.getTokenBool())
		{
			mape->endpic = "!";
		}
		else
		{
			mape->endpic.clear();
		}
	}
	else if (!stricmp(pname.c_str(), "exitpic"))
	{
		ParseOLumpName(os, mape->exitpic);
	}
	else if (!stricmp(pname.c_str(), "enterpic"))
	{
		ParseOLumpName(os, mape->enterpic);
	}
	else if (!stricmp(pname.c_str(), "nointermission"))
	{
		os.mustScanBool();
		if (os.getTokenBool())
		{
			mape->flags |= LEVEL_NOINTERMISSION;
		}
	}
	else if (!stricmp(pname.c_str(), "partime"))
	{
		os.mustScanInt();
		mape->partime = TICRATE * os.getTokenInt();
	}
	else if (!stricmp(pname.c_str(), "intertext"))
	{
		const std::string lname = ParseMultiString(os);
		if (lname.empty())
			return 0;
		mape->intertext = lname;
	}
	else if (!stricmp(pname.c_str(), "intertextsecret"))
	{
		const std::string lname = ParseMultiString(os);
		if (lname.empty())
			return 0;
		mape->intertextsecret = lname;
	}
	else if (!stricmp(pname.c_str(), "interbackdrop"))
	{
		ParseOLumpName(os, mape->interbackdrop);
	}
	else if (!stricmp(pname.c_str(), "intermusic"))
	{
		os.mustScan(8);
		const std::string musicname = os.getToken();

		if (W_CheckNumForName(musicname.c_str()) != -1)
			mape->intermusic = musicname;
	}
	else if (!stricmp(pname.c_str(), "episode"))
	{
		if (!episodes_modified && gamemode == commercial)
		{
			episodenum = 0;
			episodes_modified = true;
		}

		const std::string lname = ParseMultiString(os);
		if (lname.empty())
			return 0;

		if (lname == "-") // means "clear"
		{
			episodenum = 0;
		}
		else
		{
			const StringTokens tokens = TokenizeString(lname, "\n");

			if (episodenum >= 8)
				return 0;

			EpisodeMaps[episodenum] = mape->mapname;
			EpisodeInfos[episodenum].pic_name = tokens[0];
			EpisodeInfos[episodenum].menu_name = tokens[1];
			EpisodeInfos[episodenum].fulltext = false;
			EpisodeInfos[episodenum].noskillmenu = false;
			EpisodeInfos[episodenum].key = (tokens.size() > 2) ? tokens[2][0] : 0;
			++episodenum;
		}
	}
	else if (!stricmp(pname.c_str(), "bossaction"))
	{
		MustGetIdentifier(os);

		if (!stricmp(os.getToken().c_str(), "clear"))
		{
			// mark level free of boss actions
			mape->bossactions.clear();
		}
		else
		{
			const std::string actor_name = os.getToken();
			const mobjtype_t i = P_NameToMobj(actor_name);
			if (i == MT_NULL)
			{
				os.error("Unknown thing type %s", os.getToken().c_str());
				return 0;
			}

			// skip comma token
			os.mustScan();
			os.assertTokenNoCaseIs(",");
			os.mustScanInt();
			const int special = os.getTokenInt();
			os.mustScan();
			os.assertTokenNoCaseIs(",");
			os.mustScanInt();
			const int tag = os.getTokenInt();
			// allow no 0-tag specials here, unless a level exit.
			if (tag != 0 || special == 11 || special == 51 || special == 52 ||
			    special == 124)
			{
				bossaction_t new_bossaction;

				new_bossaction.special = static_cast<short>(special);
				new_bossaction.tag = static_cast<short>(tag);

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
	os.scan();

	return 1;
}

void ParseUMapInfoLump(int lump, const char* lumpname)
{
	LevelInfos& levels = getLevelInfos();

	const char* buffer = static_cast<char*>(W_CacheLumpNum(lump, PU_STATIC));

	const OScannerConfig config = {
	    lumpname, // lumpName
	    false,    // semiComments
	    true,     // cComments
	};
	OScanner os = OScanner::openBuffer(config, buffer, buffer + W_LumpLength(lump));

	while (os.scan())
	{
		if (!os.compareTokenNoCase("map"))
		{
			os.error("Expected map definition, got %s", os.getToken().c_str());
		}

		os.mustScan(8);
		const OLumpName mapname = os.getToken();

		if (!ValidateMapName(mapname))
		{
			os.error("Invalid map name %s", mapname.c_str());
		}

		// Find the level.
		level_pwad_info_t& info = (levels.findByName(mapname).exists())
			? levels.findByName(mapname)
			: levels.create();

		// for maps above 32, if no sky is defined, it will show texture 0 (aastinky)
		// so instead, lets just try to give it the first defined sky in the level set.
		if (levels.size() > 0)
		{
			level_pwad_info_t& def = levels.at(0);
			info.skypic = def.skypic;
		}

		info.mapname = mapname;

		G_MapNameToLevelNum(info);

		os.mustScan();
		os.assertTokenNoCaseIs("{");

		os.scan();
		while (!os.compareToken("}"))
		{
			ParseStandardUmapInfoProperty(os, &info);
		}
		// if any episode title patches are missing, fall back on text name
		bool missingeppatch = false;
		for (int i = 0; i < MAX_EPISODES; i++) {
			if (EpisodeInfos[i].pic_name.empty()) {
				missingeppatch = true;
				break;
			}
		}
		for (int i = 0; i < MAX_EPISODES; i++) {
			EpisodeInfos[i].fulltext = missingeppatch;
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
