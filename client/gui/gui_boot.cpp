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

#include <algorithm>

#include "FL/Fl.H"
#include "FL/Fl_Box.H"
#include "FL/Fl_Button.H"
#include "FL/Fl_Check_Button.H"
#include "FL/Fl_Hold_Browser.H"
#include "FL/Fl_Check_Browser.H"
#include "FL/Fl_Native_File_Chooser.H"
#include "FL/Fl_Return_Button.H"
#include "FL/Fl_Tabs.H"
#include "FL/Fl_Window.H"
#include "FL/fl_ask.H"

#include "gui_common.h"

#include "i_system.h"
#include "w_ident.h"

// -- Externals --

void CL_QuitCommand();
EXTERN_CVAR(i_skipbootwin);
EXTERN_CVAR(waddirs);

// ---------------

const scannedIWAD_t* g_SelectedIWAD;
scannedWADs_t g_SelectedWADs;

const int WINDOW_WIDTH = 320;
const int WINDOW_HEIGHT = 240;

class BootWindow : public Fl_Window
{
	Fl_Group* m_tabIWAD;
	Fl_Group* m_tabPWADs;
	std::string m_genWaddirs;
	std::vector<scannedIWAD_t> m_IWADs;
	std::vector<scannedPWAD_t> m_PWADs;
	std::vector<std::string> m_selectedPWADs;
	Fl_Hold_Browser* m_IWADBrowser;
	Fl_Check_Browser* m_PWADSelectBrowser;
	Fl_Hold_Browser* m_PWADOrderBrowser;
	std::vector<std::string> m_WADDirs;
	Fl_Hold_Browser* m_WADDirList;

  public:
	BootWindow(int X, int Y, int W, int H, const char* L)
	    : Fl_Window(X, Y, W, H, L), m_IWADs()
	{
		{
			Fl_Tabs* tabs = new Fl_Tabs(0, 0, 425, 200);
			{
				tabs->callback(tabsCB, static_cast<void*>(this));
				tabs->when(FL_WHEN_CHANGED);
				m_tabIWAD = new Fl_Group(0, 25, 425, 175, "Game Select");
				{
					Fl_Box* logo = new Fl_Box(10, 35, 115, 155);
					logo->image(GUIRes::icon_odamex_128());
					logo->align(Fl_Align(512));
				} // Fl_Box* logo
				{
					m_IWADBrowser = new Fl_Hold_Browser(135, 35, 280, 155);
				} // Fl_Browser* m_IWADBrowser
				m_tabIWAD->end();
			} // Fl_Group* tabIWAD
			{
				m_tabPWADs = new Fl_Group(0, 25, 425, 175, "PWAD Select");
				{
					m_PWADSelectBrowser = new Fl_Check_Browser(10, 35, 183, 155);
					m_PWADSelectBrowser->callback(BootWindow::scanCheckedPWADsCB, static_cast<void*>(this));
					m_PWADSelectBrowser->when(FL_WHEN_CHANGED);
				} // Fl_Check_Browser* m_PWADSelectBrowser
				{
					m_PWADOrderBrowser = new Fl_Hold_Browser(203, 65, 182, 125);
				} // Fl_Hold_Browser* m_PWADOrderBrowser
				{
					Fl_Box* o = new Fl_Box(
					    203, 35, 182, 20,
					    "Change Load Order");
				} // Fl_Box* o
				{
					Fl_Button* doWADUp = new Fl_Button(395, 90, 20, 20, "@2<<");
					doWADUp->callback(BootWindow::doWADUpCB, static_cast<void*>(this));
				} // Fl_Button* doWADUp
				{
					Fl_Button* doWADDown = new Fl_Button(395, 115, 20, 20, "@2>>");
					doWADDown->callback(BootWindow::doWADDownCB,
					                    static_cast<void*>(this));
				} // Fl_Button* doWADDown
				{
					Fl_Button* doWADRemove = new Fl_Button(395, 140, 20, 20, "@1+");
					doWADRemove->callback(BootWindow::doWADRemoveCB,
					                      static_cast<void*>(this));
				}  // Fl_Button* doWADRemove
				m_tabPWADs->end();
			} // Fl_Group* tabPWADs
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
					m_WADDirList = new Fl_Hold_Browser(10, 65, 375, 125);
				} // Fl_Browser* wadDirList
				{
					Fl_Button* doDirAdd = new Fl_Button(395, 65, 20, 20, "@+");
					doDirAdd->callback(BootWindow::doDirAddCB, static_cast<void*>(this));
				} // Fl_Button* doDirAdd
				{
					Fl_Button* doDirUp = new Fl_Button(395, 90, 20, 20, "@2<<");
					doDirUp->callback(BootWindow::doDirUpCB, static_cast<void*>(this));
				} // Fl_Button* doDirUp
				{
					Fl_Button* doDirDown = new Fl_Button(395, 115, 20, 20, "@2>>");
					doDirDown->callback(BootWindow::doDirDownCB,
					                    static_cast<void*>(this));
				} // Fl_Button* doDirDown
				{
					Fl_Button* doDirRemove = new Fl_Button(395, 140, 20, 20, "@1+");
					doDirRemove->callback(BootWindow::doDirRemoveCB,
					                      static_cast<void*>(this));
				} // Fl_Button* doDirRemove
				tabWADDirs->end();
			} // Fl_Group* tabWADDirs
			tabs->end();
		} // Fl_Tabs* tabs
		{
			Fl_Check_Button* notAgain =
			    new Fl_Check_Button(10, 210, 20, 20, "Don\'t Show This Again");
			notAgain->down_box(FL_DOWN_BOX);
			notAgain->callback(BootWindow::doDontShowCB);
			notAgain->value(::i_skipbootwin);
		} // Fl_Check_Button* notAgain
		{
			Fl_Button* doQuit = new Fl_Button(275, 210, 65, 20, "Quit");
			doQuit->callback(BootWindow::doQuitCB);
		} // Fl_Button* doQuit
		{
			Fl_Return_Button* doPlay = new Fl_Return_Button(350, 210, 65, 20, "Play!");
			doPlay->callback(BootWindow::doPlayCB, static_cast<void*>(this));
		} // Fl_Return_Button* doPlay
		end();
		callback(BootWindow::doCallback);
		GUI_SetIcon(this);
	}

	// -- Game Select --

	static void tabsCB(Fl_Widget* w, void* data)
	{
		Fl_Tabs* tabs = static_cast<Fl_Tabs*>(w);
		BootWindow* boot = static_cast<BootWindow*>(data);

		Fl_Group* clicked = static_cast<Fl_Group*>(tabs->value());

		// Have waddirs changed?
		bool waddirsChanged = boot->m_genWaddirs != ::waddirs;

		// User clicked on the first tab, regenerate the
		// list of IWADs if waddirs changed.
		if ((clicked == boot->m_tabIWAD) && waddirsChanged)
			return boot->rescanIWADs();

		bool pwadsIsEmpty = boot->m_PWADSelectBrowser->nitems() == 0;

		// User clicked on the second tab, regenerate the
		// list of IWADs if waddirs changed or browser is empty.
		if ((clicked == boot->m_tabPWADs) && (pwadsIsEmpty || waddirsChanged))
				return boot->rescanPWADs();
	}

	static void doCallback(Fl_Widget*, void*) { CL_QuitCommand(); }

	static void doDontShowCB(Fl_Widget* w, void*)
	{
		Fl_Check_Button* check = static_cast<Fl_Check_Button*>(w);
		::i_skipbootwin.Set(check->value());
	}

	static void doQuitCB(Fl_Widget*, void*) { CL_QuitCommand(); }

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

		boot->selectedWADs();
		Fl::delete_widget(boot);
	}

	// -- PWAD Boot Order --

	static void doWADUpCB(Fl_Widget*, void* data)
	{
		BootWindow* boot = static_cast<BootWindow*>(data);

		const int val = boot->m_PWADOrderBrowser->value() - 1;
		if (val <= 0 || val >= boot->m_selectedPWADs.size())
			return;

		std::iter_swap(boot->m_selectedPWADs.begin() + val, boot->m_selectedPWADs.begin() + val - 1);
		boot->updatePWADOrderBrowser();
		boot->m_PWADOrderBrowser->value(val);
	}

	static void doWADDownCB(Fl_Widget*, void* data)
	{
		BootWindow* boot = static_cast<BootWindow*>(data);

		const int val = boot->m_PWADOrderBrowser->value() - 1;
		if (val < 0 || val >= boot->m_selectedPWADs.size() - 1)
			return;

		std::iter_swap(boot->m_selectedPWADs.begin() + val, boot->m_selectedPWADs.begin() + val + 1);
		boot->updatePWADOrderBrowser();
		boot->m_PWADOrderBrowser->value(val + 2);
	}

	/**
	 * @brief Removes a WAD from the PWAD selection list.
	 */
	static void doWADRemoveCB(Fl_Widget*, void* data)
	{
		BootWindow* boot = static_cast<BootWindow*>(data);

		const int val = boot->m_PWADOrderBrowser->value() - 1;
		if (val < 0 || val >= boot->m_selectedPWADs.size())
			return;

		// deselect the wad in m_PWADSelectBrowser
		for (size_t i = 1; i <= boot->m_PWADSelectBrowser->nitems(); i ++)
		{
			if (!strcmp(boot->m_PWADSelectBrowser->text(i), (boot->m_PWADOrderBrowser->text(val + 1))))
				boot->m_PWADSelectBrowser->checked(i, 0);
		}

		boot->m_selectedPWADs.erase(boot->m_selectedPWADs.begin() + val);
		boot->updatePWADOrderBrowser();
	}

	// -- Resource Locations --

	/**
	 * @brief Adds a directory to the WAD dir list.
	 */
	static void doDirAddCB(Fl_Widget*, void* data)
	{
		BootWindow* boot = static_cast<BootWindow*>(data);

		Fl_Native_File_Chooser chooser;
		chooser.title("Add Directory");
		chooser.type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);
		switch (chooser.show())
		{
		case 1:  // Cancelled.
		case -1: // Failed.
			return;
		default: // Picked a directory.
			boot->m_WADDirs.push_back(chooser.filename());
			boot->setWADDirs();
			boot->updateWADDirBrowser();
			break;
		}
	}

	static void doDirUpCB(Fl_Widget*, void* data)
	{
		BootWindow* boot = static_cast<BootWindow*>(data);

		const int val = boot->m_WADDirList->value() - 1;
		if (val <= 0 || val >= boot->m_WADDirs.size())
			return;

		std::iter_swap(boot->m_WADDirs.begin() + val, boot->m_WADDirs.begin() + val - 1);
		boot->setWADDirs();
		boot->updateWADDirBrowser();
		boot->m_WADDirList->value(val);
	}

	static void doDirDownCB(Fl_Widget*, void* data)
	{
		BootWindow* boot = static_cast<BootWindow*>(data);

		const int val = boot->m_WADDirList->value() - 1;
		if (val < 0 || val >= boot->m_WADDirs.size() - 1)
			return;

		std::iter_swap(boot->m_WADDirs.begin() + val, boot->m_WADDirs.begin() + val + 1);
		boot->setWADDirs();
		boot->updateWADDirBrowser();
		boot->m_WADDirList->value(val + 2);
	}

	/**
	 * @brief Removes a directory from the WAD dir list.
	 */
	static void doDirRemoveCB(Fl_Widget*, void* data)
	{
		BootWindow* boot = static_cast<BootWindow*>(data);

		const int val = boot->m_WADDirList->value() - 1;
		if (val < 0 || val >= boot->m_WADDirs.size())
			return;

		boot->m_WADDirs.erase(boot->m_WADDirs.begin() + val);
		boot->setWADDirs();
		boot->updateWADDirBrowser();
	}

	void rescanIWADs()
	{
		m_IWADBrowser->clear();
		m_IWADs = M_ScanIWADs();
		for (size_t i = 0; i < m_IWADs.size(); i++)
		{
			m_IWADBrowser->add(m_IWADs[i].id->mNiceName.c_str(), (void*)m_IWADs[i].id);
		}
		m_genWaddirs = ::waddirs.str();
	}

	void rescanPWADs()
	{
		m_PWADSelectBrowser->clear();
		m_PWADs = M_ScanPWADs();
		for (size_t i = 0; i < m_PWADs.size(); i++)
		{
			m_PWADSelectBrowser->add(m_PWADs[i].filename.c_str());
		}
		m_genWaddirs = ::waddirs.str();
		// clear order browser since selection browser is being reset
		m_PWADOrderBrowser->clear();
		m_selectedPWADs.clear();
	}


	/**
	 * @brief Places selected PWADs into the order browser for load order selection.
	 */
	static void scanCheckedPWADsCB(Fl_Widget*, void* data)
	{
		BootWindow* boot = static_cast<BootWindow*>(data);
		std::vector<std::string>* selected = &boot->m_selectedPWADs;

		boot->m_PWADOrderBrowser->clear();
		for (size_t i = 1; i <= boot->m_PWADSelectBrowser->nitems(); i++) 
		{
			if (boot->m_PWADSelectBrowser->checked(i))
			{
				if (!std::count(selected->begin(), selected->end(), boot->m_PWADs[i - 1].filename.c_str()))
					selected->push_back(boot->m_PWADs[i - 1].filename.c_str());
			} else
			{
				std::vector<std::string>::iterator removed = 
					std::remove(selected->begin(), selected->end(), boot->m_PWADs[i - 1].filename.c_str());
				selected->erase(removed, selected->end());
			}
		}
		
		for (size_t i = 0; i < boot->m_selectedPWADs.size(); i++)
		{
			boot->m_PWADOrderBrowser->add(boot->m_selectedPWADs[i].c_str());
		}
	}

	/**
	 * @brief Update the PWAD order browser widget from the vector.
	 */
	void updatePWADOrderBrowser()
	{
		const int val = m_PWADOrderBrowser->value();
		m_PWADOrderBrowser->clear();
		for (size_t i = 0; i < m_selectedPWADs.size(); i++)
		{
			m_PWADOrderBrowser->add(m_selectedPWADs[i].c_str());
		}
		m_PWADOrderBrowser->value(val);
	}

	/**
	 * @brief Get WAD information for selected IWAD.
	 */
	const scannedIWAD_t* selectedIWAD()
	{
		const size_t value = static_cast<size_t>(m_IWADBrowser->value());
		if (value == 0)
			return NULL;
		return &m_IWADs.at(value - 1);
	}

	/**
	 * @brief Get WAD information for selected WADs.
	 */
	void selectedWADs()
	{
		const size_t value = static_cast<size_t>(m_IWADBrowser->value());
		::g_SelectedWADs.iwad = m_IWADs.at(value - 1).path;
		for (size_t i = 0; i < m_selectedPWADs.size(); i++)
		{
			::g_SelectedWADs.pwads.push_back(m_selectedPWADs[i]);
		}
	}

	/**
	 * @brief Update the WAD dir browser widget from the vector.
	 */
	void updateWADDirBrowser()
	{
		const int val = m_WADDirList->value();
		m_WADDirList->clear();
		for (size_t i = 0; i < m_WADDirs.size(); i++)
		{
			m_WADDirList->add(m_WADDirs[i].c_str());
		}
		m_WADDirList->value(val);
	}

	/**
	 * @brief Initialize WAD directories from CVar.
	 */
	void initWADDirs() { m_WADDirs = TokenizeString(::waddirs.str(), PATHLISTSEP); }

	/**
	 * @brief Update the CVar from WAD directories list.
	 */
	void setWADDirs() { ::waddirs.Set(JoinStrings(m_WADDirs, PATHLISTSEP).c_str()); }
};

/**
 * @brief Shim function to call new BootWindow() with proper params.
 */
static BootWindow* MakeBootWindow()
{
	return new BootWindow(0, 0, 425, 240, "Odamex " DOTVERSIONSTR);
}

/**
 * @brief Create the boot window for Odamex.
 *
 * @return The IWAD file to use when starting the game.
 */
scannedWADs_t GUI_BootWindow()
{
	// Scale according to 1600x900.
	Fl::screen_scale(0, MAX(Fl::h() / 900.0f, 1.0f));

	// This feature is too clever by half, and in my experience just
	// deforms the window.
	Fl::keyboard_screen_scaling(0);

	BootWindow* win = MakeBootWindow();
	win->initWADDirs();
	win->updateWADDirBrowser();
	win->rescanIWADs();
	win->position((Fl::w() - win->w()) / 2, (Fl::h() - win->h()) / 2);
	win->show(0, NULL);

	// Blocks until the boot window has been closed.
	Fl::run();

	// Return the full IWAD path.
	return ::g_SelectedWADs;
}
