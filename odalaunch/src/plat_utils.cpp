// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//  wxWidgets-specific fixes and workarounds that apply to different platforms
//  that this program runs under
//
//-----------------------------------------------------------------------------

#include "plat_utils.h"

#include <wx/menu.h>
#include <wx/filefn.h>

#ifdef __WXMSW__
#include <windows.h>
#endif

// Apply hack for the titlebar under windows vista and 7 so it will display
// properly
void OdaMswFixTitlebarIcon(WXWidget Handle, wxIcon MainIcon)
{
#ifdef _WIN32
	SendMessage((HWND)Handle, WM_SETICON, ICON_SMALL,
	            (LPARAM)MainIcon.GetHICON());
	// Uncomment this if it doesn't work under xp
	//SendMessage((HWND)GetHandle(), WM_SETICON, ICON_BIG, (LPARAM)MainIcon.GetHICON());
#endif
}

// Stops flashing the window, wxWidgets does not have a function to do this on
// windows
void OdaMswStopFlashingWindow(WXWidget Handle)
{
#ifdef _WIN32
	FLASHWINFO fwi;

	fwi.cbSize = sizeof(fwi);
	fwi.hwnd = (HWND)Handle;
	fwi.dwFlags = FLASHW_STOP;
	fwi.uCount = 0;
	fwi.dwTimeout = 0;

	FlashWindowEx(&fwi);

#endif // _WIN32
}

// Remove the file menu on Mac as it will be empty
void OdaMacRemoveFileMenu(wxFrame* parent)
{
#ifdef __WXMAC__
	wxMenuBar* MenuBar = parent->GetMenuBar();

	// Remove the file menu on Mac as it will be empty
	wxMenu* fileMenu = MenuBar->Remove(MenuBar->FindMenu(_("File")));

	if(fileMenu)
	{
		wxMenuItem* prefMenuItem = fileMenu->Remove(wxID_PREFERENCES);
		wxMenu* helpMenu = MenuBar->GetMenu(MenuBar->FindMenu(_("Help")));

		// Before deleting the file menu the preferences menu item must be moved or
		// it will not work after this even though it has been placed somewhere else.
		// Attaching it to the help menu is the only way to not duplicate it as Help is
		// a special menu just as Preferences is a special menu itme.
		if(helpMenu)
			helpMenu->Append(prefMenuItem);

		delete fileMenu;
	}

#endif
}

wxString OdaGetInstallDir()
{
	wxString InstallDir;

#if defined(ODAMEX_INSTALL_BINDIR)
	const char* bindir_cstr = ODAMEX_INSTALL_BINDIR;
	InstallDir = wxString::FromAscii(bindir_cstr);
#else
	InstallDir = wxGetCwd();
#endif

	return InstallDir;
}

wxString OdaGetDataDir()
{
	wxString DataDir;

#if defined(ODAMEX_INSTALL_DATADIR)
	const char* datadir_cstr = ODAMEX_INSTALL_DATADIR;
	DataDir = wxString::FromAscii(datadir_cstr);
#else
	DataDir =  wxGetCwd();
#endif

	return DataDir;
}
