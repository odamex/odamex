// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//   Game functions that are common between the client and server.
//
//-----------------------------------------------------------------------------

#include <utility>

#include "odamex.h"

#include "g_game.h"

void G_SetFast(const bool wantFast)
{
	static bool isFast = false;
	if (wantFast != isFast)
	{
		for (auto&& [_, minfo] : mobjinfo)
		{
			if (minfo.altspeed != NO_ALTSPEED)
				std::swap(minfo.speed, minfo.altspeed);
		}

		if (wantFast)
		{
			for (auto&& [_, state] : states)
			{
				if (state.flags & STATEF_SKILL5FAST && (state.tics != 1 || demoplayback))
					state.tics >>= 1; // don't change 1->0 since it causes cycles
			}
		}
		else
		{
			for (auto&& [_, state] : states)
			{
				if (state.flags & STATEF_SKILL5FAST)
					state.tics <<= 1;
			}
		}

		isFast = wantFast;
	}
}