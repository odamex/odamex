// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2021 by The Odamex Team.
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
//  Server table
//
//-----------------------------------------------------------------------------

#pragma once

#include "odalaunch.h"

#include "FL/Fl_Table.H"

#include "db.h"

class ServerTable : public Fl_Table
{
	serverRows_t m_servers;
  public:
	ServerTable(int X, int Y, int W, int H, const char* l = 0);
	void draw_cell(TableContext context, int R = 0, int C = 0, int X = 0, int Y = 0,
	               int W = 0, int H = 0);
};
