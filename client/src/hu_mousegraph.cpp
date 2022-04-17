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
//	Draw statistics about mouse input.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "hu_mousegraph.h"

#include "v_text.h"
#include "v_video.h"
#include "i_video.h"

MouseGraph mousegraph;

MouseGraph::MouseGraph() : index(MouseGraph::MAX_HISTORY_TICS)
{
	for (int i = 0;i < MouseGraph::MAX_HISTORY_TICS;i++)
	{
		this->mousex[i] = 0;
		this->mousey[i] = 0;
	}
}

void MouseGraph::append(int x, int y)
{
	this->index = (this->index + 1) % MAX_HISTORY_TICS;
	this->mousex[this->index] = x;
	this->mousey[this->index] = y;
}

void MouseGraph::draw(int type)
{
	const int x = I_GetSurfaceWidth() - MAX_HISTORY_TICS - 10;
	const int y = 100;

	screen->DrawText(CR_GREY, x, y, "Mouse X/Y");
	if (type == TYPE_LINE)
		this->drawLine(x, y + 8);
	else if (type == TYPE_PLOT)
		this->drawPlot(x, y + 8);
}

void MouseGraph::drawPlot(int x1, int y1)
{
	int limit = 128;

	const int x2 = x1 + MAX_HISTORY_TICS;
	const int y2 = y1 + MAX_HISTORY_TICS;
	const int xc = ((x2 - x1) / 2) + x1;
	const int yc = ((y2 - y1) / 2) + y1;

	// Draw the background
	const argb_t bgcolor1(V_GetDefaultPalette()->basecolors[187]);
	const argb_t bgcolor2(V_GetDefaultPalette()->basecolors[207]);

	screen->Clear(xc - 1, y1, xc, y2, bgcolor1);
	screen->Clear(x1, yc - 1, x2, yc, bgcolor2);

	for (int i = MAX_HISTORY_TICS - 1; i >= 0; i--)
	{
		const int in = (MAX_HISTORY_TICS + this->index - i) % MAX_HISTORY_TICS;

		if (abs(mousex[in]) > limit)
			limit = abs(mousex[in]);
		if (abs(mousey[in]) > limit)
			limit = abs(mousey[in]);

		const int scaledx = (mousex[in] * (MAX_HISTORY_TICS / 2)) / limit;
		const int scaledy = (mousey[in] * (MAX_HISTORY_TICS / 2)) / limit;

		argb_t color;
		
		if (i == 0)
			color = V_GetDefaultPalette()->basecolors[160];
		else
			color = V_GetDefaultPalette()->basecolors[80 + i/2];

		screen->Clear(xc + scaledx - 1, yc - scaledy - 1, xc + scaledx, yc - scaledy, color);
	}
}

void MouseGraph::drawLine(int x1, int y1)
{
	int limit = 128;

	const int x2 = x1 + MAX_HISTORY_TICS;
	const int y2 = y1 + MAX_HISTORY_TICS;
	const int xc = ((x2 - x1) / 2) + x1;
	const int yc = ((y2 - y1) / 2) + y1;

	// Draw the background
	const argb_t bgcolor1(V_GetDefaultPalette()->basecolors[187]);
	const argb_t bgcolor2(V_GetDefaultPalette()->basecolors[207]);

	const argb_t color1(V_GetDefaultPalette()->basecolors[200]);
	const argb_t color2(V_GetDefaultPalette()->basecolors[176]);

	screen->Clear(xc - 1, y1, xc, y2, bgcolor1);
	screen->Clear(x1, yc - 1, x2, yc, bgcolor2);

	for (int i = MAX_HISTORY_TICS - 1;i >= 0;i--)
	{
		const int in = (MAX_HISTORY_TICS + this->index - i) % MAX_HISTORY_TICS;

		if (abs(mousex[in]) > limit)
			limit = abs(mousex[in]);
		if (abs(mousey[in]) > limit)
			limit = abs(mousey[in]);

		const int scaledx = (mousex[in] * (MAX_HISTORY_TICS / 2)) / limit;
		const int scaledy = (mousey[in] * (MAX_HISTORY_TICS / 2)) / limit;

		screen->Clear(x1 + i - 1, yc - scaledy - 1, x1 + i, yc - scaledy, color1);
		screen->Clear(xc + scaledx - 1, y2 - i - 1, xc + scaledx, y2 - i, color2);
	}
}
