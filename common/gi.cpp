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

static gameborder_t DefaultBorder;

gameinfo_t DefaultGameInfo =
{
	0,
	"",
	"",
	"",
	"",
	0,
	0,
	false,
	0,
	"",
	"",
	"",
	"",
	"",
	"",
	{ "", "", "" },
	NULL,
	1,
	"0",
	DefaultBorder,
	true,
	"",
	100,
	"",
	"",
	1.f,
	0,
	0,
	0,
	"Unknown IWAD"
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
	false,
	5,
	"misc/chat2",
	"D_VICTOR",
	"FLOOR4_8",
	"HELP2",
	"VICTORY2",
	"ENDPIC",
	{ "HELP1", "HELP2", "CREDIT" },
	"menu/quit1",
	1,
	"FLOOR7_2",
	DefaultBorder,
	true,
	"",
	100,
	"",
	"",
	1.f,
	0,
	0,
	0,
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
	false,
	5,
	"misc/chat2",
	"D_VICTOR",
	"FLOOR4_8",
	"HELP2",
	"VICTORY2",
	"ENDPIC",
	{ "HELP1", "HELP2", "CREDIT" },
	"menu/quit1",
	2,
	"FLOOR7_2",
	DefaultBorder,
	true,
	"",
	100,
	"",
	"",
	1.f,
	0,
	0,
	0,
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
	false,
	5,
	"misc/chat2",
	"D_VICTOR",
	"FLOOR4_8",
	"CREDIT",
	"VICTORY2",
	"ENDPIC",
	{ "HELP1", "CREDIT", "CREDIT"  },
	"menu/quit1",
	2,
	"FLOOR7_2",
	DefaultBorder,
	true,
	"",
	100,
	"",
	"",
	1.f,
	0,
	0,
	0,
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
	false,
	5,
	"misc/chat2",
	"D_VICTOR",
	"FLOOR4_8",
	"CREDIT",
	"VICTORY2",
	"ENDPIC",
	{ "HELP1", "CREDIT", "CREDIT"  },
	"menu/quit1",
	2,
	"FLOOR7_2",
	DefaultBorder,
	true,
	"",
	100,
	"",
	"",
	1.f,
	0,
	0,
	0,
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
	false,
	5,
	"misc/chat",
	"D_READ_M",
	"SLIME16",
	"CREDIT",
	"CREDIT",
	"CREDIT",
	{ "HELP", "CREDIT", "CREDIT" },
	"menu/quit2",
	3,
	"GRNROCK",
	DefaultBorder,
	true,
	"",
	100,
	"",
	"",
	1.f,
	0,
	0,
	0,
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
	false,
	5,
	"misc/chat",
	"D_READ_M",
	"SLIME16",
	"CREDIT",
	"CREDIT",
	"CREDIT",
	{ "HELP", "CREDIT", "CREDIT" },
	"menu/quit2",
	3,
	"GRNROCK",
	DefaultBorder,
	true,
	"",
	100,
	"",
	"",
	1.f,
	0,
	0,
	0,
	"DOOM 2: Hell on Earth (BFG Edition)"
};

VERSION_CONTROL (gi_cpp, "$Id$")

