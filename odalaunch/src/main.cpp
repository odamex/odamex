// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2009 by The Odamex Team.
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
//  Michael Wood (mwoodj at knology dot net)
//
//-----------------------------------------------------------------------------


// main dialog resource
#include "xrc_resource.h"

#include "main.h"

#include "net_io.h"

#include <wx/xrc/xmlres.h>
#include <wx/image.h>



IMPLEMENT_APP(Application)

bool Application::OnInit()
{   
    wxFileConfig ConfigInfo;   
    wxInt32 WindowWidth, WindowHeight;
    
    if (InitializeSocketAPI() == false)
        return false;
    
    ::wxInitAllImageHandlers();

	wxXmlResource::Get()->InitAllHandlers();

    // load resources
    InitXmlResource();

    // create main window, get size dimensions and show it
    MAIN_DIALOG = new dlgMain(0L);

    ConfigInfo.Read(wxT("MainWindowWidth"), 
                    &WindowWidth, 
                    MAIN_DIALOG->GetSize().GetWidth());
                    
    ConfigInfo.Read(wxT("MainWindowHeight"), 
                    &WindowHeight, 
                    MAIN_DIALOG->GetSize().GetHeight());
    
    MAIN_DIALOG->SetSize(WindowWidth, WindowHeight);
    
    if (MAIN_DIALOG) 
        MAIN_DIALOG->Show();
        
    SetTopWindow(MAIN_DIALOG);
        
    return true;
}

wxInt32 Application::OnExit()
{
    ShutdownSocketAPI();
    
    return 0;
}
