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
//  Server list control class
//
//-----------------------------------------------------------------------------

#include "lst_servers.h"

#include "str_utils.h"

#include <wx/fileconf.h>
#include <wx/xrc/xmlres.h>

using namespace odalpapi;

IMPLEMENT_DYNAMIC_CLASS(LstOdaServerList, wxAdvancedListCtrl)

/* XPM */
static const char *padlock_xpm[] = 
{
"16 16 157 2 0 0",
"   c #000000",
"!  c #1F0000",
"#  c #230100",
"$  c #740700",
"%  c #790900",
"&  c #1D0A00",
"'  c #240C00",
"(  c #560D00",
")  c #621200",
"*  c #FFAD00",
"+  c #EB9401",
",  c #FFAF01",
"-  c #FFB901",
".  c #AB6302",
"/  c #DB6B03",
"0  c #FFB707",
"1  c #FFBB08",
"2  c #BC8209",
"3  c #FFAE09",
"4  c #E27C0B",
"5  c #FFBA0B",
"6  c #FFBE0B",
"7  c #FFC50B",
"8  c #E78F0C",
"9  c #E7900D",
":  c #F6A00D",
";  c #FFB10D",
"<  c #FFC70E",
"=  c #B8770F",
">  c #F49F0F",
"?  c #FFB910",
"@  c #FFCB13",
"A  c #FFCA14",
"B  c #FFCD15",
"C  c #F4AE17",
"D  c #FFD517",
"E  c #FFD018",
"F  c #FFD819",
"G  c #FFD91A",
"H  c #FFD81B",
"I  c #FFD81C",
"J  c #C9921D",
"K  c #FFD91D",
"L  c #FFDF1D",
"M  c #FFE11E",
"N  c #FFE51E",
"O  c #FFB620",
"P  c #FFE321",
"Q  c #FFC322",
"R  c #FFD323",
"S  c #FFB924",
"T  c #FFE324",
"U  c #FFDB25",
"V  c #FFC026",
"W  c #FFF12A",
"X  c #B2742C",
"Y  c #E5B72C",
"Z  c #FFF02C",
"[  c #FFF02E",
"\\  c #FFF12E",
"]  c #B57B30",
"^  c #FFD530",
"_  c #FFD432",
"`  c #B37933",
"a  c #FFFB33",
"b  c #D69A34",
"c  c #FFDC34",
"d  c #B97E35",
"e  c #FFFB35",
"f  c #D69C36",
"g  c #D69A37",
"h  c #E7BA37",
"i  c #CF9939",
"j  c #D6A039",
"k  c #D0953A",
"l  c #CF963B",
"m  c #CF993B",
"n  c #D6A33B",
"o  c #FFDC3B",
"p  c #D5A83D",
"q  c #D6A83D",
"r  c #D6A93D",
"s  c #FFE03D",
"t  c #D0973E",
"u  c #D79D3E",
"v  c #D19F3E",
"w  c #D5A83E",
"x  c #D6A83E",
"y  c #D7AB40",
"z  c #FFC940",
"{  c #FFD340",
"|  c #D9AD41",
"}  c #D9AE41",
"~  c #FFC841",
" ! c #FFD742",
"!! c #FFE043",
"#! c #D09944",
"$! c #FFCF45",
"%! c #DAA448",
"&! c #D9A649",
"'! c #DBAA4B",
"(! c #DDAA4C",
")! c #D29F4D",
"*! c #FFD74D",
"+! c #FFDF4F",
",! c #FFD750",
"-! c #DAAA52",
".! c #DEB452",
"/! c #DAA953",
"0! c #DBAB53",
"1! c #FFD854",
"2! c #DDB356",
"3! c #DAAC58",
"4! c #FFDF5A",
"5! c #FFE35C",
"6! c #DFB25D",
"7! c #D8AD61",
"8! c #D7AF62",
"9! c #FFE662",
":! c #6C5E65",
";! c #FFDC66",
"<! c #FFDD6B",
"=! c #FFE36E",
">! c #5D5F74",
"?! c #636477",
"@! c #FFE978",
"A! c #FFE17E",
"B! c #FFE880",
"C! c #666981",
"D! c #676C84",
"E! c #FFE986",
"F! c #7E7A8D",
"G! c #8C808E",
"H! c #7C7C92",
"I! c #808092",
"J! c #828294",
"K! c #FFEE96",
"L! c #8A8A98",
"M! c #9191A2",
"N! c #9595A3",
"O! c #FFF9A3",
"P! c #888BA5",
"Q! c #9292A5",
"R! c #8B8FA9",
"S! c #9E9AAD",
"T! c #9A99AE",
"U! c #A5A5B4",
"V! c #A4A4B6",
"W! c #A6A5B6",
"X! c #B0B0BB",
"Y! c #AFAFBF",
"Z! c #B4B4C0",
"[! c #AAADC3",
"\\! c #BABAC8",
"]! c #BDBCCD",
"^! c #FFFFFF",
"_! c None",
"_!_!_!_!_!_!L!X!Z!N!_!_!_!_!_!_!",
"_!_!_!_!_!U!V!I!J!Y!\\!_!_!_!_!_!",
"_!_!_!_!M!Q!_!_!_!_!W!Y!_!_!_!_!",
"_!_!_!?!T!>!_!_!_!_!C!]!H!_!_!_!",
"_!_!_!D!P!_!_!_!_!_!_![!R!_!_!_!",
"_!_!_!:!F!_!_!_!_!_!_!S!G!_!_!_!",
"_!` 6!7!8!-!0!2!.!'!%!3!/!(!d _!",
"_!)!O!K!B!@!9!h Y o $!~ z *!&!_!",
"_!#!E!A!<!5!J ' & 2 Q S O V u _!",
"_!t =!;!1!!!= # ! . 5 ; 3 ? g _!",
"_!k 4!,!{ c C ) ( + - , * 0 b _!",
"_!l +! !_ U 4 % $ / 7 6 1 < f _!",
"_!m s ^ R E : 8 9 > A B @ D j _!",
"_!i T F I H L N N M K K G P n _!",
"_!v e W [ [ \\ [ [ [ [ [ Z a y _!",
"_!X | r q q x w w w w w p } ] _!"
};

static int ImageList_Padlock = -1;
static int ImageList_PingGreen = -1;
static int ImageList_PingOrange = -1;
static int ImageList_PingRed = -1;
static int ImageList_PingGray = -1;

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
    ImageList_Padlock = AddImageSmall(padlock_xpm);
    ImageList_PingGreen = AddImageSmall(wxXmlResource::Get()->LoadBitmap(wxT("bullet_green")).ConvertToImage());
    ImageList_PingOrange = AddImageSmall(wxXmlResource::Get()->LoadBitmap(wxT("bullet_orange")).ConvertToImage());
    ImageList_PingRed = AddImageSmall(wxXmlResource::Get()->LoadBitmap(wxT("bullet_red")).ConvertToImage());
    ImageList_PingGray = AddImageSmall(wxXmlResource::Get()->LoadBitmap(wxT("bullet_gray")).ConvertToImage());
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
    
    // Colour the entire text column (wx/windows bug - exploited) if there are
    // players
    // TODO: Allow the user to select prefered colours
    if (s.Info.Players.size())
        li.SetTextColour(wxColor(0,192,0));
    else
        li.SetTextColour(GetTextColour());

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
