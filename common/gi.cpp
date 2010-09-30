// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2010 by The Odamex Team.
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


#include <stdio.h>
#include <stdlib.h>
#include "info.h"
#include "gi.h"
#include "st_stuff.h"

gameinfo_t gameinfo;


//
// The vector graphics for the automap.
//  A line drawing of the player pointing right,
//   starting from the middle.
//
#define R ((8*PLAYERRADIUS)/7)
static mline_t player_arrow[] = {
	{ { -R+R/8, 0 }, { R, 0 } }, // -----
	{ { R, 0 }, { R-R/2, R/4 } },  // ----->
	{ { R, 0 }, { R-R/2, -R/4 } },
	{ { -R+R/8, 0 }, { -R-R/8, R/4 } }, // >---->
	{ { -R+R/8, 0 }, { -R-R/8, -R/4 } },
	{ { -R+3*R/8, 0 }, { -R+R/8, R/4 } }, // >>--->
	{ { -R+3*R/8, 0 }, { -R+R/8, -R/4 } }
};

static mline_t heretic_player_arrow[] = {
  { { -R+R/4, 0 }, { 0, 0} }, // center line.
  { { -R+R/4, R/8 }, { R, 0} }, // blade
  { { -R+R/4, -R/8 }, { R, 0 } },
  { { -R+R/4, -R/4 }, { -R+R/4, R/4 } }, // crosspiece
  { { -R+R/8, -R/4 }, { -R+R/8, R/4 } },
  { { -R+R/8, -R/4 }, { -R+R/4, -R/4} }, //crosspiece connectors
  { { -R+R/8, R/4 }, { -R+R/4, R/4} },
  { { -R-R/4, R/8 }, { -R-R/4, -R/8 } }, //pommel
  { { -R-R/4, R/8 }, { -R+R/8, R/8 } },
  { { -R-R/4, -R/8}, { -R+R/8, -R/8 } }
  };
#undef R

#define NUMPLYRLINES (sizeof(player_arrow)/sizeof(mline_t))
#define NUMHTICPLYRLINES (sizeof(heretic_player_arrow)/sizeof(mline_t))

const char *GameNames[5] =
{
	NULL, "Doom", "Heretic", NULL, "Hexen"
};

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

static gameborder_t HereticBorder =
{
	4, 16,
	"bordtl", "bordt", "bordtr",
	"bordl",           "bordr",
	"bordbl", "bordb", "bordbr"
};

//
// Automap Objects for GameInfo
//
#define DOOMMARKS    "AMMNUM%d"
#define HTICMARKS    "SMALLIN%d"

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

gameinfo_t HereticGameInfo =
{
	GI_PAGESARERAW | GI_INFOINDEXED,
	"TITLE",
	"CREDIT",
	"CREDIT",
	{ 'M','U','S','_','T','I','T','L' },
	280/35,
	210/35,
	200/35,
	"misc/chat",
	"TITLE",
	{ 'M','U','S','_','C','P','T','D' },
	"FLOOR25",
	"CREDIT",
	"CREDIT",
	"CREDIT",
	{ { "HELP1", "HELP2", "CREDIT" } },
	NULL,
	17,
	"FLAT513",
	"ENDTEXT",
	&HticStatusBar,
	HTICMARKS,        // markNumFmt
	heretic_player_arrow,
	NUMHTICPLYRLINES,
	&HereticBorder,
	GAME_Heretic,
	hticmobjinfo
};

gameinfo_t HereticSWGameInfo =
{
	GI_PAGESARERAW | GI_SHAREWARE | GI_INFOINDEXED,
	"TITLE",
	"CREDIT",
	"ORDER",
	{ 'M','U','S','_','T','I','T','L' },
	280/35,
	210/35,
	200/35,
	"misc/chat",
	"TITLE",
	{ 'M','U','S','_','C','P','T','D' },
	"FLOOR25",
	"ORDER",
	"CREDIT",
	"CREDIT",
	{ { "TITLE", 5 } },
	NULL,
	17,
	"FLOOR04",
	"ENDTEXT",
	&HticStatusBar,
	HTICMARKS,        // markNumFmt	
	heretic_player_arrow,
	NUMHTICPLYRLINES,	
	&HereticBorder,
	GAME_Heretic,
	hticmobjinfo
};

gameinfo_t SharewareGameInfo =
{
	GI_SHAREWARE | GI_NOCRAZYDEATH,
	{ 'T','I','T','L','E','P','I','C' },
	"CREDIT",
	"HELP2",
	"D_INTRO",
	5,
	0,
	200/35,
	"misc/chat2",
	"CONBACK",
	{ 'D','_','V','I','C','T','O','R' },
	{ 'F','L','O','O','R','4','_','8' },
	"HELP2",
	{ 'V','I','C','T','O','R','Y','2' },
	"ENDPIC",
	{ { "HELP1", "HELP2", "CREDIT" } },
	doomquitsounds,
	1,
	{ 'F','L','O','O','R','7','_','2' },
	"ENDOOM",
	&DoomStatusBar,
	DOOMMARKS,
	player_arrow,
	NUMHTICPLYRLINES,	
	&DoomBorder,
	GAME_Doom,
	mobjinfo
};

gameinfo_t RegisteredGameInfo =
{
	GI_NOCRAZYDEATH,
	{ 'T','I','T','L','E','P','I','C' },
	"CREDIT",
	"HELP2",
	"D_INTRO",
	5,
	0,
	200/35,
	"misc/chat2",
	"CONBACK",
	{ 'D','_','V','I','C','T','O','R' },
	{ 'F','L','O','O','R','4','_','8' },
	"HELP2",
	{ 'V','I','C','T','O','R','Y','2' },
	"ENDPIC",
	{ { "HELP1", "HELP2", "CREDIT" } },
	doomquitsounds,
	2,
	{ 'F','L','O','O','R','7','_','2' },
	"ENDOOM",
	&DoomStatusBar,
	DOOMMARKS,
	player_arrow,
	NUMPLYRLINES,
	&DoomBorder,
	GAME_Doom,
	mobjinfo
};

gameinfo_t RetailGameInfo =
{
	GI_MENUHACK_RETAIL | GI_NOCRAZYDEATH,
	{ 'T','I','T','L','E','P','I','C' },
	"CREDIT",
	"CREDIT",
	"D_INTRO",
	5,
	0,
	200/35,
	"misc/chat2",
	"CONBACK",
	{ 'D','_','V','I','C','T','O','R' },
	{ 'F','L','O','O','R','4','_','8' },
	"CREDIT",
	{ 'V','I','C','T','O','R','Y','2' },
	"ENDPIC",
	{ { "HELP1", "CREDIT", "CREDIT"  } },
	doomquitsounds,
	2,
	{ 'F','L','O','O','R','7','_','2' },
	"ENDOOM",
	&DoomStatusBar,
	DOOMMARKS,
	player_arrow,
	NUMPLYRLINES,	
	&DoomBorder,
	GAME_Doom,
	mobjinfo
};

gameinfo_t CommercialGameInfo =
{
	GI_MAPxx | GI_MENUHACK_COMMERCIAL,
	{ 'T','I','T','L','E','P','I','C' },
	"CREDIT",
	"CREDIT",
	{ 'D','_','D','M','2','T','T','L' },
	11,
	0,
	200/35,
	"misc/chat",
	"CONBACK",
	{ 'D','_','R','E','A','D','_','M' },
	"SLIME16",
	"CREDIT",
	"CREDIT",
	"CREDIT",
	{ { "HELP", "CREDIT", "CREDIT" } },
	doom2quitsounds,
	3,
	"GRNROCK",
	"ENDOOM",
	&DoomStatusBar,
	DOOMMARKS,
	player_arrow,
	NUMPLYRLINES,	
	&DoomBorder,
	GAME_Doom,
	mobjinfo
};

VERSION_CONTROL (gi_cpp, "$Id$")
