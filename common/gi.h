// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2026 by The Odamex Team.
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
//	GI
//
//-----------------------------------------------------------------------------


#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

#include "am_map.h"
#include "olumpname.h"
#include "s_sound.h"

#define GI_MAPxx				0x00000001
#define GI_PAGESARERAW			0x00000002
#define GI_SHAREWARE			0x00000004
#define GI_NOLOOPFINALEMUSIC	0x00000008
#define GI_INFOINDEXED			0x00000010
#define GI_MENUHACK				0x00000060
#define GI_MENUHACK_RETAIL		0x00000020
#define GI_MENUHACK_COMMERCIAL	0x00000060
#define GI_NOCRAZYDEATH			0x00000080

typedef struct
{
	byte offset;
	byte size;
	OLumpName tl;
	OLumpName t;
	OLumpName tr;
	OLumpName l;
	OLumpName r;
	OLumpName bl;
	OLumpName b;
	OLumpName br;
} gameborder_t;

typedef enum
{
	ENGINE_DOOM,
	ENGINE_HERETIC,
} enginetype_t;

typedef enum
{
	DEMOFORMAT_DOOM_VANILLA,
	DEMOFORMAT_HERETIC_VANILLA,
} demoformat_t;

struct fontdef_t
{
	std::string pattern;
	int lumpStart;
	int lineHeight;

	fontdef_t()
		: pattern("")
		, lumpStart(1)
		, lineHeight(0)
	{
	}

	fontdef_t(const char* fontPattern, int fontLumpStart, int fontLineHeight)
		: pattern(fontPattern)
		, lumpStart(fontLumpStart)
		, lineHeight(fontLineHeight)
	{
	}
};

inline std::unordered_map<std::string, fontdef_t> fontdefs;

inline void G_ResetFontDefs()
{
	fontdefs.clear();
	fontdefs.emplace("BIGFONT", fontdef_t("FONTB%02d", 1, 16));
	fontdefs.emplace("SMALLFONT", fontdef_t("STCFN%03d", 33, 8));
}

typedef struct gameinfo_s
{
	int flags;
	enginetype_t enginetype;
	demoformat_t demoformat;
	OLumpName titlePage;
	OLumpName demoLoop;
	OLumpName creditPages[2];
	OLumpName titleMusic;
	int titleTime;
	int advisoryTime;
	bool noLoopFinaleMusic;
	int pageTime;
	char chatSound[MAX_SNDNAME + 1];
	OLumpName finaleMusic;
	OLumpName finaleFlat;
	OLumpName finalePage[3];
	OLumpName infoPage[3];
	char quitSound[MAX_SNDNAME + 1];
	int maxSwitch;
	OLumpName borderFlat;
	gameborder_t border;
	bool intermissionCounter;
	OLumpName intermissionMusic;
	int defKickback;
	OLumpName endoom;
	OLumpName pauseSign;
	float gibFactor;
	int telefogHeight;
	int textScreenX;
	int textScreenY;

	// automap features
	am_default_colors_t defaultAutomapColors;
	am_colors_t currentAutomapColors;
	bool showLocks; // not implemented
	std::vector<mline_t> mapArrow;
	std::vector<mline_t> mapArrowCheat;
	std::vector<mline_t> cheatKey;
	std::vector<mline_t> easyKey;

	std::string titleString;
	OLumpName baseMapinfoLump;
	OLumpName sharewareMapinfoLump;
	OLumpName menuTitle;
	int menuTitleOffsetX;
	std::string bigFont;
	std::string smallFont;
	std::array<OLumpName, 2> menuIndicatorLumps;
	int menuIndicatorOffsetX;
	int menuIndicatorOffsetY;
	int menuCursorOffsetY;
	int defaultWipeType;

	gameinfo_s()
		: flags(0)
		, enginetype(ENGINE_DOOM)
		, demoformat(DEMOFORMAT_DOOM_VANILLA)
		, titlePage("")
		, demoLoop("")
		, creditPages()
		, titleMusic("")
		, titleTime(0)
		, advisoryTime(0)
		, noLoopFinaleMusic(false)
		, pageTime(0)
		, chatSound()
		, finaleMusic("")
		, finaleFlat("")
		, finalePage()
		, infoPage()
		, quitSound()
		, maxSwitch(1)
		, borderFlat("")
		, border()
		, intermissionCounter(true)
		, intermissionMusic("")
		, defKickback(100)
		, endoom("")
		, pauseSign("")
		, gibFactor(1.f)
		, telefogHeight(0)
		, textScreenX(0)
		, textScreenY(0)
		, titleString("Unknown IWAD")
		, baseMapinfoLump("")
		, sharewareMapinfoLump("")
		, menuTitle("")
		, menuTitleOffsetX(0)
		, bigFont("BIGFONT")
		, smallFont("SMALLFONT")
		, menuIndicatorLumps{ "M_SKULL1", "M_SKULL2" }
		, menuIndicatorOffsetX(-32)
		, menuIndicatorOffsetY(-5)
		, menuCursorOffsetY(0)
		, defaultWipeType(1)
	{
	}

} gameinfo_t;

inline gameinfo_t gameinfo;
