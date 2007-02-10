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
//	Miscellaneous stuff for gui
//	AUTHOR:	Russell Rice, John D Corrado
//
//-----------------------------------------------------------------------------


#include "lst_custom.h"

#include <wx/settings.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/msgdlg.h>

#include "net_packet.h"
#include "misc.h"

/*
    Takes a server structure and adds it to the list control
    if insert is 1, then add an item to the list, otherwise it will
    update the current item with new data
*/
void AddServerToList(wxListCtrl *list, Server &s, wxInt32 index, wxInt8 insert)
{
    wxInt32 i = 0, idx = 0;
    
    if (s.GetAddress() == _T("0.0.0.0:0"))
        return;
        
    // are we adding a new item?
    if (insert == 1)    
        idx = list->InsertItem(index, s.info.name);
    else
    {
        idx = index;
        // update server name if we have a blank one
        list->SetItem(idx, 0, s.info.name, -1);
    }
    
    wxInt32 ping = s.GetPing();
    
    list->SetItem(idx, 7, s.GetAddress());
    
    if (ping > 0)
    {
        list->SetItem(idx, 1, wxString::Format(_T("%d"),ping));
        list->SetItem(idx, 2, wxString::Format(_T("%d/%d"),s.info.numplayers,s.info.maxplayers));       
    
        // build a list of pwads
        if (s.info.numpwads > 0)
        {
            // pwad list
            wxString wadlist = _T("");
            wxString pwad = _T("");
            
            for (i = 0; i < s.info.numpwads; i++)
            {
                pwad = s.info.pwads[i].Mid(0, s.info.pwads[i].Find('.'));
                wadlist += wxString::Format(_T("%s "), pwad.c_str());
            }
            
            list->SetItem(idx, 3, wadlist);
        }
            
        list->SetItem(idx, 4, s.info.map.Upper());
    
        wxString gmode = _T("");
    
        switch(s.info.teamplay)
        {
            case 1:
                gmode += _T("TM ");
                break;
        }

        switch (s.info.gametype)
        {
            case 0:
                gmode += _T("COOP ");
                break;
            case 1:
                gmode += _T("DM ");
                break;
            case 2:
                gmode += _T("DM2 ");
                break;
            default:
                gmode += _T("?? ");
                break;
        }

        switch(s.info.ctf)
        {
            case 1:
                gmode += _T("CTF ");
                break;
        }
          
        list->SetItem(idx, 5, gmode);

        // trim off the .wad
        wxString iwad = s.info.iwad.Mid(0, s.info.iwad.Find('.'));
        
        list->SetItem(idx, 6, iwad);
    }
}

void AddPlayersToList(wxAdvancedListCtrl *list, Server &s)
{   
    if (s.info.numplayers > 0)
    for (wxInt32 i = 0; i < s.info.numplayers; i++)
    {
        wxInt32 idx = list->InsertItem(i,s.info.playerinfo[i].name);
        
        wxString teamstr = _T("");
        
        list->SetItem(idx, 1, wxString::Format(_T("%d"),s.info.playerinfo[i].frags));
        list->SetItem(idx, 2, wxString::Format(_T("%d"),s.info.playerinfo[i].ping));
        
        if (s.info.teamplay == 1)
		{
            switch(s.info.playerinfo[i].team)
			{
                case 0:
                    teamstr = _T("NONE");
					break;
                case 1:
					teamstr = _T("BLUE");
					break;
				case 2:
                    teamstr = _T("RED");
					break;
				case 3:
                    teamstr = _T("GOLD");
					break;
				default:
                    teamstr = _T("UNKNOWN!");
					break;
			}
            list->SetItem(idx, 3, teamstr);
        }
            
        list->ColourListItem(i);
    }
    
}

void LaunchGame(Server &s, wxString waddirs)
{
    #ifdef __WXMSW__
    wxString binname = _T("odamex");
    #else
    wxString binname = _T("./odamex");
    #endif
    
    wxString cmdline = _T("");

    // when adding waddir string, return 1 less, to get rid of extra delimiter
    wxString dirs = waddirs.Mid(0, waddirs.Length() - 1);
    
    cmdline += wxString::Format(_T("%s -connect %s"), 
                                binname.c_str(), 
                                s.GetAddress().c_str());
	
	// this is so the client won't mess up parsing
	if (!dirs.IsEmpty())
        cmdline += wxString::Format(_T(" -waddir \"%s\""), 
                                    dirs.c_str());

	if (wxExecute(cmdline, wxEXEC_ASYNC, NULL) == -1)
        wxMessageBox(wxString::Format(_T("Could not start %s!"), 
                                        binname.c_str()));
	
}
