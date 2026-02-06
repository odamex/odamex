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
//	Common level routines
//
//-----------------------------------------------------------------------------

#pragma once

#include <array>

class NetGraph
{
public:
	NetGraph(int x, int y);

	void setMisprediction(bool val);
	void setWorldIndexSync(int val);
	void setInterpolation(int val);
	void addTrafficIn(int val);
	void addTrafficOut(int val);
	void addPacketIn();
	void setReliableSendDepth(int val);
	void setReliableNonContiguousRetransmits(int val);
	void setServerQueueDepth(int val);
	void draw();

private:
	static constexpr size_t MAX_HISTORY_TICS = 64;

	static constexpr int BAR_HEIGHT_WORLD_INDEX = 4;
	static constexpr int BAR_WIDTH_WORLD_INDEX = 2;

	static constexpr int BAR_HEIGHT_MISPREDICTION = 2;
	static constexpr int BAR_WIDTH_MISPREDICTION = 2;

	static constexpr int MAX_WORLD_INDEX = 6;
	static constexpr int MIN_WORLD_INDEX = -6;

	void drawWorldIndexSync(int x, int y);
	void drawMispredictions(int x, int y);
	void drawTrafficIn(int x, int y);
	void drawTrafficOut(int x, int y);
	void drawPackets(int x, int y);
	void drawQueueDepth(int x, int y, const std::array<int, NetGraph::MAX_HISTORY_TICS>& data, int color);
	void drawReliableSendDepth(int x, int y);
	void drawServerQueueDepth(int x, int y);

	int		mX;
	int		mY;

	std::array<bool, NetGraph::MAX_HISTORY_TICS> mMisprediction;
	std::array<int, NetGraph::MAX_HISTORY_TICS> mWorldIndexSync;
	std::array<int, NetGraph::MAX_HISTORY_TICS> mTrafficIn;
	std::array<int, NetGraph::MAX_HISTORY_TICS> mTrafficOut;
	std::array<int, NetGraph::MAX_HISTORY_TICS> mPacketsIn;
	std::array<int, NetGraph::MAX_HISTORY_TICS> mServerQueueDepth;
	std::array<int, NetGraph::MAX_HISTORY_TICS> mServerQueueDepthLastUpdate;
	std::array<int, NetGraph::MAX_HISTORY_TICS> mReliableSendDepth;
	std::array<int, NetGraph::MAX_HISTORY_TICS> mReliableNonContiguousRetransmits;
	int     mInterpolation;
};
