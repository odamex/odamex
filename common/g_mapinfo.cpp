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


#include "odamex.h"

#include "g_episode.h"
#include "gi.h"
#include "gstrings.h"
#include "g_skill.h"
#include "i_system.h"
#include "oscanner.h"
#include "p_setup.h"
#include "r_sky.h"
#include "v_video.h"
#include "w_wad.h"
#include "infomap.h"
#include "p_mapformat.h"

/// Globals
BOOL HexenHack;

namespace
{

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

//////////////////////////////////////////////////////////////////////
/// MustGet

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
	os.mustScanInt();
}

// ensure token is float
template <>
void MustGet<float>(OScanner& os)
{
	os.mustScanFloat();
}

// ensure token is bool
template <>
void MustGet<bool>(OScanner& os)
{
	os.mustScanBool();
}

// ensure token is std::string
template <>
void MustGet<std::string>(OScanner& os)
{
	os.mustScan();
}

// ensure token is OLumpName
template <>
void MustGet<OLumpName>(OScanner& os)
{
	os.mustScan();

	if (os.getToken().length() > 8)
	{
		os.error("Lump name \"%s\" too long. Maximum size is 8 characters.",
		        os.getToken().c_str());
	}
}

//////////////////////////////////////////////////////////////////////
/// Misc

bool IsIdentifier(const OScanner& os)
{
	// [A-Za-z_]+[A-Za-z0-9_]*

	if (os.getToken().empty())
		return false;

	const std::string token = os.getToken();
	for (std::string::const_iterator it = token.begin(); it != token.end(); ++it)
	{
		const char& ch = *it;
		if (ch == '_')
			continue;

		if (ch >= 'A' && ch <= 'Z')
			continue;

		if (ch >= 'a' && ch <= 'z')
			continue;

		if (it != token.begin() && ch >= '0' && ch <= '9')
			continue;

		return false;
	}

	return true;
}

void MustGetIdentifier(OScanner& os)
{
	os.mustScan();
	if (!IsIdentifier(os))
	{
		os.error("Expected identifier (unexpected end of file).");
	}
}

bool ContainsMapInfoTopLevel(const OScanner& os)
{
	return os.compareTokenNoCase("map") || os.compareTokenNoCase("defaultmap") ||
	       os.compareTokenNoCase("cluster") || os.compareTokenNoCase("clusterdef") ||
	       os.compareTokenNoCase("episode") || os.compareTokenNoCase("clearepisodes") ||
	       os.compareTokenNoCase("skill") || os.compareTokenNoCase("clearskills") ||
	       os.compareTokenNoCase("gameinfo") || os.compareTokenNoCase("intermission") ||
	       os.compareTokenNoCase("automap");
}

void MustGetStringName(OScanner& os, const char* name)
{
	os.mustScan();
	if (os.compareTokenNoCase(name) == false)
	{
		os.error("Expected '%s', got '%s'.", name, os.getToken().c_str());
	}
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
		build += '\n';			// Add newline
		
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
		os.error("Expected identifier, got \"%s\".", os.getToken().c_str());
	}
	const std::string pname = os.getToken();
	MustGetStringName(os, "=");

	if (!stricmp(pname.c_str(), "levelname"))
	{
		os.mustScan();
		mape->level_name = os.getToken();
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
		MustGet<OLumpName>(os);
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
		MustGet<OLumpName>(os);
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
			EpisodeInfos[episodenum].name = tokens[0];
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
			MustGetStringName(os, ",");
			os.mustScanInt();
			const int special = os.getTokenInt();
			MustGetStringName(os, ",");
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
		const int mapnum = std::atoi(info.mapname.c_str() + 3);
		if (mapnum >= 0 && mapnum <= 99)
		{
			info.levelnum = mapnum;
		}
	}
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

		MustGet<OLumpName>(os);
		const OLumpName mapname = os.getToken();

		if (!ValidateMapName(mapname))
		{
			os.error("Invalid map name %s", mapname.c_str());
		}

		// Find the level.
		level_pwad_info_t& info = (levels.findByName(mapname).exists())
		                              ? levels.findByName(mapname)
		                              : levels.create();

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
		MustGetStringName(os, "=");

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

// Literally does nothing
void MIType_DoNothing(OScanner& os, bool doEquals, void* data, unsigned int flags,
                    unsigned int flags2)
{
}

// Sets the inputted data as an int
void MIType_Int(OScanner& os, bool doEquals, void* data, unsigned int flags,
                unsigned int flags2)
{
	ParseMapInfoHelper<int>(os, doEquals);

	*static_cast<int*>(data) = os.getTokenInt();
}

// Sets the inputted data as a float
void MIType_Float(OScanner& os, bool doEquals, void* data, unsigned int flags,
                  unsigned int flags2)
{
	ParseMapInfoHelper<float>(os, doEquals);

	*static_cast<float*>(data) = os.getTokenFloat();
}

// Sets the inputted data as a bool (that is, if flags != 0, set to true; else false)
void MIType_Bool(OScanner& os, bool doEquals, void* data, unsigned int flags,
                  unsigned int flags2)
{
	*static_cast<bool*>(data) = flags;
}

// Sets the inputted data as a bool (that is, if flags != 0, set to true; else false)
void MIType_MustConfirm(OScanner& os, bool doEquals, void* data, unsigned int flags,
                 unsigned int flags2)
{
	SkillInfo& info = *static_cast<SkillInfo*>(data);
	info.must_confirm = true;

	if (doEquals)
	{
		os.scan();
		if (os.compareTokenNoCase("="))
		{
			info.must_confirm_text.clear();
			
			do
			{
				os.mustScan();
				info.must_confirm_text += os.getToken();
				info.must_confirm_text += "\n";
				os.scan();
			} while (os.compareToken(","));
			os.unScan();

			// Trim trailing newline.
			if (info.must_confirm_text.length() > 0)
			{
				info.must_confirm_text.resize(info.must_confirm_text.length() - 1);
			}
		}
		else
		{
			os.unScan();
		}
	}
	else
	{
		os.scan();
		info.must_confirm_text.clear();

		if (os.isQuotedString())
		{
			os.unScan();
			do
			{
				os.mustScan();
				info.must_confirm_text += os.getToken();
				info.must_confirm_text += "\n";
				os.scan();
			} while (os.compareToken(","));
			os.unScan();

			// Trim trailing newline.
			if (info.must_confirm_text.length() > 0)
			{
				info.must_confirm_text.resize(info.must_confirm_text.length() - 1);
			}
		}
		else
		{
			os.unScan();
		}
	}
}

// Sets the inputted data as a char
void MIType_Char(OScanner& os, bool doEquals, void* data, unsigned int flags,
                 unsigned int flags2)
{
	ParseMapInfoHelper<std::string>(os, doEquals);

	if (os.getToken().size() > 1)
		os.error("Expected single character string, got multi-character string");

	*static_cast<char*>(data) = os.getToken()[0];
}

// Sets the inputted data as a std::string
void MIType_String(OScanner& os, bool doEquals, void* data, unsigned int flags,
                 unsigned int flags2)
{
	ParseMapInfoHelper<std::string>(os, doEquals);

	*static_cast<std::string*>(data) = os.getToken();
}

// Sets the inputted data as a color
void MIType_Color(OScanner& os, bool doEquals, void* data, unsigned int flags,
                  unsigned int flags2)
{
	ParseMapInfoHelper<std::string>(os, doEquals);

	const argb_t color(V_GetColorFromString(os.getToken()));
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
	ParseMapInfoHelper<std::string>(os, doEquals);

	if (IsIdentifier(os))
	{
		if (os.compareTokenNoCase("EndPic"))
		{
			// todo
			if (doEquals)
				MustGetStringName(os, ",");

			os.mustScan();
		}
		else if (os.compareTokenNoCase("EndSequence"))
		{
			// todo
			if (doEquals)
				MustGetStringName(os, ",");

			os.mustScan();
		}
	}

	// If not identifier, check if it's a lumpname
	os.unScan();
	MustGet<OLumpName>(os);

	if (os.compareTokenNoCase("endgame"))
	{
		// endgame block
		MustGetStringName(os, "{");

		while (os.scan())
		{
			if (os.compareToken("}"))
			{
				break;
			}

			if (os.compareTokenNoCase("pic"))
			{
				ParseMapInfoHelper<OLumpName>(os, doEquals);

				// todo
			}
			else if (os.compareTokenNoCase("hscroll"))
			{
				ParseMapInfoHelper<std::string>(os, doEquals);

				// todo
			}
			else if (os.compareTokenNoCase("vscroll"))
			{
				ParseMapInfoHelper<std::string>(os, doEquals);

				// todo
			}
			else if (os.compareTokenNoCase("cast"))
			{
				*static_cast<OLumpName*>(data) = "EndGameC";
			}
			if (os.compareTokenNoCase("music"))
			{
				ParseMapInfoHelper<OLumpName>(os, doEquals);

				// todo
				os.scan();
				if (os.compareTokenNoCase(","))
				{
					os.mustScanFloat();
					// todo
				}
				else
				{
					os.unScan();
				}
			}
		}
	}
	else
	{
		char map_name[9];
		strncpy(map_name, os.getToken().c_str(), 8);

		if (IsNum(map_name))
		{
			const int map = std::atoi(map_name);
			sprintf(map_name, "MAP%02d", map);
		}
		else if (os.compareTokenNoCase("EndBunny"))
		{
			*static_cast<OLumpName*>(data) = "EndGame3";
			return;
		}

		*static_cast<OLumpName*>(data) = map_name;
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
	const std::string tok = os.getToken();
	if (!tok.empty() && tok.at(0) == '$')
	{
		os.mustScan();
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
		const OString& s = GStrings(StdStringToUpper(os.getToken()).c_str() + 1);
		if (s.empty())
		{
			os.error("Unknown lookup string \"%s\".", os.getToken().c_str());
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
	const std::string musicname = os.getToken();

	if (musicname[0] == '$')
	{
		// It is possible to pass a DeHackEd string
		// prefixed by a $.
		const OString& s = GStrings(StdStringToUpper(musicname.c_str() + 1));
		if (s.empty())
		{
			os.error("Unknown lookup string \"%s\".", os.getToken().c_str());
		}

		// Music lumps in the stringtable do not begin
		// with a D_, so we must add it.
		char lumpname[9];
		snprintf(lumpname, ARRAY_LENGTH(lumpname), "D_%s", s.c_str());
		if (W_CheckNumForName(lumpname) != -1)
		{
			*static_cast<OLumpName*>(data) = lumpname;
		}
	}
	else
	{
		if (W_CheckNumForName(musicname.c_str()) != -1)
		{
			*static_cast<OLumpName*>(data) = musicname;
		}
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
	os.scan();
	if (IsRealNum(os.getToken().c_str()))
	{
		/*if (HexenHack)
		{
		    *((fixed_t *)(info + handler->data2)) = sc_Number << 8;
		}
		 else
		{
		    *((fixed_t *)(info + handler->data2)) = (fixed_t)(sc_Float * 65536.0f);
		}*/
	}
	else
	{
		os.unScan();
	}
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

	*static_cast<int*>(data) = os.getTokenInt();
	if (HexenHack)
	{
		ClusterInfos& clusters = getClusterInfos();
		cluster_info_t& clusterH = clusters.findByCluster(os.getTokenInt());
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
		if (os.compareTokenNoCase("lookup"))
		{
			if (doEquals)
			{
				MustGetStringName(os, ",");
			}

			os.mustScan();
			const OString& s = GStrings(StdStringToUpper(os.getToken()));
			if (s.empty())
			{
				os.error("Unknown lookup string \"%s\".", os.getToken().c_str());
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
				os.mustScan();
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
		if (os.compareTokenNoCase("lookup"))
		{
			os.mustScan();
			const OString& s = GStrings(StdStringToUpper(os.getToken()));
			if (s.empty())
			{
				os.error("Unknown lookup string \"%s\".", os.getToken().c_str());
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

// Sets the credit pages from a gameinfo lump
void MIType_Pages(OScanner& os, bool doEquals, void* data, unsigned int flags,
                        unsigned int flags2)
{
	ParseMapInfoHelper<OLumpName>(os, doEquals);

	const std::string page = os.getToken();
	static_cast<OLumpName*>(data)[0] = page;

	os.scan();
	if (os.compareToken(","))
	{
		os.mustScan();
		if (os.isQuotedString())
			static_cast<OLumpName*>(data)[1] = os.getToken();
		else
			os.error("Trailing comma in Page definition; expected lump name");

		// Do a third page if finalePage/infoPage instead of creditPages
		if (flags)
		{
			os.scan();
			if (os.compareToken(","))
			{
				os.mustScan();
				if (os.isQuotedString())
					static_cast<OLumpName*>(data)[2] = os.getToken();
				else
					os.error("Trailing comma in Page definition; expected lump name");
			}
			else
			{
				os.unScan();
				static_cast<OLumpName*>(data)[2] = page;
			}
		}
	}
	else
	{
		os.unScan();
		static_cast<OLumpName*>(data)[1] = page;
		static_cast<OLumpName*>(data)[2] = page;
	}

	SkipUnknownType(os);
}

// Sets multiple lumpnames in a vector
void MIType_$VectorLumpName(OScanner& os, bool doEquals, void* data, unsigned int flags,
                            unsigned int flags2)
{
	ParseMapInfoHelper<OLumpName>(os, doEquals);

	const std::string page = os.getToken();
	static_cast<std::vector<OLumpName>*>(data)->push_back(page);

	while (os.scan())
	{
		if (os.compareToken(","))
		{
			os.mustScan();
			if (os.isQuotedString())
				static_cast<OLumpName*>(data)[1] = os.getToken();
			else
				os.error("Unexpected trailing comma; expected lump name");
		}
		else
		{
			os.unScan();
			break;
		}
	}
}

// Sets the inputted data as a std::string
void MIType_SpawnFilter(OScanner& os, bool doEquals, void* data, unsigned int flags,
                   unsigned int flags2)
{
	ParseMapInfoHelper<std::string>(os, doEquals);

	if (IsNum(os.getToken().c_str()))
	{
		const int num = os.getTokenInt();
		if (num > 0)
			*static_cast<int*>(data) |= (1 << (num - 1));
	}
	else
	{
		if (os.compareTokenNoCase("baby"))
			*static_cast<int*>(data) |= 1;
		else if (os.compareTokenNoCase("easy"))
			*static_cast<int*>(data) |= 1;
		else if (os.compareTokenNoCase("normal"))
			*static_cast<int*>(data) |= 2;
		else if (os.compareTokenNoCase("hard"))
			*static_cast<int*>(data) |= 4;
		else if (os.compareTokenNoCase("nightmare"))
			*static_cast<int*>(data) |= 4;
	}
}

// Sets the map to use the specific map07 bossactions
void MIType_Map07Special(OScanner& os, bool doEquals, void* data, unsigned int flags,
                         unsigned int flags2)
{
	std::vector<bossaction_t>& bossactionvector =
	    *static_cast<std::vector<bossaction_t>*>(data);

	// mancubus
	bossactionvector.push_back(bossaction_t());
	std::vector<bossaction_t>::iterator it = (bossactionvector.end() - 1);
	
	it->type = MT_FATSO;
	it->special = 23;
	it->tag = 666;

	// arachnotron
	bossactionvector.push_back(bossaction_t());
	it = (bossactionvector.end() - 1);
	
	it->type = MT_BABY;
	it->special = 30;
	it->tag = 667;
}

// Sets the map to use the baron bossaction
void MIType_BaronSpecial(OScanner& os, bool doEquals, void* data, unsigned int flags,
                    unsigned int flags2)
{
	std::vector<bossaction_t>& bossactionvector = *static_cast<std::vector<bossaction_t>*>(data);

	if (bossactionvector.size() == 0)
		bossactionvector.push_back(bossaction_t());

	for (std::vector<bossaction_t>::iterator it = bossactionvector.begin();
	     it != bossactionvector.end(); ++it)
	{
		it->type = MT_BRUISER;
	}
}

// Sets the map to use the cyberdemon bossaction
void MIType_CyberdemonSpecial(OScanner& os, bool doEquals, void* data, unsigned int flags,
                         unsigned int flags2)
{
	std::vector<bossaction_t>& bossactionvector =
	    *static_cast<std::vector<bossaction_t>*>(data);

	if (bossactionvector.size() == 0)
		bossactionvector.push_back(bossaction_t());

	for (std::vector<bossaction_t>::iterator it = bossactionvector.begin();
	     it != bossactionvector.end(); ++it)
	{
		it->type = MT_CYBORG;
	}
}

// Sets the map to use the cyberdemon bossaction
void MIType_SpiderMastermindSpecial(OScanner& os, bool doEquals, void* data,
                                    unsigned int flags, unsigned int flags2)
{
	std::vector<bossaction_t>& bossactionvector =
	    *static_cast<std::vector<bossaction_t>*>(data);

	if (bossactionvector.size() == 0)
		bossactionvector.push_back(bossaction_t());

	for (std::vector<bossaction_t>::iterator it = bossactionvector.begin();
	     it != bossactionvector.end(); ++it)
	{
		it->type = MT_SPIDER;
	}
}

//
void MIType_SpecialAction_ExitLevel(OScanner& os, bool doEquals, void* data,
                                    unsigned int flags, unsigned int flags2)
{
	std::vector<bossaction_t>& bossactionvector = *static_cast<std::vector<bossaction_t>*>(data);

	std::vector<bossaction_t>::iterator it;
	for (it = bossactionvector.begin(); it != bossactionvector.end(); ++it)
	{
		if (it->type != MT_NULL)
		{
			it->special = 11;
			it->tag = 0;
			return;
		}
	}

	bossactionvector.push_back(bossaction_t());
	it = bossactionvector.end() - 1;
	it->special = 11;
	it->tag = 0;
}

//
void MIType_SpecialAction_OpenDoor(OScanner& os, bool doEquals, void* data,
                                   unsigned int flags, unsigned int flags2)
{
	std::vector<bossaction_t>& bossactionvector = *static_cast<std::vector<bossaction_t>*>(data);

	std::vector<bossaction_t>::iterator it;
	for (it = bossactionvector.begin(); it != bossactionvector.end(); ++it)
	{
		if (it->type != MT_NULL)
		{
			it->special = 29;
			it->tag = 666;
			return;
		}
	}

	bossactionvector.push_back(bossaction_t());
	it = bossactionvector.end() - 1;
	it->special = 29;
	it->tag = 666;
}

//
void MIType_SpecialAction_LowerFloor(OScanner& os, bool doEquals, void* data,
                                    unsigned int flags, unsigned int flags2)
{
	std::vector<bossaction_t>& bossactionvector = *static_cast<std::vector<bossaction_t>*>(data);

	std::vector<bossaction_t>::iterator it;
	for (it = bossactionvector.begin(); it != bossactionvector.end(); ++it)
	{
		if (it->type != MT_NULL)
		{
			it->special = 23;
			it->tag = 666;
			return;
		}
	}

	bossactionvector.push_back(bossaction_t());
	it = (bossactionvector.end() - 1);
	it->special = 23;
	it->tag = 666;
}

//
void MIType_SpecialAction_KillMonsters(OScanner& os, bool doEquals, void* data,
                                    unsigned int flags, unsigned int flags2)
{
	// todo
}

// border around smaller screen sizes
void MIType_Border(OScanner& os, bool doEquals, void* data,
                                       unsigned int flags, unsigned int flags2)
{
	if (doEquals)
		MustGetStringName(os, "=");

	os.mustScan(); // can be string or int

	if (IsNum(os.getToken().c_str()))
	{
		gameborder_t& border = gameinfo.border;

		border.offset = os.getTokenInt();

		os.mustScanInt();
		border.offset = os.getTokenInt();

		os.mustScan(); border.tl = os.getToken();
		os.mustScan(); border.t  = os.getToken();
		os.mustScan(); border.tr = os.getToken();
		os.mustScan(); border.l  = os.getToken();
		os.mustScan(); border.r  = os.getToken();
		os.mustScan(); border.bl = os.getToken();
		os.mustScan(); border.b  = os.getToken();
		os.mustScan(); border.br = os.getToken();
	}
	else
	{
		if (os.compareTokenNoCase("doomborder"))
		{
			static const gameborder_t DoomBorder =
			{
				8, 8,
				"brdr_tl", "brdr_t", "brdr_tr",
				"brdr_l",			 "brdr_r",
				"brdr_bl", "brdr_b", "brdr_br"
			};

			gameinfo.border = DoomBorder;
		}
		else if (os.compareTokenNoCase("hereticborder"))
		{
			static const gameborder_t HereticBorder =
			{
				4, 16,
				"bordtl", "bordt", "bordtr",
				"bordl",           "bordr",
				"bordbl", "bordb", "bordbr"
			};

			gameinfo.border = HereticBorder;
		}
		else if (os.compareTokenNoCase("strifeborder"))
		{
			static const gameborder_t StrifeBorder =
			{
				8, 8,
				"brdr_tl", "brdr_t", "brdr_tr",
				"brdr_l",			 "brdr_r",
				"brdr_bl", "brdr_b", "brdr_br"
			};

			gameinfo.border = StrifeBorder;
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

	MapInfoDataSetter()
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
		ENTRY3("map07special", &MIType_Map07Special, &ref.bossactions)
		ENTRY3("baronspecial", &MIType_BaronSpecial, &ref.bossactions)
		ENTRY3("cyberdemonspecial", &MIType_CyberdemonSpecial, &ref.bossactions)
		ENTRY3("spidermastermindspecial", &MIType_SpiderMastermindSpecial, &ref.bossactions)
		ENTRY3("specialaction_exitlevel", &MIType_SpecialAction_ExitLevel, &ref.bossactions)
		ENTRY3("specialaction_opendoor", &MIType_SpecialAction_OpenDoor, &ref.bossactions)
		ENTRY3("specialaction_lowerfloor", &MIType_SpecialAction_LowerFloor, &ref.bossactions)
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
		ENTRY2("compat_shorttex", &MIType_DoNothing)
		ENTRY2("compat_limitpain", &MIType_DoNothing)
		ENTRY4("compat_dropoff", &MIType_SetFlag, &ref.flags, LEVEL_COMPAT_DROPOFF)
		ENTRY2("compat_trace", &MIType_DoNothing)
		ENTRY2("compat_boomscroll", &MIType_DoNothing)
		ENTRY2("compat_sectorsounds", &MIType_DoNothing)
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
		mapInfoDataContainer.reserve(10);

		ENTRY3("advisorytime", &MIType_Int, &gameinfo.advisoryTime)
		ENTRY2("border", &MIType_Border)
		ENTRY3("borderflat", &MIType_LumpName, &gameinfo.borderFlat)
		// ENTRY3("chatsound",			)
		ENTRY3("creditpage", &MIType_Pages, &gameinfo.creditPages)
		ENTRY3("pagetime", &MIType_Int, &gameinfo.pageTime)
		ENTRY3("finaleflat", &MIType_LumpName, &gameinfo.finaleFlat)
		ENTRY3("finalemusic", &MIType_MusicLumpName, &gameinfo.finaleMusic)
		ENTRY4("finalepage", &MIType_Pages, &gameinfo.finalePage, 1)
		ENTRY4("infopage", &MIType_Pages, &gameinfo.infoPage, 1)
		ENTRY3("titlemusic", &MIType_MusicLumpName, &gameinfo.titleMusic)
		ENTRY3("titlepage", &MIType_LumpName, &gameinfo.titlePage)
		ENTRY3("titletime", &MIType_Int, &gameinfo.titleTime)
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
		    !os.compareTokenNoCase("cluster"))
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
			if (os.compareTokenNoCase(it->name))
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
				os.error("Unknown MAPINFO token \"%s\"", os.getToken().c_str());
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

	os.mustScan(); // Map lump
	map = os.getToken();

	os.mustScan();
	if (os.compareTokenNoCase("teaser"))
	{
		// Teaser lump
		os.mustScan();
		if (gameinfo.flags & GI_SHAREWARE)
		{
			map = os.getToken();
		}
		os.mustScan();
	}
	else
	{
		os.unScan();
	}

	if (os.compareToken("{"))
	{
		// Detected new-style MAPINFO
		new_mapinfo = true;
		os.mustScan();
	}

	while (os.scan())
	{
		if (os.compareToken("{"))
		{
			// Detected new-style MAPINFO
			os.error("Detected incorrectly placed curly brace in MAPINFO episode definiton");
		}
		else if (os.compareToken("}"))
		{
			if (new_mapinfo == false)
				os.error("Detected incorrectly placed curly brace in MAPINFO episode "
				        "definiton");
			else
				break;
		}
		else if (os.compareTokenNoCase("name"))
		{
			ParseMapInfoHelper<std::string>(os, new_mapinfo);

			if (picisgfx == false)
				pic = os.getToken();
		}
		else if (os.compareTokenNoCase("lookup"))
		{
			ParseMapInfoHelper<std::string>(os, new_mapinfo);

			// Not implemented
		}
		else if (os.compareTokenNoCase("picname"))
		{
			ParseMapInfoHelper<std::string>(os, new_mapinfo);

			pic = os.getToken();
			picisgfx = true;
		}
		else if (os.compareTokenNoCase("key"))
		{
			ParseMapInfoHelper<std::string>(os, new_mapinfo);

			key = os.getToken()[0];
		}
		else if (os.compareTokenNoCase("remove"))
		{
			remove = true;
		}
		else if (os.compareTokenNoCase("noskillmenu"))
		{
			noskillmenu = true;
		}
		else if (os.compareTokenNoCase("optional"))
		{
			optional = true;
		}
		else if (os.compareTokenNoCase("extended"))
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
				const int saved_i = i;

				for (; i < episodenum; ++i)
				{
					EpisodeMaps[i] = EpisodeMaps[i + 1];
				}
				EpisodeMaps[i].clear();

				i = saved_i;

				for (; i < episodenum; ++i)
				{
					EpisodeInfos[i] = EpisodeInfos[i + 1];
				}
				EpisodeInfos[i] = EpisodeInfo();
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
				i = episodenum - 1;
			else
				i = episodenum++;
		}

		EpisodeInfos[i].name = pic;
		EpisodeInfos[i].key = static_cast<char>(tolower(key));
		EpisodeInfos[i].fulltext = !picisgfx;
		EpisodeInfos[i].noskillmenu = noskillmenu;
		EpisodeMaps[i] = map;
	}
}

// SkillInfo
template <>
struct MapInfoDataSetter<SkillInfo>
{
	MapInfoDataContainer mapInfoDataContainer;

	MapInfoDataSetter(SkillInfo& ref)
	{
		mapInfoDataContainer.reserve(33);

		ENTRY3("ammofactor", &MIType_Float, &ref.ammo_factor)
		ENTRY3("doubleammofactor", &MIType_Float, &ref.double_ammo_factor)
		ENTRY3("dropammofactor", &MIType_Float, &ref.drop_ammo_factor)
		ENTRY3("damagefactor", &MIType_Float, &ref.damage_factor)
		ENTRY3("armorfactor", &MIType_Float, &ref.armor_factor)
		ENTRY3("healthfactor", &MIType_Float, &ref.health_factor)
		ENTRY3("kickbackfactor", &MIType_Float, &ref.kickback_factor)

		ENTRY4("fastmonsters", &MIType_Bool, &ref.fast_monsters, true)
		ENTRY4("slowmonsters", &MIType_Bool, &ref.slow_monsters, true)
		ENTRY4("disablecheats", &MIType_Bool, &ref.disable_cheats, true)
		ENTRY4("autousehealth", &MIType_Bool, &ref.auto_use_health, true)

		ENTRY4("easybossbrain", &MIType_Bool, &ref.easy_boss_brain, true)
		ENTRY4("easykey", &MIType_Bool, &ref.easy_key, true)
		ENTRY4("nomenu", &MIType_Bool, &ref.no_menu, true)
		ENTRY3("respawntime", &MIType_Int, &ref.respawn_counter)
		ENTRY3("respawnlimit", &MIType_Int, &ref.respawn_limit)
		ENTRY3("aggressiveness", &MIType_Float, &ref.aggressiveness)
		ENTRY3("spawnfilter", &MIType_SpawnFilter, &ref.spawn_filter)
		ENTRY4("spawnmulti", &MIType_Bool, &ref.spawn_multi, true)
		ENTRY4("instantreaction", &MIType_Bool, &ref.instant_reaction, true)
		ENTRY3("acsreturn", &MIType_Int, &ref.ACS_return)
		ENTRY3("name", &MIType_String, &ref.menu_name)
		ENTRY3("picname", &MIType_String, &ref.pic_name)
		//ENTRY3("playerclassname", &???, &ref.menu_names_for_player_class) // todo - requires special MIType to work properly
		ENTRY4("mustconfirm", &MIType_MustConfirm, &ref, true)
		ENTRY3("key", &MIType_Char, &ref.shortcut)
		ENTRY3("textcolor", &MIType_Color, &ref.text_color)
		// ENTRY3("replaceactor", &???, &ref.replace) // todo - requires special MIType to work properly
		ENTRY3("monsterhealth", &MIType_Float, &ref.monster_health)
		ENTRY3("friendlyhealth", &MIType_Float, &ref.friendly_health)
		ENTRY4("nopain", &MIType_Bool, &ref.no_pain, true)
		ENTRY3("infighting", &MIType_Int, &ref.infighting)
		ENTRY4("playerrespawn", &MIType_Bool, &ref.player_respawn, true)
	}
};

void ParseMapInfoLump(int lump, const char* lumpname)
{
	LevelInfos& levels = getLevelInfos();
	ClusterInfos& clusters = getClusterInfos();

	level_pwad_info_t defaultinfo;

	const char* buffer = static_cast<char*>(W_CacheLumpNum(lump, PU_STATIC));

	const OScannerConfig config = {
	    lumpname, // lumpName
	    false,    // semiComments
	    true,     // cComments
	};
	OScanner os = OScanner::openBuffer(config, buffer, buffer + W_LumpLength(lump));

	while (os.scan())
	{
		if (os.compareTokenNoCase("defaultmap"))
		{
			defaultinfo = level_pwad_info_t();

			MapInfoDataSetter<level_pwad_info_t> defaultsetter(defaultinfo);
			ParseMapInfoLower<level_pwad_info_t>(os, defaultsetter);
		}
		else if (os.compareTokenNoCase("map"))
		{
			uint32_t& levelflags = defaultinfo.flags;
			os.mustScan();

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
			os.mustScan();
			if (os.compareTokenNoCase("lookup"))
			{
				os.mustScan();
				const OString& s = GStrings(StdStringToUpper(os.getToken()));
				if (s.empty())
				{
					info.level_name = os.getToken();
				}
				info.level_name = s;
			}
			else
			{
				info.level_name = os.getToken();
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
		else if (os.compareTokenNoCase("cluster") ||
		         os.compareTokenNoCase("clusterdef"))
		{
			os.mustScanInt();

			// Find the cluster.
			cluster_info_t& info = (clusters.findByCluster(os.getTokenInt()).cluster != 0)
			        ? clusters.findByCluster(os.getTokenInt())
			        : clusters.create();

			info.cluster = os.getTokenInt();

			MapInfoDataSetter<cluster_info_t> setter(info);
			ParseMapInfoLower<cluster_info_t>(os, setter);
		}
		else if (os.compareTokenNoCase("episode"))
		{
			ParseEpisodeInfo(os);
		}
		else if (os.compareTokenNoCase("clearepisodes"))
		{
			episodenum = 0;
			// Set this for UMAPINFOs sake (UMAPINFO doesn't consider Doom 2's episode a
			// real episode)
			episodes_modified = false;
		}
		else if (os.compareTokenNoCase("skill"))
		{
			os.mustScan(); // Name

			if (skillnum < MAX_SKILLS)
			{
				SkillInfo &info = SkillInfos[skillnum];
				info = SkillInfo();

				info.name = os.getToken();

				MapInfoDataSetter<SkillInfo> setter(info);
				ParseMapInfoLower<SkillInfo>(os, setter);

				++skillnum;
			}
			else
			{
				MapInfoDataSetter<void> setter;
				ParseMapInfoLower<void>(os, setter);
			}
		}
		else if (os.compareTokenNoCase("clearskills"))
		{
			skillnum = 0;
		}
		else if (os.compareTokenNoCase("gameinfo"))
		{
			MapInfoDataSetter<gameinfo_t> setter;
			ParseMapInfoLower<gameinfo_t>(os, setter);
		}
		else if (os.compareTokenNoCase("intermission"))
		{
			// Not implemented
			os.mustScan(); // Name

			MapInfoDataSetter<void> setter;
			ParseMapInfoLower<void>(os, setter);
		}
		else if (os.compareTokenNoCase("automap"))
		{
			// Not implemented
			MapInfoDataSetter<void> setter;
			ParseMapInfoLower<void>(os, setter);
		}
		else
		{
			os.error("Unimplemented top-level type \"%s\"", os.getToken().c_str());
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

	// Reset skill definitions
	skillnum = 0;

	//if (gamemission != heretic)
	{
		ParseMapInfoLump(W_GetNumForName("_DCOMNFO"), "_DCOMNFO");
	}

	switch (gamemission)
	{
	case doom:
	case retail_freedoom:
		baseinfoname = "_D1NFO";
		if (gamemode == shareware)
		{
			lump = W_GetNumForName(baseinfoname);
			ParseMapInfoLump(lump, baseinfoname);
			baseinfoname = "_D1SWNFO";
		}
		break;
	case doom2:
	case commercial_freedoom:
	case commercial_hacx:
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
	case none:
	default:
		I_Error("%s: This IWAD is unknown to Odamex", __FUNCTION__);
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
		return;

	lump = -1;
	while ((lump = W_FindLump("UMAPINFO", lump)) != -1)
	{
		ParseUMapInfoLump(lump, "UMAPINFO");
	}

	// If UMAPINFO exists, we don't parse a normal MAPINFO
	if (found_mapinfo == true)
		return;

	lump = -1;
	while ((lump = W_FindLump("MAPINFO", lump)) != -1)
	{
		ParseMapInfoLump(lump, "MAPINFO");
	}

	if (episodenum == 0)
		I_FatalError("%s: You cannot use clearepisodes in a MAPINFO if you do not define any "
		             "new episodes after it.", __FUNCTION__);

	if (defaultskillmenu > skillnum - 1)
		defaultskillmenu = skillnum - 1;

	if (skillnum == 0)
		I_FatalError("%s: You cannot use clearskills in a MAPINFO if you do not define any "
					"new skills after it.", __FUNCTION__);
}
