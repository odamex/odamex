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
	os.mustScan();
	os.mustScanInt();
	int newLevel = os.getTokenInt();

	if (newLevel <= spreeLevels.size() || newLevel > spreeLevels.size() + 1 ||
	    newLevel <= 0)
		os.error("Spree levels must be defined in ascending order.");

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
			os.mustScanInt();
			spree.color = static_cast<EColorRange>(os.getTokenInt());
		}
		else if (os.compareTokenNoCase("text"))
		{
			os.mustScan();
			os.assertTokenIs("=");
			os.mustScan();
			spree.spreeText = os.getToken();
		}
		else if (os.compareTokenNoCase("broadcasttext"))
		{
			os.mustScan();
			os.assertTokenIs("=");
			os.mustScan();
			std::string broadcastText = os.getToken();
			// Use LANGUAGE lump for this string.
			if (broadcastText.find_first_of("$") == 0)
					{
				// This is a reference to a string.
				spree.spreeBroadcastText = GStrings(broadcastText.substr(1));
			}
			else
			{
				spree.spreeBroadcastText = broadcastText;
			}
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
	os.mustScan();
	os.mustScanInt();
	int newLevel = os.getTokenInt();

	if (newLevel <= multiKillLevels.size() || newLevel > multiKillLevels.size() + 1 ||
	    newLevel <= 1)
		os.error("Multi kill levels must be defined in ascending order.");

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
			os.mustScanInt();
			level.color = static_cast<EColorRange>(os.getTokenInt());
		}
		else if (os.compareTokenNoCase("text"))
		{
			os.mustScan();
			os.assertTokenIs("=");
			os.mustScan();
			level.multikilltext = os.getToken();
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

	// Spree variables
	int spreedamageinterval = 0;
	int spreekillinterval = 0;
	int multikillinterval = 0;

	std::vector<Spree_s> spreeLevels;
	std::vector<MultiKillLevel_s> multiKillLevels;

	multiKillLevels.push_back(MultiKillLevel_s()); // Level 0 placeholder
	multiKillLevels.push_back(MultiKillLevel_s()); // Level 1 placeholder

	std::string repeatingSpreeText = "";
	std::string spreeEndPlayer = "";
	std::string spreeEndSelf = "";
	std::string spreeEndMonster = "";

	while (os.scan())
	{
		if (os.compareTokenNoCase("spreekillinterval"))
		{
			ParseSpreeKillInterval(os, spreekillinterval);
		}
		else if (os.compareTokenNoCase("spreedamageinterval"))
		{
			ParseSpreeDamageInterval(os, spreedamageinterval);
		}
		else if (os.compareTokenNoCase("multikillinterval"))
		{
			ParseMultiInterval(os, multikillinterval);
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
}

/// <summary>
/// Parses all SPREEDEF lumps for consumption by
/// the spree and multi kill managers.
/// </summary>
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
