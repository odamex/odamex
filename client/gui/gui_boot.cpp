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
#include "FL/Fl_Check_Browser.H"
#include "FL/Fl_Check_Button.H"
#include "FL/Fl_Hold_Browser.H"
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

typedef std::vector<scannedIWAD_t> scannedIWADs_t;
typedef std::vector<scannedPWAD_t> scannedPWADs_t;
typedef std::vector<scannedPWAD_t*> scannedPWADPtrs_t;

/**
 * @brief Find the PWAD pointer in the scanned WAD array.
 */
static scannedPWADs_t::iterator FindScanned(scannedPWADs_t& mut, scannedPWAD_t* pwad)
{
	for (scannedPWADs_t::iterator it = mut.begin(); it != mut.end(); ++it)
	{
		if (&*it == pwad)
		{
			return it;
		}
	}
	return mut.end();
}

/**
 * @brief Add a scanned PWAD from the pointer list - unless there's a dupe.
 */
static void AddSelected(scannedPWADPtrs_t& mut, scannedPWAD_t* pwad)
{
	scannedPWADPtrs_t::iterator it = std::find(mut.begin(), mut.end(), pwad);
	if (it == mut.end())
	{
		mut.push_back(pwad);
	}
}

/**
 * @brief Erase a scanned PWAD from the pointer list.
 */
static void EraseSelected(scannedPWADPtrs_t& mut, scannedPWAD_t* pwad)
{
	scannedPWADPtrs_t::iterator it = std::find(mut.begin(), mut.end(), pwad);
	if (it != mut.end())
	{
		mut.erase(it);
	}
}

const scannedIWAD_t* g_SelectedIWAD;
scannedWADs_t g_SelectedWADs;

const int WINDOW_WIDTH = 425;
const int WINDOW_HEIGHT = 240;

class BootWindow : public Fl_Window
{
	Fl_Group* m_tabIWAD;
	Fl_Group* m_tabPWADs;
	std::string m_genWaddirs;
	scannedIWADs_t m_IWADs;
	scannedPWADs_t m_PWADs;
	scannedPWADPtrs_t m_selectedPWADs;
	Fl_Hold_Browser* m_IWADBrowser;
	Fl_Check_Browser* m_PWADSelectBrowser;
	Fl_Hold_Browser* m_PWADOrderBrowser;
	Fl_Check_Browser* m_gameOptionsBrowser;
	StringTokens m_WADDirs;
	Fl_Hold_Browser* m_WADDirList;
	// display strings for options tab and their corresponding command line arguments
	std::vector<std::pair<std::string, std::string> > OPTIONS_LIST;

  public:
	BootWindow(int X, int Y, int W, int H, const char* L)
	    : Fl_Window(X, Y, W, H, L), m_IWADs()
	{
		OPTIONS_LIST.push_back(std::make_pair("No Monsters", "-nomonsters"));
		OPTIONS_LIST.push_back(std::make_pair("Fast Monsters", "-fast"));
		OPTIONS_LIST.push_back(std::make_pair("Respawn Monsters", "-respawn"));
		OPTIONS_LIST.push_back(std::make_pair("Pistol Start", "-pistolstart"));
		OPTIONS_LIST.push_back(std::make_pair("Multiplayer Things", "-coop-things"));
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
					m_IWADBrowser->callback(BootWindow::doPlayCB, static_cast<void*>(this));
					m_IWADBrowser->when(FL_WHEN_ENTER_KEY);
				} // Fl_Browser* m_IWADBrowser
				m_tabIWAD->end();
			} // Fl_Group* tabIWAD
			{
				m_tabPWADs = new Fl_Group(0, 25, 425, 175, "PWAD Select");
				{
					m_PWADSelectBrowser = new Fl_Check_Browser(10, 35, 183, 155);
					m_PWADSelectBrowser->callback(BootWindow::scanCheckedPWADsCB,
					                              static_cast<void*>(this));
					m_PWADSelectBrowser->when(FL_WHEN_CHANGED);
				} // Fl_Check_Browser* m_PWADSelectBrowser
				{
					m_PWADOrderBrowser = new Fl_Hold_Browser(203, 65, 182, 125);
				} // Fl_Hold_Browser* m_PWADOrderBrowser
				{
					Fl_Box* o = new Fl_Box(203, 35, 182, 20, "Change Load Order");
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
				} // Fl_Button* doWADRemove
				m_tabPWADs->end();
			} // Fl_Group* tabPWADs
			{
				Fl_Group* tabGameOptions =
					new Fl_Group(0, 25, 425, 175, "Game Options");
				{
					Fl_Box* o = new Fl_Box(
					    10, 35, 405, 20,
					    "Set gameplay options to modify your experience.");
					o->align(Fl_Align(132 | FL_ALIGN_INSIDE));
				} // Fl_Box* o
				{
					m_gameOptionsBrowser = new Fl_Check_Browser(10, 65, 405, 125);
					for (std::vector<std::pair<std::string, std::string> >::const_iterator it = OPTIONS_LIST.begin();
		    	 		 it != OPTIONS_LIST.end(); ++it)
					{
						m_gameOptionsBrowser->add((*it).first.c_str());
					}
				}
				tabGameOptions->end();

			} // Fl_Group* tabGameOptions
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
		{
			boot->rescanIWADs();
			return;
		}

		bool pwadsIsEmpty = boot->m_PWADSelectBrowser->nitems() == 0;

		// User clicked on the second tab, regenerate the
		// list of IWADs if waddirs changed or browser is empty.
		if ((clicked == boot->m_tabPWADs) && (pwadsIsEmpty || waddirsChanged))
		{
			boot->rescanPWADs();
			return;
		}
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
		boot->setOptions();
		Fl::delete_widget(boot);
	}

	// -- PWAD Boot Order --

	static void doWADUpCB(Fl_Widget*, void* data)
	{
		BootWindow* boot = static_cast<BootWindow*>(data);

		const int val = boot->m_PWADOrderBrowser->value() - 1;
		if (val <= 0 || val >= boot->m_selectedPWADs.size())
			return;

		std::iter_swap(boot->m_selectedPWADs.begin() + val,
		               boot->m_selectedPWADs.begin() + val - 1);
		boot->updatePWADOrderBrowser();
		boot->m_PWADOrderBrowser->value(val);
	}

	static void doWADDownCB(Fl_Widget*, void* data)
	{
		BootWindow* boot = static_cast<BootWindow*>(data);

		const int val = boot->m_PWADOrderBrowser->value() - 1;
		if (val < 0 || val >= boot->m_selectedPWADs.size() - 1)
			return;

		std::iter_swap(boot->m_selectedPWADs.begin() + val,
		               boot->m_selectedPWADs.begin() + val + 1);
		boot->updatePWADOrderBrowser();
		boot->m_PWADOrderBrowser->value(val + 2);
	}

	/**
	 * @brief Removes a WAD from the PWAD selection list.
	 */
	static void doWADRemoveCB(Fl_Widget*, void* data)
	{
		BootWindow* boot = static_cast<BootWindow*>(data);

		// Figure out which PWAD we're removing.
		const size_t removeIDX = size_t(boot->m_PWADOrderBrowser->value()) - 1;
		if (removeIDX > boot->m_selectedPWADs.size())
		{
			return;
		}
		scannedPWAD_t* selected = boot->m_selectedPWADs[removeIDX];

		// Uncheck the selected PWAD from the selection array.
		scannedPWADs_t::iterator it = FindScanned(boot->m_PWADs, selected);
		if (it == boot->m_PWADs.end())
		{
			return;
		}
		ptrdiff_t index = it - boot->m_PWADs.begin();
		boot->m_PWADSelectBrowser->checked(index + 1, 0);

		// Erase the selected PWAD from the order array.
		EraseSelected(boot->m_selectedPWADs, boot->m_selectedPWADs[removeIDX]);
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
		for (scannedPWADs_t::iterator it = m_PWADs.begin(); it != m_PWADs.end(); ++it)
		{
			m_PWADSelectBrowser->add(it->filename.c_str());
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

		// Scan all PWADs in the selection browser to see if they're checked.
		for (int i = 1; i <= boot->m_PWADSelectBrowser->nitems(); i++)
		{
			scannedPWAD_t* selected = &boot->m_PWADs[size_t(i) - 1];
			if (boot->m_PWADSelectBrowser->checked(i))
			{
				AddSelected(boot->m_selectedPWADs, selected);
			}
			else
			{
				EraseSelected(boot->m_selectedPWADs, selected);
			}
		}

		// Update the PWAD order browser.
		boot->updatePWADOrderBrowser();
	}

	/**
	 * @brief Update the visible PWAD order browser widget from the vector.
	 */
	void updatePWADOrderBrowser()
	{
		const int val = m_PWADOrderBrowser->value();
		m_PWADOrderBrowser->clear();
		for (scannedPWADPtrs_t::iterator it = m_selectedPWADs.begin();
		     it != m_selectedPWADs.end(); ++it)
		{
			m_PWADOrderBrowser->add((*it)->filename.c_str());
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
		return &m_IWADs[value - 1];
	}

	/**
	 * @brief Get WAD information for selected WADs.
	 */
	void selectedWADs()
	{
		// IWADs
		const size_t value = static_cast<size_t>(m_IWADBrowser->value());
		g_SelectedWADs.iwad = m_IWADs[value - 1].path;

		// PWADs
		for (scannedPWADPtrs_t::iterator it = m_selectedPWADs.begin();
		     it != m_selectedPWADs.end(); ++it)
		{
			g_SelectedWADs.pwads.push_back((*it)->path);
		}
	}

	void setOptions()
	{
		for (int i = 1; i <= m_gameOptionsBrowser->nitems(); i++) {
			if (m_gameOptionsBrowser->checked(i))
				g_SelectedWADs.options.push_back(OPTIONS_LIST[i - 1].second);
		}
	}

	/**
	 * @brief Update the WAD dir browser widget from the vector.
	 */
	void updateWADDirBrowser()
	{
		const int val = m_WADDirList->value();
		m_WADDirList->clear();
		for (StringTokens::iterator it = m_WADDirs.begin(); it != m_WADDirs.end(); ++it)
		{
			m_WADDirList->add(it->c_str());
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
	return new BootWindow(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, "Odamex " DOTVERSIONSTR);
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
	return g_SelectedWADs;
}
