// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Situation Report
//
//-----------------------------------------------------------------------------

#include "hu_sitrep.h"

#include "cmdlib.h"
#include "d_player.h"
#include "hu_drawers.h"
#include "v_text.h"
#include "v_video.h"

namespace hud
{

void SitRep()
{
	int y = 0;
	float scale = 1.0f;
	x_align_t x_align = X_LEFT;
	y_align_t y_align = Y_TOP;
	x_align_t x_origin = X_LEFT;
	y_align_t y_origin = Y_TOP;

	// Find the widest name.
	int maxwidth = 0;
	for (Players::iterator it = players.begin(); it != players.end(); ++it)
	{
		int width = V_StringWidth(it->userinfo.netname.c_str());
		maxwidth = MAX(maxwidth, width);
	}

	std::string buf;
	int iy = y;
	for (Players::iterator it = players.begin(); it != players.end(); ++it)
	{
		int ix = 0;

		// Name
		hud::DrawText(ix, iy, scale, x_align, y_align, x_origin, y_origin,
		              it->userinfo.netname.c_str(), CR_GREY);

		// Only draw health and armor if they have a body.
		if (it->mo)
		{
			// Health
			ix += maxwidth;
			StrFormat(buf, "%4d", it->mo->health);
			hud::DrawText(ix, iy, scale, x_align, y_align, x_origin, y_origin,
			              buf.c_str(), CR_GREY);

			// Armor
			ix += V_StringWidth(buf.c_str());
			StrFormat(buf, "%4d", it->armorpoints);
			hud::DrawText(ix, iy, scale, x_align, y_align, x_origin, y_origin,
			              buf.c_str(), CR_GREY);
		}

		iy += 8;
	}
}

} // namespace hud
