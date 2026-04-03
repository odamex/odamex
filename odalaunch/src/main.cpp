// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2026 by The Odamex Team.
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
//  Main application sequence
//
// AUTHORS:
//  John Corrado
//  Russell Rice (russell at odamex dot net)
//  Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

#include "main.h"

// main dialog resource
#include "xrc_resource.h"

#include "net_io.h"

#include <wx/xrc/xmlres.h>
#include <wx/image.h>
#include <wx/sysopt.h>

using namespace odalpapi;

IMPLEMENT_APP(Application)

bool Application::OnInit()
{
	#ifdef __linux__
		SetClassName("net.odamex.Odamex.Launcher");

		#if wxCHECK_VERSION(3, 3, 0)
			const auto res = wxFileConfig::MigrateLocalFile("odalaunch", wxCONFIG_USE_XDG);
			if(!res.oldPath.empty())
			{
				if(res.error.empty())
				{
					wxLogMessage("Config file was migrated from \"%s\" to \"%s\"",
					             res.oldPath, res.newPath);
				}
				else
				{
					wxLogWarning("Migrating old config failed: %s.", res.error);
				}
			}
			wxStandardPaths::Get().SetFileLayout(wxStandardPaths::FileLayout_XDG);
		#endif
	#endif

	if(BufferedSocket::InitializeSocketAPI() == false)
		return false;

	::wxInitAllImageHandlers();

	wxXmlResource::Get()->InitAllHandlers();

	// load resources
	InitXmlResource();

	// create main window, get size dimensions and show it
	MAIN_DIALOG = new dlgMain(0L);

	if(MAIN_DIALOG)
		MAIN_DIALOG->Show();

	SetTopWindow(MAIN_DIALOG);

	return true;
}

wxInt32 Application::OnExit()
{
	BufferedSocket::ShutdownSocketAPI();

	return 0;
}
