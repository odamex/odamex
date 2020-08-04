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
#include "i_video.h"
#include "v_text.h"
#include "v_video.h"

float hud_sitrep_scale = 0.75f;
int hud_sitrep_x = 2;
int hud_sitrep_y = 0;
int hud_sitrep_origin = 9;

namespace hud
{

static void ToLeftTop(int& x, int& y, const unsigned short w, const unsigned short h,
                      const float scale, const x_align_t x_align, const y_align_t y_align)
{
	// Get our clean scale factors.
	int x_scale = MAX(1, int(scale * ::CleanXfac));
	int y_scale = MAX(1, int(scale * ::CleanYfac));

	int scaled_width = I_GetSurfaceWidth() / x_scale;
	int scaled_height = I_GetSurfaceHeight() / y_scale;

	switch (x_align)
	{
	case X_LEFT:
		x = x;
		break;
	case X_CENTER:
		x = (scaled_width / 2) - (w / 2) + x;
		break;
	case X_RIGHT:
		x = scaled_width - x - w;
		break;
	default:
		break;
	}

	switch (y_align)
	{
	case Y_TOP:
		y = y;
		break;
	case Y_MIDDLE:
		y = (scaled_height / 2) - (h / 2) + y;
		break;
	case Y_BOTTOM:
		y = scaled_height - y - h;
		break;
	default:
		break;
	}
}

void SitRep()
{
	const int ROW_HEIGHT = 8;
	const int BORDER_WIDTH = 2;

	int x = hud_sitrep_x;
	int y = hud_sitrep_y;
	int w = 0;
	int h = 0;

	// Find the widest name.
	int maxwidth = 0;
	for (Players::iterator it = players.begin(); it != players.end(); ++it)
	{
		if (it->ingame() && !it->spectator)
			continue;

		int width = V_StringWidth(it->userinfo.netname.c_str());
		maxwidth = MAX(maxwidth, width);
		h += ROW_HEIGHT;
	}
	w += maxwidth + BORDER_WIDTH * 2;
	h += BORDER_WIDTH * 2;

	// Fix our origin so we can draw assuming a top-left origin.
	ToLeftTop(x, y, w, h, hud_sitrep_scale, X_LEFT, Y_MIDDLE);

	// Dim the background.
	hud::Dim(x, y, w, h, hud_sitrep_scale, X_LEFT, Y_TOP, X_LEFT, Y_TOP);

	std::string buf;
	int iy = y + BORDER_WIDTH;
	for (Players::iterator it = players.begin(); it != players.end(); ++it)
	{
		if (it->ingame() && !it->spectator)
			continue;

		int ix = x + BORDER_WIDTH;

		// Name
		hud::DrawText(ix, iy, hud_sitrep_scale, X_LEFT, Y_TOP, X_LEFT, Y_TOP,
		              it->userinfo.netname.c_str(), CR_GREY);

		// Only draw health and armor if they have a body.
		if (it->mo)
		{
			// Health
			ix += maxwidth;
			StrFormat(buf, "%4d", it->mo->health);
			hud::DrawText(ix, iy, hud_sitrep_scale, X_LEFT, Y_TOP, X_LEFT, Y_TOP,
			              buf.c_str(), CR_GREY);

			// Armor
			ix += V_StringWidth(buf.c_str());
			StrFormat(buf, "%4d", it->armorpoints);
			hud::DrawText(ix, iy, hud_sitrep_scale, X_LEFT, Y_TOP, X_LEFT, Y_TOP,
			              buf.c_str(), CR_GREY);
		}

		iy += 8;
	}
}

} // namespace hud
