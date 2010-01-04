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
//	Miscellaneous stuff for gui
//	AUTHOR:	Russell Rice, John D Corrado
//
//-----------------------------------------------------------------------------

#include <wx/settings.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/msgdlg.h>
#include <wx/colour.h>
#include <wx/fileconf.h>

#include "dlg_config.h"
#include "net_packet.h"
#include "misc.h"


wxString *CheckPWADS(wxString pwads, wxString waddirs)
{
    wxStringTokenizer wads(pwads, _T(' '));
    wxStringTokenizer dirs(waddirs, _T(' '));
    wxString wadfn = _T("");
    
    // validity array counter
    wxUint32 i = 0;
    
    // allocate enough space for all wads
    wxString *inv_wads = new wxString [wads.CountTokens()];
    
    if (!inv_wads)
        return NULL;
       
    // begin checking
    while (dirs.HasMoreTokens())
    {
        while (wads.HasMoreTokens())
        {
            #ifdef __WXMSW__
            wadfn = wxString::Format(wxT("%s\\%s"), dirs.GetNextToken().c_str(), wads.GetNextToken().c_str());
            #else
            wadfn = wxString::Format(wxT("%s/%s"), dirs.GetNextToken().c_str(), wads.GetNextToken().c_str());
            #endif
            
            if (wxFileExists(wadfn))
            {
                inv_wads[i] = wadfn;                

                i++;                
            }
        }
    }
    
    if (i)
        return inv_wads;
    else
    {
        delete[] inv_wads;
        
        return NULL;
    }
}

void LaunchGame(wxString Address, wxString ODX_Path, wxString waddirs, wxString Password)
{
    wxFileConfig ConfigInfo;
    wxString ExtraCmdLineArgs;
    
    if (ODX_Path.IsEmpty())
    {
        wxMessageBox(wxT("Your Odamex path is empty!"));
        
        return;
    }
    
    #ifdef __WXMSW__
      wxString binname = ODX_Path + wxT('\\') + _T("odamex");
    #elif __WXMAC__
      wxString binname = ODX_Path + wxT("/odamex.app/Contents/MacOS/odamex");
    #else
      wxString binname = ODX_Path + wxT("/odamex");
    #endif

    wxString cmdline = wxT("");

    wxString dirs = waddirs.Mid(0, waddirs.Length());
    
    cmdline += wxString::Format(wxT("%s"), binname.c_str());
    
    if (!Address.IsEmpty())
		cmdline += wxString::Format(wxT(" -connect %s"),
									Address.c_str());
	
	if (!Password.IsEmpty())
        cmdline += wxString::Format(wxT(" %s"),
									Password.c_str());
	
	// this is so the client won't mess up parsing
	if (!dirs.IsEmpty())
        cmdline += wxString::Format(wxT(" -waddir \"%s\""), 
                                    dirs.c_str());

    // Check for any user command line arguments
    ConfigInfo.Read(wxT(EXTRACMDLINEARGS), &ExtraCmdLineArgs, wxT(""));
    
    if (!ExtraCmdLineArgs.IsEmpty())
        cmdline += wxString::Format(_T(" %s"), 
                                    ExtraCmdLineArgs.c_str());

	if (wxExecute(cmdline, wxEXEC_ASYNC, NULL) == -1)
        wxMessageBox(wxString::Format(wxT("Could not start %s!"), 
                                        binname.c_str()));
	
}
