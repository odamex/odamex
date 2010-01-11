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
//  Player list control class
//
//-----------------------------------------------------------------------------

#include "lst_players.h"

#include <wx/fileconf.h>

IMPLEMENT_DYNAMIC_CLASS(LstOdaPlayerList, wxAdvancedListCtrl)

typedef enum
{
    playerlist_field_name
    ,playerlist_field_ping
    ,playerlist_field_timeingame
    ,playerlist_field_frags
    ,playerlist_field_killcount
    ,playerlist_field_deathcount
    ,playerlist_field_team
    ,playerlist_field_teamscore
    
    ,max_playerlist_fields
} playerlist_fields_t;

void LstOdaPlayerList::SetupPlayerListColumns()
{
    DeleteAllItems();
	DeleteAllColumns();

	wxFileConfig ConfigInfo;
    wxInt32 PlayerListSortOrder, PlayerListSortColumn;

    // Read from the global configuration
    
    // TODO: Column widths
	InsertColumn(playerlist_field_name,_T("Player name"),wxLIST_FORMAT_LEFT,150);
	InsertColumn(playerlist_field_ping,_T("Ping"),wxLIST_FORMAT_LEFT,60);
	InsertColumn(playerlist_field_frags,_T("Frags"),wxLIST_FORMAT_LEFT,70);
    InsertColumn(playerlist_field_killcount,_T("Kill count"),wxLIST_FORMAT_LEFT,85);
    InsertColumn(playerlist_field_deathcount,_T("Death count"),wxLIST_FORMAT_LEFT,100);
    InsertColumn(playerlist_field_timeingame,_T("Time"),wxLIST_FORMAT_LEFT,65);
    
    // Sorting info
    ConfigInfo.Read(wxT("PlayerListSortOrder"), &PlayerListSortOrder, 1);
    ConfigInfo.Read(wxT("PlayerListSortColumn"), &PlayerListSortColumn, 0);

    SetSortColumnAndOrder(PlayerListSortColumn, PlayerListSortOrder);
    
    AddImageSmall(wxArtProvider::GetBitmap(wxART_FIND).ConvertToImage());
}

LstOdaPlayerList::~LstOdaPlayerList() 
{
    wxFileConfig ConfigInfo;
    wxInt32 PlayerListSortOrder, PlayerListSortColumn;

	// Write to the global configuration
	
	// Sorting info
    GetSortColumnAndOrder(PlayerListSortColumn, PlayerListSortOrder);

    ConfigInfo.Write(wxT("PlayerListSortOrder"), PlayerListSortOrder);
    ConfigInfo.Write(wxT("PlayerListSortColumn"), PlayerListSortColumn);
}

void LstOdaPlayerList::AddPlayersToList(const Server &s)
{   
    SetupPlayerListColumns();
    
    if (s.Info.GameType == GT_TeamDeathmatch || 
        s.Info.GameType == GT_CaptureTheFlag)
    {
        InsertColumn(playerlist_field_team,
                           _T("Team"),
                           wxLIST_FORMAT_LEFT,
                           50);
        
        InsertColumn(playerlist_field_teamscore,
                           _T("Team Score"),
                           wxLIST_FORMAT_LEFT,
                           80);
    }
    
    wxUint8 PlayerCount = s.Info.Players.size();
    
    if (!PlayerCount)
        return;
        
    for (wxUint8 i = 0; i < PlayerCount; ++i)
    {
        wxListItem li;
        
        li.SetColumn(playerlist_field_name);
        
        li.SetMask(wxLIST_MASK_TEXT | wxLIST_MASK_IMAGE);

        // We don't want the sort arrow.
        SetColumnImage(li, -1);
        
        if (s.Info.Players[i].Spectator)
        {
            li.SetMask(wxLIST_MASK_TEXT | wxLIST_MASK_IMAGE);
            
            li.SetText(s.Info.Players[i].Name);
            SetColumnImage(li, s.Info.Players[i].Spectator ? 0 : -1);
        }
        else
        {
            li.SetText(s.Info.Players[i].Name);
        }
        
        li.SetId(ALCInsertItem(li));
        
        li.SetColumn(playerlist_field_ping);
        li.SetMask(wxLIST_MASK_TEXT);

        li.SetText(wxString::Format(_T("%d"),
                                    s.Info.Players[i].Ping));
        
        SetItem(li);
        
        li.SetColumn(playerlist_field_frags);        
        li.SetText(wxString::Format(_T("%d"),
                                    s.Info.Players[i].Frags));
        
        SetItem(li);

        li.SetColumn(playerlist_field_killcount);        
        li.SetText(wxString::Format(_T("%d"),
                                    s.Info.Players[i].Kills));
        
        SetItem(li);

        li.SetColumn(playerlist_field_deathcount);        
        li.SetText(wxString::Format(_T("%d"),
                                    s.Info.Players[i].Deaths));
        
        SetItem(li);

        li.SetColumn(playerlist_field_timeingame);        
        li.SetText(wxString::Format(_T("%d"),
                                    s.Info.Players[i].Time));
        
        SetItem(li);
        
        if (s.Info.GameType == GT_TeamDeathmatch || 
            s.Info.GameType == GT_CaptureTheFlag)
		{
            wxString teamstr = _T("Unknown");
            wxInt32 teamscore = 0;
            wxUint16 scorelimit = s.Info.ScoreLimit;
            
            li.SetColumn(playerlist_field_team); 
            
            switch(s.Info.Players[i].Team)
			{
                case 0:
                    li.SetTextColour(*wxBLUE);      
                    teamstr = _T("Blue");
//                    teamscore = s.Info.BlueScore;
                    break;
				case 1:
                    li.SetTextColour(*wxRED);
                    teamstr = _T("Red");
//					teamscore = s.Info.RedScore;
					break;
				case 2:
                    // no gold in 'dem mountains boy.
                    li.SetTextColour(wxColor(255,200,40));
//                    teamscore = s.Info.GoldScore;
                    teamstr = _T("Gold");
					break;
				default:
                    li.SetTextColour(*wxBLACK);
                    teamstr = _T("Unknown");
                    teamscore = 0;
                    scorelimit = 0;
					break;
			}

            li.SetText(teamstr);

            SetItem(li);
            
            li.SetColumn(playerlist_field_teamscore);
            
            li.SetText(wxString::Format(_T("%d/%d"), 
                                        teamscore, 
                                        scorelimit));
            
            SetItem(li);
        }
        
        Sort();
    }
}
