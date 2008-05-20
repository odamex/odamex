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

#include <wx/settings.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/msgdlg.h>
#include <wx/colour.h>

#include "net_packet.h"
#include "misc.h"

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


typedef enum
{
    serverlist_field_name
    ,serverlist_field_ping
    ,serverlist_field_players
    ,serverlist_field_wads
    ,serverlist_field_map
    ,serverlist_field_type
    ,serverlist_field_iwad
    ,serverlist_field_address
    
    ,max_serverlist_fields
} serverlist_fields_t;

void SetupServerListColumns(wxAdvancedListCtrl *list)
{
	list->DeleteAllColumns();

	// set up the list columns
    list->InsertColumn(serverlist_field_name,_T("Server name"),wxLIST_FORMAT_LEFT,150);
	list->InsertColumn(serverlist_field_ping,_T("Ping"),wxLIST_FORMAT_LEFT,60);
	list->InsertColumn(serverlist_field_players,_T("Players"),wxLIST_FORMAT_LEFT,80);
	list->InsertColumn(serverlist_field_wads,_T("WADs"),wxLIST_FORMAT_LEFT,150);
	list->InsertColumn(serverlist_field_map,_T("Map"),wxLIST_FORMAT_LEFT,60);
	list->InsertColumn(serverlist_field_type,_T("Type"),wxLIST_FORMAT_LEFT,80);
	list->InsertColumn(serverlist_field_iwad,_T("Game IWAD"),wxLIST_FORMAT_LEFT,100);
	list->InsertColumn(serverlist_field_address,_T("Address : Port"),wxLIST_FORMAT_LEFT,130);
	
	// Passworded server icon
    list->AddImageSmall(padlock_xpm);
}

/*
    Takes a server structure and adds it to the list control
    if insert is 1, then add an item to the list, otherwise it will
    update the current item with new data
*/
void AddServerToList(wxAdvancedListCtrl *list, Server &s, wxInt32 index, wxInt8 insert)
{
    wxInt32 i = 0;
    
    if (s.GetAddress() == _T("0.0.0.0:0"))
        return;
    
    wxListItem li;
    
    li.SetMask(wxLIST_MASK_TEXT | wxLIST_MASK_IMAGE);
    
    // We don't want the sort arrow
    list->SetColumnImage(li, -1);
    
    // are we adding a new item?
    if (insert)    
    {
        li.SetColumn(serverlist_field_name);
        // TODO: Uncomment when the server has a full password implementation
        //list->SetColumnImage(li, (s.info.passworded ? 0 : -1));
        li.SetText(s.info.name);
        
        li.SetId(list->ALCInsertItem(li));
    }
    else
    {
        li.SetId(index);
        li.SetColumn(serverlist_field_name);
        // TODO: Uncomment when the server has a full password implementation
        //list->SetColumnImage(li, (s.info.passworded ? 0 : -1));
        li.SetText(s.info.name);
        
        list->SetItem(li);
    }
    
    li.SetMask(wxLIST_MASK_TEXT); 
    
    li.SetColumn(serverlist_field_address);
    li.SetText(s.GetAddress());
    
    list->SetItem(li);

    li.SetColumn(serverlist_field_ping);
    li.SetText(wxString::Format(_T("%u"),s.GetPing()));
    
    list->SetItem(li);

    li.SetColumn(serverlist_field_players);
    li.SetText(wxString::Format(_T("%d/%d"),s.info.numplayers,s.info.maxplayers));
    
    list->SetItem(li); 
    
    // build a list of pwads
    if (s.info.numpwads)
    {
        // pwad list
        wxString wadlist = _T("");
        wxString pwad = _T("");
            
        for (i = 0; i < s.info.numpwads; i++)
        {
            pwad = s.info.pwads[i].Mid(0, s.info.pwads[i].Find('.'));
            wadlist += wxString::Format(_T("%s "), pwad.c_str());
        }
            
        li.SetColumn(serverlist_field_wads);
        li.SetText(wadlist);
    
        list->SetItem(li);
    }

    li.SetColumn(serverlist_field_map);
    li.SetText(s.info.map.Upper());
    
    list->SetItem(li);
    
    // what game type do we like to play
    wxString gmode = _T("");

    if (s.info.gametype == 0)
        gmode = _T("COOP");
    else if (s.info.gametype == 1)
        gmode = _T("DM");
    if(s.info.gametype && s.info.teamplay)
        gmode = _T("TEAM DM");
    if(s.info.ctf)
        gmode = _T("CTF");

    li.SetColumn(serverlist_field_type);
    li.SetText(gmode);
    
    list->SetItem(li);

    // trim off the .wad
    wxString iwad = s.info.iwad.Mid(0, s.info.iwad.Find('.'));
        
    li.SetColumn(serverlist_field_iwad);
    li.SetText(iwad);
    
    list->SetItem(li);
    
    list->Sort();
}

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

void SetupPlayerListHeader(wxAdvancedListCtrl *list)
{ 
    list->DeleteAllItems();
	list->DeleteAllColumns();
	
	list->InsertColumn(playerlist_field_name,_T("Player name"),wxLIST_FORMAT_LEFT,150);
	list->InsertColumn(playerlist_field_ping,_T("Ping"),wxLIST_FORMAT_LEFT,60);
	list->InsertColumn(playerlist_field_frags,_T("Frags"),wxLIST_FORMAT_LEFT,70);
    list->InsertColumn(playerlist_field_killcount,_T("Kill count"),wxLIST_FORMAT_LEFT,85);
    list->InsertColumn(playerlist_field_deathcount,_T("Death count"),wxLIST_FORMAT_LEFT,100);
    list->InsertColumn(playerlist_field_timeingame,_T("Time"),wxLIST_FORMAT_LEFT,65);
}

void AddPlayersToList(wxAdvancedListCtrl *list, Server &s)
{   
    SetupPlayerListHeader(list);
    
    if (s.info.teamplay)
    {
        list->InsertColumn(playerlist_field_team,
                           _T("Team"),
                           wxLIST_FORMAT_LEFT,
                           50);
        
        list->InsertColumn(playerlist_field_teamscore,
                           _T("Team score"),
                           wxLIST_FORMAT_LEFT,
                           80);
    }
    
    if (!s.info.numplayers)
        return;
        
    for (wxInt32 i = 0; i < s.info.numplayers; i++)
    {
        wxListItem li;
        
        li.SetColumn(playerlist_field_name);
        
        li.SetMask(wxLIST_MASK_TEXT | wxLIST_MASK_IMAGE);

        // We don't want the sort arrow.
        list->SetColumnImage(li, -1);
        
        if (s.info.spectating)
        {
            li.SetMask(wxLIST_MASK_TEXT | wxLIST_MASK_IMAGE);
            
            li.SetText(s.info.playerinfo[i].name);
            list->SetColumnImage(li, s.info.playerinfo[i].spectator ? 0 : -1);
        }
        else
        {
            li.SetText(s.info.playerinfo[i].name);
        }
        
        li.SetId(list->ALCInsertItem(li));
        
        li.SetColumn(playerlist_field_ping);
        li.SetMask(wxLIST_MASK_TEXT);

        li.SetText(wxString::Format(_T("%d"),
                                    s.info.playerinfo[i].ping));
        
        list->SetItem(li);
        
        li.SetColumn(playerlist_field_frags);        
        li.SetText(wxString::Format(_T("%d"),
                                    s.info.playerinfo[i].frags));
        
        list->SetItem(li);

        li.SetColumn(playerlist_field_killcount);        
        li.SetText(wxString::Format(_T("%d"),
                                    s.info.playerinfo[i].killcount));
        
        list->SetItem(li);

        li.SetColumn(playerlist_field_deathcount);        
        li.SetText(wxString::Format(_T("%d"),
                                    s.info.playerinfo[i].deathcount));
        
        list->SetItem(li);

        li.SetColumn(playerlist_field_timeingame);        
        li.SetText(wxString::Format(_T("%d"),
                                    s.info.playerinfo[i].timeingame));
        
        list->SetItem(li);
        
        if (s.info.teamplay)
		{
            wxString teamstr = _T("UNKNOWN");
            wxInt32 teamscore = 0;
            
            li.SetColumn(playerlist_field_team); 
            
            switch(s.info.playerinfo[i].team)
			{
                case 0:
                    li.SetTextColour(*wxBLUE);      
                    teamstr = _T("BLUE");
                    teamscore = s.info.teamplayinfo.bluescore;
                    break;
				case 1:
                    li.SetTextColour(*wxRED);
                    teamstr = _T("RED");
					teamscore = s.info.teamplayinfo.redscore;
					break;
				case 2:
                    // no gold in 'dem mountains boy.
                    li.SetTextColour(wxColor(255,200,40));
                    teamscore = s.info.teamplayinfo.goldscore;
                    teamstr = _T("GOLD");
					break;
				default:
					break;
			}

            li.SetText(teamstr);

            list->SetItem(li);
            
            li.SetColumn(playerlist_field_teamscore);
            
            li.SetText(wxString::Format(_T("%d/%d"), 
                                        teamscore, 
                                        s.info.teamplayinfo.scorelimit));
            
            list->SetItem(li);
        }
        
        list->Sort();
    }
    
}

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
            wadfn = wxString::Format(_T("%s\%s"), dirs.GetNextToken().c_str(), wads.GetNextToken().c_str());
            #else
            wadfn = wxString::Format(_T("%s/%s"), dirs.GetNextToken().c_str(), wads.GetNextToken().c_str());
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

void LaunchGame(wxString Address, wxString ODX_Path, wxString waddirs)
{
    if (ODX_Path.IsEmpty())
    {
        wxMessageBox(_T("Your Odamex path is empty!"));
        
        return;
    }
    
    #ifdef __WXMSW__
    wxString binname = ODX_Path + _T('\\') + _T("odamex");
    #else
    wxString binname = ODX_Path + _T("/odamex");
    #endif

    wxString cmdline = _T("");

    wxString dirs = waddirs.Mid(0, waddirs.Length());
    
    cmdline += wxString::Format(_T("%s"), binname.c_str());
    
    if (!Address.IsEmpty())
		cmdline += wxString::Format(_T(" -connect %s"),
									Address.c_str());
	
	// this is so the client won't mess up parsing
	if (!dirs.IsEmpty())
        cmdline += wxString::Format(_T(" -waddir \"%s\""), 
                                    dirs.c_str());

	if (wxExecute(cmdline, wxEXEC_ASYNC, NULL) == -1)
        wxMessageBox(wxString::Format(_T("Could not start %s!"), 
                                        binname.c_str()));
	
}
