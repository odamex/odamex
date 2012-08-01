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
//	User interface
//	AUTHOR:	Russell Rice, John D Corrado
//
//-----------------------------------------------------------------------------
#include <iostream>

#include "dlg_main.h"
#include "query_thread.h"
#include "str_utils.h"

#include "md5.h"

#include <wx/settings.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/msgdlg.h>
#include <wx/utils.h>
#include <wx/tipwin.h>
#include <wx/app.h>
#include <wx/imaglist.h>
#include <wx/artprov.h>
#include <wx/iconbndl.h>
#include <wx/regex.h>
#include <wx/process.h>
#include <wx/toolbar.h>
#include <wx/xrc/xmlres.h>

#ifdef __WXMSW__
    #include <windows.h>
    #include <winsock.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <sys/wait.h>
    #include <netdb.h>
#endif

using namespace odalpapi;

// Control ID assignments for events
// application icon

static wxInt32 Id_MnuItmLaunch = XRCID("Id_MnuItmLaunch");
static wxInt32 Id_MnuItmGetList = XRCID("Id_MnuItmGetList");

// custom events
DEFINE_EVENT_TYPE(wxEVT_THREAD_MONITOR_SIGNAL)
DEFINE_EVENT_TYPE(wxEVT_THREAD_WORKER_SIGNAL)

// Event handlers
BEGIN_EVENT_TABLE(dlgMain, wxFrame)
	EVT_MENU(wxID_EXIT, dlgMain::OnExit)

	// menu item events
    EVT_MENU(XRCID("Id_MnuItmCustomServers"), dlgMain::OnMenuServers)
    EVT_MENU(XRCID("Id_MnuItmManualConnect"), dlgMain::OnManualConnect)

	EVT_MENU(Id_MnuItmLaunch, dlgMain::OnLaunch)
	EVT_MENU(XRCID("Id_MnuItmRunOffline"), dlgMain::OnQuickLaunch)

	EVT_MENU(Id_MnuItmGetList, dlgMain::OnGetList)
	EVT_MENU(XRCID("Id_MnuItmRefreshServer"), dlgMain::OnRefreshServer)
	EVT_MENU(XRCID("Id_MnuItmRefreshAll"), dlgMain::OnRefreshAll)

    EVT_MENU(XRCID("Id_MnuItmDownloadWad"), dlgMain::OnOpenOdaGet)

	EVT_MENU(wxID_PREFERENCES, dlgMain::OnOpenSettingsDialog)

	EVT_MENU(XRCID("Id_MnuItmVisitWebsite"), dlgMain::OnOpenWebsite)
	EVT_MENU(XRCID("Id_MnuItmVisitForum"), dlgMain::OnOpenForum)
	EVT_MENU(XRCID("Id_MnuItmVisitWiki"), dlgMain::OnOpenWiki)
    EVT_MENU(XRCID("Id_MnuItmViewChangelog"), dlgMain::OnOpenChangeLog)
    EVT_MENU(XRCID("Id_MnuItmSubmitBugReport"), dlgMain::OnOpenReportBug)
	EVT_MENU(wxID_ABOUT, dlgMain::OnAbout)

	EVT_SHOW(dlgMain::OnShow)
	EVT_CLOSE(dlgMain::OnClose)

    EVT_WINDOW_CREATE(dlgMain::OnWindowCreate)

    // thread events
    EVT_COMMAND(-1, wxEVT_THREAD_MONITOR_SIGNAL, dlgMain::OnMonitorSignal)
    EVT_COMMAND(-1, wxEVT_THREAD_WORKER_SIGNAL, dlgMain::OnWorkerSignal)

    // misc events
    EVT_LIST_ITEM_SELECTED(XRCID("Id_LstCtrlServers"), dlgMain::OnServerListClick)
    EVT_LIST_ITEM_ACTIVATED(XRCID("Id_LstCtrlServers"), dlgMain::OnServerListDoubleClick)
END_EVENT_TABLE()

void dlgMain::SetupToolbar()
{
    wxBitmap toolLaunch, toolRunOffline, toolRefresh, toolRefreshAll, 
        toolGetList, toolPreferences, toolAbout, toolExit;

    wxToolBar *ToolBar;

    ToolBar = GetToolBar();

    if (!ToolBar)
        return;

    toolLaunch = wxXmlResource::Get()->LoadBitmap(wxT("btnlaunch"));
    toolRunOffline = wxXmlResource::Get()->LoadBitmap(wxT("btnqlaunch"));
    toolRefresh = wxXmlResource::Get()->LoadBitmap(wxT("btnrefresh"));
    toolRefreshAll = wxXmlResource::Get()->LoadBitmap(wxT("btnrefresha"));
    toolGetList = wxXmlResource::Get()->LoadBitmap(wxT("btngetlist"));
    toolPreferences = wxXmlResource::Get()->LoadBitmap(wxT("btnprefs"));
    toolAbout = wxXmlResource::Get()->LoadBitmap(wxT("btnabout"));
    toolExit = wxXmlResource::Get()->LoadBitmap(wxT("btnexit"));

    ToolBar->SetToolNormalBitmap(XRCID("Id_MnuItmLaunch"), toolLaunch);
    ToolBar->SetToolNormalBitmap(XRCID("Id_MnuItmRunOffline"), toolRunOffline);
    ToolBar->SetToolNormalBitmap(XRCID("Id_MnuItmRefreshServer"), toolRefresh);
    ToolBar->SetToolNormalBitmap(XRCID("Id_MnuItmRefreshAll"), toolRefreshAll);
    ToolBar->SetToolNormalBitmap(XRCID("Id_MnuItmGetList"), toolGetList);
    ToolBar->SetToolNormalBitmap(wxID_PREFERENCES, toolPreferences);
    ToolBar->SetToolNormalBitmap(wxID_ABOUT, toolAbout);
    ToolBar->SetToolNormalBitmap(wxID_EXIT, toolExit);
}

// Main window creation
dlgMain::dlgMain(wxWindow* parent, wxWindowID id)
{
    wxString Version;
    wxIcon MainIcon;

    // Loads the frame from the xml resource file
	wxXmlResource::Get()->LoadFrame(this, parent, wxT("dlgMain"));

    // Not needed
    //SetupToolbar();

    // Set window icon
    MainIcon = wxXmlResource::Get()->LoadIcon(wxT("mainicon"));

    SetIcon(MainIcon);

    #ifdef _WIN32
    // Hack for windows vista/7 titlebar icon
    SendMessage((HWND)GetHandle(), WM_SETICON, ICON_SMALL, 
                (LPARAM)MainIcon.GetHICON());
    // Uncomment this if it doesn't work under xp            
    //SendMessage((HWND)GetHandle(), WM_SETICON, ICON_BIG, (LPARAM)MainIcon.GetHICON());
    #endif

    // Sets the title of the application with a version string to boot
    Version = wxString::Format(
        wxT("The Odamex Launcher v%d.%d.%d"),
        VERSIONMAJOR(VERSION), VERSIONMINOR(VERSION), VERSIONPATCH(VERSION));

    SetLabel(Version);

    #ifdef __WXMAC__
    {
        // Remove the file menu on Mac as it will be empty
        wxMenu* fileMenu = GetMenuBar()->Remove(GetMenuBar()->FindMenu(_("File")));
        if(fileMenu)
        {
            wxMenuItem* prefMenuItem = fileMenu->Remove(wxID_PREFERENCES);
            wxMenu* helpMenu = GetMenuBar()->GetMenu(GetMenuBar()->FindMenu(_("Help")));

            // Before deleting the file menu the preferences menu item must be moved or
            // it will not work after this even though it has been placed somewhere else.
            // Attaching it to the help menu is the only way to not duplicate it as Help is
            // a special menu just as Preferences is a special menu itme.
            if(helpMenu)
                helpMenu->Append(prefMenuItem);

            delete fileMenu;
        }
    }
    #endif

    launchercfg_s.get_list_on_start = 1;
    launchercfg_s.show_blocked_servers = 0;
    launchercfg_s.wad_paths = wxGetCwd();
    launchercfg_s.odamex_directory = wxGetCwd();

    m_LstCtrlServers = XRCCTRL(*this, "Id_LstCtrlServers", LstOdaServerList);
    m_LstCtrlPlayers = XRCCTRL(*this, "Id_LstCtrlPlayers", LstOdaPlayerList);
    m_LstOdaSrvDetails = XRCCTRL(*this, "Id_LstCtrlServerDetails", LstOdaSrvDetails);

	// set up the master server information
    MServer.AddMaster("master1.odamex.net", 15000);
    MServer.AddMaster("voxelsoft.com", 15000);

    /* Init sub dialogs and load settings */
    config_dlg = new dlgConfig(&launchercfg_s, this);
    server_dlg = new dlgServers(&MServer, this);
    AboutDialog = new dlgAbout(this);

    /* Get the first directory for wad downloading */
    wxInt32 Pos = launchercfg_s.wad_paths.Find(wxT(PATH_DELIMITER), false);
    wxString FirstDirectory = launchercfg_s.wad_paths.Mid(0, Pos);

    OdaGet = new frmOdaGet(this, -1, FirstDirectory);

    QServer = NULL;

    // get master list on application start
    if (launchercfg_s.get_list_on_start)
    {
        wxCommandEvent event(wxEVT_COMMAND_TOOL_CLICKED, Id_MnuItmGetList);

        wxPostEvent(this, event);
    }
}

// Window Destructor
dlgMain::~dlgMain()
{
    // Cleanup
    delete[] QServer;

    QServer = NULL;

    if (config_dlg != NULL)
        config_dlg->Destroy();

    if (server_dlg != NULL)
        server_dlg->Destroy();

    if (OdaGet != NULL)
        OdaGet->Destroy();
}

void dlgMain::OnWindowCreate(wxWindowCreateEvent &event)
{
    wxFileConfig ConfigInfo;
    wxInt32 WindowPosX, WindowPosY, WindowWidth, WindowHeight;
    bool WindowMaximized;

    // Sets the window size
    ConfigInfo.Read(wxT("MainWindowWidth"),
                    &WindowWidth,
                    -1);

    ConfigInfo.Read(wxT("MainWindowHeight"),
                    &WindowHeight,
                    -1);

    if (WindowWidth >= 0 && WindowHeight >= 0)
        SetClientSize(WindowWidth, WindowHeight);

    // Set Window position
    ConfigInfo.Read(wxT("MainWindowPosX"),
                    &WindowPosX,
                    -1);

    ConfigInfo.Read(wxT("MainWindowPosY"),
                    &WindowPosY,
                    -1);

    if (WindowPosX >= 0 && WindowPosY >= 0)
        Move(WindowPosX, WindowPosY);

    // Set whether this window is maximized or not
    ConfigInfo.Read(wxT("MainWindowMaximized"), &WindowMaximized, false);

    Maximize(WindowMaximized);
}

// Called when the menu exit item or exit button is clicked
void dlgMain::OnExit(wxCommandEvent& event)
{
    Close();
}

// Called when the window X button or Close(); function is called
void dlgMain::OnClose(wxCloseEvent &event)
{
    if (GetThread() && GetThread()->IsRunning())
        GetThread()->Delete();

    // Save GUI layout
    wxFileConfig ConfigInfo;

    ConfigInfo.Write(wxT("MainWindowWidth"), GetClientSize().GetWidth());
    ConfigInfo.Write(wxT("MainWindowHeight"), GetClientSize().GetHeight());
    ConfigInfo.Write(wxT("MainWindowPosX"), GetPosition().x);
    ConfigInfo.Write(wxT("MainWindowPosY"), GetPosition().y);
    ConfigInfo.Write(wxT("MainWindowMaximized"), IsMaximized());

    ConfigInfo.Flush();

    event.Skip();
}

// Called when the window is shown
void dlgMain::OnShow(wxShowEvent &event)
{

}

// manually connect to a server
void dlgMain::OnManualConnect(wxCommandEvent &event)
{
    wxFileConfig ConfigInfo;
    wxInt32 ServerTimeout;
    Server tmp_server;
    wxString server_hash;
    wxString ped_hash;
    wxString ped_result;
    wxString ted_result;
    wxString IPHost;
    long Port;

    const wxString HelpText = wxT("Please enter an IP Address or Hostname. \n\nAn "
                            "optional port number can exist for IPs or Hosts\n"
                            "by putting a : after the address.");

    wxTextEntryDialog ted(this, HelpText, wxT("Manual Connect"),
        wxT("0.0.0.0:0"));

    wxPasswordEntryDialog ped(this, wxT("Server is password-protected. \n\n"
        "Please enter the password"), wxT("Manual Connect"), wxT(""));

    ConfigInfo.Read(wxT(SERVERTIMEOUT), &ServerTimeout, 500);

    // Keep asking for a valid ip/port number
    while (1)
    {
        bool good = false;

        if (ted.ShowModal() == wxID_CANCEL)
            return;

        ted_result = ted.GetValue();

        switch (IsAddressValid(ted_result, IPHost, Port))
        {
            // Correct address
            case _oda_iav_SUCCESS:
            {
                good = true;
            }
            break;

            // Empty string
            case _oda_iav_emptystr:
            {
                continue;
            }

            // Colon syntax bad
            case _oda_iav_colerr:
            {
                wxMessageBox(wxT("A number > 0 must exist after the :"));
                continue;
            }

            // Internal error
            case _oda_iav_interr:
            {
                wxMessageBox(wxT("Regex compiler failure, please report this"));
                return;
            }

            // Unknown error (usually bad regex match)
            case _oda_iav_FAILURE:
            {
                wxMessageBox(wxT("Invalid IP address/hostname format"));
                continue;
            }
        }

        // Address is good to use
        if (good == true)
            break;
    }

    // Query the server and try to acquire its password hash
    tmp_server.SetAddress(wxstr_tostdstr(IPHost), Port);
    tmp_server.Query(ServerTimeout);

    if (tmp_server.GotResponse() == false)
    {
        // Server is unreachable
        wxMessageDialog Message(this, wxT("No response from server"),
            wxT("Manual Connect"), wxOK | wxICON_HAND);

        Message.ShowModal();

        return;
    }

    server_hash = stdstr_towxstr(tmp_server.Info.PasswordHash);

    // Uppercase both hashes for easier comparison
    server_hash.MakeUpper();

    // Show password entry dialog only if the server has a password
    if (!server_hash.IsEmpty())
    {
        while(1)
        {
            if (ped.ShowModal() == wxID_CANCEL)
                return;

            ped_result = ped.GetValue();

            ped_hash = MD5SUM(ped_result);

            ped_hash.MakeUpper();

            if (ped_hash != server_hash)
            {
                wxMessageDialog Message(this, wxT("Incorrect password"),
                    wxT("Manual Connect"), wxOK | wxICON_HAND);

                Message.ShowModal();

                ped.SetValue(wxT(""));

                continue;
            }
            else
                break;
        }
    }


    LaunchGame(ted_result, launchercfg_s.odamex_directory,
        launchercfg_s.wad_paths, ped_result);
}

// Posts a message from the main thread to the monitor thread
bool dlgMain::MainThrPostEvent(mtcs_t CommandSignal, wxInt32 Index,
    wxInt32 ListIndex)
{
    if (GetThread() && GetThread()->IsRunning())
        return false;

    // Create monitor thread
    if (this->wxThreadHelper::Create() != wxTHREAD_NO_ERROR)
    {
        wxMessageBox(_T("Could not create monitor thread!"),
                     _T("Error"),
                     wxOK | wxICON_ERROR);

        wxExit();
    }

	mtcs_Request.Signal = CommandSignal;
    mtcs_Request.Index = Index;
    mtcs_Request.ServerListIndex = ListIndex;

    GetThread()->Run();

    return true;
}

// Posts a thread message to the main thread
void dlgMain::MonThrPostEvent(wxEventType EventType, int win_id, mtrs_t Signal,
    wxInt32 Index, wxInt32 ListIndex)
{
    static wxCommandEvent event(EventType, win_id);

    mtrs_struct_t *Result = new mtrs_struct_t;

    Result->Signal = Signal;
    Result->Index = Index;
    Result->ServerListIndex = ListIndex;

    event.SetClientData(Result);

    wxPostEvent(this, event);
}

bool dlgMain::MonThrGetMasterList()
{
    wxFileConfig ConfigInfo;
    wxInt32 MasterTimeout;
    wxInt32 RetryCount;
    bool UseBroadcast;
    size_t ServerCount;
    mtrs_t Signal;

    // Get the masters timeout from the config file
    ConfigInfo.Read(wxT(MASTERTIMEOUT), &MasterTimeout, 500);
    ConfigInfo.Read(wxT(RETRYCOUNT), &RetryCount, 2);
    ConfigInfo.Read(wxT(USEBROADCAST), &UseBroadcast, false);

    // Query the masters with the timeout
    MServer.QueryMasters(MasterTimeout, UseBroadcast, RetryCount);

    // Get the amount of servers found
    ServerCount = MServer.GetServerCount();

    // Check if we timed out or we were successful
    Signal = (ServerCount > 0) ? mtrs_master_success : mtrs_master_timeout;

    // Free the server list array (if it exists) and reallocate a new sized
    // array of server objects
    delete[] QServer;
    QServer = NULL;

    if (ServerCount > 0)
        QServer = new Server [ServerCount];

    // Post the result to our main thread and exit
    MonThrPostEvent(wxEVT_THREAD_MONITOR_SIGNAL, -1, Signal, -1, -1);

    return (Signal == mtrs_master_success) ? true : false;
}

void dlgMain::MonThrGetServerList()
{
    wxFileConfig ConfigInfo;
    wxInt32 ServerTimeout;
    wxInt32 RetryCount;
    size_t ServerCount;

    size_t count = 0;
    size_t serverNum = 0;
    std::string Address;
    uint16_t Port = 0;

    // [Russell] - This includes custom servers.
    if (!(ServerCount = MServer.GetServerCount()))
    {
        MonThrPostEvent(wxEVT_THREAD_MONITOR_SIGNAL, -1,
            mtrs_server_noservers, -1, -1);

        return;
    }

    ConfigInfo.Read(wxT(SERVERTIMEOUT), &ServerTimeout, 500);
    ConfigInfo.Read(wxT(RETRYCOUNT), &RetryCount, 2);

    delete[] QServer;
    QServer = new Server [ServerCount];

    /*
        Thread pool manager:
        Executes a number of threads that contain the same amount of
        servers, when a thread finishes, it gets deleted and another
        gets executed with a different server, eventually all the way
        down to 0 servers.
    */
    while(count < ServerCount)
    {
        for(size_t i = 0; i < NUM_THREADS; i++)
        {
            if((!threadVector.empty()) && ((threadVector.size() - 1) >= i))
            {
                // monitor our thread vector, delete ONLY if the thread is
                // finished
                if(threadVector[i]->IsRunning())
                    continue;
                else
                {
                    threadVector[i]->Wait();
                    delete threadVector[i];
                    threadVector.erase(threadVector.begin() + i);
                    count++;
                }
            }
            if(serverNum < ServerCount)
            {
                MServer.GetServerAddress(serverNum, Address, Port);
                QServer[serverNum].SetAddress(Address, Port);

                // add the thread to the vector
                threadVector.push_back(new QueryThread(this,
                    &QServer[serverNum], serverNum, ServerTimeout, RetryCount));

                // create and run the thread
                if(threadVector.back()->Create() == wxTHREAD_NO_ERROR)
                    threadVector.back()->Run();

                // DUMB: our next server will be this incremented value
                serverNum++;
            }

            // Let other threads get some time
            GetThread()->Sleep(20);

            // We got told to exit, so we should wait for these worker threads
            // to gracefully exit
            if (GetThread()->TestDestroy())
            {
                for (size_t j = 0; j < threadVector.size(); ++j)
                {
                    if (threadVector[j]->IsRunning())
                    {
                        threadVector[j]->Wait();
                        delete threadVector[j];
                    }
                }

                return;
            }
        }
    }

    MonThrPostEvent(wxEVT_THREAD_MONITOR_SIGNAL, -1,
        mtrs_servers_querydone, -1, -1);
}

void dlgMain::MonThrGetSingleServer()
{
    wxFileConfig ConfigInfo;
    wxInt32 ServerTimeout;
    wxInt32 RetryCount;

    if (!MServer.GetServerCount())
        return;

    ConfigInfo.Read(wxT(SERVERTIMEOUT), &ServerTimeout, 500);
    ConfigInfo.Read(wxT(RETRYCOUNT), &RetryCount, 2);

    QServer[mtcs_Request.Index].SetRetries(RetryCount);

    if (QServer[mtcs_Request.Index].Query(ServerTimeout))
    {
        MonThrPostEvent(wxEVT_THREAD_MONITOR_SIGNAL, -1,
            mtrs_server_singlesuccess, mtcs_Request.Index,
            mtcs_Request.ServerListIndex);
    }
    else
    {
        MonThrPostEvent(wxEVT_THREAD_MONITOR_SIGNAL,
            mtrs_server_singletimeout, mtrs_server_singletimeout,
            mtcs_Request.Index, mtcs_Request.ServerListIndex);
    }
}

// [Russell] - Monitor thread entry point
void *dlgMain::Entry()
{
    switch (mtcs_Request.Signal)
    {
        // Retrieve server data from all available master servers and then fall
        // through to querying those servers
        case mtcs_getmaster:
        {
            if (MonThrGetMasterList() == false)
                break;
        }

        // Query the current list of servers that are available to us
        case mtcs_getservers:
        {
            MonThrGetServerList();
        }
        break;

        // Query a single server
        case mtcs_getsingleserver:
        {
            MonThrGetSingleServer();
        }
        break;

        default:
            break;
    }

    // Reset the signal and then exit out
    mtcs_Request.Signal = mtcs_none;

    return NULL;
}

void dlgMain::OnMonitorSignal(wxCommandEvent& event)
{
    mtrs_struct_t *Result = (mtrs_struct_t *)event.GetClientData();
    wxInt32 i;

    switch (Result->Signal)
    {
        case mtrs_master_timeout:
        {
            // We use multiple masters you see, if one fails and the others are
            // working, atleast we can get some useful data
            if (!MServer.GetServerCount())
            {
                wxMessageBox(wxT("No master servers could be contacted"),
                    wxT("Error"), wxOK | wxICON_ERROR);

                break;
            }
        }

        case mtrs_master_success:
            break;
        case mtrs_server_noservers:
        {
            wxMessageBox(wxT("There are no servers to query"),
                wxT("Error"), wxOK | wxICON_ERROR);
        }
        break;

        case mtrs_server_singletimeout:
        {
            i = FindServerInList(stdstr_towxstr(QServer[Result->Index].GetAddress()));

            m_LstOdaSrvDetails->LoadDetailsFromServer(NullServer);

            QServer[Result->Index].ResetData();

            if (launchercfg_s.show_blocked_servers == false)
                break;

            if (i == -1)
                m_LstCtrlServers->AddServerToList(QServer[Result->Index], Result->Index);
            else
                m_LstCtrlServers->AddServerToList(QServer[Result->Index], i, false);
        }
        break;

        case mtrs_server_singlesuccess:
        {
            m_LstCtrlServers->AddServerToList(QServer[Result->Index], Result->ServerListIndex, false);

            m_LstCtrlPlayers->AddPlayersToList(QServer[Result->Index]);

            m_LstOdaSrvDetails->LoadDetailsFromServer(QServer[Result->Index]);

            TotalPlayers += QServer[Result->Index].Info.Players.size();
        }
        break;

        case mtrs_servers_querydone:
        {
            // Sort server list after everything has been queried
            m_LstCtrlServers->Sort();
        }
        break;

        default:
            break;
    }

    GetStatusBar()->SetStatusText(wxString::Format(_T("Master Ping: %llu"), MServer.GetPing()), 1);
    GetStatusBar()->SetStatusText(wxString::Format(_T("Total Players: %d"), TotalPlayers), 3);

    delete Result;
}

// worker threads post to this callback
void dlgMain::OnWorkerSignal(wxCommandEvent& event)
{
    wxInt32 i;
    switch (event.GetId())
    {
        case 0: // server query timed out
        {
            i = FindServerInList(stdstr_towxstr(QServer[event.GetInt()].GetAddress()));

            m_LstCtrlPlayers->DeleteAllItems();

            QServer[event.GetInt()].ResetData();

            if (launchercfg_s.show_blocked_servers == false)
                break;

            if (i == -1)
                m_LstCtrlServers->AddServerToList(QServer[event.GetInt()], event.GetInt());
            else
                m_LstCtrlServers->AddServerToList(QServer[event.GetInt()], i, false);

            break;
        }
        case 1: // server queried successfully
        {
            m_LstCtrlServers->AddServerToList(QServer[event.GetInt()], event.GetInt());

            TotalPlayers += QServer[event.GetInt()].Info.Players.size();

            break;
        }
    }

    ++QueriedServers;

    GetStatusBar()->SetStatusText(wxString::Format(_T("Queried Server %d of %d"),
                                                   QueriedServers,
                                                   MServer.GetServerCount()),
                                                   2);

    GetStatusBar()->SetStatusText(wxString::Format(_T("Total Players: %d"),
                                                   TotalPlayers),
                                                   3);
}

// Custom Servers menu item
void dlgMain::OnMenuServers(wxCommandEvent &event)
{
    if (server_dlg)
        server_dlg->Show();
}


void dlgMain::OnOpenSettingsDialog(wxCommandEvent &event)
{
    if (config_dlg)
        config_dlg->Show();
}

void dlgMain::OnOpenOdaGet(wxCommandEvent &event)
{
    if (OdaGet)
        OdaGet->Show();
}

// Quick-Launch button click
void dlgMain::OnQuickLaunch(wxCommandEvent &event)
{
	LaunchGame(_T(""),
				launchercfg_s.odamex_directory,
				launchercfg_s.wad_paths);
}

// Launch button click
void dlgMain::OnLaunch(wxCommandEvent &event)
{
    wxString Password;
    wxString UsrPwHash;
    wxString SrvPwHash;
    wxInt32 i;

    i = GetSelectedServerArrayIndex();

    if (i == -1)
        return;

    // If the server is passworded, pop up a password entry dialog for them to
    // specify one before going any further
    SrvPwHash = stdstr_towxstr(QServer[i].Info.PasswordHash);

    if (SrvPwHash.IsEmpty() == false)
    {
        wxPasswordEntryDialog ped(this, wxT("Please enter a password"),
            wxT("This server is passworded"), wxT(""));

        SrvPwHash.MakeUpper();

        while (1)
        {
            // Show the dialog box and get the resulting value
            ped.ShowModal();

            Password = ped.GetValue();

            // User possibly hit cancel or did not enter anything, just exit
            if (Password.IsEmpty())
                return;

            UsrPwHash = MD5SUM(Password);
            UsrPwHash.MakeUpper();

            // Do an MD5 comparison of the password with the servers one, if it
            // fails, keep asking the user to enter a valid password, otherwise
            // dive out and connect to the server
            if (SrvPwHash != UsrPwHash)
            {
                wxMessageDialog Message(this, wxT("Incorrect password"),
                    wxT("Incorrect password"), wxOK | wxICON_HAND);

                Message.ShowModal();

                // Reset the text so weird things don't happen
                ped.SetValue(wxT(""));
            }
            else
                break;
        }
    }

    LaunchGame(stdstr_towxstr(QServer[i].GetAddress()), launchercfg_s.odamex_directory,
        launchercfg_s.wad_paths, Password);
}

// Get Master List button click
void dlgMain::OnGetList(wxCommandEvent &event)
{
    m_LstCtrlServers->DeleteAllItems();
    m_LstCtrlPlayers->DeleteAllItems();

    QueriedServers = 0;
    TotalPlayers = 0;

    MainThrPostEvent(mtcs_getmaster);
}

void dlgMain::OnRefreshServer(wxCommandEvent &event)
{
    wxInt32 li, ai;

    li = GetSelectedServerListIndex();
    ai = GetSelectedServerArrayIndex();

    if (li == -1 || ai == -1)
        return;

    m_LstCtrlPlayers->DeleteAllItems();

    TotalPlayers -= QServer[ai].Info.Players.size();

    MainThrPostEvent(mtcs_getsingleserver, ai, li);
}

void dlgMain::OnRefreshAll(wxCommandEvent &event)
{
    if (!MServer.GetServerCount())
        return;

    m_LstCtrlServers->DeleteAllItems();
    m_LstCtrlPlayers->DeleteAllItems();

    QueriedServers = 0;
    TotalPlayers = 0;

    MainThrPostEvent(mtcs_getservers, -1, -1);
}

// when the user clicks on the server list
void dlgMain::OnServerListClick(wxListEvent& event)
{
    wxInt32 i;

    i = GetSelectedServerArrayIndex();

    if (i == -1)
        return;

    m_LstCtrlPlayers->DeleteAllItems();

    m_LstCtrlPlayers->AddPlayersToList(QServer[i]);

    if (QServer[i].GotResponse() == false)
        m_LstOdaSrvDetails->LoadDetailsFromServer(NullServer);
    else
        m_LstOdaSrvDetails->LoadDetailsFromServer(QServer[i]);
}

void dlgMain::LaunchGame(const wxString &Address, const wxString &ODX_Path,
    const wxString &waddirs, const wxString &Password)
{
    wxFileConfig ConfigInfo;
    wxString ExtraCmdLineArgs;

    if (ODX_Path.IsEmpty())
    {
        wxMessageBox(wxT("Your Odamex path is empty!"));

        return;
    }

    #ifdef __WXMSW__
      wxString binname = ODX_Path + wxT('\\') + wxT("odamex");
    #elif __WXMAC__
      wxString binname = ODX_Path + wxT("/odamex.app/Contents/MacOS/odamex");
    #else
      wxString binname = ODX_Path + wxT("/odamex");
    #endif

    wxString cmdline = wxT("");

    wxString dirs = waddirs.Mid(0, waddirs.Length());

    cmdline += wxString::Format(wxT("%s"), binname.c_str());

    if (!Address.IsEmpty())
		cmdline += wxString::Format(wxT(" -connect %s"),
									Address.c_str());

	if (!Password.IsEmpty())
        cmdline += wxString::Format(wxT(" %s"),
									Password.c_str());

	// this is so the client won't mess up parsing
	if (!dirs.IsEmpty())
        cmdline += wxString::Format(wxT(" -waddir \"%s\""),
                                    dirs.c_str());

    // Check for any user command line arguments
    ConfigInfo.Read(wxT(EXTRACMDLINEARGS), &ExtraCmdLineArgs, wxT(""));

    if (!ExtraCmdLineArgs.IsEmpty())
        cmdline += wxString::Format(wxT(" %s"),
                                    ExtraCmdLineArgs.c_str());

    // wxWidgets likes to spit out its own message box on msw after our one
    #ifndef __WXMSW__
	wxProcess *process = new wxProcess(wxPROCESS_REDIRECT);

	if (wxExecute(cmdline, wxEXEC_ASYNC, process) <= 0)
        wxMessageBox(wxString::Format(wxT("Could not start %s!"),
                                        binname.c_str()));
    #else
    wxExecute(cmdline, wxEXEC_ASYNC, NULL);
    #endif
}


// when the user double clicks on the server list
void dlgMain::OnServerListDoubleClick(wxListEvent& event)
{
    wxCommandEvent LaunchEvent(wxEVT_COMMAND_TOOL_CLICKED, Id_MnuItmLaunch);

    wxPostEvent(this, LaunchEvent);
}

// returns a index of the server address as the internal array index
wxInt32 dlgMain::FindServer(wxString Address)
{
    for (size_t i = 0; i < MServer.GetServerCount(); i++)
        if (stdstr_towxstr(QServer[i].GetAddress()) == Address)
            return i;

    return -1;
}

// Finds an index in the server list, via Address
wxInt32 dlgMain::FindServerInList(wxString Address)
{
    if (!m_LstCtrlServers->GetItemCount())
        return -1;

    for (wxInt32 i = 0; i < m_LstCtrlServers->GetItemCount(); i++)
    {
        wxListItem item;
        item.SetId(i);
        item.SetColumn(7);
        item.SetMask(wxLIST_MASK_TEXT);

        m_LstCtrlServers->GetItem(item);

        if (item.GetText().IsSameAs(Address))
            return i;
    }

    return -1;
}

// Retrieves the currently selected server in list index form
wxInt32 dlgMain::GetSelectedServerListIndex()
{
    wxInt32 i;

    if (!m_LstCtrlServers->GetItemCount() ||
        !m_LstCtrlServers->GetSelectedItemCount())
    {
        return -1;
    }

    i = m_LstCtrlServers->GetFirstSelected();

    return i;
}

// Retrieves the currently selected server in array index form
wxInt32 dlgMain::GetSelectedServerArrayIndex()
{
    wxListItem item;
    wxInt32 i;

    i = GetSelectedServerListIndex();

    if (i == -1)
        return -1;

    item.SetId(i);
    item.SetColumn(serverlist_field_address);
    item.SetMask(wxLIST_MASK_TEXT);

    m_LstCtrlServers->GetItem(item);

    i = FindServer(item.GetText());

    return i;
}

// Checks whether an odamex-style address format is valid, also gives the
// separated ip/hostname and port number back to the caller
_oda_iav_err_t dlgMain::IsAddressValid(wxString Address, wxString &OutIPHost,
    long &OutPort)
{
    wxInt32 Colon;
    wxString RegEx;
    wxRegEx ReValIP;
    wxString IPHost;
    long Port = 10666;

    // Get rid of any whitespace on either side of the string
    Address.Trim(false);
    Address.Trim(true);

    // Don't accept nothing
    if (Address.IsEmpty() == true)
    {
        return _oda_iav_emptystr;
    }

    // Set the regular expression and load it in
    RegEx = wxT("^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4]"
                    "[0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9]"
                    "[0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");

    ReValIP.Compile(RegEx);

    if (ReValIP.IsValid() == false)
    {
        return _oda_iav_interr;
    }

    // Find the colon that separates the address and the port number
    Colon = Address.Find(wxT(':'), true);

    if (Colon != wxNOT_FOUND)
    {
        wxString PortStr;
        bool IsGood;

        // Try to convert the substring after the : to a port number
        PortStr = Address.Mid(Colon + 1);

        IsGood = PortStr.ToLong(&Port);

        // Check if there is something after the colon and if its actually a
        // numeric value
        if ((Colon + 1 >= Address.Len()) || (IsGood == false) || (Port <= 0))
        {
            return _oda_iav_colerr;
        }

    }

    // Finally get the address portion from the main string
    IPHost = Address.Mid(0, Colon);

    // Finally do the comparison
    if (ReValIP.Matches(IPHost) == true)
    {
        OutIPHost = IPHost;
        OutPort = Port;

        return _oda_iav_SUCCESS;
    }
    else
    {
        struct hostent *he;

        // Check to see if its a hostname rather than an IP address
        he = gethostbyname((const char *)IPHost.char_str());

        if (he != NULL)
        {
            OutIPHost = IPHost;
            OutPort = Port;

            return _oda_iav_SUCCESS;
        }
        else
            return _oda_iav_FAILURE;
    }
}

// About information
void dlgMain::OnAbout(wxCommandEvent& event)
{
    if (AboutDialog)
        AboutDialog->Show();
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
