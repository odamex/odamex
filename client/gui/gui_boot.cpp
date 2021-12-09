// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Boot GUI for WAD selection.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "gui_boot.h"

#include "FL/Fl.H"
#include "FL/Fl_Box.H"
#include "FL/Fl_Browser.H"
#include "FL/Fl_Button.H"
#include "FL/Fl_Double_Window.H"
#include "FL/Fl_Return_Button.H"
#include "FL/Fl_Tabs.H"

static void ShowBoot()
{
	Fl_Double_Window* w;
	{
		Fl_Double_Window* o = new Fl_Double_Window(320, 240, "Odamex 10.0.0");
		w = o;
		if (w)
		{ /* empty */
		}
		{
			Fl_Tabs* o = new Fl_Tabs(0, 0, 320, 200);
			{
				Fl_Group* o = new Fl_Group(0, 25, 320, 175, "Game Select");
				{
					new Fl_Browser(135, 35, 175, 155);
				} // Fl_Browser* o
				{
					Fl_Box* o = new Fl_Box(10, 34, 115, 157);
					o->align(Fl_Align(512));
				} // Fl_Box* o
				o->end();
			} // Fl_Group* o
			{
				Fl_Group* o = new Fl_Group(0, 25, 320, 175, "Resource Locations");
				o->hide();
				{
					new Fl_Browser(10, 85, 265, 105);
				} // Fl_Browser* o
				{
					new Fl_Button(285, 86, 20, 20, "A");
				} // Fl_Button* o
				{
					new Fl_Button(285, 111, 20, 20, "M");
				} // Fl_Button* o
				{
					new Fl_Button(285, 136, 20, 20, "D");
				} // Fl_Button* o
				{
					Fl_Box* o = new Fl_Box(10, 34, 295, 42,
					                       "Add directories containing WAD and DEH "
					                       "files here so Odamex can find them.");
					o->align(Fl_Align(132 | FL_ALIGN_INSIDE));
				} // Fl_Box* o
				o->end();
			} // Fl_Group* o
			o->end();
		} // Fl_Tabs* o
		{
			new Fl_Button(10, 210, 64, 20, "Quit");
		} // Fl_Button* o
		{
			new Fl_Return_Button(240, 210, 66, 20, "Play!");
		} // Fl_Return_Button* o
		o->end();
	} // Fl_Double_Window* o
	w->show();
}

void GUI_BootWindow()
{
	// This feature is too clever by half, and in my experience just
	// deforms the window.
	Fl::keyboard_screen_scaling(0);

	// TODO: Determine the proper scaling factor for this screen.
	Fl::screen_scale(0, 1.5);

	ShowBoot();

	// Blocks until the boot window has been closed.
	Fl::run();
}
