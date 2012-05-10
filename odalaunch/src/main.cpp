// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2012 by The Odamex Team.
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
//
// AUTHORS: 
//  John Corrado
//  Russell Rice (russell at odamex dot net)
//  Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------


// main dialog resource
#include "xrc_resource.h"

#include "main.h"

#include "net_io.h"

#include <wx/xrc/xmlres.h>
#include <wx/image.h>
#include <wx/sysopt.h>

using namespace odalpapi;

IMPLEMENT_APP(Application)

bool Application::OnInit()
{   
#ifdef __WXMAC__
    // The native listctrl on Mac is problematic for us so always use the generic listctrl
    wxSystemOptions::SetOption(wxMAC_ALWAYS_USE_GENERIC_LISTCTRL, true);
#endif

    if (BufferedSocket::InitializeSocketAPI() == false)
        return false;
    
    ::wxInitAllImageHandlers();

	wxXmlResource::Get()->InitAllHandlers();

    // load resources
    InitXmlResource();

    // create main window, get size dimensions and show it
    MAIN_DIALOG = new dlgMain(0L);
   
    if (MAIN_DIALOG) 
        MAIN_DIALOG->Show();
        
    SetTopWindow(MAIN_DIALOG);
        
    return true;
}

wxInt32 Application::OnExit()
{
    BufferedSocket::ShutdownSocketAPI();
    
    return 0;
}
