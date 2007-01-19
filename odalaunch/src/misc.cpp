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
    
    if (ping > 0)
    {
        list->SetItem(idx, 1, wxString::Format(_T("%d"),ping));
        list->SetItem(idx, 2, wxString::Format(_T("%d/%d"),s.info.numplayers,s.info.maxplayers));       
    
        // pwad list
        wxString wadlist = _T("");    
    
        // build a list of pwads
        if (s.info.numpwads > 0)
            for (i = 0; i < s.info.numpwads; i++)
                wadlist += wxString::Format(_T("%s "), s.info.pwads[i].c_str());
        
        list->SetItem(idx, 3, wadlist);
     
        list->SetItem(idx, 4, s.info.map);
    
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
        list->SetItem(idx, 6, s.info.iwad);
    }
    
    list->SetItem(idx, 7, s.GetAddress());
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
    wxString cmdline = _T("odamex.exe");
#else
    wxString cmdline = _T("./odamex");
#endif

    if (!wxFileExists(cmdline))
    {
        wxMessageBox(wxString::Format(_T("%s not found!"), cmdline.c_str()));
        
        return;
    }

    // when adding waddir string, return 1 less, to get rid of extra delimiter
    wxString dirs = waddirs.Mid(0, waddirs.Length() - 1);
    
    cmdline += wxString::Format(_T(" -waddir %s -connect %s"), dirs.c_str(), s.GetAddress().c_str());
	
    #ifdef __WXMSW__
    cmdline.Replace(_T("\\\\"),_T("\\"), true);
    #else
    cmdline.Replace(_T("////"),_T("//"), true);
    #endif
	
	wxExecute(cmdline, wxEXEC_ASYNC, NULL);
	
}

wxInt32 FindServerInArray(Server s[], wxInt32 itemcnt, wxString Address)
{
    for (wxInt32 i = 0; i < itemcnt; i++)
        if (s[i].GetAddress().IsSameAs(Address))
            return i;
}

