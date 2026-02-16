// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2025 by The Odamex Team.
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
//   Handles parsing all SPREEDEF lumps.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "w_wad.h"
#include "g_multikill.h"
#include "g_spree.h"
#include "oscanner.h"
#include "gstrings.h"

static std::string UseStringTableOrToken(std::string token)
{
	if (token.find_first_of("$") == 0)
	{
		std::string text = GStrings(token.substr(1));
		if (text.empty())
		{
			return token;
		}
		else
		{
			return text;
		}
	}
	else
	{
		return token;
	}
}

static void ParseSpreeKillInterval(OScanner& os, int& killinterval)
{
	os.assertTokenIs("spreekillinterval");
	os.mustScan();
	os.assertTokenIs("=");
	os.mustScanInt();
	killinterval = os.getTokenInt();
}

static void ParseSpreeDamageInterval(OScanner& os, int& damageinterval)
{
	os.assertTokenIs("spreedamageinterval");
	os.mustScan();
	os.assertTokenIs("=");
	os.mustScanInt();
	damageinterval = os.getTokenInt();
}

static void ParseMultiInterval(OScanner& os, int& multikillinterval)
{
	os.assertTokenIs("multikillinterval");
	os.mustScan();
	os.assertTokenIs("=");
	os.mustScanInt();
	multikillinterval = os.getTokenInt();
}

static void ParseSpree(OScanner& os, std::vector<Spree_s>& spreeLevels)
{
	os.assertTokenIs("spree");
	os.mustScanInt();
	int newLevel = os.getTokenInt();

	if (newLevel <= spreeLevels.size() || newLevel > spreeLevels.size() + 1 ||
	    newLevel <= 0)
		os.error("Spree levels must be defined in ascending order.");

	os.mustScan();
	os.assertTokenIs("=");
	os.mustScan();
	os.assertTokenIs("{");
	os.mustScan();
	Spree_s spree = Spree_s();
	while (!os.compareToken("}"))
	{
		if (os.compareTokenNoCase("color"))
		{
			os.mustScan();
			os.assertTokenIs("=");
			os.mustScan();
			spree.color = TextColorFromString(os.getToken());
		}
		else if (os.compareTokenNoCase("text"))
		{
			os.mustScan();
			os.assertTokenIs("=");
			os.mustScan();
			std::string text = os.getToken();
			spree.spreeText = UseStringTableOrToken(text);
		}
		else if (os.compareTokenNoCase("broadcasttext"))
		{
			os.mustScan();
			os.assertTokenIs("=");
			os.mustScan();
			std::string broadcastText = os.getToken();
			spree.spreeBroadcastText = UseStringTableOrToken(broadcastText);
		}
		else if (os.compareTokenNoCase("gamesfx"))
		{
			os.mustScan();
			os.assertTokenIs("=");
			os.mustScan();
			std::string gamesfx = os.getToken();
			spree.gameSfxToken = gamesfx;
		}
		else
		{
			// We don't know what this token is.
			std::string buffer = fmt::sprintf("Unknown Spree Token \"%s\".", os.getToken());
			os.warning(buffer);
		}
		os.mustScan();
	}

	spreeLevels.push_back(spree);
}

static void ParseMulti(OScanner& os, std::vector<MultiKillLevel_s>& multiKillLevels)
{
	os.assertTokenIs("multi");
	os.mustScanInt();
	int newLevel = os.getTokenInt();

	if (newLevel <= multiKillLevels.size() - 1 || newLevel > multiKillLevels.size() + 1 ||
	    newLevel <= 1)
		os.error("Multi kill levels must be defined in ascending order.");

	os.mustScan();
	os.assertTokenIs("=");
	os.mustScan();
	os.assertTokenIs("{");
	os.mustScan();
	MultiKillLevel_s level = MultiKillLevel_s();
	while (!os.compareToken("}"))
	{
		if (os.compareTokenNoCase("color"))
		{
			os.mustScan();
			os.assertTokenIs("=");
			os.mustScan();
			level.color = TextColorFromString(os.getToken());
		}
		else if (os.compareTokenNoCase("text"))
		{
			os.mustScan();
			os.assertTokenIs("=");
			os.mustScan();
			std::string text = os.getToken();
			level.multikilltext = UseStringTableOrToken(text);
		}
		else if (os.compareTokenNoCase("gamesfx"))
		{
			os.mustScan();
			os.assertTokenIs("=");
			os.mustScan();
			std::string gamesfx = os.getToken();
			level.gameSfxToken = gamesfx;
		}
		else
		{
			// We don't know what this token is.
			std::string buffer =
			    fmt::sprintf("Unknown Multi Kill Token \"%s\".", os.getToken());
			os.warning(buffer);
		}
		os.mustScan();
	}

	multiKillLevels.push_back(level);
}

static void ParseSpreeText(OScanner& os, std::string& text, std::string token)
{
	os.assertTokenIs(token);
	os.mustScan();
	os.assertTokenIs("=");
	os.mustScan();
	std::string newText = os.getToken();
	text = UseStringTableOrToken(newText);
}

static void ParseSpreeDef(const int lump, const OLumpName name)
{
	char* buffer = static_cast<char*>(W_CacheLumpNum(lump, PU_CACHE));

	const OScannerConfig config = {
	    "SPREEDEF", // lumpName
	    false,      // semiComments
	    true,       // cComments
	};
	OScanner os = OScanner::openBuffer(config, buffer, buffer + W_LumpLength(lump));

	// Reset everything before parsing
	MultiKillManager::getInstance().reset();
	SpreeManager::getInstance().reset();

	// Spree variables (required)
	int spreeDamageInterval = 0;
	int spreeKillInterval = 0;
	int multiKillInterval = 0;
	std::string repeatingSpreeText = "";
	std::string spreeEndPlayer = "";
	std::string spreeEndSelf = "";
	std::string spreeEndMonster = "";

	// Spree and multi kill levels
	std::vector<Spree_s> spreeLevels;
	std::vector<MultiKillLevel_s> multiKillLevels;

	multiKillLevels.push_back(MultiKillLevel_s()); // Level 0 placeholder
	multiKillLevels.push_back(MultiKillLevel_s()); // Level 1 placeholder

	while (os.scan())
	{
		if (os.compareTokenNoCase("spreekillinterval"))
		{
			ParseSpreeKillInterval(os, spreeKillInterval);
		}
		else if (os.compareTokenNoCase("spreedamageinterval"))
		{
			ParseSpreeDamageInterval(os, spreeDamageInterval);
		}
		else if (os.compareTokenNoCase("multikillinterval"))
		{
			ParseMultiInterval(os, multiKillInterval);
		}
		else if (os.compareTokenNoCase("spreeendedplayertext"))
		{
			ParseSpreeText(os, spreeEndPlayer, "spreeendedplayertext");
		}
		else if (os.compareTokenNoCase("spreeendedselftext"))
		{
			ParseSpreeText(os, spreeEndSelf, "spreeendedselftext");
		}
		else if (os.compareTokenNoCase("spreeendedmonstertext"))
		{
			ParseSpreeText(os, spreeEndMonster, "spreeendedmonstertext");
		}
		else if (os.compareTokenNoCase("repeatingspreetext"))
		{
			ParseSpreeText(os, repeatingSpreeText, "repeatingspreetext");
		}
		else if (os.compareTokenNoCase("spree"))
		{
			ParseSpree(os, spreeLevels);
		}
		else if (os.compareTokenNoCase("multi"))
		{
			ParseMulti(os, multiKillLevels);
		}
		else
		{
			// We don't know what this token is.
			std::string buffer = fmt::sprintf("Unknown Token \"%s\".", os.getToken());
			os.error(buffer);
		}
	}

	// Update the spree and multi kill managers
	// If there's nothing here just load defaults
	if (spreeLevels.size() > 0)
	{
		// Check required variables, error if missing
		if (spreeKillInterval == 0)
			os.error("Missing required keyword 'spreekillinterval'.");
		if (spreeDamageInterval == 0)
			os.error("Missing required keyword 'spreedamageinterval'.");
		if (repeatingSpreeText.empty())
			os.error("Missing required keyword 'repeatingspreetext'.");
		if (spreeEndPlayer.empty())
			os.error("Missing required keyword 'spreeendedplayertext'.");
		if (spreeEndSelf.empty())
			os.error("Missing required keyword 'spreeendedselftext'.");
		if (spreeEndMonster.empty())
			os.error("Missing required keyword 'spreeendedmonstertext'.");

		NewSprees_s newSprees = {
		    spreeLevels,    spreeKillInterval, spreeDamageInterval, repeatingSpreeText,
		    spreeEndPlayer, spreeEndSelf,      spreeEndMonster
		};

		SpreeManager::getInstance().setSpreeLevels(newSprees);
	}
	else
	{
		SpreeManager::getInstance().loadSpreeDefaults();
	}

	if (multiKillLevels.size() > 2)
	{
		// Check required variables, error if missing
		if (multiKillInterval == 0)
			os.error("Missing required keyword 'multikillinterval'.");

		MultiKillManager::getInstance().setMultiKillLevels(multiKillLevels,
		                                                   multiKillInterval);
	}
	else
	{
		MultiKillManager::getInstance().loadMultiKillDefaults();
	}
}

void G_ParseSpreeDef()
{
	int lump = -1;

	// No SPREEDEF? Load defaults and continue.
	if (W_FindLump("SPREEDEF", lump) == -1)
	{
		MultiKillManager::getInstance().loadMultiKillDefaults();
		SpreeManager::getInstance().loadSpreeDefaults();
		return;
	}

	while ((lump = W_FindLump("SPREEDEF", lump)) != -1)
	{
		ParseSpreeDef(lump, "SPREEDEF");
	}
}
