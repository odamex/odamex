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
#include "FL/Fl_PNG_Image.H"
#include "FL/Fl_Return_Button.H"
#include "FL/Fl_Tabs.H"

#include "gui_resource.h"

static Fl_Image* image_icon_odamex_128()
{
	static Fl_Image* image = new Fl_PNG_Image("icon_odamex_128", __icon_odamex_128_png,
	                                          __icon_odamex_128_png_len);
	return image;
}

static Fl_Double_Window* BootWindow()
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
					Fl_Box* o = new Fl_Box(10, 34, 115, 157);
					o->image(image_icon_odamex_128());
					o->align(Fl_Align(512));
				} // Fl_Box* o
				{
					new Fl_Browser(135, 35, 175, 155);
				} // Fl_Browser* o
				o->end();
			} // Fl_Group* o
			{
				Fl_Group* o = new Fl_Group(0, 25, 320, 175, "Resource Locations");
				o->hide();
				{
					Fl_Box* o = new Fl_Box(10, 35, 295, 35,
					                       "Add directories containing WAD and DEH files "
					                       "here so Odamex can find them.");
					o->align(Fl_Align(132 | FL_ALIGN_INSIDE));
				} // Fl_Box* o
				{
					new Fl_Browser(10, 80, 265, 110);
				} // Fl_Browser* o
				{
					new Fl_Button(285, 81, 20, 20, "@+");
				} // Fl_Button* o
				{
					new Fl_Button(285, 104, 20, 20, "@2<<");
				} // Fl_Button* o
				{
					new Fl_Button(285, 127, 20, 20, "@2>>");
				} // Fl_Button* o
				{
					new Fl_Button(285, 150, 20, 20, "@1+");
				} // Fl_Button* o
				o->end();
			} // Fl_Group* o
			o->end();
		} // Fl_Tabs* o
		{
			new Fl_Button(10, 210, 65, 20, "Quit");
		} // Fl_Button* o
		{
			new Fl_Return_Button(245, 210, 65, 20, "Play!");
		} // Fl_Return_Button* o
		o->end();
	} // Fl_Double_Window* o
	return w;
}

void GUI_BootWindow()
{
	// Scale according to 720p.
	Fl::screen_scale(0, Fl::h() / 720.0f);

	// This feature is too clever by half, and in my experience just
	// deforms the window.
	Fl::keyboard_screen_scaling(0);

	Fl_Double_Window* win = BootWindow();
	win->position((Fl::w() - win->w()) / 2, (Fl::h() - win->h()) / 2);
	win->show();

	// Blocks until the boot window has been closed.
	Fl::run();
}
