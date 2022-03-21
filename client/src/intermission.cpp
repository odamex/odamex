// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//   Intermissions.
// 
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "intermission.h"

#include "gstrings.h"
#include "m_ostring.h"
#include "w_wad.h"
                                    
namespace
{
void TicRateConversion(OScanner& os, int& i)
{
	const std::string str = os.getToken();
	if (!IsRealNum(str.c_str()))
		os.error("Expected number, got \"%s\"", str.c_str());

	if (str[0] == '-')
		i = static_cast<int>(-os.getTokenFloat() * TICRATE);
	else
		i = os.getTokenInt();
}
} // namespace

//-----------------------------------------------------------------------------
//
// OIntermissionAction
//
//-----------------------------------------------------------------------------

OIntermissionAction::OIntermissionAction()
    : music(), musicOrder(0), musicLooping(true), duration(0), flatFill(false)
{
}

bool OIntermissionAction::ParseKey(OScanner& os)
{
	if (os.compareTokenNoCase("music"))
	{
		os.mustScan();
		os.assertTokenIs("=");

		os.mustScan();
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
				music = lumpname;
			}
		}
		else
		{
			if (W_CheckNumForName(musicname.c_str()) != -1)
			{
				music = musicname;
			}
		}

		musicOrder = 0;
		os.scan();
		if (os.compareTokenNoCase(","))
		{
			os.mustScanInt();
			musicOrder = os.getTokenInt();
		}
		else
			os.unScan();

		return true;
	}
	else if (os.compareTokenNoCase("cdmusic"))
	{
		// none of this is actually implemented, it's just here because GZDoom uses it
		os.mustScan();
		os.assertTokenIs("=");

		os.mustScanInt();
		os.scan();
		if (os.compareTokenNoCase(","))
		{
			os.mustScanInt();
		}
		else
			os.unScan();

		return true;
	}
	else if (os.compareTokenNoCase("time"))
	{
		os.mustScan();
		os.assertTokenIs("=");

		os.mustScan();
		TicRateConversion(os, duration);

		return true;
	}
	else if (os.compareTokenNoCase("background"))
	{
		os.mustScan();
		os.assertTokenIs("=");

		os.mustScan();
		background = os.getToken();

		os.scan();
		if (os.compareTokenNoCase(","))
		{
			os.mustScanInt();
			flatFill = !!os.getTokenInt();

			os.scan();
			if (os.compareTokenNoCase(","))
			{
				os.mustScan();
				backgroundPalette = os.getToken();
			}
			else
				os.unScan();
		}
		else
			os.unScan();

		return true;
	}
	else if (os.compareTokenNoCase("sound"))
	{
		os.mustScan();
		os.assertTokenIs("=");
		os.mustScan();
		strncpy(sound, os.getToken().c_str(), MAX_SNDNAME);
		return true;
	}
	else if (os.compareTokenNoCase("subtitle"))
	{
		os.mustScan();
		os.assertTokenIs("=");
		os.mustScan();
		// [DE] I have no idea what this is for
		return true;
	}
	else if (os.compareTokenNoCase("draw"))
	{
		OIntermissionPatch patch;

		os.mustScan();
		os.assertTokenIs("=");

		os.mustScan();
		patch.name = os.getToken();
		os.mustScan();
		os.assertTokenIs(",");
		os.mustScanInt();
		patch.x = os.getTokenInt();
		os.mustScan();
		os.assertTokenIs(",");
		os.mustScanInt();
		patch.y = os.getTokenInt();

		overlays.push_back(patch);

		return true;
	}
	else if (os.compareTokenNoCase("drawconditional"))
	{
		os.mustScan();
		os.assertTokenIs("=");

		OIntermissionPatch patch;

		os.mustScan();
		os.assertTokenIs("=");

		os.mustScan();
		patch.condition = os.getToken();
		os.mustScan();
		os.assertTokenIs(",");
		os.mustScan();
		patch.name = os.getToken();
		os.mustScan();
		os.assertTokenIs(",");
		os.mustScanInt();
		patch.x = os.getTokenInt();
		os.mustScan();
		os.assertTokenIs(",");
		os.mustScanInt();
		patch.y = os.getTokenInt();

		overlays.push_back(patch);

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
//
// OIntermissionActionFader
//
//-----------------------------------------------------------------------------

OIntermissionActionFader::OIntermissionActionFader()
	: fadeType(FadeIn)
{
}

bool OIntermissionActionFader::ParseKey(OScanner& os)
{
	if (os.compareTokenNoCase("fadetype"))
	{
		os.mustScan();
		os.assertTokenIs("=");

		os.mustScan();
		if (os.compareTokenNoCase("FadeIn"))
			fadeType = FadeIn;
		else if (os.compareTokenNoCase("FadeOut"))
			fadeType = FadeOut;
		else
			os.error("Expected \"FadeIn\" or \"FadeOut\", got \"%s\"", os.getToken().c_str());

		return true;
	}

	return OIntermissionAction::ParseKey(os);
}

//-----------------------------------------------------------------------------
//
// OIntermissionActionWiper
//
//-----------------------------------------------------------------------------

OIntermissionActionWiper::OIntermissionActionWiper()
	: wipeType(GS_FORCEWIPE)
{
}

bool OIntermissionActionWiper::ParseKey(OScanner& os)
{
	if (os.compareTokenNoCase("wipetype"))
	{
		os.mustScan();
		os.assertTokenIs("=");

		os.mustScan();

		// [DE] Odamex doesn't have other wiper types afaik so do nothing

		return true;
	}

	return OIntermissionAction::ParseKey(os);
}

//-----------------------------------------------------------------------------
//
// OIntermissionActionTextscreen
//
//-----------------------------------------------------------------------------

OIntermissionActionTextscreen::OIntermissionActionTextscreen()
	: text("In the quiet following your last battle,\n"
			"you suddenly get the feeling that something is\n"
			"...missing.  Like there was supposed to be intermission\n"
			" text here, but somehow it couldn't be found.\n"
			"\n"
			"No matter.  You ready your weapon and continue on \n"
           "into the chaos.")
	, textDelay(10)
	, textSpeed(2)
	, textX(-1) // -1 == gameinfo default
	, textY(-1) // -1 == gameinfo default
{
}

bool OIntermissionActionTextscreen::ParseKey(OScanner& os)
{
	if (os.compareTokenNoCase("position"))
	{
		os.mustScan();
		os.assertTokenIs("=");

		os.mustScanInt();
		textX = os.getTokenInt();
		os.mustScan();
		os.assertTokenIs(",");
		os.mustScanInt();
		textY = os.getTokenInt();

		return true;
	}
	else if (os.compareTokenNoCase("textlump"))
	{
		os.mustScan();
		os.assertTokenIs("=");

		os.mustScan();
		const int lump = W_CheckNumForName(os.getToken().c_str());
		bool done = false;
		if (lump > 0)
		{
			// Check if this comes from either Hexen.wad or Hexdd.wad and if so, map to
			// the string table.
			// todo: the thing from above

			if (!done)
				text = static_cast<char*>(W_CacheLumpNum(lump, PU_STATIC));
		}
		else
		{
			os.warning("Missing text lump \"%s\"", os.getToken().c_str());
		}

		return true;
	}
	else if (os.compareTokenNoCase("text"))
	{
		os.mustScan();
		os.assertTokenIs("=");

		do
		{
			os.mustScan();
			text += os.getToken();
			os.scan();
		} while (os.compareToken(","));

		os.unScan();

		return true;
	}
	else if (os.compareTokenNoCase("textcolor"))
	{
		os.mustScan();
		os.assertTokenIs("=");

		os.mustScan();
		// todo

		return true;
	}
	else if (os.compareTokenNoCase("textdelay"))
	{
		os.mustScan();
		os.assertTokenIs("=");

		os.mustScan();
		TicRateConversion(os, textDelay);

		return true;
	}
	else if (os.compareTokenNoCase("textspeed"))
	{
		os.mustScan();
		os.assertTokenIs("=");

		os.mustScanInt();
		textSpeed = os.getTokenInt();

		return true;
	}

	return OIntermissionAction::ParseKey(os);
}

//-----------------------------------------------------------------------------
//
// OIntermissionActionCast
//
//-----------------------------------------------------------------------------

OIntermissionActionCast::OIntermissionActionCast()
{
}

bool OIntermissionActionCast::ParseKey(OScanner& os)
{
	if (os.compareTokenNoCase("castname"))
	{
		os.mustScan();
		os.assertTokenIs("=");
		os.mustScan();
		name = os.getToken(); // todo: DEHACKED support

		return true;
	}
	else if (os.compareTokenNoCase("castclass"))
	{
		os.mustScan();
		os.assertTokenIs("=");
		os.mustScan();
		castClass = os.getToken(); // todo: is this correct?

		return true;
	}
	else if (os.compareTokenNoCase("attacksound"))
	{
		OCastSound castSound;

		os.mustScan();
		os.assertTokenIs("=");

		os.mustScan();
		if (os.compareTokenNoCase("missile"))
			castSound.sequence = 0; // todo: is this correct?
		else if (os.compareTokenNoCase("missile"))
			castSound.sequence = 1;
		else
			os.error("Exepcted \"Melee\" or \"Missile\", got \"%s\"", os.getToken().c_str());

		os.mustScan();
		os.assertTokenIs(",");
		os.mustScanInt();
		castSound.index = os.getTokenInt();

		os.mustScan();
		os.assertTokenIs(",");
		os.mustScan();
		strncpy(castSound.sound,os.getToken().c_str(), MAX_SNDNAME);

		castSounds.push_back(castSound);

		return true;
	}

	return OIntermissionAction::ParseKey(os);
}

//-----------------------------------------------------------------------------
//
// OIntermissionActionScroller
//
//-----------------------------------------------------------------------------

OIntermissionActionScroller::OIntermissionActionScroller()
	: scrollDelay(0)
	, scrollTime(640)
	, scrollDir(ScrollRight)
{
}

bool OIntermissionActionScroller::ParseKey(OScanner& os)
{
	if (os.compareTokenNoCase("scrolldirection"))
	{
		os.mustScan();
		os.assertTokenIs("=");

		os.mustScan();
		if (os.compareTokenNoCase("left"))
			scrollDir = ScrollLeft;
		else if (os.compareTokenNoCase("right"))
			scrollDir = ScrollRight;
		else if (os.compareToken("up"))
			scrollDir = ScrollUp;
		else if (os.compareTokenNoCase("down"))
			scrollDir = ScrollDown;

		return true;
	}
	else if (os.compareTokenNoCase("initialdelay"))
	{
		os.mustScan();
		os.assertTokenIs("=");

		os.mustScan();
		TicRateConversion(os, scrollDelay);

		return true;
	}
	else if (os.compareTokenNoCase("scrolltime"))
	{
		os.mustScan();
		os.assertTokenIs("=");

		os.mustScan();
		TicRateConversion(os, scrollTime);

		return true;
	}
	else if (os.compareTokenNoCase("background2"))
	{
		os.mustScan();
		os.assertTokenIs("=");

		os.mustScan();
		secondBackground = os.getToken();

		return true;
	}

	return OIntermissionAction::ParseKey(os);
}

//-----------------------------------------------------------------------------

void ParseIntermissionInfo(OScanner& os)
{
	OIntermissionAction *action = NULL;

	os.mustScan(); // Name
	//name = os.getToken();

	os.mustScan();
	os.assertTokenIs("{");

	while (os.scan())
	{
		if (os.compareToken("}"))
			break;

		if (os.compareTokenNoCase("image"))
		{
			action = new OIntermissionAction;
		}
		else if (os.compareTokenNoCase("scroller"))
		{
			action = new OIntermissionActionScroller;
		}
		else if (os.compareTokenNoCase("cast"))
		{
			action = new OIntermissionActionCast;
		}
		else if (os.compareTokenNoCase("fader"))
		{
			action = new OIntermissionActionFader;
		}
		else if (os.compareTokenNoCase("wiper"))
		{
			action = new OIntermissionActionWiper;
		}
		else if (os.compareTokenNoCase("textscreen"))
		{
			action = new OIntermissionActionTextscreen;
		}
		else if (os.compareTokenNoCase("gototitle"))
		{
			action = new OIntermissionAction;
			// todo: set as gototitle
		}
		else if (os.compareTokenNoCase("link"))
		{
			os.mustScan();
			os.assertTokenIs("=");

			os.mustScan();
			// todo: link = os.getToken();

			return;
		}
		else
		{
			os.warning("Unknown intermission type \"%s\"", os.getToken().c_str());
		}

		os.mustScan();
		os.assertTokenIs("{");
		while (os.scan())
		{
			bool success = false;
			if (action)
			{
				success = action->ParseKey(os);
				if (!success)
					os.warning("Unknown intermission key \"%s\"", os.getToken().c_str());
			}
			if (!success)
			{
				while (os.scan())
					if (os.crossed())
						break;
			}
		}
		if (action)
			; // todo: add to list
	}
}
