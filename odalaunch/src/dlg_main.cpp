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
//  User interface
//  AUTHOR: Russell Rice, John D Corrado
//
//-----------------------------------------------------------------------------
#include <algorithm>
#include <iostream>

#include "dlg_main.h"
#include "query_thread.h"
#include "plat_utils.h"
#include "str_utils.h"
#include "oda_defs.h"
#include "net_utils.h"

#include "md5.h"

#include <wx/settings.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/msgdlg.h>
#include <wx/richmsgdlg.h>
#include <wx/utils.h>
#include <wx/tipwin.h>
#include <wx/app.h>
#include <wx/imaglist.h>
#include <wx/artprov.h>
#include <wx/iconbndl.h>
#include <wx/regex.h>
#include <wx/process.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/cmdline.h>
#include <wx/sound.h>
#include <wx/msgout.h>

#include <wx/protocol/http.h>
#include <wx/stream.h>
#include <wx/sstream.h>

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

extern int NUM_THREADS;

// Control ID assignments for events
// application icon

static wxInt32 Id_MnuItmLaunch = XRCID("Id_MnuItmLaunch");
static wxInt32 Id_MnuItmGetList = XRCID("Id_MnuItmGetList");
static wxInt32 Id_MnuItmOpenChat = XRCID("Id_MnuItmOpenChat");
static wxInt32 Id_MnuItmCheckVersion = XRCID("Id_MnuItmCheckVersion");

// Timer id definitions
#define TIMER_ID_REFRESH 1
#define TIMER_ID_NEWLIST 2

// custom events
DEFINE_EVENT_TYPE(wxEVT_THREAD_MONITOR_SIGNAL)
DEFINE_EVENT_TYPE(wxEVT_THREAD_WORKER_SIGNAL)

// Event handlers
BEGIN_EVENT_TABLE(dlgMain, wxFrame)
	// main events
	EVT_MENU(wxID_EXIT, dlgMain::OnExit)

	EVT_SHOW(dlgMain::OnShow)
	EVT_CLOSE(dlgMain::OnClose)

	EVT_WINDOW_CREATE(dlgMain::OnWindowCreate)

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

	EVT_MENU(Id_MnuItmCheckVersion, dlgMain::OnCheckVersion)
	EVT_MENU(XRCID("Id_MnuItmVisitWebsite"), dlgMain::OnOpenWebsite)
	EVT_MENU(XRCID("Id_MnuItmVisitForum"), dlgMain::OnOpenForum)
	EVT_MENU(XRCID("Id_MnuItmVisitWiki"), dlgMain::OnOpenWiki)
	EVT_MENU(XRCID("Id_MnuItmViewChangelog"), dlgMain::OnOpenChangeLog)
	EVT_MENU(XRCID("Id_MnuItmSubmitBugReport"), dlgMain::OnOpenReportBug)
	EVT_MENU(wxID_ABOUT, dlgMain::OnAbout)
	EVT_MENU(Id_MnuItmOpenChat, dlgMain::OnConnectToIRC)

	EVT_MENU(XRCID("Id_MnuItmServerFilter"), dlgMain::OnShowServerFilter)
	EVT_TEXT(XRCID("Id_SrchCtrlGlobal"), dlgMain::OnTextSearch)

	// thread events
	EVT_COMMAND(-1, wxEVT_THREAD_MONITOR_SIGNAL, dlgMain::OnMonitorSignal)
	EVT_COMMAND(-1, wxEVT_THREAD_WORKER_SIGNAL, dlgMain::OnWorkerSignal)

	// misc events
	EVT_LIST_ITEM_SELECTED(XRCID("Id_LstCtrlServers"), dlgMain::OnServerListClick)
	EVT_LIST_ITEM_ACTIVATED(XRCID("Id_LstCtrlServers"), dlgMain::OnServerListDoubleClick)

	// Timers
	EVT_TIMER(TIMER_ID_REFRESH, dlgMain::OnTimer)
	EVT_TIMER(TIMER_ID_NEWLIST, dlgMain::OnTimer)

	// Process termination
	EVT_END_PROCESS(-1, dlgMain::OnProcessTerminate)
END_EVENT_TABLE()

// Main window creation
dlgMain::dlgMain(wxWindow* parent, wxWindowID id)
{
	wxString Version;
	wxIcon MainIcon;
	bool GetListOnStart, LoadChatOnLS, CheckForUpdates;

	// Allows us to auto-refresh the list due to the client not being run
	m_ClientIsRunning = false;

	// Loads the frame from the xml resource file
	wxXmlResource::Get()->LoadFrame(this, parent, "dlgMain");

	// Set window icon
	MainIcon = wxXmlResource::Get()->LoadIcon("mainicon");

	SetIcon(MainIcon);

	// wxMSW: Apply a hack to fix the titlebar icon on windows vista and 7
	OdaMswFixTitlebarIcon(GetHandle(), MainIcon);

	// Sets the title of the application with a version string to boot
	Version = wxString::Format(
	              "The Odamex Launcher v%d.%d.%d",
	              VERSIONMAJOR(VERSION), VERSIONMINOR(VERSION), VERSIONPATCH(VERSION));

	SetLabel(Version);

	// wxMAC: There is no file menu on OSX platforms
	OdaMacRemoveFileMenu(this);

	m_LstCtrlServers = XRCCTRL(*this, "Id_LstCtrlServers", LstOdaServerList);
	m_LstCtrlPlayers = XRCCTRL(*this, "Id_LstCtrlPlayers", LstOdaPlayerList);
	m_LstOdaSrvDetails = XRCCTRL(*this, "Id_LstCtrlServerDetails", LstOdaSrvDetails);
	m_PnlServerFilter = XRCCTRL(*this, "Id_PnlServerFilter", wxPanel);
	m_SrchCtrlGlobal = XRCCTRL(*this, "Id_SrchCtrlGlobal", wxSearchCtrl);
	m_StatusBar = GetStatusBar();

	/* Init sub dialogs and load settings */
	config_dlg = new dlgConfig(this);
	server_dlg = new dlgServers(&MServer, this);
	AboutDialog = new dlgAbout(this);

	// Init timers
	m_TimerRefresh = new wxTimer(this, TIMER_ID_REFRESH);
	m_TimerNewList = new wxTimer(this, TIMER_ID_NEWLIST);

	LoadMasterServers();

	/* Get the first directory for wad downloading */
	/*
	wxInt32 Pos = launchercfg_s.wad_paths.Find(PATH_DELIMITER), false);
	wxString FirstDirectory = launchercfg_s.wad_paths.Mid(0, Pos);

	OdaGet = new frmOdaGet(this, -1, FirstDirectory);*/

    InfoBar = new OdaInfoBar(this);

	QServer = NULL;

	NUM_THREADS = QueryThread::GetIdealThreadCount();

	for(size_t i = 0; i < NUM_THREADS; ++i)
	{
		threadVector.push_back(new QueryThread(this));
	}

	{
		wxFileConfig ConfigInfo;

		ConfigInfo.Read(GETLISTONSTART, &GetListOnStart,
		                ODA_UIGETLISTONSTART);

		ConfigInfo.Read(LOADCHATONLS, &LoadChatOnLS,
		                ODA_UILOADCHATCLIENTONLS);

		ConfigInfo.Read(CHECKFORUPDATES, &CheckForUpdates,
		                ODA_UIAUTOCHECKFORUPDATES);
		                
		ConfigInfo.Read(ARTENABLE, &m_UseRefreshTimer,
		                ODA_UIARTENABLE);

		ConfigInfo.Read(ARTREFINTERVAL, &m_RefreshInterval,
		                ODA_UIARTREFINTERVAL);

		ConfigInfo.Read(ARTNEWLISTINTERVAL, &m_NewListInterval,
		                ODA_UIARTLISTINTERVAL);


		// Calculate intervals from minutes to milliseconds
		m_RefreshInterval = m_RefreshInterval * 60 * 1000;
		m_NewListInterval = m_NewListInterval * 60 * 1000;

		// Prevent malicious under-ranged values from causing flooding of our
		// services
		m_RefreshInterval = clamp(m_RefreshInterval,
		                          ODA_UIARTREFINTERVAL,
		                          ODA_UIARTREFMAX);

		m_NewListInterval = clamp(m_NewListInterval,
		                          ODA_UIARTLISTINTERVAL,
		                          ODA_UIARTLISTMAX);

		// Make sure time intervals do not clash
		if((m_RefreshInterval % m_NewListInterval) == 0)
		{
			// If they do, reduce the master interval by 5 minutes
			m_NewListInterval -= ODA_UIARTLISTRED;
		}
	}

	// get master list on application start
	if(GetListOnStart)
	{
		wxCommandEvent event(wxEVT_COMMAND_TOOL_CLICKED, Id_MnuItmGetList);

		wxPostEvent(this, event);
	}

	// load chat client when launcher starts
	if(LoadChatOnLS)
	{
		wxCommandEvent event(wxEVT_COMMAND_TOOL_CLICKED, Id_MnuItmOpenChat);

		wxPostEvent(this, event);
	}

	// Check for a new version
    if(CheckForUpdates)
	{
        wxCommandEvent event(wxEVT_COMMAND_TOOL_CLICKED, Id_MnuItmCheckVersion);

        // Tell command handler that this is an automatic check
        event.SetClientData((void *)0x1);

        wxPostEvent(this, event);
	}

	// Enable the auto refresh timer
	if(m_UseRefreshTimer)
	{
		m_TimerNewList->Start(m_NewListInterval);
		m_TimerRefresh->Start(m_RefreshInterval);
	}
}

// Window Destructor
dlgMain::~dlgMain()
{
	delete[] QServer;

	QServer = NULL;
}

void dlgMain::OnWindowCreate(wxWindowCreateEvent& event)
{
	wxFileConfig ConfigInfo;
	wxInt32 WindowPosX, WindowPosY, WindowWidth, WindowHeight;
	bool WindowMaximized;

	// Sets the window size
	ConfigInfo.Read("MainWindowWidth",
	                &WindowWidth,
	                -1);

	ConfigInfo.Read("MainWindowHeight",
	                &WindowHeight,
	                -1);

	if(WindowWidth >= 0 && WindowHeight >= 0)
		SetSize(WindowWidth, WindowHeight);

	// Set Window position
	ConfigInfo.Read("MainWindowPosX",
	                &WindowPosX,
	                -1);

	ConfigInfo.Read("MainWindowPosY",
	                &WindowPosY,
	                -1);

	if(WindowPosX >= 0 && WindowPosY >= 0)
		Move(WindowPosX, WindowPosY);

	// Set whether this window is maximized or not
	ConfigInfo.Read("MainWindowMaximized", &WindowMaximized, false);

	Maximize(WindowMaximized);
}

// Called when the window X button or Close(); function is called
void dlgMain::OnClose(wxCloseEvent& event)
{
    /* Threading system shutdown */
    // Wait for the monitor thread to finish
	if(GetThread() && GetThread()->IsRunning())
		GetThread()->Wait();

	// Gracefully terminate any running worker threads
	for(size_t j = 0; j < threadVector.size(); ++j)
	{
		QueryThread* OdaQT = threadVector[j];

		if(OdaQT->IsRunning())
		{
			OdaQT->GracefulExit();
			delete OdaQT;
		}
	}

	// Save the UI layout and shut it all down
	wxFileConfig ConfigInfo;

	ConfigInfo.Write("MainWindowWidth", GetSize().GetWidth());
	ConfigInfo.Write("MainWindowHeight", GetSize().GetHeight());
	ConfigInfo.Write("MainWindowPosX", GetPosition().x);
	ConfigInfo.Write("MainWindowPosY", GetPosition().y);
	ConfigInfo.Write("MainWindowMaximized", IsMaximized());

	ConfigInfo.Flush();

	delete InfoBar;
    delete m_TimerRefresh;
    delete m_TimerNewList;

    if(config_dlg != NULL)
		config_dlg->Destroy();

	if(server_dlg != NULL)
		server_dlg->Destroy();

	Destroy();
}

// Called when the window is shown
void dlgMain::OnShow(wxShowEvent& event)
{

}

// Called when the menu exit item or exit button is clicked
void dlgMain::OnExit(wxCommandEvent& event)
{
	Close();
}

void dlgMain::OnCheckVersion(wxCommandEvent &event)
{
    wxString SiteSrc, VerMsg;

    GetWebsitePageSource(SiteSrc);
    //GetVersionInfoFromWesbite(SiteSrc, VerStr);

    if (SiteSrc.IsEmpty())
    {
        InfoBar->ShowMessage("Unable to check for updates");
        return;
    }

    VerMsg = wxString::Format("New! Odamex version %s is available", SiteSrc);

    // Remove version separators 
    SiteSrc.erase(std::remove(SiteSrc.begin(), SiteSrc.end(), '.'), SiteSrc.end());

    // Same or older version
    if (wxAtoi(SiteSrc) <= VERSION)
    {
        // Automatic check?
        if (event.GetClientData())
            return;

        // User generated event
        VerMsg = "No new version available.";

        InfoBar->ShowMessage(VerMsg);

        return;
    }

    InfoBar->ShowMessage(VerMsg, XRCID("Id_MnuItmVisitWebsite"),
        wxCommandEventHandler(dlgMain::OnOpenWebsite), "Visit Website");
}

// Master server setup
static const wxCmdLineEntryDesc cmdLineDesc[] =
{
	{
		wxCMD_LINE_OPTION,  wxTRANSLATE("m"), wxTRANSLATE("master"),
		wxTRANSLATE("Override all master servers with this one, example: /m 127.0.0.1:12345"),
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_NEEDS_SEPARATOR
	},

	{ wxCMD_LINE_NONE }
};

void dlgMain::LoadMasterServers()
{
	// set up the master server information
	wxCmdLineParser CmdLineParser(wxTheApp->argc, wxTheApp->argv);
	wxString MasterAddress;
	wxFileConfig ConfigInfo;
	int i = 0;
	wxString Key, Val;

	CmdLineParser.SetDesc(cmdLineDesc);

	CmdLineParser.Parse();

	if(CmdLineParser.Found(cmdLineDesc[0].shortName, &MasterAddress) ||
	        CmdLineParser.Found(cmdLineDesc[0].longName, &MasterAddress))
	{
		MServer.AddMaster(wxstr_tostdstr(MasterAddress));

		return;
	}

	// Add default master servers
	while(def_masterlist[i] != NULL)
	{
		MServer.AddMaster(def_masterlist[i]);
		++i;
	}

	// Add secondary master servers from the config file
	i = 0;

	Key = wxString::Format("%s%d", MASTERSERVER_ID, i);

	while(ConfigInfo.Read(Key, &Val, ""))
	{
		MServer.AddMaster(wxstr_tostdstr(Val));

		++i;

		Key = wxString::Format("%s%d", MASTERSERVER_ID, i);
	}
}

void dlgMain::OnShowServerFilter(wxCommandEvent& event)
{
	m_PnlServerFilter->Show(event.IsChecked());

	Layout();
}

// Gets the Odamex websites page source for version number extraction,
// This could have other uses, what those are? we do not know yet..
void dlgMain::GetWebsitePageSource(wxString &SiteSrc)
{
    wxURL Http("http://odamex.net/api/app-version");
    wxInputStream *inStream;

    // Get the websites source
    inStream = Http.GetInputStream();

    if (inStream)
    {
        wxStringOutputStream out_stream(&SiteSrc);
        inStream->Read(out_stream);
    }
}

// Parses the Odamex websites page source to find the version number, here is
// hoping that the sites layout doesn't change too much!
void dlgMain::GetVersionInfoFromWebsite(const wxString &SiteSrc, wxString &ver)
{  
    wxString VerStr = "Latest version: ";
    int Ch;
    
    // Extract version number from website source
    size_t Pos = SiteSrc.find(VerStr);
    
    if (Pos == wxNOT_FOUND)
        return;
    
    // Skip past the search string
    Pos += VerStr.Length();
    
    // Find the end of the data we need
    size_t EndPos = SiteSrc.find("<", Pos);
    
    if (EndPos == wxNOT_FOUND)
        return;
    
    // Copy only the version number back out
    ver = SiteSrc.Mid(Pos, EndPos - Pos);
}

// manually connect to a server
void dlgMain::OnManualConnect(wxCommandEvent& event)
{
	wxFileConfig ConfigInfo;
	wxInt32 ServerTimeout;
	Server tmp_server;
	odalpapi::BufferedSocket Socket;
	wxString server_hash;
	wxString ped_hash;
	wxString ped_result;
	wxString ted_result;
	std::string IPHost;
	uint16_t Port;

	const wxString HelpText = "Please enter a Hostname or an IP address. \n\nAn "
	                              "optional port number can exist for IPs or Hosts\n"
	                              "by putting a : after the address.";

	wxTextEntryDialog ted(this, HelpText, "Manual Connect",
	                      "0.0.0.0:0");

	wxPasswordEntryDialog ped(this, "Server is password-protected. \n\n"
	                                    "Please enter the password", "Manual Connect", "");

	ConfigInfo.Read(SERVERTIMEOUT, &ServerTimeout, ODA_QRYSERVERTIMEOUT);

	// Keep asking for a valid ip/port number
	while(1)
	{
		bool good = false;

		if(ted.ShowModal() == wxID_CANCEL)
			return;

		ted_result = ted.GetValue();

		// Remove any whitespace
		ted_result.Trim(false);
		ted_result.Trim(true);

		switch(odalpapi::OdaAddrToComponents(wxstr_tostdstr(ted_result), IPHost, Port))
		{
		// Correct address
		case 0:
		case 3:
		{
			good = true;
			
			// Use the servers default port number if none was specified
			if (!Port)
                Port = ODA_NETDEFSERVERPORT;
		}
		break;

		// Empty string
		case 1:
		{
			continue;
		}

		// Colon syntax bad
		case 2:
		{
			wxMessageBox("A number > 0 must exist after the :");
			continue;
		}
		}

		// Address is good to use
		if(good == true)
			break;
	}

	// Query the server and try to acquire its password hash
	tmp_server.SetSocket(&Socket);
	tmp_server.SetAddress(IPHost, Port);
	tmp_server.Query(ServerTimeout);

	if(tmp_server.GotResponse() == false)
	{
		// Server is unreachable
		wxMessageDialog Message(this, "No response from server",
		                        "Manual Connect", wxOK | wxICON_HAND);

		Message.ShowModal();

		return;
	}

	server_hash = stdstr_towxstr(tmp_server.Info.PasswordHash);

	// Uppercase both hashes for easier comparison
	server_hash.MakeUpper();

	// Show password entry dialog only if the server has a password
	if(!server_hash.IsEmpty())
	{
		while(1)
		{
			if(ped.ShowModal() == wxID_CANCEL)
				return;

			ped_result = ped.GetValue();

			ped_hash = MD5SUM(ped_result);

			ped_hash.MakeUpper();

			if(ped_hash != server_hash)
			{
				wxMessageDialog Message(this, "Incorrect password",
				                        "Manual Connect", wxOK | wxICON_HAND);

				Message.ShowModal();

				ped.SetValue("");

				continue;
			}
			else
				break;
		}
	}

	wxString OdamexDirectory, DelimWadPaths;

	{
		ConfigInfo.Read(ODAMEX_DIRECTORY, &OdamexDirectory,
		                OdaGetInstallDir());

		ConfigInfo.Read(DELIMWADPATHS, &DelimWadPaths, OdaGetDataDir());
	}

	LaunchGame(ted_result, OdamexDirectory, DelimWadPaths, ped_result);
}

// Various timers
void dlgMain::OnTimer(wxTimerEvent& event)
{
	// Don't wipe the server list if a refresh is already running
	if(GetThread() && GetThread()->IsRunning())
		return;

	// Don't update the list if the client is still running
	if(ClientIsRunning())
		return;

	// What timer generated this event and what actions to perform
	switch(event.GetId())
	{

	case TIMER_ID_NEWLIST:
	{
		DoGetList(true);
	}
	break;

	case TIMER_ID_REFRESH:
	{
		DoRefreshList(true);
	}
	break;

	}
}

// Called when the odamex client process terminates
void dlgMain::OnProcessTerminate(wxProcessEvent& event)
{
	m_ClientIsRunning = false;

	delete m_Process;
}

// Posts a message from the main thread to the monitor thread
bool dlgMain::MainThrPostEvent(mtcs_t CommandSignal, wxInt32 Index,
                               wxInt32 ListIndex)
{
	if(GetThread() && GetThread()->IsRunning())
		return false;

	// Create monitor thread
	if(this->wxThreadHelper::CreateThread() != wxTHREAD_NO_ERROR)
	{
		wxMessageBox("Could not create monitor thread!",
		             "Error",
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

	mtrs_struct_t* Result = new mtrs_struct_t;

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
	odalpapi::BufferedSocket Socket;

	// Get the masters timeout from the config file
	ConfigInfo.Read(MASTERTIMEOUT, &MasterTimeout, ODA_QRYMASTERTIMEOUT);
	ConfigInfo.Read(RETRYCOUNT, &RetryCount, ODA_QRYGSRETRYCOUNT);
	ConfigInfo.Read(USEBROADCAST, &UseBroadcast, ODA_QRYUSEBROADCAST);

	MServer.SetSocket(&Socket);

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

	if(ServerCount > 0)
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

	wxThread* OdaTH = GetThread();

	// [Russell] - This includes custom servers.
	if(!(ServerCount = MServer.GetServerCount()))
	{
		MonThrPostEvent(wxEVT_THREAD_MONITOR_SIGNAL, -1,
		                mtrs_server_noservers, -1, -1);

		return;
	}

	ConfigInfo.Read(SERVERTIMEOUT, &ServerTimeout, ODA_QRYSERVERTIMEOUT);
	ConfigInfo.Read(RETRYCOUNT, &RetryCount, ODA_QRYGSRETRYCOUNT);

	delete[] QServer;
	QServer = new Server [ServerCount];

	size_t thrvec_size = threadVector.size();

	while(count < ServerCount)
	{
		for(size_t i = 0; i < thrvec_size; ++i)
		{
			QueryThread* OdaQT = threadVector[i];
			QueryThread::Status Status = OdaQT->GetStatus();

			// Check if the user wants us to exit
			if(OdaTH->TestDestroy())
			{
				return;
			}

			if(Status == QueryThread::Running)
			{
				// Give up some timeslice for this thread so worker thread slots
				// become available
				OdaTH->Sleep(15);

				continue;
			}
			else
				++count;

			// If we got this far, it means a thread has finished and needs more
			// work, give it a job to do
			if(serverNum < ServerCount)
			{
				//OdaTH->Sleep(1);
				MServer.GetServerAddress(serverNum, Address, Port);

				OdaQT->Signal(&QServer[serverNum], Address, Port, serverNum,
				              ServerTimeout, RetryCount);

				++serverNum;
			}
		}
	}

	// Wait until all threads have finished before posting an event
	for(size_t i = 0; i < thrvec_size; ++i)
	{
		QueryThread* OdaQT = threadVector[i];

		while(OdaQT->GetStatus() == QueryThread::Running)
			OdaTH->Sleep(15);
	}

	MonThrPostEvent(wxEVT_THREAD_MONITOR_SIGNAL, -1,
	                mtrs_servers_querydone, -1, -1);
}

void dlgMain::MonThrGetSingleServer()
{
	wxFileConfig ConfigInfo;
	wxInt32 ServerTimeout;
	wxInt32 RetryCount;
    odalpapi::BufferedSocket Socket;
	Server &ThisServer = QServer[mtcs_Request.Index];

	if(!MServer.GetServerCount())
		return;

	ConfigInfo.Read(SERVERTIMEOUT, &ServerTimeout, ODA_QRYSERVERTIMEOUT);
	ConfigInfo.Read(RETRYCOUNT, &RetryCount, ODA_QRYGSRETRYCOUNT);

	ThisServer.SetSocket(&Socket);
	ThisServer.SetRetries(RetryCount);

	if(ThisServer.Query(ServerTimeout))
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
void* dlgMain::Entry()
{
	switch(mtcs_Request.Signal)
	{
	// Retrieve server data from all available master servers and then fall
	// through to querying those servers
	case mtcs_getmaster:
	{
		if(MonThrGetMasterList() == false)
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
	mtrs_struct_t* Result = (mtrs_struct_t*)event.GetClientData();
	wxInt32 i;

	switch(Result->Signal)
	{
	case mtrs_master_timeout:
	{
		// We use multiple masters you see, if one fails and the others are
		// working, atleast we can get some useful data
		if(!MServer.GetServerCount())
		{
			// Report ART failures to stderr instead
			if(m_WasARTRefresh)
			{
				wxMessageOutputStderr err;

				err.Printf("No master servers could be contacted\n");
			}
			else
				wxMessageBox("No master servers could be contacted",
				             "Error", wxOK | wxICON_ERROR);

			break;
		}
	}

	case mtrs_master_success:
		break;

	case mtrs_server_noservers:
	{
		// Report ART failures to stderr instead
		if(m_WasARTRefresh)
		{
			wxMessageOutputStderr err;

			err.Printf("There are no servers to query\n");
		}
		else
			wxMessageBox("There are no servers to query",
			             "Error", wxOK | wxICON_ERROR);

		m_SrchCtrlGlobal->Enable(true);
	}
	break;

	case mtrs_server_singletimeout:
	{
		bool ShowBlockedServers;
		Server &ThisServer = QServer[Result->Index];
        std::string Address = ThisServer.GetAddress();

		i = m_LstCtrlServers->FindServer(stdstr_towxstr(Address));

		m_LstOdaSrvDetails->LoadDetailsFromServer(NullServer);

		ThisServer.ResetData();

		{
			wxFileConfig ConfigInfo;

			ConfigInfo.Read(SHOWBLOCKEDSERVERS, &ShowBlockedServers,
			                ODA_UISHOWBLOCKEDSERVERS);
		}

		if(ShowBlockedServers == false)
			break;

        bool cs = MServer.IsCustomServer(Address);

		if(i == -1)
			m_LstCtrlServers->AddServerToList(ThisServer, Result->Index, true, cs);
		else
			m_LstCtrlServers->AddServerToList(ThisServer, i, false, cs);
	}
	break;

	case mtrs_server_singlesuccess:
	{
		Server &ThisServer = QServer[Result->Index];

		bool cs = MServer.IsCustomServer(ThisServer.GetAddress());

		m_LstCtrlServers->AddServerToList(ThisServer, Result->ServerListIndex, 
                                    false, cs);

		m_LstCtrlPlayers->AddPlayersToList(ThisServer);

		m_LstOdaSrvDetails->LoadDetailsFromServer(ThisServer);

		TotalPlayers += ThisServer.Info.Players.size();
	}
	break;

	case mtrs_servers_querydone:
	{
		bool FlashTaskbar;
		bool PlaySystemBell;
		wxString SoundFile;

		{
			wxFileConfig ConfigInfo;
			bool PS;

			ConfigInfo.Read(POLFLASHTBAR, &FlashTaskbar,
			                ODA_UIPOLFLASHTASKBAR);
			ConfigInfo.Read(POLPLAYSYSTEMBELL, &PlaySystemBell,
			                ODA_UIPOLPLAYSYSTEMBELL);
			ConfigInfo.Read(POLPLAYSOUND, &PS,
			                ODA_UIPOLPLAYSOUND);

			if(PS)
				ConfigInfo.Read(POLPSWAVFILE, &SoundFile, "");
		}

		// Sort server list after everything has been queried
		m_LstCtrlServers->Sort();

		// Allow items to be sorted by user
		m_LstCtrlServers->HeaderUsable(true);

		m_SrchCtrlGlobal->Enable(true);

		// User notification of players online (including spectators)
		if(TotalPlayers)
		{
			// Flashes the taskbar (if any)
			if(FlashTaskbar)
				RequestUserAttention();

			// Plays the system beep
			if(PlaySystemBell)
				wxBell();

			// Plays a sound through the sound card (if any)
			if(!SoundFile.empty())
				wxSound::Play(SoundFile, wxSOUND_ASYNC);
		}
		else
		{
			// Stop flashing the taskbar on windows
			if(FlashTaskbar)
				OdaMswStopFlashingWindow(GetHandle());
		}
	}
	break;

	default:
		break;
	}

	m_StatusBar->SetStatusText(wxString::Format("Master Ping: %d",
	                           (wxInt32)MServer.GetPing()), 1);
	m_StatusBar->SetStatusText(wxString::Format("Total Players: %d",
	                           (wxInt32)TotalPlayers), 3);

	delete Result;
}

// worker threads post to this callback
void dlgMain::OnWorkerSignal(wxCommandEvent& event)
{
	wxInt32 i;

	switch(event.GetId())
	{
	case 0: // server query timed out
	{
		bool ShowBlockedServers;
        int ServerIndex = event.GetInt();
		Server &ThisServer = QServer[ServerIndex];
        std::string Address = ThisServer.GetAddress();

		i = m_LstCtrlServers->FindServer(stdstr_towxstr(Address));

		m_LstCtrlPlayers->DeleteAllItems();

		ThisServer.ResetData();

		{
			wxFileConfig ConfigInfo;

			ConfigInfo.Read(SHOWBLOCKEDSERVERS, &ShowBlockedServers,
			                ODA_UISHOWBLOCKEDSERVERS);
		}

		if(ShowBlockedServers == false)
			break;

        bool cs = MServer.IsCustomServer(Address);

		if(i == -1)
			m_LstCtrlServers->AddServerToList(ThisServer, ServerIndex, true, cs);
		else
			m_LstCtrlServers->AddServerToList(ThisServer, i, false, cs);

		break;
	}

	case 1: // server queried successfully
	{
        int ServerIndex = event.GetInt();
		Server &ThisServer = QServer[ServerIndex];

		bool cs = MServer.IsCustomServer(ThisServer.GetAddress());

		m_LstCtrlServers->AddServerToList(ThisServer, ServerIndex, true, cs);

		TotalPlayers += ThisServer.Info.Players.size();

		break;
	}
	}

	++QueriedServers;

	m_StatusBar->SetStatusText(wxString::Format("Queried Server %d of %d",
	                           (wxInt32)QueriedServers, (wxInt32)MServer.GetServerCount()), 2);
	m_StatusBar->SetStatusText(wxString::Format("Total Players: %d",
	                           (wxInt32)TotalPlayers), 3);
}

// Custom Servers menu item
void dlgMain::OnMenuServers(wxCommandEvent& event)
{
	if(server_dlg)
		server_dlg->Show();
}


void dlgMain::OnOpenSettingsDialog(wxCommandEvent& event)
{
	if(config_dlg)
		config_dlg->Show();

	// Restart the ART
	{
		wxFileConfig ConfigInfo;

		ConfigInfo.Read(ARTENABLE, &m_UseRefreshTimer,
		                ODA_UIARTENABLE);

		ConfigInfo.Read(ARTREFINTERVAL, &m_RefreshInterval,
		                ODA_UIARTREFINTERVAL);

		ConfigInfo.Read(ARTNEWLISTINTERVAL, &m_NewListInterval,
		                ODA_UIARTLISTINTERVAL);


		// Calculate intervals from minutes to milliseconds
		m_RefreshInterval = m_RefreshInterval * 60 * 1000;
		m_NewListInterval = m_NewListInterval * 60 * 1000;

		// Prevent malicious under-ranged values from causing flooding of our
		// services
		m_RefreshInterval = clamp(m_RefreshInterval,
		                          ODA_UIARTREFINTERVAL,
		                          ODA_UIARTREFMAX);

		m_NewListInterval = clamp(m_NewListInterval,
		                          ODA_UIARTLISTINTERVAL,
		                          ODA_UIARTLISTMAX);

		// Make sure time intervals do not clash
		if((m_RefreshInterval % m_NewListInterval) == 0)
		{
			// If they do, reduce the master interval by 5 minutes
			m_NewListInterval -= ODA_UIARTLISTRED;
		}
	}

	if(!m_UseRefreshTimer)
	{
		m_TimerNewList->Stop();
		m_TimerRefresh->Stop();
	}
	else
	{
		m_TimerNewList->Start(m_NewListInterval);
		m_TimerRefresh->Start(m_RefreshInterval);
	}
}

void dlgMain::OnOpenOdaGet(wxCommandEvent& event)
{
	//    if (OdaGet)
	//      OdaGet->Show();
}

// Quick-Launch button click
void dlgMain::OnQuickLaunch(wxCommandEvent& event)
{
	wxString OdamexDirectory, DelimWadPaths;

	{
		wxFileConfig ConfigInfo;

		ConfigInfo.Read(ODAMEX_DIRECTORY, &OdamexDirectory,
		                OdaGetInstallDir());
		ConfigInfo.Read(DELIMWADPATHS, &DelimWadPaths, OdaGetDataDir());
	}

	LaunchGame("", OdamexDirectory, DelimWadPaths);

}

void dlgMain::OnTextSearch(wxCommandEvent& event)
{
	m_LstCtrlServers->ApplyFilter(event.GetString());
}

// Launch button click
void dlgMain::OnLaunch(wxCommandEvent& event)
{
	wxString Password;
	wxString UsrPwHash;
	wxString SrvPwHash;
	wxInt32 i;

	i = GetSelectedServerArrayIndex();

	if(i == -1)
		return;

	// If the server is passworded, pop up a password entry dialog for them to
	// specify one before going any further
	SrvPwHash = stdstr_towxstr(QServer[i].Info.PasswordHash);

	if(SrvPwHash.IsEmpty() == false)
	{
		wxPasswordEntryDialog ped(this, "Please enter a password",
		                          "This server is passworded", "");

		SrvPwHash.MakeUpper();

		while(1)
		{
			// Show the dialog box and get the resulting value
			ped.ShowModal();

			Password = ped.GetValue();

			// User possibly hit cancel or did not enter anything, just exit
			if(Password.IsEmpty())
				return;

			UsrPwHash = MD5SUM(Password);
			UsrPwHash.MakeUpper();

			// Do an MD5 comparison of the password with the servers one, if it
			// fails, keep asking the user to enter a valid password, otherwise
			// dive out and connect to the server
			if(SrvPwHash != UsrPwHash)
			{
				wxMessageDialog Message(this, "Incorrect password",
				                        "Incorrect password", wxOK | wxICON_HAND);

				Message.ShowModal();

				// Reset the text so weird things don't happen
				ped.SetValue("");
			}
			else
				break;
		}
	}

	wxString OdamexDirectory, DelimWadPaths;

	{
		wxFileConfig ConfigInfo;

		ConfigInfo.Read(ODAMEX_DIRECTORY, &OdamexDirectory,
		                OdaGetInstallDir());
		ConfigInfo.Read(DELIMWADPATHS, &DelimWadPaths, OdaGetDataDir());
	}

	LaunchGame(stdstr_towxstr(QServer[i].GetAddress()), OdamexDirectory,
	           DelimWadPaths, Password);
}

// Update program state and get a new list of servers
void dlgMain::DoGetList(bool IsARTRefresh)
{
	// Reset search results
	m_SrchCtrlGlobal->SetValue("");
	m_SrchCtrlGlobal->Enable(false);

	m_LstCtrlServers->DeleteAllItems();
	m_LstCtrlPlayers->DeleteAllItems();

	QueriedServers = 0;
	TotalPlayers = 0;

	// Disable sorting of items by user during a query
	m_LstCtrlServers->HeaderUsable(false);

	m_WasARTRefresh = IsARTRefresh;

	MainThrPostEvent(mtcs_getmaster);
}

// Update program state and refresh existing servers in the list
void dlgMain::DoRefreshList(bool IsARTRefresh)
{
	if(!MServer.GetServerCount())
		return;

	// Reset search results
	m_SrchCtrlGlobal->SetValue("");
	m_SrchCtrlGlobal->Enable(false);

	m_LstCtrlServers->DeleteAllItems();
	m_LstCtrlPlayers->DeleteAllItems();

	QueriedServers = 0;
	TotalPlayers = 0;

	// Disable sorting of items by user during a query
	m_LstCtrlServers->HeaderUsable(false);

	m_WasARTRefresh = IsARTRefresh;

	MainThrPostEvent(mtcs_getservers, -1, -1);
}

// Get Master List button click
void dlgMain::OnGetList(wxCommandEvent& event)
{
	// Restart all ARTs since the user clicked this button
	if(m_UseRefreshTimer)
	{
		m_TimerNewList->Start(m_NewListInterval);
		m_TimerRefresh->Start(m_RefreshInterval);
	}

	DoGetList(false);
}

// Refresh All/List button click
void dlgMain::OnRefreshAll(wxCommandEvent& event)
{
    // Restart all ARTs since the user clicked this button
	if(m_UseRefreshTimer)
	{
		m_TimerNewList->Start(m_NewListInterval);
		m_TimerRefresh->Start(m_RefreshInterval);
	}

	DoRefreshList(false);
}

void dlgMain::OnRefreshServer(wxCommandEvent& event)
{
	wxInt32 li, ai;

	// Reset search results
	//m_SrchCtrlGlobal = "");

	li = m_LstCtrlServers->GetSelectedServerIndex();
	ai = GetSelectedServerArrayIndex();

	if(li == -1 || ai == -1)
		return;

	m_LstCtrlPlayers->DeleteAllItems();

	TotalPlayers -= QServer[ai].Info.Players.size();

	MainThrPostEvent(mtcs_getsingleserver, ai, li);
}

// when the user clicks on the server list
void dlgMain::OnServerListClick(wxListEvent& event)
{
	wxInt32 i;

	i = GetSelectedServerArrayIndex();

	if(i == -1)
		return;

	m_LstCtrlPlayers->DeleteAllItems();

	m_LstCtrlPlayers->AddPlayersToList(QServer[i]);

	if(QServer[i].GotResponse() == false)
		m_LstOdaSrvDetails->LoadDetailsFromServer(NullServer);
	else
		m_LstOdaSrvDetails->LoadDetailsFromServer(QServer[i]);
}

void dlgMain::LaunchGame(const wxString& Address, const wxString& ODX_Path,
                         const wxString& waddirs, const wxString& Password)
{
	wxFileConfig ConfigInfo;

	// Supresses wx error popup under windows, regardless if wxExecute fails or
	// not
	wxLogNull NoLog;

	wxString BinName, CmdLine;
	wxString ExtraCmdLineArgs;
	wxString MsgStr = "Could not start %s\n\nPlease check that Settings->"
	                      "File Locations->Odamex Path points to your "
	                      "Odamex directory";

	if(ODX_Path.IsEmpty())
	{
		wxMessageBox("Your Odamex path is empty!");

		return;
	}

#ifdef __WXMSW__
	BinName = ODX_Path + '\\' + "odamex.exe";
#elif __WXMAC__
	BinName = ODX_Path + "/odamex.app/Contents/MacOS/odamex";
#else
	BinName = ODX_Path + "/odamex";
#endif

	CmdLine = BinName;

	if(!Address.IsEmpty())
	{
		CmdLine += " -connect ";
		CmdLine += Address;
	}

	if(!Password.IsEmpty())
	{
		CmdLine += " ";
		CmdLine += Password;
	}

	if(!waddirs.IsEmpty())
	{
		CmdLine += " -waddir \"";
		CmdLine += waddirs;
		CmdLine += "\"";
	}

	// Check for any user command line arguments
	ConfigInfo.Read(EXTRACMDLINEARGS, &ExtraCmdLineArgs, "");

	if(!ExtraCmdLineArgs.IsEmpty())
	{
		CmdLine += " ";
		CmdLine += ExtraCmdLineArgs;
	}

	// Redirect I/O of child process under non-windows platforms
	m_Process = new wxProcess(this, wxPROCESS_REDIRECT);

	m_ClientIsRunning = true;

	if(wxExecute(CmdLine, wxEXEC_ASYNC, m_Process) <= 0)
		wxMessageBox(wxString::Format(MsgStr, BinName.c_str()));
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
	for(size_t i = 0; i < MServer.GetServerCount(); i++)
		if(stdstr_towxstr(QServer[i].GetAddress()) == Address)
			return i;

	return -1;
}

// Retrieves the currently selected server in array index form
wxInt32 dlgMain::GetSelectedServerArrayIndex()
{
	wxListItem item;
	wxInt32 i;

	i = m_LstCtrlServers->GetSelectedServerIndex();

	if(i == -1)
		return -1;

	item.SetId(i);
	item.SetColumn(serverlist_field_address);
	item.SetMask(wxLIST_MASK_TEXT);

	m_LstCtrlServers->GetItem(item);

	i = FindServer(item.GetText());

	return i;
}

// About information
void dlgMain::OnAbout(wxCommandEvent& event)
{
	if(AboutDialog)
		AboutDialog->Show();
}

void dlgMain::OnOpenWebsite(wxCommandEvent& event)
{
	wxLaunchDefaultBrowser("http://odamex.net");
}

void dlgMain::OnOpenForum(wxCommandEvent& event)
{
	wxLaunchDefaultBrowser("http://odamex.net/boards");
}

void dlgMain::OnOpenWiki(wxCommandEvent& event)
{
	wxLaunchDefaultBrowser("http://odamex.net/wiki");
}

void dlgMain::OnOpenChangeLog(wxCommandEvent& event)
{
	wxLaunchDefaultBrowser("http://odamex.net/changelog");
}

void dlgMain::OnOpenReportBug(wxCommandEvent& event)
{
	wxLaunchDefaultBrowser("http://odamex.net/bugs/enter_bug.cgi");
}

void dlgMain::OnConnectToIRC(wxCommandEvent& event)
{
	bool ok;
	// Used to disable annoying message under MSW if url cannot be opened
	wxLogNull NoLog;

	ok = wxLaunchDefaultBrowser("irc://irc.quakenet.org/odaplayers");

	// Ask user if they would like to get an irc client
	if(!ok)
	{
		wxFileConfig ConfigInfo;
		int ret;
		wxRichMessageDialog Message(this, "", "IRC client not found",
		                            wxYES_NO | wxICON_INFORMATION | wxCANCEL);

		Message.ShowCheckBox("Load chat client when launcher starts", true);

		Message.SetMessage("No IRC client found! HexChat is recommend\n\n"
		                       "Would you like to visit the HexChat website?");

		ret = Message.ShowModal();

		if(ret == wxID_CANCEL)
			return;

		if(ret == wxID_YES)
			wxLaunchDefaultBrowser("http://hexchat.github.io/");

		// Write out selection if user wants to load client when the
		// launcher is run
		ConfigInfo.Write(LOADCHATONLS, Message.IsCheckBoxChecked());

		// Ensures setting is reflected in configuration dialog
		ConfigInfo.Flush();
	}
}
