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

static void ParseKillInterval(OScanner& os, int& killinterval)
{
	os.assertTokenIs("killinterval");
	os.mustScan();
	os.assertTokenIs("=");
	os.mustScanInt();
	killinterval = os.getTokenInt();
}

static void ParseDamageInterval(OScanner& os, int& damageinterval)
{
	os.assertTokenIs("damageinterval");
	os.mustScan();
	os.assertTokenIs("=");
	os.mustScanInt();
	damageinterval = os.getTokenInt();
}

static void ParseMultiInterval(OScanner& os, int& multikillinterval)
{
	os.assertTokenIs("multitimeinterval");
	os.mustScan();
	os.assertTokenIs("=");
	os.mustScanInt();
	multikillinterval = os.getTokenInt();
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

	int damageinterval = 0;
	int killinterval = 0;
	int multikillinterval = 0;

	while (os.scan())
	{
		if (os.compareTokenNoCase("killinterval"))
		{
			ParseKillInterval(os, killinterval);
		}
		else if (os.compareTokenNoCase("damageinterval"))
		{
			ParseDamageInterval(os, damageinterval);
		}
		else if (os.compareTokenNoCase("multitimeinterval"))
		{
			ParseMultiInterval(os, multikillinterval);
		}
		else if (os.compareTokenNoCase("spree"))
		{
			// ParseSpree(os);
		}
		else if (os.compareTokenNoCase("multi"))
		{
			// ParseMulti(os);
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
