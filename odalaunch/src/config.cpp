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
//  Config object that saves where Odamex usually saves configs.
//
//-----------------------------------------------------------------------------

#include "config.h"

#include <wx/app.h>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>

namespace config
{

wxFileName GetWriteDir()
{
#if defined(_WIN32)
	// Has Odamex been installed?
	wxFileName installed(wxStandardPaths::Get().GetExecutablePath());
	installed.SetFullName("odamex-installed.txt");
	if (installed.FileExists())
	{
		// Does the user folder exist?
		wxFileName userPath(wxStandardPaths::Get().GetDocumentsDir(), "");
		userPath.AppendDir("My Games");
		userPath.AppendDir("Odamex");
		OutputDebugStringA(userPath.GetFullPath());
		if (userPath.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL))
		{
			return userPath;
		}
		else
		{
			wxMessageBox(wxString::Format("Failed to create %s directory.",
			                              userPath.GetFullPath()),
			             "Error", wxOK | wxICON_ERROR);
			wxExit();
		}
	}

	return wxFileName::wxFileName(wxFileName::GetCwd(), "");
#else

#endif
}

wxFileName GetConfigFile()
{
	wxFileName config = GetWriteDir();
	config.SetFullName("odalaunch.ini");
	return config;
}

} // namespace config
