// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2006-2007 by The Odamex Team.
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
//	Main application sequence
//	AUTHOR:	Russell Rice, John D Corrado
//
//-----------------------------------------------------------------------------


// main dialog resource
#include "res/xrc_resource.h"

#include "main.h"

#include <wx/xrc/xmlres.h>
#include <wx/image.h>

IMPLEMENT_APP(Application)

bool Application::OnInit()
{
    // multiple instance checking
    wxString InstanceName = wxString::Format(_T("Odamex Launcher - %s"), wxGetUserId().c_str());
    
    m_checker = new wxSingleInstanceChecker(InstanceName);
    
    if (m_checker->IsAnotherRunning())
    {
        wxLogError(_T("Odamex Launcher is already running, aborting."));

        return false;
    }
    
    // now run application
	::wxInitAllImageHandlers();

	wxXmlResource::Get()->InitAllHandlers();

    // load resources
    InitXmlResource();

    // create main window and show it
    MAIN_DIALOG = new dlgMain(0L);
    
    if (MAIN_DIALOG) 
        MAIN_DIALOG->Show();
    
    return true;
}

wxInt32 Application::OnExit()
{
	if(m_checker)
		delete m_checker;
		
	//if(MAIN_DIALOG)
	//	delete MAIN_DIALOG;

    return 0;
}
