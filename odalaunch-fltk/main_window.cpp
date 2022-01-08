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
//  Main window
//
//-----------------------------------------------------------------------------

#include "odalaunch.h"

#include "main_window.h"

#include "FL/Fl_Tabs.H"

#include "server_group.h"

MainWindow::MainWindow(int w, int h, const char* title) : Fl_Window(w, h, title)
{
	Fl_Tabs* tabs = new Fl_Tabs(0, 0, w, h);
	ServerGroup* server = new ServerGroup(0, 20, w, h - 20);
	tabs->add(server);
	resizable(server);
}
