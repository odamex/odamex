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
//	User interface
//	AUTHOR:	Russell Rice, John D Corrado
//
//-----------------------------------------------------------------------------


#include "dlg_main.h"

#include <wx/settings.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/msgdlg.h>
#include <wx/utils.h>

#include "main.h"
#include "misc.h"

// Control ID assignments for events
// application icon

// lists
wxInt32 spWINMAIN = XRCID("spWINMAIN");
wxInt32 ID_LSTSERVERS = XRCID("ID_LSTSERVERS");
wxInt32 ID_LSTPLAYERS = XRCID("ID_LSTPLAYERS");

// menus
wxInt32 ID_MNULAUNCH = XRCID("ID_MNULAUNCH");
wxInt32 ID_MNUGETLIST = XRCID("ID_MNUGETLIST");
wxInt32 ID_MNUREFRESHSERVER = XRCID("ID_MNUREFRESHSERVER");
wxInt32 ID_MNUREFRESHALL = XRCID("ID_MNUREFRESHALL");
wxInt32 ID_MNUABOUT = XRCID("ID_MNUABOUT");
wxInt32 ID_MNUSETTINGS = XRCID("ID_MNUSETTINGS");
wxInt32 ID_MNUEXIT = XRCID("ID_MNUEXIT");

wxInt32 ID_MNUWEBSITE = XRCID("ID_MNUWEBSITE");
wxInt32 ID_MNUFORUM = XRCID("ID_MNUFORUM");
wxInt32 ID_MNUWIKI = XRCID("ID_MNUWIKI");
wxInt32 ID_MNUCHANGELOG = XRCID("ID_MNUCHANGELOG");
wxInt32 ID_MNUREPORTBUG = XRCID("ID_MNUREPORTBUG");

// Event handlers
BEGIN_EVENT_TABLE(dlgMain,wxFrame)
	// normal events
	EVT_CLOSE(dlgMain::OnQuit)

	// menu item events
	EVT_MENU(ID_MNULAUNCH, dlgMain::OnLaunch)

	EVT_MENU(ID_MNUGETLIST, dlgMain::OnGetList)
	EVT_MENU(ID_MNUREFRESHSERVER, dlgMain::OnRefreshServer)
	EVT_MENU(ID_MNUREFRESHALL, dlgMain::OnRefreshAll)

	EVT_MENU(ID_MNUABOUT, dlgMain::OnAbout)
	EVT_MENU(ID_MNUSETTINGS, dlgMain::OnOpenSettingsDialog)
	EVT_MENU(ID_MNUEXIT, dlgMain::OnExitClick)

	EVT_MENU(ID_MNUWEBSITE, dlgMain::OnOpenWebsite)
	EVT_MENU(ID_MNUFORUM, dlgMain::OnOpenForum)
	EVT_MENU(ID_MNUWIKI, dlgMain::OnOpenWiki)
    EVT_MENU(ID_MNUCHANGELOG, dlgMain::OnOpenChangeLog)
    EVT_MENU(ID_MNUREPORTBUG, dlgMain::OnOpenReportBug)
        
    // misc events
    EVT_LIST_ITEM_SELECTED(ID_LSTSERVERS, dlgMain::OnServerListClick)
    EVT_LIST_ITEM_ACTIVATED(ID_LSTSERVERS, dlgMain::OnServerListDoubleClick)
//    EVT_LIST_COL_CLICK(ID_LSTSERVERS, dlgMain::OnListColClick)
//    EVT_LIST_COL_CLICK(ID_LSTPLAYERS, dlgMain::OnListColClick)
END_EVENT_TABLE()

// Main window creation
dlgMain::dlgMain(wxWindow* parent, wxWindowID id)
{
    launchercfg_s.get_list_on_start = 1;
    launchercfg_s.show_blocked_servers = 1;
    launchercfg_s.wad_paths = _T("");

	wxXmlResource::Get()->LoadFrame(this, parent, _T("dlgMain")); 
  
    SERVER_LIST = wxStaticCast((*this).FindWindow(ID_LSTSERVERS), wxAdvancedListCtrl);
    PLAYER_LIST = wxStaticCast((*this).FindWindow(ID_LSTPLAYERS), wxAdvancedListCtrl);

    SPLITTER_WINDOW = wxStaticCast((*this).FindWindow(spWINMAIN),wxSplitterWindow);

    SPLITTER_WINDOW->SetSashGravity(1.0);

    /* Init config dialog and load settings */
    config_dlg = new dlgConfig(&launchercfg_s, NULL);

    // Set menu and status bar
    // Menu item id's are determined automatically when its loaded it seems
	SetMenuBar(wxXmlResource::Get()->LoadMenuBar(_T("ID_MENUBAR")));

	status_bar = new wxStatusBar(this, -1);
	SetStatusBar(status_bar);
	
	GetStatusBar()->SetFieldsCount(4);

    SetToolBar(wxXmlResource::Get()->LoadToolBar(this, _T("ID_TOOLBAR")));
	
	// set up the list controls
    SERVER_LIST->InsertColumn(0,_T("Server name"),wxLIST_FORMAT_LEFT,150);
	SERVER_LIST->InsertColumn(1,_T("Ping"),wxLIST_FORMAT_LEFT,50);
	SERVER_LIST->InsertColumn(2,_T("Players"),wxLIST_FORMAT_LEFT,50);
	SERVER_LIST->InsertColumn(3,_T("WADs"),wxLIST_FORMAT_LEFT,150);
	SERVER_LIST->InsertColumn(4,_T("Map"),wxLIST_FORMAT_LEFT,50);
	SERVER_LIST->InsertColumn(5,_T("Type"),wxLIST_FORMAT_LEFT,80);
	SERVER_LIST->InsertColumn(6,_T("Game IWAD"),wxLIST_FORMAT_LEFT,80);
	SERVER_LIST->InsertColumn(7,_T("Address : Port"),wxLIST_FORMAT_LEFT,130);
	
	PLAYER_LIST->InsertColumn(0,_T("Player name"),wxLIST_FORMAT_LEFT,150);
	PLAYER_LIST->InsertColumn(1,_T("Frags"),wxLIST_FORMAT_LEFT,70);
	PLAYER_LIST->InsertColumn(2,_T("Ping"),wxLIST_FORMAT_LEFT,50);
	PLAYER_LIST->InsertColumn(3,_T("Team"),wxLIST_FORMAT_LEFT,50);

	// set up the master server information
	MServer = new MasterServer;
    
    QServer = NULL;
    
    // get master list on application start
    if (launchercfg_s.get_list_on_start)
    {
        wxCommandEvent event(wxEVT_COMMAND_TOOL_CLICKED, ID_MNUGETLIST);
    
        wxPostEvent(this, event);
    }

    // set window size
    this->SetSize(wxSize(760, 500));
    // set minimum window size
    this->SetSizeHints(wxSize(760, 500));
    
    this->CentreOnScreen();
}

// Window Destructor
dlgMain::~dlgMain()
{
    if (MServer != NULL)
        delete MServer;
        
    if (QServer != NULL)
        delete[] QServer;
        
    if (config_dlg != NULL)
        delete config_dlg;
		
	if (status_bar != NULL)
		delete status_bar;
}

void dlgMain::OnOpenSettingsDialog(wxCommandEvent &event)
{
    if (config_dlg)
        config_dlg->Show();
}

// Exit button click
void dlgMain::OnExitClick(wxCommandEvent& event)
{ 
    // calls OnQuit
    Close();
}

// Our shutting down event
void dlgMain::OnQuit(wxCloseEvent& event)
{               
    wxExit();
}

// About information
void dlgMain::OnAbout(wxCommandEvent& event)
{
    wxString strAbout = _T("Odamex Launcher 1.0 - "
                            "Copyright 2007 The Odamex Team");
    
    wxMessageBox(strAbout, strAbout);
}

// Launch button click
void dlgMain::OnLaunch(wxCommandEvent &event)
{
    wxInt32 i = SERVER_LIST->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        
    wxListItem item;
    item.SetId(i);
    item.SetColumn(7);
    item.SetMask(wxLIST_MASK_TEXT);
        
    SERVER_LIST->GetItem(item);
        
    i = FindServer(item.GetText()); 
       
    if (i > -1)
    {
        LaunchGame(QServer[i], launchercfg_s.wad_paths);
    }
}

// Get Master List button click
void dlgMain::OnGetList(wxCommandEvent &event)
{
    static const wxString masters[2] = {
        _T("odamex.net"),
        _T("odamex.org")
    };
    static int index = 0;
    
    wxString Address = _T("");
    wxInt16 Port = 0;
    
	MServer->SetAddress(masters[index], 15000);

    if (!MServer->Query(500))
    {
        index = !index;
        
        MServer->SetAddress(masters[index], 15000);
            
        if (!MServer->Query(500))
        {
            wxMessageBox(_T("Could not query any of the master servers"), _T("Error"), wxOK | wxICON_ERROR);
            
            return;
        }
    }
    
    if (MServer->GetServerCount() > 0)
    {    
        if (QServer != NULL)
        {
            delete[] QServer;
            
            QServer = new Server [MServer->GetServerCount()];
        }
        else
            QServer = new Server [MServer->GetServerCount()];
        
        PLAYER_LIST->DeleteAllItems();
        
        SERVER_LIST->DeleteAllItems();
        
        totalPlayers = 0;
               
        for (wxInt32 i = 0; i < MServer->GetServerCount(); i++)
        {    
            MServer->GetServerAddress(i, Address, Port);

            QServer[i].SetAddress(Address, Port);
            
            if (QServer[i].Query(500))
            {
                AddServerToList(SERVER_LIST, QServer[i], i);
            
                totalPlayers += QServer[i].info.numplayers;
            }
            else
            {
                if (launchercfg_s.show_blocked_servers)
                    AddServerToList(SERVER_LIST, QServer[i], i);
            }

            SERVER_LIST->ColourListItem(i);
            
            wxSafeYield(this, true);
        }
        
        GetStatusBar()->SetStatusText(wxString::Format(_T("Master Ping: %d"), MServer->GetPing()), 1);
        GetStatusBar()->SetStatusText(wxString::Format(_T("Total Servers: %d"), MServer->GetServerCount()), 2);
        GetStatusBar()->SetStatusText(wxString::Format(_T("Total Players: %d"), totalPlayers), 3);
    }
}

void dlgMain::OnRefreshServer(wxCommandEvent &event)
{   
    if ((SERVER_LIST->GetItemCount() > 0) && (SERVER_LIST->GetSelectedItemCount() == 1))
    {
        PLAYER_LIST->DeleteAllItems();
        
        wxInt32 listindex = SERVER_LIST->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        
        wxListItem item;
        item.SetId(listindex);
        item.SetColumn(7);
        item.SetMask(wxLIST_MASK_TEXT);
        
        SERVER_LIST->GetItem(item);
        
        wxInt32 arrayindex = FindServer(item.GetText()); 
        
        if (arrayindex == -1)
            return;
                
        totalPlayers -= QServer[arrayindex].info.numplayers;
        
        QServer[arrayindex].Query(500);
        
        AddServerToList(SERVER_LIST, QServer[arrayindex], listindex, 0);
        
        AddPlayersToList(PLAYER_LIST, QServer[arrayindex]);
        
        totalPlayers += QServer[arrayindex].info.numplayers;
        
        GetStatusBar()->SetStatusText(wxString::Format(_T("Total Servers: %d"), MServer->GetServerCount()), 2);
        GetStatusBar()->SetStatusText(wxString::Format(_T("Total Players: %d"), totalPlayers), 3);
    }
}

void dlgMain::OnRefreshAll(wxCommandEvent &event)
{
    if (MServer->GetServerCount() > 0)
    {
        SERVER_LIST->DeleteAllItems();
        PLAYER_LIST->DeleteAllItems();
        
        totalPlayers = 0;
        
        for (wxInt32 i = 0; i < MServer->GetServerCount(); i++)
        {    
            if (QServer[i].Query(500))
            {
                AddServerToList(SERVER_LIST, QServer[i], i);
            
                totalPlayers += QServer[i].info.numplayers;
            }
            else
            {
                if (launchercfg_s.show_blocked_servers)
                    AddServerToList(SERVER_LIST, QServer[i], i);
            }
              
            SERVER_LIST->ColourListItem(i);
        
            wxSafeYield(this, true);
        }
        
        GetStatusBar()->SetStatusText(wxString::Format(_T("Master Ping: %d"), MServer->GetPing()), 1);
        GetStatusBar()->SetStatusText(wxString::Format(_T("Total Servers: %d"), MServer->GetServerCount()), 2);
        GetStatusBar()->SetStatusText(wxString::Format(_T("Total Players: %d"), totalPlayers), 3);
    }
    
}

// when the user clicks on the server list
void dlgMain::OnServerListClick(wxListEvent& event)
{
    if ((SERVER_LIST->GetItemCount() > 0) && (SERVER_LIST->GetSelectedItemCount() == 1))
    {
        PLAYER_LIST->DeleteAllItems();
        
        wxInt32 i = SERVER_LIST->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        
        wxListItem item;
        item.SetId(i);
        item.SetColumn(7);
        item.SetMask(wxLIST_MASK_TEXT);
        
        SERVER_LIST->GetItem(item);
        
        i = FindServer(item.GetText()); 
        
        if (i > -1)
            AddPlayersToList(PLAYER_LIST, QServer[i]);
    }
}

// when the user double clicks on the server list
void dlgMain::OnServerListDoubleClick(wxListEvent& event)
{
    if ((SERVER_LIST->GetItemCount() > 0) && (SERVER_LIST->GetSelectedItemCount() == 1))
    {
        wxCommandEvent event(wxEVT_COMMAND_TOOL_CLICKED, ID_MNULAUNCH);
    
        wxPostEvent(this, event);
    }
}

// returns a index of the server address as the internal array index
wxInt32 dlgMain::FindServer(wxString Address)
{
    for (wxInt32 i = 0; i < MServer->GetServerCount(); i++)
        if (QServer[i].GetAddress().IsSameAs(Address))
            return i;
    
    return -1;
}


void dlgMain::OnOpenWebsite(wxCommandEvent &event)
{
    wxLaunchDefaultBrowser(_T("http://odamex.net"));
}

void dlgMain::OnOpenForum(wxCommandEvent &event)
{
    wxLaunchDefaultBrowser(_T("http://odamex.net/boards"));
}

void dlgMain::OnOpenWiki(wxCommandEvent &event)
{
    wxLaunchDefaultBrowser(_T("http://odamex.net/wiki"));
}

void dlgMain::OnOpenChangeLog(wxCommandEvent &event)
{
    wxLaunchDefaultBrowser(_T("http://odamex.net/changelog"));
}

void dlgMain::OnOpenReportBug(wxCommandEvent &event)
{
    wxLaunchDefaultBrowser(_T("http://odamex.net/bugs/enter_bug.cgi"));
}
