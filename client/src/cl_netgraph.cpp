// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	Draw statistics about the network connection
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <sstream>
#include <iomanip>

#include "math.h"
#include "g_game.h"
#include "v_video.h"
#include "v_text.h"
#include "cl_netgraph.h"
#include "r_draw.h"

#include "SequenceQueueEntryType.h"

NetGraph::NetGraph(int x, int y) :
	mX(x), mY(y), mInterpolation(0)
{
	mMisprediction.fill(false);
	mWorldIndexSync.fill(0);
	mTrafficIn.fill(0);
	mTrafficOut.fill(0);
	mPacketsIn.fill(0);
	mReliableSendDepth.fill(0);
}

template <typename ElementType, size_t N>
static void SetClamped(std::array<ElementType, N>& io_array, const ElementType& i_value, const ElementType& i_min, const ElementType& i_max)
{
    io_array[gametic % N] = std::max(i_min, std::min(i_value, i_max));
}

void NetGraph::setMisprediction(bool val)
{
	mMisprediction[gametic % NetGraph::MAX_HISTORY_TICS] = val;
}

void NetGraph::setWorldIndexSync(int val)
{
    SetClamped(mWorldIndexSync, val, NetGraph::MIN_WORLD_INDEX, NetGraph::MAX_WORLD_INDEX);
}

void NetGraph::setReliableSendDepth(int val)
{
    SetClamped(mReliableSendDepth, val, 0, static_cast<int>(DEFAULT_RELIABILITY_QUEUE_SIZE));
}

void NetGraph::setReliableNonContiguousRetransmits(int val)
{
    SetClamped(mReliableNonContiguousRetransmits, val, 0, static_cast<int>(DEFAULT_RELIABILITY_QUEUE_SIZE));
}

void NetGraph::addTrafficIn(int val)
{
	static int lastgametic = -1;
	if (gametic == lastgametic)
		mTrafficIn[gametic % NetGraph::MAX_HISTORY_TICS] += val;
	else
		mTrafficIn[gametic % NetGraph::MAX_HISTORY_TICS] = val;

	lastgametic = gametic;
}

void NetGraph::addTrafficOut(int val)
{
	static int lastgametic = -1;
	if (gametic == lastgametic)
		mTrafficOut[gametic % NetGraph::MAX_HISTORY_TICS] += val;
	else
		mTrafficOut[gametic % NetGraph::MAX_HISTORY_TICS] = val;

	lastgametic = gametic;
}

void NetGraph::addPacketIn()
{
	static int lastgametic = -1;
	if (gametic == lastgametic)
		mPacketsIn[gametic % NetGraph::MAX_HISTORY_TICS] += 1;
	else
		mPacketsIn[gametic % NetGraph::MAX_HISTORY_TICS] = 1;

	lastgametic = gametic;
}

void NetGraph::setInterpolation(int val)
{
	mInterpolation = val;
}

static void NetGraphDrawBar(int startx, int starty, int width, int height, int color)
{
	if (starty + height >= viewheight)
		height = viewheight - starty - 1;

	dspan.color = color;
	dspan.x1 = startx;
	dspan.x2 = startx + width - 1;

	for (dspan.y = starty; dspan.y < starty + height; dspan.y++)
		R_FillSpan();
}

void NetGraph::drawWorldIndexSync(int x, int y)
{
	static constexpr int graphwidth = NetGraph::BAR_WIDTH_WORLD_INDEX * NetGraph::MAX_HISTORY_TICS;
	const int centery = y + NetGraph::MAX_WORLD_INDEX * NetGraph::BAR_HEIGHT_WORLD_INDEX;

	// draw the center line
	for (size_t i = 0; i < NetGraph::MAX_HISTORY_TICS; i++)
		NetGraphDrawBar(x, centery, graphwidth, 1, 0);

	for (size_t i = 0; i < NetGraph::MAX_HISTORY_TICS; i++)
	{
		const int index = (gametic - (NetGraph::MAX_HISTORY_TICS - i)) % MAX_HISTORY_TICS;
		static constexpr int width = NetGraph::BAR_WIDTH_WORLD_INDEX;
		const int height = abs(mWorldIndexSync[index] * NetGraph::BAR_HEIGHT_WORLD_INDEX);
		const int startx = x + i * NetGraph::BAR_WIDTH_WORLD_INDEX;
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
		const int liney = centery - NetGraph::BAR_HEIGHT_WORLD_INDEX * mInterpolation;
		for (size_t i = 0; i < NetGraph::MAX_HISTORY_TICS; i++)
			NetGraphDrawBar(x, liney, graphwidth, 1, 1);
	}
}

template <int BAR_WIDTH, int BAR_UNIT_HEIGHT, typename ElementType, size_t N >
static void DrawSimpleBarGraph(int x, int y, const std::array<ElementType, N>& i_data, int color)
{
	static constexpr int graphwidth = BAR_WIDTH * static_cast<int>(N);
	const int centery = y + BAR_UNIT_HEIGHT;

	// draw the center line
	for (size_t i = 0; i < N; i++)
		NetGraphDrawBar(x, centery, graphwidth, 1, 0);

	for (size_t i = 0; i < N; i++)
	{
		const int index = (gametic - (N - i)) % N;
		constexpr int width = BAR_WIDTH;
		const int height = static_cast<int>(i_data[index]) * BAR_UNIT_HEIGHT;
		const int startx = x + i * BAR_WIDTH;
		const int starty = y;

		if (i_data[index])
			NetGraphDrawBar(startx, starty, width, height, color);
	}
}

void NetGraph::drawReliableSendDepth(int x, int y)
{
    DrawSimpleBarGraph<2, 4>(x, y, mReliableSendDepth,                0xA0);    // yellow
    DrawSimpleBarGraph<2, 4>(x, y, mReliableNonContiguousRetransmits, 0xB0);    // red
}

void NetGraph::drawMispredictions(int x, int y)
{
	static constexpr int graphwidth = NetGraph::BAR_WIDTH_MISPREDICTION * NetGraph::MAX_HISTORY_TICS;
	const int centery = y + NetGraph::BAR_HEIGHT_MISPREDICTION;

	// draw the center line
	for (size_t i = 0; i < NetGraph::MAX_HISTORY_TICS; i++)
		NetGraphDrawBar(x, centery, graphwidth, 1, 0);

	for (size_t i = 0; i < NetGraph::MAX_HISTORY_TICS; i++)
	{
		const int index = (gametic - (NetGraph::MAX_HISTORY_TICS - i)) % MAX_HISTORY_TICS;
		static constexpr int width = NetGraph::BAR_WIDTH_MISPREDICTION;
		static constexpr int height = NetGraph::BAR_HEIGHT_MISPREDICTION;
		const int startx = x + i * NetGraph::BAR_WIDTH_MISPREDICTION;
		const int starty = y;

		if (mMisprediction[index])
			NetGraphDrawBar(startx, starty, width, height, 0xB0);
	}
}

void NetGraph::drawTrafficIn(int x, int y)
{
	static constexpr int textcolor = CR_GREY;

	int totalTraffic = 0;
	for (int i = 0;i < TICRATE;i++)
	{
		const int backtic = gametic - i;
		if (backtic < 0) {
			break;
		}
		totalTraffic += mTrafficIn[backtic % NetGraph::MAX_HISTORY_TICS];
	}

	std::ostringstream buf;
	buf.precision(2);
	buf << "Traffic In: " << std::fixed << totalTraffic / 1024.0 << " kb/s";
	screen->DrawText(textcolor, x, y, buf.str().c_str());
}

void NetGraph::drawTrafficOut(int x, int y)
{
	static constexpr int textcolor = CR_GREY;

	int totalTraffic = 0;
	for (int i = 0;i < TICRATE;i++)
	{
		const int backtic = gametic - i;
		if (backtic < 0) {
			break;
		}
		totalTraffic += mTrafficOut[backtic % NetGraph::MAX_HISTORY_TICS];
	}

	std::ostringstream buf;
	buf.precision(2);
	buf << "Traffic Out: " << std::fixed << totalTraffic / 1024.0 << " kb/s";
	screen->DrawText(textcolor, x, y, buf.str().c_str());
}

void NetGraph::drawPackets(int x, int y)
{
	static constexpr int textcolor = CR_GREY;

	int maxPackets = 0;

	for (size_t i = 0; i < NetGraph::MAX_HISTORY_TICS; i++)
	{
		const int index = (gametic - (NetGraph::MAX_HISTORY_TICS - i)) % MAX_HISTORY_TICS;
		const int packets = mPacketsIn[index];
		if (packets > maxPackets) {
			maxPackets = packets;
		}

		int height = packets;
		if (height > 200) height = 200;
		NetGraphDrawBar(x + i * 2, y + 8, 2, height, 0xB0);
	}

	std::ostringstream buf;
	buf << "Packets In: " << std::setw(5) << maxPackets;
	screen->DrawText(textcolor, x, y, buf.str().c_str());
}

void NetGraph::draw()
{
	static constexpr int textcolor = CR_GREY;
	static constexpr int fontheight = 8;

    screen->DrawText(textcolor, mX, mY, "World Index Sync");
	drawWorldIndexSync(mX, mY + fontheight);

    screen->DrawText(textcolor, mX, mY + 64, "Mispredictions");
	drawMispredictions(mX, mY + 64 + fontheight);

    screen->DrawText(textcolor, mX + 128, mY, ("Reliable Send Queue: " + std::to_string(mReliableSendDepth[gametic % MAX_HISTORY_TICS])).c_str());
    drawReliableSendDepth(mX + 128, mY + fontheight);

	drawTrafficIn(mX, mY + 128 + fontheight);
	drawTrafficOut(mX, mY + 128 + fontheight * 3);
	drawPackets(mX, mY + 128 + fontheight * 6);
}

VERSION_CONTROL (cl_netgraph_cpp, "$Id$")
