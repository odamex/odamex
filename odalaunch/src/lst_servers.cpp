// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2015 by The Odamex Team.
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
#include "oda_defs.h"

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
	m_mnuPopup = wxXmlResource::Get()->LoadMenu("Id_mnuServersPopup");
}

void LstOdaServerList::OnCreateControl(wxWindowCreateEvent& event)
{
	SetupServerListColumns();

	// Propagate the event to the base class as well
	event.Skip();
}

// Finds an index in the server list, via Address
wxInt32 LstOdaServerList::FindServer(wxString Address)
{
	if(!GetItemCount())
		return -1;

	for(wxInt32 i = 0; i < GetItemCount(); i++)
	{
		wxListItem item;
		item.SetId(i);
		item.SetColumn(serverlist_field_address);
		item.SetMask(wxLIST_MASK_TEXT);

		GetItem(item);

		if(item.GetText().IsSameAs(Address))
			return i;
	}

	return -1;
}

// Retrieves the currently selected server in list index form
wxInt32 LstOdaServerList::GetSelectedServerIndex()
{
	wxInt32 i;

	if(!GetItemCount() || !GetSelectedItemCount())
	{
		return -1;
	}

	i = GetFirstSelected();

	return i;
}

void LstOdaServerList::OnCopyAddress(wxCommandEvent& event)
{
	wxListItem li;
	long item = -1;

	li.m_mask = wxLIST_MASK_TEXT;
	li.m_itemId = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);;

	if(li.m_itemId == -1)
		return;

	li.m_col = (int)serverlist_field_address;

	GetItem(li);

	if(wxTheClipboard->Open())
	{
#ifdef __WXGTK__
		wxTheClipboard->UsePrimarySelection(true);
#endif
		wxTheClipboard->SetData(new wxTextDataObject(li.m_text));
		wxTheClipboard->Close();
	}
}

void LstOdaServerList::OnOpenContextMenu(wxContextMenuEvent& event)
{
	wxPoint MousePosition = event.GetPosition();

	if(MousePosition == wxDefaultPosition)
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
	//ConfigInfo.Read("ServerListWidthAttr"), &WidthAttr, 40);
	WidthAttr = 24; // fixed column size
	ConfigInfo.Read("ServerListWidthName", &WidthName, 150);
	ConfigInfo.Read("ServerListWidthPing", &WidthPing, 60);
	ConfigInfo.Read("ServerListWidthPlayers", &WidthPlayers, 80);
	ConfigInfo.Read("ServerListWidthWads", &WidthWads, 150);
	ConfigInfo.Read("ServerListWidthMap", &WidthMap, 60);
	ConfigInfo.Read("ServerListWidthType", &WidthType, 80);
	ConfigInfo.Read("ServerListWidthIwad", &WidthIwad, 100);
	ConfigInfo.Read("ServerListWidthAddress", &WidthAddress, 130);

	// set up the list columns
	InsertColumn(serverlist_field_attr,
	             "",
	             wxLIST_FORMAT_LEFT,
	             WidthAttr);

	// We sort by the numerical value of the item data field, so we can sort
	// passworded servers
	SetSortColumnIsSpecial((wxInt32)serverlist_field_attr);

	InsertColumn(serverlist_field_name,
	             "Server name",
	             wxLIST_FORMAT_LEFT,
	             WidthName);

	InsertColumn(serverlist_field_ping,
	             "Ping",
	             wxLIST_FORMAT_LEFT,
	             WidthPing);

	InsertColumn(serverlist_field_players,
	             "Players",
	             wxLIST_FORMAT_LEFT,
	             WidthPlayers);

	InsertColumn(serverlist_field_wads,
	             "WADs",
	             wxLIST_FORMAT_LEFT,
	             WidthWads);

	InsertColumn(serverlist_field_map,
	             "Map",
	             wxLIST_FORMAT_LEFT,
	             WidthMap);

	InsertColumn(serverlist_field_type,
	             "Type",
	             wxLIST_FORMAT_LEFT,
	             WidthType);

	InsertColumn(serverlist_field_iwad,
	             "Game IWAD",
	             wxLIST_FORMAT_LEFT,
	             WidthIwad);

	InsertColumn(serverlist_field_address,
	             "Address : Port",
	             wxLIST_FORMAT_LEFT,
	             WidthAddress);

	// Passworded server icon
	ImageList_Padlock = AddImageSmall(wxXmlResource::Get()->LoadBitmap("locked_server").ConvertToImage());
	ImageList_PingGreen = AddImageSmall(wxXmlResource::Get()->LoadBitmap("bullet_green").ConvertToImage());
	ImageList_PingOrange = AddImageSmall(wxXmlResource::Get()->LoadBitmap("bullet_orange").ConvertToImage());
	ImageList_PingRed = AddImageSmall(wxXmlResource::Get()->LoadBitmap("bullet_red").ConvertToImage());
	ImageList_PingGray = AddImageSmall(wxXmlResource::Get()->LoadBitmap("bullet_gray").ConvertToImage());

	// Sorting info
	wxInt32 ServerListSortOrder, ServerListSortColumn;

	ConfigInfo.Read("ServerListSortOrder", &ServerListSortOrder, 1);
	ConfigInfo.Read("ServerListSortColumn", &ServerListSortColumn, (int)serverlist_field_name);

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

	//ConfigInfo.Write("ServerListWidthAttr"), WidthAttr);
	ConfigInfo.Write("ServerListWidthName", WidthName);
	ConfigInfo.Write("ServerListWidthPing", WidthPing);
	ConfigInfo.Write("ServerListWidthPlayers", WidthPlayers);
	ConfigInfo.Write("ServerListWidthWads", WidthWads);
	ConfigInfo.Write("ServerListWidthMap", WidthMap);
	ConfigInfo.Write("ServerListWidthType", WidthType);
	ConfigInfo.Write("ServerListWidthIwad", WidthIwad);
	ConfigInfo.Write("ServerListWidthAddress", WidthAddress);

	// Sorting info
	wxInt32 ServerListSortOrder, ServerListSortColumn;

	GetSortColumnAndOrder(ServerListSortColumn, ServerListSortOrder);

	ConfigInfo.Write("ServerListSortOrder", ServerListSortOrder);
	ConfigInfo.Write("ServerListSortColumn", ServerListSortColumn);
}

// Clears text and images located in all cells of a particular item
void LstOdaServerList::ClearItemCells(long item)
{
	int ColumnCount;
	wxListItem ListItem;

	ListItem.m_itemId = item;
	ListItem.m_mask = wxLIST_MASK_TEXT | wxLIST_MASK_IMAGE;
	ListItem.m_text = "";
	ListItem.m_image = -1;

	ColumnCount = GetColumnCount();

	// iterate through each column, resetting everything to 'nothing'
	for(int i = 0; i < ColumnCount; ++i)
	{
		ListItem.m_col = i;

		SetItem(ListItem);
	}
}

void LstOdaServerList::SetBlockedInfo(long item)
{
	wxListItem ListItem;
    wxString Msg;

    Msg = ODA_QRYNORESPONSE;

    // Set ping icon to gray
    SetItemColumnImage(item, serverlist_field_ping, ImageList_PingGray);

	ListItem.m_itemId = item;
	ListItem.m_mask = wxLIST_MASK_TEXT;
	ListItem.m_text = Msg;

	ListItem.m_col = serverlist_field_name;
	SetItem(ListItem);

	// A malicious server can change its name to Msg, so do this for the wad
	// column too
	ListItem.m_col = serverlist_field_wads;
	SetItem(ListItem);
}

/*
    Takes a server structure and adds it to the list control
    if insert is 1, then add an item to the list, otherwise it will
    update the current item with new data
*/
void LstOdaServerList::AddServerToList(const Server& s,
                                       wxInt32 index,
                                       bool insert,
                                       bool IsCustomServer)
{
	wxFileConfig ConfigInfo;
	wxInt32 PQGood, PQPlayable, PQLaggy;
	bool LineHighlight;
	wxString HighlightColour;

	wxInt32 i = 0;
	wxListItem li;

	wxUint64 Ping = 0;
	wxString GameType = "";
	size_t WadCount = 0;

	li.m_mask = wxLIST_MASK_TEXT;

	if(insert)
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

	// Custom server highlighting
	if (IsCustomServer)
    {
        wxColour Colour;

        ConfigInfo.Read(CSHLSERVERS, &LineHighlight, ODA_UICSHIGHTLIGHTSERVERS);
        ConfigInfo.Read(CSHLCOLOUR, &HighlightColour, ODA_UICSHSHIGHLIGHTCOLOUR);

		Colour.Set(HighlightColour);
        
        if (LineHighlight)
            SetItemTextColour(li.GetId(), Colour);
        else
            SetItemTextColour(li.GetId(), GetTextColour());
    }

	// break here so atleast we have an ip address to go by
	if(s.GotResponse() == false)
	{
        SetBlockedInfo(li.GetId());

		return;
	}

	// Server name column
	li.m_col = serverlist_field_name;
	li.m_text = stdstr_towxstr(s.Info.Name);

	SetItem(li);

	// Ping column
	Ping = s.GetPing();

	li.m_col = serverlist_field_ping;
	li.m_text = wxString::Format("%lu", (wxInt32)Ping);

	SetItem(li);

	// Number of players, Maximum players column
	// TODO: acquire max players, max clients and spectators from these 2 and
	// create some kind of graphical column maybe
	li.m_col = serverlist_field_players;
	li.m_text = wxString::Format("%d/%d",(wxInt32)s.Info.Players.size(),(wxInt32)s.Info.MaxClients);

	// Colour the entire text row if there are players
	ConfigInfo.Read(POLHLSERVERS, &LineHighlight, ODA_UIPOLHIGHLIGHTSERVERS);
	ConfigInfo.Read(POLHLSCOLOUR, &HighlightColour, ODA_UIPOLHSHIGHLIGHTCOLOUR);

	if(LineHighlight)
	{
		wxColour Colour;

		Colour.Set(HighlightColour);

		if(s.Info.Players.size())
			SetItemTextColour(li.GetId(), Colour);
		else
			SetItemTextColour(li.GetId(), GetTextColour());
	}

	SetItem(li);

	// WAD files column
	WadCount = s.Info.Wads.size();

	// build a list of pwads
	if(WadCount)
	{
		// pwad list
		std::string wadlist;
		std::string pwad;

		for(i = 2; i < WadCount; ++i)
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
	switch(s.Info.GameType)
	{
	case GT_Cooperative:
	{
		// Detect a single player server
		if(s.Info.MaxPlayers > 1)
			GameType = "Cooperative";
		else
			GameType = "Single Player";
	}
	break;

	case GT_Deathmatch:
	{
		// Detect a 'duel' server
		if(s.Info.MaxPlayers < 3)
			GameType = "Duel";
		else
			GameType = "Deathmatch";
	}
	break;

	case GT_TeamDeathmatch:
	{
		GameType = "Team Deathmatch";
	}
	break;

	case GT_CaptureTheFlag:
	{
		GameType = "Capture The Flag";
	}
	break;

	default:
	{
		GameType = "Unknown";
	}
	break;
	}

	li.m_col = serverlist_field_type;
	li.m_text = GameType;

	SetItem(li);

	// IWAD column
	if(WadCount)
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

	ConfigInfo.Read(ICONPINGQGOOD, &PQGood, ODA_UIPINGQUALITYGOOD);
	ConfigInfo.Read(ICONPINGQPLAYABLE, &PQPlayable,
	                ODA_UIPINGQUALITYPLAYABLE);
	ConfigInfo.Read(ICONPINGQLAGGY, &PQLaggy, ODA_UIPINGQUALITYLAGGY);

	// Coloured bullets for ping quality
	if(Ping < PQGood)
	{
		SetItemColumnImage(li.m_itemId, serverlist_field_ping,
		                   ImageList_PingGreen);
	}
	else if(Ping < PQPlayable)
	{
		SetItemColumnImage(li.m_itemId, serverlist_field_ping,
		                   ImageList_PingOrange);
	}
	else if(Ping < PQLaggy)
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
