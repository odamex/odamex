// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2012 by The Odamex Team.
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

#ifndef __HU_MOUSEGRAPH_H__
#define __HU_MOUSEGRAPH_H__

class MouseGraph {
public:
	static const char TYPE_OFF = 0;
	static const char TYPE_LINE = 1;
	static const char TYPE_PLOT = 2;

	MouseGraph();
	void append(int x, int y);
	void draw(int type = 0);
private:
	static const int MAX_HISTORY_TICS = 64;

	int index;
	int mousex[MouseGraph::MAX_HISTORY_TICS];
	int mousey[MouseGraph::MAX_HISTORY_TICS];

	void drawPlot(int x, int y);
	void drawLine(int x, int y);
};

extern MouseGraph mousegraph;

#endif