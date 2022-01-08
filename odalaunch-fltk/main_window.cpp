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

#include "FL//Fl_Menu_Button.H"
#include "FL//Fl_Toggle_Button.H"
#include "FL/Fl_Button.H"
#include "FL/Fl_Input.H"
#include "FL/Fl_Tabs.H"

#include "server_table.h"

MainWindow::MainWindow(int w, int h, const char* title) : Fl_Window(w, h, title)
{
	// Filter row
	new Fl_Toggle_Button(5, 5, 105, 20, "Passworded");
	new Fl_Toggle_Button(115, 5, 105, 20, "Full");
	new Fl_Toggle_Button(225, 5, 105, 20, "Empty");
	new Fl_Menu_Button(335, 5, 105, 20, "Ping");
	new Fl_Input(465, 5, w - 470, 20, "@search");

	// Verb row
	new Fl_Button(w - 330, h - 25, 105, 20, "Get New List");
	new Fl_Button(w - 220, h - 25, 105, 20, "Refresh List");
	new Fl_Button(w - 110, h - 25, 105, 20, "Join");

	// Main list - must be last or FLTK gets grouchy
	new ServerTable(5, 30, w - 10, h - 60);

	resizable(this);
}
