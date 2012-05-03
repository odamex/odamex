// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2010 by The Odamex Team.
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

#include <wx/fileconf.h>
#include <wx/xrc/xmlres.h>

#include "lst_players.h"
#include "str_utils.h"

using namespace odalpapi;

/* XPM */
static const char *find_xpm[] = {
/* columns rows colors chars-per-pixel */
"16 16 41 1",
"y c #A06959",
"9 c #A7DAF2",
"$ c #B5CAD7",
"> c #35B4E1",
"t c #6B98B8",
"w c #B6E0F4",
"q c #AEC9D7",
"1 c #5A89A6",
"+ c #98B3C6",
"4 c #EAF6FC",
"3 c #DEF1FA",
"= c #4CBCE3",
"d c #DB916B",
"X c #85A7BC",
"s c #D8BCA4",
"o c #749BB4",
"e c #BCD9EF",
"* c #62B4DD",
"< c #91D2EF",
"a c #E6DED2",
"0 c #E9F4FB",
"  c None",
"@ c #A0BACB",
"O c #AABFCD",
"i c #6591AE",
": c #B9CBD5",
"- c #71C5E7",
"5 c #D3ECF8",
"% c #81A3B9",
"6 c #8AD0EE",
"8 c #FDFDFE",
"p c #8EA9BC",
"r c #B6D5EE",
", c #81CCEB",
". c #ACC4D3",
"; c #AFD1DE",
"7 c #EFF8FC",
"u c #C2CBDB",
"# c #C0D1DC",
"2 c #CAD6E1",
"& c #8FB0C3",
/* pixels */
"                ",
"       .XooXO   ",
"      +@###$+%  ",
"     .&#*==-;@@ ",
"     o:*>,<--:X ",
"     12>-345-#% ",
"     12>678392% ",
"     %$*,3059q& ",
"     @Oq,wwer@@ ",
"      t@q22q&+  ",
"    yyui+%o%p   ",
"   yasy         ",
"  yasdy         ",
"  asdy          ",
"  sdy           ",
"                "
};


IMPLEMENT_DYNAMIC_CLASS(LstOdaPlayerList, wxAdvancedListCtrl)

typedef enum
{
     playerlist_field_attr
    ,playerlist_field_name
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

// Special case
static wxInt32 WidthTeam, WidthTeamScore;

void LstOdaPlayerList::SetupPlayerListColumns()
{
    DeleteAllItems();
	DeleteAllColumns();

	wxFileConfig ConfigInfo;
    wxInt32 PlayerListSortOrder, PlayerListSortColumn;

    // Read from the global configuration
	wxInt32 WidthAttr, WidthName, WidthPing, WidthFrags, WidthKillCount, 
        WidthDeathCount, WidthTime;

    //ConfigInfo.Read(wxT("PlayerListWidthName"), &WidthName, 150);
    WidthAttr = 24; // fixed column size
    ConfigInfo.Read(wxT("PlayerListWidthName"), &WidthName, 150);
    ConfigInfo.Read(wxT("PlayerListWidthPing"), &WidthPing, 60);
    ConfigInfo.Read(wxT("PlayerListWidthFrags"), &WidthFrags, 70);
    ConfigInfo.Read(wxT("PlayerListWidthKillCount"), &WidthKillCount, 85);
    ConfigInfo.Read(wxT("PlayerListWidthDeathCount"), &WidthDeathCount, 100);
    ConfigInfo.Read(wxT("PlayerListWidthTime"), &WidthTime, 65);
    ConfigInfo.Read(wxT("PlayerListWidthTeam"), &WidthTeam, 65);
    ConfigInfo.Read(wxT("PlayerListWidthTeamScore"), &WidthTeamScore, 100);
    
	InsertColumn(playerlist_field_attr,
                wxT(""),
                wxLIST_FORMAT_LEFT,
                WidthAttr);

	// We sort by the numerical value of the item data field, so we can sort
	// spectators/downloaders etc
	SetSortColumnIsSpecial((wxInt32)playerlist_field_attr);

	InsertColumn(playerlist_field_name,
                wxT("Player name"),
                wxLIST_FORMAT_LEFT,
                WidthName);

	InsertColumn(playerlist_field_ping,
                wxT("Ping"),
                wxLIST_FORMAT_LEFT,
                WidthPing);

	InsertColumn(playerlist_field_frags,
                wxT("Frags"),
                wxLIST_FORMAT_LEFT,
                WidthFrags);

    InsertColumn(playerlist_field_killcount,
                wxT("Kill count"),
                wxLIST_FORMAT_LEFT,
                WidthKillCount);

    InsertColumn(playerlist_field_deathcount, 
                wxT("Death count"),
                wxLIST_FORMAT_LEFT,
                WidthDeathCount);

    InsertColumn(playerlist_field_timeingame,
                wxT("Time"),
                wxLIST_FORMAT_LEFT,
                WidthTime);
    
    // Sorting info
    ConfigInfo.Read(wxT("PlayerListSortOrder"), &PlayerListSortOrder, 1);
    ConfigInfo.Read(wxT("PlayerListSortColumn"), &PlayerListSortColumn, 0);

    SetSortColumnAndOrder(PlayerListSortColumn, PlayerListSortOrder);
    
    ImageList_Spectator = AddImageSmall(find_xpm);
    ImageList_BlueBullet = AddImageSmall(wxXmlResource::Get()->LoadBitmap(wxT("bullet_blue")).ConvertToImage());
    ImageList_RedBullet = AddImageSmall(wxXmlResource::Get()->LoadBitmap(wxT("bullet_red")).ConvertToImage());
}

LstOdaPlayerList::~LstOdaPlayerList() 
{
    wxFileConfig ConfigInfo;
    wxInt32 PlayerListSortOrder, PlayerListSortColumn;
    wxListItem li;

	// Write to the global configuration
	
	// Sorting info
    GetSortColumnAndOrder(PlayerListSortColumn, PlayerListSortOrder);

    ConfigInfo.Write(wxT("PlayerListSortOrder"), PlayerListSortOrder);
    ConfigInfo.Write(wxT("PlayerListSortColumn"), PlayerListSortColumn);

	wxInt32 WidthName, WidthPing, WidthFrags, WidthKillCount, WidthDeathCount, 
        WidthTime;

	WidthName = GetColumnWidth(playerlist_field_name);
	WidthPing = GetColumnWidth(playerlist_field_ping);
	WidthFrags = GetColumnWidth(playerlist_field_frags);
	WidthKillCount = GetColumnWidth(playerlist_field_killcount);
	WidthDeathCount = GetColumnWidth(playerlist_field_deathcount);
	WidthTime = GetColumnWidth(playerlist_field_timeingame);

    ConfigInfo.Write(wxT("PlayerListWidthName"), WidthName);
    ConfigInfo.Write(wxT("PlayerListWidthPing"), WidthPing);
    ConfigInfo.Write(wxT("PlayerListWidthFrags"), WidthFrags);
    ConfigInfo.Write(wxT("PlayerListWidthKillCount"), WidthKillCount);
    ConfigInfo.Write(wxT("PlayerListWidthDeathCount"), WidthDeathCount);
    ConfigInfo.Write(wxT("PlayerListWidthTime"), WidthTime);

    // Team and Team Scores are shown dynamically, so handle the case of them
    // not existing
    if (!GetColumn((int)playerlist_field_team, li) || 
        !GetColumn((int)playerlist_field_teamscore, li))
    {
        return;
    }

    WidthTeam = GetColumnWidth(playerlist_field_team);
    WidthTeamScore = GetColumnWidth(playerlist_field_teamscore);

    ConfigInfo.Write(wxT("PlayerListWidthTeam"), WidthTeam);
    ConfigInfo.Write(wxT("PlayerListWidthTeamScore"), WidthTeamScore);
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
                           WidthTeam);
        
        InsertColumn(playerlist_field_teamscore,
                           _T("Team Score"),
                           wxLIST_FORMAT_LEFT,
                           WidthTeamScore);
    }
    
    size_t PlayerCount = s.Info.Players.size();
    
    if (!PlayerCount)
        return;
    
    for (size_t i = 0; i < PlayerCount; ++i)
    {
        wxListItem li;
        
        li.m_itemId = ALCInsertItem();
        
        li.SetMask(wxLIST_MASK_TEXT);
        
        li.SetColumn(playerlist_field_name);
        li.SetText(stdstr_towxstr(s.Info.Players[i].Name));

        SetItem(li);

        li.SetColumn(playerlist_field_ping);

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
            wxUint8 TeamId;
            wxString TeamName = wxT("Unknown");
            wxUint32 TeamColour = 0;
            wxInt16 TeamScore = 0;
            
            li.SetColumn(playerlist_field_team); 
            
            // Player has a team index it is associated with
            TeamId = s.Info.Players[i].Team;

            // Acquire info about the team the player is in
            // TODO: Hack alert, we only accept blue and red teams, since
            // dynamic teams are not implemented and GOLD/NONE teams exist
            // Just accept these 2 for now
            if ((TeamId == 0) || (TeamId == 1))
            {
                wxUint8 TC_Red = 0, TC_Green = 0, TC_Blue = 0;

                TeamName = stdstr_towxstr(s.Info.Teams[TeamId].Name);
                TeamColour = s.Info.Teams[TeamId].Colour;
                TeamScore = s.Info.Teams[TeamId].Score;

                TC_Red = ((TeamColour >> 16) & 0x00FFFFFF);
                TC_Green = ((TeamColour >> 8) & 0x0000FFFF);
                TC_Blue = (TeamColour & 0x000000FF);

                li.SetTextColour(wxColour(TC_Red, TC_Green, TC_Blue));
            }
            else
                TeamId = 3;

            li.SetText(TeamName);

            SetItem(li);

            // Set the team colour and bullet icon for the player
            switch(TeamId)
			{
                case 0:
                {
                    SetItemColumnImage(li.m_itemId, playerlist_field_team,
                        ImageList_BlueBullet);
                }
                break;
				
				case 1:
                {
                    SetItemColumnImage(li.m_itemId, playerlist_field_team,
                        ImageList_RedBullet);
                }
                break;

				default:
                {
                    li.SetTextColour(*wxBLACK);
                }
                break;
			}

            li.SetColumn(playerlist_field_teamscore);
            
            li.SetText(wxString::Format(wxT("%d"), TeamScore));
            
            SetItem(li);
        }
        
        // Icons
        // -----

        bool IsSpectator = s.Info.Players[i].Spectator;
        
        // Magnifying glass icon for spectating players
        SetItemColumnImage(li.m_itemId, playerlist_field_attr, 
             IsSpectator ? ImageList_Spectator : -1);

        // Allows us to sort by spectators
        SetItemData(li.m_itemId, (IsSpectator ? 1 : 0));
    }
        
    Sort();
}
