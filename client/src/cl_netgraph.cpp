// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2013 by The Odamex Team.
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
//	Draw statistics about the network connection
//
//-----------------------------------------------------------------------------

#include "math.h"
#include "g_game.h"
#include "v_video.h"
#include "v_text.h"
#include "cl_netgraph.h"

NetGraph::NetGraph(int x, int y) :
	mX(x), mY(y)
{
	for (size_t i = 0; i < NetGraph::MAX_HISTORY_TICS; i++)
	{
		mMisprediction[i] = false;
		mWorldIndexSync[i] = 0;
	}
}

void NetGraph::setMisprediction(bool val)
{
	mMisprediction[gametic % NetGraph::MAX_HISTORY_TICS] = val;
}

void NetGraph::setWorldIndexSync(int val)
{
	if (val > NetGraph::MAX_WORLD_INDEX)
		val = NetGraph::MAX_WORLD_INDEX;
	else if (val < NetGraph::MIN_WORLD_INDEX)
		val = NetGraph::MIN_WORLD_INDEX;

	mWorldIndexSync[gametic % NetGraph::MAX_HISTORY_TICS] = val;
}

void NetGraph::setInterpolation(int val)
{
	mInterpolation = val;
}

extern "C" byte **ylookup;
extern "C" int *columnofs;

static void NetGraphDrawBar(int startx, int starty, int width, int height, int color)
{
	for (int x = startx; x < startx + width; x++)
		for (int y = starty; y < starty + height; y++)
		{
			byte *dest = ylookup[y] + columnofs[x];
			*dest = color;
		}
}

void NetGraph::drawWorldIndexSync(int x, int y)
{
	const int centery = y + NetGraph::MAX_WORLD_INDEX * NetGraph::BAR_HEIGHT_WORLD_INDEX;
	const int graphwidth = NetGraph::BAR_WIDTH_WORLD_INDEX * NetGraph::MAX_HISTORY_TICS;

	// draw the center line
	for (size_t i = 0; i < NetGraph::MAX_HISTORY_TICS; i++)
		NetGraphDrawBar(x, centery, graphwidth, 1, 0);

	for (size_t i = 0; i < NetGraph::MAX_HISTORY_TICS; i++)
	{
		int index = (gametic - (NetGraph::MAX_HISTORY_TICS - i)) % MAX_HISTORY_TICS;
		int width = NetGraph::BAR_WIDTH_WORLD_INDEX;
		int height = abs(mWorldIndexSync[index] * NetGraph::BAR_HEIGHT_WORLD_INDEX);
		int startx = x + i * NetGraph::BAR_WIDTH_WORLD_INDEX;
		int starty, color;
		if (mWorldIndexSync[index] >= 0)
		{
			color = 160;
			starty = centery - mWorldIndexSync[index] * NetGraph::BAR_HEIGHT_WORLD_INDEX;
		}
		else
		{
			color = 152;
			starty = centery - (mWorldIndexSync[index] + 1) * NetGraph::BAR_HEIGHT_WORLD_INDEX;
		}

		if (height != 0)
			NetGraphDrawBar(startx, starty, width, height, color);
	}

	// draw the interpolation line
	if (mInterpolation > 0)
	{
		int liney = centery - NetGraph::BAR_HEIGHT_WORLD_INDEX * mInterpolation;
		for (size_t i = 0; i < NetGraph::MAX_HISTORY_TICS; i++)
			NetGraphDrawBar(x, liney, graphwidth, 1, 1);
	}
}

void NetGraph::drawMispredictions(int x, int y)
{
	const int graphwidth = NetGraph::BAR_WIDTH_MISPREDICTION * NetGraph::MAX_HISTORY_TICS;
	const int centery = y + NetGraph::BAR_HEIGHT_MISPREDICTION;

	// draw the center line
	for (size_t i = 0; i < NetGraph::MAX_HISTORY_TICS; i++)
		NetGraphDrawBar(x, centery, graphwidth, 1, 0);

	for (size_t i = 0; i < NetGraph::MAX_HISTORY_TICS; i++)
	{
		int index = (gametic - (NetGraph::MAX_HISTORY_TICS - i)) % MAX_HISTORY_TICS;
		int width = NetGraph::BAR_WIDTH_MISPREDICTION;
		int height = NetGraph::BAR_HEIGHT_MISPREDICTION;
		int startx = x + i * NetGraph::BAR_WIDTH_MISPREDICTION;
		int starty = y;

		if (mMisprediction[index])
			NetGraphDrawBar(startx, starty, width, height, 0xB0);
	}
}

void NetGraph::draw()
{
	static const int textcolor = CR_GREY;
	static const int fontheight = 8;

    screen->DrawText(textcolor, mX, mY, "World Index Sync");
	drawWorldIndexSync(mX, mY + fontheight);

    screen->DrawText(textcolor, mX, mY + 64, "Mispredictions");
	drawMispredictions(mX, mY + 64 + fontheight);
}

VERSION_CONTROL (cl_netgraph_cpp, "$Id$")
