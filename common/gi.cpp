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

static const char *quitsounds[8] =
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

static const char *quitsounds2[8] =
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

static gameborder_t DefaultBorder;

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
	"VICTORY2",
	"ENDPIC",
	{ { "HELP1", "HELP2", "CREDIT" } },
	quitsounds,
	1,
	"FLOOR7_2",
	DefaultBorder,
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
	"VICTORY2",
	"ENDPIC",
	{ { "HELP1", "HELP2", "CREDIT" } },
	quitsounds,
	2,
	"FLOOR7_2",
	DefaultBorder,
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
	5,
	"misc/chat2",
	"D_VICTOR",
	"FLOOR4_8",
	"CREDIT",
	"VICTORY2",
	"ENDPIC",
	{ { "HELP1", "CREDIT", "CREDIT"  } },
	quitsounds,
	2,
	"FLOOR7_2",
	DefaultBorder,
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
	"FLOOR4_8",
	"CREDIT",
	"VICTORY2",
	"ENDPIC",
	{ { "HELP1", "CREDIT", "CREDIT"  } },
	quitsounds,
	2,
	"FLOOR7_2",
	DefaultBorder,
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
	quitsounds2,
	3,
	"GRNROCK",
	DefaultBorder,
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
	quitsounds2,
	3,
	"GRNROCK",
	DefaultBorder,
	"DOOM 2: Hell on Earth (BFG Edition)"
};

VERSION_CONTROL (gi_cpp, "$Id$")

