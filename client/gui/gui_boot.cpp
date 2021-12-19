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
#include "FL/fl_ask.H"
#include "FL/Fl_Box.H"
#include "FL/Fl_Button.H"
#include "FL/Fl_Double_Window.H"
#include "FL/Fl_Hold_Browser.H"
#include "FL/Fl_PNG_Image.H"
#include "FL/Fl_Return_Button.H"
#include "FL/Fl_Tabs.H"

#include "i_system.h"
#include "gui_resource.h"
#include "w_ident.h"

const scannedIWAD_t* g_SelectedIWAD;

const int WINDOW_WIDTH = 320;
const int WINDOW_HEIGHT = 240;

static Fl_Image* image_icon_odamex_128()
{
	static Fl_Image* image = new Fl_PNG_Image("icon_odamex_128", __icon_odamex_128_png,
	                                          __icon_odamex_128_png_len);
	return image;
}

class BootWindow : public Fl_Window
{
	std::vector<scannedIWAD_t> m_IWADs;
	Fl_Hold_Browser* m_IWADBrowser;

  public:
	BootWindow(int X, int Y, int W, int H, const char* L)
	    : Fl_Window(X, Y, W, H, L), m_IWADs()
	{
		{
			Fl_Tabs* tabs = new Fl_Tabs(0, 0, 425, 200);
			{
				Fl_Group* tabIWAD = new Fl_Group(0, 25, 425, 175, "Game Select");
				{
					Fl_Box* logo = new Fl_Box(10, 35, 115, 155);
					logo->image(image_icon_odamex_128());
					logo->align(Fl_Align(512));
				} // Fl_Box* logo
				{
					m_IWADBrowser = new Fl_Hold_Browser(135, 35, 280, 155);
				} // Fl_Browser* m_IWADBrowser
				tabIWAD->end();
			} // Fl_Group* tabIWAD
			{
				Fl_Group* tabWADDirs =
				    new Fl_Group(0, 25, 425, 175, "Resource Locations");
				tabWADDirs->hide();
				{
					Fl_Box* o = new Fl_Box(
					    10, 35, 405, 20,
					    "Add folders containing WAD files here so Odamex can find them.");
					o->align(Fl_Align(132 | FL_ALIGN_INSIDE));
				} // Fl_Box* o
				{
					Fl_Browser* wadDirList = new Fl_Browser(10, 65, 375, 125);
				} // Fl_Browser* wadDirList
				{
					Fl_Button* doDirAdd = new Fl_Button(395, 65, 20, 20, "@+");
				} // Fl_Button* doDirAdd
				{
					Fl_Button* doDirUp = new Fl_Button(395, 90, 20, 20, "@2<<");
				} // Fl_Button* doDirUp
				{
					Fl_Button* doDirDown = new Fl_Button(395, 115, 20, 20, "@2>>");
				} // Fl_Button* doDirDown
				{
					Fl_Button* doDirRemove = new Fl_Button(395, 140, 20, 20, "@1+");
				} // Fl_Button* doDirRemove
				tabWADDirs->end();
			} // Fl_Group* tabWADDirs
			tabs->end();
		} // Fl_Tabs* tabs
		{
			Fl_Button* doQuit = new Fl_Button(10, 210, 65, 20, "Quit");
			doQuit->callback(BootWindow::doQuitCB);
		} // Fl_Button* doQuit
		{
			Fl_Return_Button* doPlay = new Fl_Return_Button(350, 210, 65, 20, "Play!");
			doPlay->callback(BootWindow::doPlayCB, static_cast<void*>(this));
		} // Fl_Return_Button* doPlay
		end();
		callback(BootWindow::doCallback);
	}

	static void doCallback(Fl_Widget*, void*) { exit(0); }

	static void doQuitCB(Fl_Widget*, void*) { exit(0); }

	static void doPlayCB(Fl_Widget*, void* data)
	{
		BootWindow* boot = static_cast<BootWindow*>(data);
		if (boot->selectedIWAD() == NULL)
		{
			// A valid IWAD isn't selected.
			fl_message_title("Play Odamex");
			fl_alert("Please select a game to play.");
			return;
		}

		::g_SelectedIWAD = boot->selectedIWAD();
		Fl::delete_widget(boot);
	}

	void doPlayCB() { }

	void rescanIWADs()
	{
		m_IWADBrowser->clear();
		m_IWADs = M_ScanIWADs();
		for (size_t i = 0; i < m_IWADs.size(); i++)
		{
			m_IWADBrowser->add(m_IWADs[i].id->mNiceName.c_str(), (void*)m_IWADs[i].id);
		}
	}

	const scannedIWAD_t* selectedIWAD()
	{
		const size_t value = static_cast<size_t>(m_IWADBrowser->value());
		if (value == 0)
			return NULL;
		return &m_IWADs.at(value - 1);
	}
};

static BootWindow* MakeBootWindow()
{
	return new BootWindow(0, 0, 425, 240, "Odamex 10.0.0");
}

/**
 * @brief Create the boot window for Odamex.
 * 
 * @return The IWAD file to use when starting the game.
 */
std::string GUI_BootWindow()
{
	Fl::scheme("gtk+");

	// Scale according to 720p.
	Fl::screen_scale(0, Fl::h() / 720.0f);

	// This feature is too clever by half, and in my experience just
	// deforms the window.
	Fl::keyboard_screen_scaling(0);

	BootWindow* win = MakeBootWindow();
	win->rescanIWADs();
	win->position((Fl::w() - win->w()) / 2, (Fl::h() - win->h()) / 2);
	win->show();

	// Blocks until the boot window has been closed.
	Fl::run();

	// Return the full IWAD path.
	return ::g_SelectedIWAD->path;
}
