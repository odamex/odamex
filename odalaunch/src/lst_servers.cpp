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
//  Server list control class
//
//-----------------------------------------------------------------------------

#include "lst_servers.h"

#include "str_utils.h"

#include <wx/fileconf.h>
#include <wx/xrc/xmlres.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>

using namespace odalpapi;

IMPLEMENT_DYNAMIC_CLASS(LstOdaServerList, wxAdvancedListCtrl)

BEGIN_EVENT_TABLE(LstOdaServerList, wxAdvancedListCtrl)
    EVT_CONTEXT_MENU(LstOdaServerList::OnOpenContextMenu)

    EVT_MENU(XRCID("Id_mnuServersCopyAddress"), LstOdaServerList::OnCopyAddress)
    
    EVT_WINDOW_CREATE(LstOdaServerList::OnCreateControl)
END_EVENT_TABLE()


static int ImageList_Padlock = -1;
static int ImageList_PingGreen = -1;
static int ImageList_PingOrange = -1;
static int ImageList_PingRed = -1;
static int ImageList_PingGray = -1;

LstOdaServerList::LstOdaServerList()
{
     m_mnuPopup = wxXmlResource::Get()->LoadMenu(wxT("Id_mnuServersPopup"));
}

void LstOdaServerList::OnCreateControl(wxWindowCreateEvent &event)
{
    SetupServerListColumns();
    
    // Propagate the event to the base class as well
    event.Skip();
}

void LstOdaServerList::OnCopyAddress(wxCommandEvent& event)
{
    wxListItem li;
    long item = -1;

    li.m_mask = wxLIST_MASK_TEXT;
    li.m_itemId = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);;

    if (li.m_itemId == -1)
        return;

    li.m_col = (int)serverlist_field_address;

    GetItem(li);

    if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData( new wxTextDataObject(li.m_text) );
        wxTheClipboard->Close();
    }   
}

void LstOdaServerList::OnOpenContextMenu(wxContextMenuEvent& event)
{
    wxPoint MousePosition = event.GetPosition();

    if (MousePosition == wxDefaultPosition)
        MousePosition = wxGetMousePosition();

    PopupMenu(m_mnuPopup, ScreenToClient(MousePosition));
}

void LstOdaServerList::SetupServerListColumns()
{
	wxFileConfig ConfigInfo;
	wxInt32 WidthAttr, WidthName, WidthPing, WidthPlayers, WidthWads, WidthMap, 
        WidthType, WidthIwad, WidthAddress;
	
	DeleteAllColumns();

    // Read in the column widths
    //ConfigInfo.Read(wxT("ServerListWidthAttr"), &WidthAttr, 40);
    WidthAttr = 24; // fixed column size
    ConfigInfo.Read(wxT("ServerListWidthName"), &WidthName, 150);
    ConfigInfo.Read(wxT("ServerListWidthPing"), &WidthPing, 60);
    ConfigInfo.Read(wxT("ServerListWidthPlayers"), &WidthPlayers, 80);
    ConfigInfo.Read(wxT("ServerListWidthWads"), &WidthWads, 150);
    ConfigInfo.Read(wxT("ServerListWidthMap"), &WidthMap, 60);
    ConfigInfo.Read(wxT("ServerListWidthType"), &WidthType, 80);
    ConfigInfo.Read(wxT("ServerListWidthIwad"), &WidthIwad, 100);
    ConfigInfo.Read(wxT("ServerListWidthAddress"), &WidthAddress, 130);

	// set up the list columns
    InsertColumn(serverlist_field_attr, 
        wxT(""), 
        wxLIST_FORMAT_LEFT,
        WidthAttr);
	
	// We sort by the numerical value of the item data field, so we can sort
	// passworded servers
	SetSortColumnIsSpecial((wxInt32)serverlist_field_attr);
	
    InsertColumn(serverlist_field_name, 
        wxT("Server name"), 
        wxLIST_FORMAT_LEFT,
        WidthName);
        
	InsertColumn(serverlist_field_ping,
        wxT("Ping"),
        wxLIST_FORMAT_LEFT,
        WidthPing);
	
	InsertColumn(serverlist_field_players,
        wxT("Players"),
        wxLIST_FORMAT_LEFT,
        WidthPlayers);
        
	InsertColumn(serverlist_field_wads,
        wxT("WADs"),
        wxLIST_FORMAT_LEFT,
        WidthWads);
        
	InsertColumn(serverlist_field_map,
        wxT("Map"),
        wxLIST_FORMAT_LEFT,
        WidthMap);
        
	InsertColumn(serverlist_field_type,
        wxT("Type"),
        wxLIST_FORMAT_LEFT,
        WidthType);
        
	InsertColumn(serverlist_field_iwad,
        wxT("Game IWAD"),
        wxLIST_FORMAT_LEFT,
        WidthIwad);
        
	InsertColumn(serverlist_field_address,
        wxT("Address : Port"),
        wxLIST_FORMAT_LEFT,
        WidthAddress);
	
	// Passworded server icon
    ImageList_Padlock = AddImageSmall(wxXmlResource::Get()->LoadBitmap(wxT("locked_server")).ConvertToImage());
    ImageList_PingGreen = AddImageSmall(wxXmlResource::Get()->LoadBitmap(wxT("bullet_green")).ConvertToImage());
    ImageList_PingOrange = AddImageSmall(wxXmlResource::Get()->LoadBitmap(wxT("bullet_orange")).ConvertToImage());
    ImageList_PingRed = AddImageSmall(wxXmlResource::Get()->LoadBitmap(wxT("bullet_red")).ConvertToImage());
    ImageList_PingGray = AddImageSmall(wxXmlResource::Get()->LoadBitmap(wxT("bullet_gray")).ConvertToImage());

    // Sorting info
    wxInt32 ServerListSortOrder, ServerListSortColumn;

    ConfigInfo.Read(wxT("ServerListSortOrder"), &ServerListSortOrder, 1);
    ConfigInfo.Read(wxT("ServerListSortColumn"), &ServerListSortColumn, (int)serverlist_field_name);

    SetSortColumnAndOrder(ServerListSortColumn, ServerListSortOrder);
}

LstOdaServerList::~LstOdaServerList()
{
	wxFileConfig ConfigInfo;
	wxInt32 WidthAttr, WidthName, WidthPing, WidthPlayers, WidthWads, WidthMap, 
        WidthType, WidthIwad, WidthAddress;
	
	//WidthAttr = GetColumnWidth(serverlist_field_attr);
	WidthName = GetColumnWidth(serverlist_field_name);
	WidthPing = GetColumnWidth(serverlist_field_ping);
	WidthPlayers = GetColumnWidth(serverlist_field_players);
	WidthWads = GetColumnWidth(serverlist_field_wads);
	WidthMap = GetColumnWidth(serverlist_field_map);
	WidthType = GetColumnWidth(serverlist_field_type);
	WidthIwad = GetColumnWidth(serverlist_field_iwad);
	WidthAddress = GetColumnWidth(serverlist_field_address);

    //ConfigInfo.Write(wxT("ServerListWidthAttr"), WidthAttr);
    ConfigInfo.Write(wxT("ServerListWidthName"), WidthName);
    ConfigInfo.Write(wxT("ServerListWidthPing"), WidthPing);
    ConfigInfo.Write(wxT("ServerListWidthPlayers"), WidthPlayers);
    ConfigInfo.Write(wxT("ServerListWidthWads"), WidthWads);
    ConfigInfo.Write(wxT("ServerListWidthMap"), WidthMap);
    ConfigInfo.Write(wxT("ServerListWidthType"), WidthType);
    ConfigInfo.Write(wxT("ServerListWidthIwad"), WidthIwad);
    ConfigInfo.Write(wxT("ServerListWidthAddress"), WidthAddress);

	// Sorting info
    wxInt32 ServerListSortOrder, ServerListSortColumn;

    GetSortColumnAndOrder(ServerListSortColumn, ServerListSortOrder);

    ConfigInfo.Write(wxT("ServerListSortOrder"), ServerListSortOrder);
    ConfigInfo.Write(wxT("ServerListSortColumn"), ServerListSortColumn);
}

// Clears text and images located in all cells of a particular item
void LstOdaServerList::ClearItemCells(long item)
{
    int ColumnCount;
    wxListItem ListItem;
    
    ListItem.m_itemId = item;
    ListItem.m_mask = wxLIST_MASK_TEXT | wxLIST_MASK_IMAGE;
    ListItem.m_text = wxT("");
    ListItem.m_image = -1;
    
    ColumnCount = GetColumnCount();
    
    // iterate through each column, resetting everything to 'nothing'
    for (int i = 0; i < ColumnCount; ++i)
    {
        ListItem.m_col = i;
        
        SetItem(ListItem);
    }
}

/*
    Takes a server structure and adds it to the list control
    if insert is 1, then add an item to the list, otherwise it will
    update the current item with new data
*/
void LstOdaServerList::AddServerToList(const Server &s, 
                                        wxInt32 index, 
                                        bool insert)
{
    wxFileConfig ConfigInfo;
    wxInt32 PQGood, PQPlayable, PQLaggy;
    
    wxInt32 i = 0;
    wxListItem li;
    
    wxUint64 Ping = 0;
    wxString GameType = wxT("");
    size_t WadCount = 0;
    
    li.m_mask = wxLIST_MASK_TEXT;

    if (insert)
    {
        li.m_itemId = ALCInsertItem();
    }
    else
    {
        // All cells must be cleared on a "replace" operation, otherwise if the 
        // server failed to respond a second time 'round, we would end up with 
        // some stale data (which is highly dependent on the response check 
        // below)
        ClearItemCells(index);
        
        li.m_itemId = index;
    }
       
    // Address column
    li.m_col = serverlist_field_address;    
    li.m_text = stdstr_towxstr(s.GetAddress());

    SetItem(li);

    // break here so atleast we have an ip address to go by
    if (s.GotResponse() == false)
        return;

    // Server name column
    li.m_col = serverlist_field_name;
    li.m_text = stdstr_towxstr(s.Info.Name);
       
    SetItem(li);
      
    // Ping column
    Ping = s.GetPing();

    li.m_col = serverlist_field_ping;
    li.m_text = wxString::Format(_T("%llu"), Ping);

    SetItem(li);

    // Number of players, Maximum players column
    // TODO: acquire max players, max clients and spectators from these 2 and
    // create some kind of graphical column maybe
    li.m_col = serverlist_field_players;
    li.m_text = wxString::Format(_T("%d/%d"),s.Info.Players.size(),s.Info.MaxClients);
    
    // Colour the entire text row if there are players
    // TODO: Allow the user to select prefered colours
    if (s.Info.Players.size())
        SetItemTextColour(li.GetId(), wxColor(0, 192, 0));
    else
        SetItemTextColour(li.GetId(), GetTextColour());

    SetItem(li); 
    
    // WAD files column
    WadCount = s.Info.Wads.size();
    
    // build a list of pwads
    if (WadCount)
    {
        // pwad list
        std::string wadlist;
        std::string pwad;
            
        for (i = 2; i < WadCount; ++i)
        {
            pwad = s.Info.Wads[i].Name.substr(0, s.Info.Wads[i].Name.find('.'));
            
            wadlist.append(pwad);
            wadlist.append(" ");
        }
            
        li.m_col = serverlist_field_wads;
        li.m_text = stdstr_towxstr(wadlist);
    
        SetItem(li);
    }

    // Map name column
    li.m_col = serverlist_field_map;
    li.m_text = stdstr_towxstr(s.Info.CurrentMap).Upper();
    
    SetItem(li);
       
    // Game type column
    switch (s.Info.GameType)
    {
        case GT_Cooperative:
        {
            // Detect a single player server
            if (s.Info.MaxPlayers > 1)
                GameType = wxT("Cooperative");
            else
                GameType = wxT("Single Player");
        }
        break;
        
        case GT_Deathmatch:
        {
            // Detect a 'duel' server
            if (s.Info.MaxPlayers < 3)
                GameType = wxT("Duel");
            else
                GameType = wxT("Deathmatch");
        }
        break;
        
        case GT_TeamDeathmatch:
        {
            GameType = wxT("Team Deathmatch");
        }
        break;
        
        case GT_CaptureTheFlag:
        {
            GameType = wxT("Capture The Flag");
        }
        break;
        
        default:
        {
            GameType = wxT("Unknown");
        }
        break;
    }
    
    li.m_col = serverlist_field_type;
    li.m_text = GameType;
    
    SetItem(li);

    // IWAD column
    if (WadCount)
    {
        std::string iwad;
        iwad = s.Info.Wads[1].Name.substr(0, s.Info.Wads[1].Name.find('.'));
        
        li.m_col = serverlist_field_iwad;
        li.m_text = stdstr_towxstr(iwad);
    }
    
    SetItem(li);
    
    // Icons
    // -----
    
    // Padlock icon for passworded servers
    bool IsPasswordEmpty = s.Info.PasswordHash.empty();

    SetItemColumnImage(li.m_itemId, serverlist_field_attr, 
        (IsPasswordEmpty ? -1 : ImageList_Padlock));   

    // Allows us to sort by passworded servers as well
    SetItemData(li.m_itemId, (IsPasswordEmpty ? 0 : 1));
    
    ConfigInfo.Read(wxT("IconPingQualityGood"), &PQGood, 150);
    ConfigInfo.Read(wxT("IconPingQualityPlayable"), &PQPlayable, 300);
    ConfigInfo.Read(wxT("IconPingQualityLaggy"), &PQLaggy, 350);
    
    // Coloured bullets for ping quality
    if (Ping < PQGood)
    {
        SetItemColumnImage(li.m_itemId, serverlist_field_ping, 
            ImageList_PingGreen);
    }
    else if (Ping < PQPlayable)
    {
        SetItemColumnImage(li.m_itemId, serverlist_field_ping, 
            ImageList_PingOrange);
    }
    else if (Ping < PQLaggy)
    {
        SetItemColumnImage(li.m_itemId, serverlist_field_ping, 
            ImageList_PingRed);
    }
    else
    {
        SetItemColumnImage(li.m_itemId, serverlist_field_ping, 
            ImageList_PingGray);
    }
}
