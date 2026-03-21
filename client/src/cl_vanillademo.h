//-----------------------------------------------------------------------------
//
// Copyright (C) 2006-2026 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
//-----------------------------------------------------------------------------

#pragma once

#include "doomdef.h"
#include "gi.h"

struct demo_header_t
{
	demoformat_t format = DEMOFORMAT_DOOM_VANILLA;
	byte skill = 0;
	byte episode = 1;
	byte map = 1;
	byte deathmatch = 0;
	bool monstersrespawn = false;
	bool fastmonsters = false;
	bool nomonsters = false;
	byte consoleplayer = 0;
	byte playerPresent[MAXPLAYERS_VANILLA] = {};
};

bool G_TryReadDemoHeader(demo_header_t& header, demoformat_t format, byte*& demo_p, byte* demo_e);
void G_ReadDemoTiccmd(byte*& demo_p, byte* demo_e);
const char* G_GetCurrentDemoCodecName();
void G_ClearDemoCodec();
