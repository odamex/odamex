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
#include <wx/xrc/xmlres.h>

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

static int ImageList_Spectator = -1;
static int ImageList_RedBullet = -1;
static int ImageList_BlueBullet = -1;

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
    
    ImageList_Spectator = AddImageSmall(wxArtProvider::GetBitmap(wxART_FIND).ConvertToImage());
    ImageList_RedBullet = AddImageSmall(wxXmlResource::Get()->LoadBitmap(wxT("bullet_red")).ConvertToImage());
    ImageList_BlueBullet = AddImageSmall(wxXmlResource::Get()->LoadBitmap(wxT("bullet_blue")).ConvertToImage());
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
    
    size_t PlayerCount = s.Info.Players.size();
    
    if (!PlayerCount)
        return;
    
    for (size_t i = 0; i < PlayerCount; ++i)
    {
        wxListItem li;
        
        li.m_itemId = ALCInsertItem(s.Info.Players[i].Name);
        
        li.SetMask(wxLIST_MASK_TEXT);
               
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
                {
                    li.SetTextColour(*wxBLUE);      
                    teamstr = _T("Blue");
                    SetItemColumnImage(li.m_itemId, playerlist_field_team,
                        ImageList_BlueBullet);
                    teamscore = s.Info.Teams[0].Score;
                }
                break;
				
				case 1:
                {
                    li.SetTextColour(*wxRED);
                    teamstr = _T("Red");
                    SetItemColumnImage(li.m_itemId, playerlist_field_team,
                        ImageList_RedBullet);
					teamscore = s.Info.Teams[1].Score;
                }
                break;
				
				case 2:
                {
                    // no gold in 'dem mountains boy.
                    li.SetTextColour(wxColor(255,200,40));
                    teamstr = wxT("Gold");
					teamscore = s.Info.Teams[2].Score;
                }
                break;
				
				default:
                {
                    li.SetTextColour(*wxBLACK);
                    teamstr = wxT("Unknown");
                    teamscore = 0;
                    scorelimit = 0;
                }
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
        
        // Icons
        // -----
        
        // Magnifying glass icon for spectating players
        SetItemColumnImage(li.m_itemId, playerlist_field_name, 
            s.Info.Players[i].Spectator ? ImageList_Spectator : -1);
    }
        
    Sort();
}
