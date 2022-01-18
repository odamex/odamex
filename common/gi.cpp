// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	GI
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "gi.h"

gameinfo_t gameinfo;

static const char *doomquitsounds[8] =
{
	"player/male/death1",
	"demon/pain",
	"grunt/pain",
	"misc/gibbed",
	"misc/teleport",
	"grunt/sight1",
	"grunt/sight3",
	"demon/melee"
};

static const char *doom2quitsounds[8] =
{
	"vile/active",
	"misc/p_pkup",
	"brain/cube",
	"misc/gibbed",
	"skeleton/swing",
	"knight/death",
	"baby/active",
	"demon/melee"
};

static gameborder_t DoomBorder =
{
	8, 8,
	"brdr_tl", "brdr_t", "brdr_tr",
	"brdr_l",			 "brdr_r",
	"brdr_bl", "brdr_b", "brdr_br"
};

//
// Status Bar Object for GameInfo
//
stbarfns_t HticStatusBar =
{
	42,
	
	ST_HticTicker,
	ST_HticDrawer,
	ST_HticStart,
	ST_HticInit,
};

stbarfns_t DoomStatusBar =
{
	32,
	
	ST_DoomTicker,
	ST_DoomDrawer,
	ST_DoomStart,
	ST_DoomInit
};

gameinfo_t SharewareGameInfo =
{
	GI_SHAREWARE | GI_NOCRAZYDEATH,
	"TITLEPIC",
	"CREDIT",
	"HELP2",
	"D_INTRO",
	5,
	0,
	200/35,
	"misc/chat2",
	"D_VICTOR",
	"FLOOR4_8",
	"HELP2",
	{ 'V','I','C','T','O','R','Y','2' },
	"ENDPIC",
	{ { "HELP1", "HELP2", "CREDIT" } },
	doomquitsounds,
	1,
	{ 'F','L','O','O','R','7','_','2' },
	&DoomBorder,
	&DoomStatusBar,
	"DOOM Shareware"
};

gameinfo_t RegisteredGameInfo =
{
	GI_NOCRAZYDEATH,
	"TITLEPIC",
	"CREDIT",
	"HELP2",
	"D_INTRO",
	5,
	0,
	200/35,
	"misc/chat2",
	"D_VICTOR",
	"FLOOR4_8",
	"HELP2",
	{ 'V','I','C','T','O','R','Y','2' },
	"ENDPIC",
	{ { "HELP1", "HELP2", "CREDIT" } },
	doomquitsounds,
	2,
	{ 'F','L','O','O','R','7','_','2' },
	&DoomBorder,
	&DoomStatusBar,
	"DOOM Registered"
};

gameinfo_t RetailGameInfo =
{
	GI_MENUHACK_RETAIL | GI_NOCRAZYDEATH,
	"TITLEPIC",
	"CREDIT",
	"CREDIT",
	"D_INTRO",
	5,
	0,
	200/35,
	"misc/chat2",
	"D_VICTOR",
	"FLOOR4_8",
	"CREDIT",
	{ 'V','I','C','T','O','R','Y','2' },
	"ENDPIC",
	{ { "HELP1", "CREDIT", "CREDIT"  } },
	doomquitsounds,
	2,
	{ 'F','L','O','O','R','7','_','2' },
	&DoomBorder,
	&DoomStatusBar,
	"The Ultimate DOOM"
};

gameinfo_t RetailBFGGameInfo =
{
	GI_MENUHACK_RETAIL | GI_NOCRAZYDEATH,
	"TITLEPIC",
	"CREDIT",
	"CREDIT",
	"D_INTRO",
	5,
	0,
	200/35,
	"misc/chat2",
	"D_VICTOR",
	"FLOOr4_8",
	"CREDIT",
	{ 'V','I','C','T','O','R','Y','2' },
	"ENDPIC",
	{ { "HELP1", "CREDIT", "CREDIT"  } },
	doomquitsounds,
	2,
	{ 'F','L','O','O','R','7','_','2' },
	&DoomBorder,
	&DoomStatusBar,
	"The Ultimate DOOM (BFG Edition)"
};

gameinfo_t CommercialGameInfo =
{
	GI_MAPxx | GI_MENUHACK_COMMERCIAL,
	"TITLEPIC",
	"CREDIT",
	"CREDIT",
	"D_DM2TTL",
	11,
	0,
	200/35,
	"misc/chat",
	"D_READ_M",
	"SLIME16",
	"CREDIT",
	"CREDIT",
	"CREDIT",
	{ { "HELP", "CREDIT", "CREDIT" } },
	doom2quitsounds,
	3,
	"GRNROCK",
	&DoomBorder,
	&DoomStatusBar,
	"DOOM 2: Hell on Earth"
};

gameinfo_t CommercialBFGGameInfo =
{
	GI_MAPxx | GI_MENUHACK_COMMERCIAL,
	"INTERPIC",
	"CREDIT",
	"CREDIT",
	"D_DM2TTL",
	11,
	0,
	200/35,
	"misc/chat",
	"D_READ_M",
	"SLIME16",
	"CREDIT",
	"CREDIT",
	"CREDIT",
	{ { "HELP", "CREDIT", "CREDIT" } },
	doom2quitsounds,
	3,
	"GRNROCK",
	&DoomBorder,
	&DoomStatusBar,
	"DOOM 2: Hell on Earth (BFG Edition)"
};

static gameborder_t HereticBorder =
{
	4, 16,
	"bordtl", "bordt", "bordtr",
	"bordl",           "bordr",
	"bordbl", "bordb", "bordbr"
};

gameinfo_t HereticGameInfo =
{
	GI_PAGESARERAW | GI_INFOINDEXED,
	"TITLE",
	"CREDIT",
	"CREDIT",
	"MUS_TITL",
	280/35,
	210/35,
	200/35,
	"misc/chat",
	"MUS_CPTD",
	"FLOOR25",
	"CREDIT",
	"CREDIT",
	"CREDIT",
	{ { "HELP1", "HELP2", "CREDIT" } },
	NULL,
	17,
	"FLAT513",
    &HereticBorder,
	&HticStatusBar,
    "Heretic"
};

gameinfo_t HereticSWGameInfo =
{
	GI_PAGESARERAW | GI_SHAREWARE | GI_INFOINDEXED,
	"TITLE",
	"CREDIT",
	"ORDER",
	"MUS_TITL",
	280/35,
	210/35,
	200/35,
	"misc/chat",
	"MUS_CPTD",
	"FLOOR25",
	"ORDER",
	"CREDIT",
	"CREDIT",
	{ { "TITLE", 5 } },
	NULL,
	17,
	"FLOOR04",
    &HereticBorder,
	&HticStatusBar,
	"Heretic (Shareware)"
};

VERSION_CONTROL (gi_cpp, "$Id$")

