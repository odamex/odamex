// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2026 by The Odamex Team.
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

#ifdef UNIX
#undef UNIX
#include "dlg_main.h"
#define UNIX
#else
#include "dlg_main.h"
#endif

#include <algorithm>
#include <iostream>

#ifdef __WXMSW__
#include <windows.h>
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#endif

#include <wx/settings.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/msgdlg.h>
#include <wx/richmsgdlg.h>
#include <wx/utils.h>
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
#include <wx/stdpaths.h>

#include <wx/protocol/http.h>
#include <wx/stream.h>
#include <wx/sstream.h>

#include <json/json.h>

#include "net_utils.h"
#include "oda_defs.h"
#include "plat_utils.h"
#include "query_thread.h"
#include "str_utils.h"
#include "dlg_serverdetails.h"

#include "md5.h"

using namespace odalpapi;

extern int NUM_THREADS;

// Control ID assignments for events
// application icon

static wxInt32 Id_MnuItmLaunch = XRCID("Id_MnuItmLaunch");
static wxInt32 Id_MnuItmGetList = XRCID("Id_MnuItmGetList");
static wxInt32 Id_MnuItmCheckVersion = XRCID("Id_MnuItmCheckVersion");

// Timer id definitions
#define TIMER_ID_REFRESH 1
#define TIMER_ID_NEWLIST 2

// custom events
wxDEFINE_EVENT(wxEVT_THREAD_MONITOR_SIGNAL, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_THREAD_WORKER_SIGNAL, wxCommandEvent);

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
	EVT_MENU(XRCID("Id_MnuItmViewServerDetails"), dlgMain::OnViewServerDetails)
	EVT_MENU(XRCID("Id_MnuItmRefreshAll"), dlgMain::OnRefreshAll)

	EVT_MENU(wxID_PREFERENCES, dlgMain::OnOpenSettingsDialog)

	#ifdef ODALAUNCH_USE_WEB_REQUEST
	EVT_MENU(XRCID("Id_MnuItmCheckVersion"), dlgMain::OnCheckVersion)
	#else
	EVT_MENU(XRCID("Id_MnuItmCheckVersion"), dlgMain::OnOpenWebsite)
	#endif
	EVT_MENU(XRCID("Id_MnuItmVisitWebsite"), dlgMain::OnOpenWebsite)
	EVT_MENU(XRCID("Id_MnuItmVisitForum"), dlgMain::OnOpenForum)
	EVT_MENU(XRCID("Id_MnuItmVisitWiki"), dlgMain::OnOpenWiki)
	EVT_MENU(XRCID("Id_MnuItmViewChangelog"), dlgMain::OnOpenChangeLog)
	EVT_MENU(XRCID("Id_MnuItmSubmitBugReport"), dlgMain::OnOpenReportBug)
	EVT_MENU(wxID_ABOUT, dlgMain::OnAbout)
	EVT_MENU(XRCID("Id_MnuItmOpenChat"), dlgMain::OnOpenChat)

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

	// Middle-clicking a server opens the detailed server view.
	m_LstCtrlServers->Bind(wxEVT_MIDDLE_DOWN,
	                       &dlgMain::OnServerListMiddleDown, this);

	#if wxUSE_POPUPWIN
	// Frameless popovers are available: show server info / players on hover and
	// collapse the permanent panels so the server list reclaims their space.
	m_HoverItem = -1;
	m_HoverColumn = -1;

	m_ServerInfoPopover = new ServerInfoPopover(this);
	m_PlayerListPopover = new PlayerListPopover(this);

	m_LstCtrlServers->Bind(wxEVT_MOTION, &dlgMain::OnServerListMouseMove, this);
	m_LstCtrlServers->Bind(wxEVT_LEAVE_WINDOW, &dlgMain::OnServerListMouseLeave,
	                       this);

	{
		wxSplitterWindow* SrvSplitter =
		    XRCCTRL(*this, "Id_SrvSplitter", wxSplitterWindow);
		wxWindow* SrvInfoPanel = XRCCTRL(*this, "Id_PnlSrvInfo", wxPanel);

		if(SrvSplitter && SrvInfoPanel)
			SrvSplitter->Unsplit(SrvInfoPanel);
	}
	#endif

	#if defined(__linux__) && wxCHECK_VERSION(3, 3, 0)
	const auto res = wxFileConfig::MigrateLocalFile("odalaunch", wxCONFIG_USE_XDG, wxCONFIG_USE_LOCAL_FILE);
	if(!res.oldPath.empty())
	{
		if(res.error.empty())
		{
			wxLogMessage("Config file was migrated from \"%s\" to \"%s\"",
			             res.oldPath, res.newPath);
		}
		else
		{
			wxLogWarning("Migrating old config failed: %s.", res.error);
		}
	}
	wxStandardPaths::Get().SetFileLayout(wxStandardPaths::FileLayout_XDG);
	#endif

	/* Init sub dialogs and load settings */
	config_dlg = new dlgConfig(this);
	server_dlg = new dlgServers(&MServer, this);
	AboutDialog = new dlgAbout(this);

	// Init timers
	m_TimerRefresh = new wxTimer(this, TIMER_ID_REFRESH);
	m_TimerNewList = new wxTimer(this, TIMER_ID_NEWLIST);

	LoadMasterServers();

    InfoBar = new OdaInfoBar(this);

	QServer.reset();

	NUM_THREADS = QueryThread::GetIdealThreadCount();

	for(size_t i = 0; i < NUM_THREADS; ++i)
	{
		threadVector.emplace_back(new QueryThread(this));
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
	// [ML] 1/21/2019: Disable this shenanigans...
	/*
	if(LoadChatOnLS)
	{
		wxCommandEvent event(wxEVT_COMMAND_TOOL_CLICKED, Id_MnuItmOpenChat);

		wxPostEvent(this, event);
	}
	*/

	#if ODALAUNCH_USE_WEB_REQUEST
	// Check for a new version
	// [ML] 1/21/2019: Disabled for now.  This doesn't work over https.
	// [EB] 4/27/2026: Re-enabled now, using github releases
	Bind(wxEVT_WEBREQUEST_STATE, &dlgMain::OnCheckVersionResponse, this);
	if(CheckForUpdates)
	{
		// Tell command handler that this is an automatic check
		m_UpdateCheckWasAutomatic = true;
		SendCheckVersionRequest();
	}
	#endif

	// Enable the auto refresh timer
	if(m_UseRefreshTimer)
	{
		m_TimerNewList->Start(m_NewListInterval);
		m_TimerRefresh->Start(m_RefreshInterval);
	}

	// Pre-build the server details dialog now (deferred until the frame is
	// fully constructed) so the heavy one-time build is paid at startup and
	// every open, including the first, is instant.
	CallAfter([this]()
	{
		if(!m_ServerDetailsDlg)
			m_ServerDetailsDlg = new dlgServerDetails(this);
	});
}

// Window Destructor
dlgMain::~dlgMain()
{
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
    // Stop any running timers and free their memory
    delete m_TimerNewList;
    m_TimerNewList = nullptr;
    delete m_TimerRefresh;
    m_TimerRefresh = nullptr;

    /* Threading system shutdown */
    // Wait for the monitor thread to finish
	if(GetThread() && GetThread()->IsRunning())
		GetThread()->Wait();

	// Gracefully terminate any running worker threads and then deallocate
	// their memory
	{
        for(auto it = threadVector.rbegin(); it != threadVector.rend(); it++)
        {
            if((*it)->IsRunning())
            {
                (*it)->GracefulExit();
            }
        }

        // Clear the vector at the end so we don't play fast and loose with
        // iterator invalidation.
        threadVector.clear();
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
	InfoBar = nullptr;

    if(config_dlg != nullptr)
		config_dlg->Destroy();

	if(server_dlg != nullptr)
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
	m_UpdateCheckWasAutomatic = false;
	SendCheckVersionRequest();
}

void dlgMain::SendCheckVersionRequest()
{
	#if ODALAUNCH_USE_WEB_REQUEST
	wxWebRequest request = wxWebSession::GetDefault().CreateRequest(
		this,
		"https://api.github.com/repos/odamex/odamex/releases/latest"
	);

	request.SetHeader("User-Agent", "Odamex-Update-Checker");
	request.Start();
	#endif
}

#if ODALAUNCH_USE_WEB_REQUEST
void dlgMain::OnCheckVersionResponse(wxWebRequestEvent& evt)
{
    if (evt.GetState() == wxWebRequest::State_Completed)
    {
        wxInputStream* stream = evt.GetResponse().GetStream();
        if (!stream)
            return;

        wxString json;
        wxStringOutputStream out(&json);
        stream->Read(out);

        // Pull the release tag out of the GitHub API response.
        const wxScopedCharBuffer utf8 = json.utf8_str();

        Json::CharReaderBuilder builder;
        const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

        Json::Value root;
        std::string errors;
        if (!reader->parse(utf8.data(), utf8.data() + utf8.length(), &root, &errors) ||
            !root.isObject())
        {
            InfoBar->ShowMessage("Unable to check for updates.");
            wxLogWarning(
                "Odalaunch tried to parse malformed JSON when checking for updates.");
            return;
        }

        const wxString tag = wxString::FromUTF8(
            root.get("tag_name", "").asString().c_str());

        if (tag.IsEmpty())
        {
            InfoBar->ShowMessage("Unable to check for updates.");
            wxLogWarning(
                "Odalaunch couldn't find a new version to compare against when checking for updates.");
            return;
        }

        const wxString VerMsg = wxString::Format("New! Odamex version %s is available", tag);

        wxArrayString v = wxSplit(tag, '.');
        if (MAKEVER(wxAtoi(v[0]), wxAtoi(v[1]), wxAtoi(v[2])) <= VERSION)
        {
            if (m_UpdateCheckWasAutomatic)
                return;

            InfoBar->ShowMessage("No new version available.");
            return;
        }

        InfoBar->ShowMessage(VerMsg, XRCID("Id_VisitReleases"),
            wxCommandEventHandler(dlgMain::OnOpenReleases), "Download Release");
    }
    else if (evt.GetState() == wxWebRequest::State_Failed)
    {
        InfoBar->ShowMessage("Unable to check for updates.");
        wxLogWarning(
            "Odalaunch could not connect to %s to check for updates.",
            evt.GetResponse().GetURL().c_str());
    }
}
#endif

// Master server setup
static const wxCmdLineEntryDesc cmdLineDesc[] =
{
	{
		wxCMD_LINE_OPTION,  wxTRANSLATE("m"), wxTRANSLATE("master"),
		wxTRANSLATE("Override all master servers with this one, example: /m 127.0.0.1:12345"),
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_NEEDS_SEPARATOR
	},

	{ wxCMD_LINE_NONE, nullptr, nullptr, nullptr, wxCMD_LINE_VAL_NONE, 0 }
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
	while(def_masterlist[i] != nullptr)
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
	const int pid = event.GetPid();

    auto it = m_Processes.find(pid);
    if (it != m_Processes.end())
	{
		m_Processes.erase(it);
	}
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
	QServer.reset();

	if(ServerCount > 0)
		QServer = std::make_unique<Server[]>(ServerCount);

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

	QServer = std::make_unique<Server[]>(ServerCount);

	size_t thrvec_size = threadVector.size();

	while(count < ServerCount)
	{
		for(size_t i = 0; i < thrvec_size; ++i)
		{
			QueryThread* OdaQT = threadVector[i].get();
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
		const QueryThread* OdaQT = threadVector[i].get();

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
		[[fallthrough]];
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

	return nullptr;
}

void dlgMain::OnMonitorSignal(wxCommandEvent& event)
{
	const mtrs_struct_t* Result = static_cast<mtrs_struct_t*>(event.GetClientData());

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
        const std::string Address = ThisServer.GetAddress();

		const wxInt32 i = m_LstCtrlServers->FindServer(stdstr_towxstr(Address));

		HideHoverPopovers();

		#if !wxUSE_POPUPWIN
		m_LstOdaSrvDetails->LoadDetailsFromServer(NullServer);
		#endif

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

		const bool cs = MServer.IsCustomServer(ThisServer.GetAddress());

		m_LstCtrlServers->AddServerToList(ThisServer, Result->ServerListIndex,
                                    false, cs);

		#if !wxUSE_POPUPWIN
		m_LstCtrlPlayers->AddPlayersToList(ThisServer);

		m_LstOdaSrvDetails->LoadDetailsFromServer(ThisServer);
		#endif

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
	switch(event.GetId())
	{
	case 0: // server query timed out
	{
		bool ShowBlockedServers;
        const int ServerIndex = event.GetInt();
		Server &ThisServer = QServer[ServerIndex];
        const std::string Address = ThisServer.GetAddress();

		const wxInt32 i = m_LstCtrlServers->FindServer(stdstr_towxstr(Address));

		HideHoverPopovers();

		#if !wxUSE_POPUPWIN
		m_LstCtrlPlayers->DeleteAllItems();
		#endif

		ThisServer.ResetData();

		{
			wxFileConfig ConfigInfo;

			ConfigInfo.Read(SHOWBLOCKEDSERVERS, &ShowBlockedServers,
			                ODA_UISHOWBLOCKEDSERVERS);
		}

		if(ShowBlockedServers == false)
			break;

        const bool cs = MServer.IsCustomServer(Address);

		if(i == -1)
			m_LstCtrlServers->AddServerToList(ThisServer, ServerIndex, true, cs);
		else
			m_LstCtrlServers->AddServerToList(ThisServer, i, false, cs);

		break;
	}

	case 1: // server queried successfully
	{
        const int ServerIndex = event.GetInt();
		Server &ThisServer = QServer[ServerIndex];

		const bool cs = MServer.IsCustomServer(ThisServer.GetAddress());

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

// Connects to (launches the game against) a server, prompting for a password
// first if it is passworded. DialogParent owns any dialogs shown (defaults to
// the main window); callers from a modal dialog should pass themselves.
void dlgMain::ConnectToServer(const odalpapi::Server& s, wxWindow* DialogParent)
{
	if(DialogParent == NULL)
		DialogParent = this;

	wxString Password;
	wxString SrvPwHash = stdstr_towxstr(s.Info.PasswordHash);

	// If the server is passworded, pop up a password entry dialog for them to
	// specify one before going any further
	if(SrvPwHash.IsEmpty() == false)
	{
		wxPasswordEntryDialog ped(DialogParent, "Please enter a password",
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

			wxString UsrPwHash = MD5SUM(Password);
			UsrPwHash.MakeUpper();

			// Do an MD5 comparison of the password with the servers one, if it
			// fails, keep asking the user to enter a valid password, otherwise
			// dive out and connect to the server
			if(SrvPwHash != UsrPwHash)
			{
				wxMessageDialog Message(DialogParent, "Incorrect password",
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

	LaunchGame(stdstr_towxstr(s.GetAddress()), OdamexDirectory, DelimWadPaths,
	           Password);
}

void dlgMain::ApplyServerRefresh(const odalpapi::Server& Refreshed)
{
	const wxString Address = stdstr_towxstr(Refreshed.GetAddress());

	const wxInt32 ai = FindServer(Address);

	if(ai == -1)
		return;

	// Keep the running player tally in sync with the new player count.
	TotalPlayers -= QServer[ai].Info.Players.size();

	QServer[ai].Info = Refreshed.Info;
	QServer[ai].SetPing(Refreshed.GetPing());
	QServer[ai].SetValidResponse(Refreshed.GotResponse());

	TotalPlayers += QServer[ai].Info.Players.size();

	// Repaint the status bar's player total so it reflects the new count.
	m_StatusBar->SetStatusText(
	    wxString::Format("Total Players: %d", (wxInt32)TotalPlayers), 3);

	// Update the visible list row in place (it may be filtered out of view).
	const wxInt32 li = m_LstCtrlServers->FindServer(Address);

	if(li != -1)
	{
		const bool cs = MServer.IsCustomServer(QServer[ai].GetAddress());
		m_LstCtrlServers->AddServerToList(QServer[ai], li, false, cs);
	}
}

// Launch button click
void dlgMain::OnLaunch(wxCommandEvent& event)
{
	wxInt32 i = GetSelectedServerArrayIndex();

	if(i == -1)
		return;

	ConnectToServer(QServer[i]);
}

// Middle-click on the server list: open the detailed server view.
void dlgMain::OnServerListMiddleDown(wxMouseEvent& event)
{
	int Flags = 0;
	long Item = m_LstCtrlServers->HitTest(event.GetPosition(), Flags);

	if(Item != wxNOT_FOUND)
	{
		m_LstCtrlServers->Select(Item);
		m_LstCtrlServers->Focus(Item);

		wxCommandEvent Dummy;
		OnViewServerDetails(Dummy);
	}

	event.Skip();
}

// Opens the modal server details dialog for the selected server, pausing the
// main window's refresh timers while it is up.
void dlgMain::OnViewServerDetails(wxCommandEvent& event)
{
	wxInt32 i = GetSelectedServerArrayIndex();

	if(i == -1)
		return;

	HideHoverPopovers();

	// Pause the main window's automatic refresh while the dialog owns the view.
	m_TimerNewList->Stop();
	m_TimerRefresh->Stop();

	// The dialog is built once and reused so repeat opens are fast; it just
	// re-seeds from the (already-queried) list entry and repaints. No blocking
	// network query happens until the user turns on its Refresh toggle.
	if(!m_ServerDetailsDlg)
		m_ServerDetailsDlg = new dlgServerDetails(this);

	m_ServerDetailsDlg->ShowForServer(QServer[i]);

	if(m_UseRefreshTimer)
	{
		m_TimerNewList->Start(m_NewListInterval);
		m_TimerRefresh->Start(m_RefreshInterval);
	}
}

// Update program state and get a new list of servers
void dlgMain::DoGetList(bool IsARTRefresh)
{
	// Reset search results
	m_SrchCtrlGlobal->SetValue("");
	m_SrchCtrlGlobal->Enable(false);

	HideHoverPopovers();

	m_LstCtrlServers->DeleteAllItems();

	#if !wxUSE_POPUPWIN
	m_LstCtrlPlayers->DeleteAllItems();
	#endif

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

	HideHoverPopovers();

	m_LstCtrlServers->DeleteAllItems();

	#if !wxUSE_POPUPWIN
	m_LstCtrlPlayers->DeleteAllItems();
	#endif

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

	HideHoverPopovers();

	#if !wxUSE_POPUPWIN
	m_LstCtrlPlayers->DeleteAllItems();
	#endif

	TotalPlayers -= QServer[ai].Info.Players.size();

	MainThrPostEvent(mtcs_getsingleserver, ai, li);
}

// when the user clicks on the server list
void dlgMain::OnServerListClick(wxListEvent& event)
{
	#if wxUSE_POPUPWIN
	// Selection is still tracked for Launch/Refresh actions, but server
	// details and the player list are surfaced via hover popovers
	// (see OnServerListMouseMove), so there's nothing to populate here.
	event.Skip();
	#else
	// No popovers available: populate the permanent fallback panels.
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
	#endif
}

// Hides any visible hover popover and clears the hover-tracking state
void dlgMain::HideHoverPopovers()
{
	#if wxUSE_POPUPWIN
	if(m_ServerInfoPopover && m_ServerInfoPopover->IsShown())
		m_ServerInfoPopover->Hide();

	if(m_PlayerListPopover && m_PlayerListPopover->IsShown())
		m_PlayerListPopover->Hide();

	m_HoverItem = -1;
	m_HoverColumn = -1;
	#endif
}

// Shows a frameless popover when the cursor is over a server's name or players
// cell, anchored next to the cursor so it never sits underneath it.
void dlgMain::OnServerListMouseMove(wxMouseEvent& event)
{
	event.Skip();

	#if wxUSE_POPUPWIN
	const wxPoint Pt = event.GetPosition();

	int Flags = 0;
	long Item = m_LstCtrlServers->HitTest(Pt, Flags, nullptr);

	// Work out which column the cursor is over by walking the column widths
	// relative to the row's on-screen rectangle (this also accounts for any
	// horizontal scrolling, and is portable since in old wxWidgets, HitTest
	// is wxMSW only).
	int Column = -1;
	wxRect RowRect;

	if(Item != wxNOT_FOUND && m_LstCtrlServers->GetItemRect(Item, RowRect))
	{
		int x = RowRect.x;
		const int ColumnCount = m_LstCtrlServers->GetColumnCount();

		for(int pos = 0; pos < ColumnCount; ++pos)
		{
			// Get the column index from the column order, if supported,
			// otherwise just use its initial position since it can't move.
			#ifdef wxHAS_LISTCTRL_COLUMN_ORDER
			const int c = m_LstCtrlServers->GetColumnIndexFromOrder(pos);
			#else
			const int c = pos;
			#endif

			const int w = m_LstCtrlServers->GetColumnWidth(c);

			if(Pt.x >= x && Pt.x < x + w)
			{
				Column = c;
				break;
			}

			x += w;
		}
	}

	// Only the name and players columns have popovers
	const bool IsName = (Column == serverlist_field_name);
	const bool IsPlayers = (Column == serverlist_field_players);

	if(Item == wxNOT_FOUND || (!IsName && !IsPlayers))
	{
		HideHoverPopovers();
		return;
	}

	// Already showing the popover for this exact cell? Leave it where it is.
	if(Item == m_HoverItem && Column == m_HoverColumn)
		return;

	HideHoverPopovers();

	// Resolve the server's array index from the row's address cell
	wxListItem Li;
	Li.SetId(Item);
	Li.SetColumn(serverlist_field_address);
	Li.SetMask(wxLIST_MASK_TEXT);
	m_LstCtrlServers->GetItem(Li);

	const wxInt32 ai = FindServer(Li.GetText());

	if(ai == -1 || QServer[ai].GotResponse() == false)
		return;

	wxPopupWindow* Popover = nullptr;

	if(IsName)
	{
		m_ServerInfoPopover->Populate(QServer[ai]);
		Popover = m_ServerInfoPopover;
	}
	else
	{
		m_PlayerListPopover->Populate(QServer[ai]);
		Popover = m_PlayerListPopover;
	}

	// Anchor next to the cursor, offset down-right so the cursor stays over the
	// cell (keeping the popover open) rather than over the popover itself.
	wxPoint Anchor = wxGetMousePosition() + wxPoint(16, 16);

	const wxSize PopSize = Popover->GetSize();
	const wxRect Display = wxGetClientDisplayRect();

	// Clip popover to the "client Display Rect" which on Windows
	// is available screen space minus the task bar.
	if(Anchor.x + PopSize.GetWidth() > Display.GetRight())
		Anchor.x = Display.GetRight() - PopSize.GetWidth();
	if(Anchor.y + PopSize.GetHeight() > Display.GetBottom())
		Anchor.y = Display.GetBottom() - PopSize.GetHeight();
	if(Anchor.x < Display.GetLeft())
		Anchor.x = Display.GetLeft();
	if(Anchor.y < Display.GetTop())
		Anchor.y = Display.GetTop();

	Popover->Move(Anchor);
	Popover->Show();

	m_HoverItem = Item;
	m_HoverColumn = Column;
	#endif
}

// Hides the popovers once the cursor leaves the server list entirely
void dlgMain::OnServerListMouseLeave(wxMouseEvent& event)
{
	event.Skip();

	HideHoverPopovers();
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
	auto proc = std::make_unique<wxProcess>(this, wxPROCESS_REDIRECT);
	const auto pid = wxExecute(CmdLine, wxEXEC_ASYNC, proc.get());
	if(pid <= 0)
	{
		wxMessageBox(wxString::Format(MsgStr, BinName));
	}
	else
	{
		// for some reason exExecute returns a long but wxProcessEvent::GetPid returns an int
		m_Processes.emplace(static_cast<int>(pid), std::move(proc));
	}
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
	wxLaunchDefaultBrowser("https://odamex.net");
}

void dlgMain::OnOpenReleases(wxCommandEvent& event)
{
	wxLaunchDefaultBrowser("https://github.com/odamex/odamex/releases/latest");
}

void dlgMain::OnOpenForum(wxCommandEvent& event)
{
	wxLaunchDefaultBrowser("https://odamex.net/boards");
}

void dlgMain::OnOpenWiki(wxCommandEvent& event)
{
	wxLaunchDefaultBrowser("https://github.com/odamex/odamex/wiki");
}

void dlgMain::OnOpenChangeLog(wxCommandEvent& event)
{
	wxLaunchDefaultBrowser("https://odamex.net/changelog");
}

void dlgMain::OnOpenReportBug(wxCommandEvent& event)
{
	wxLaunchDefaultBrowser("https://github.com/odamex/odamex/issues/new/choose");
}

void dlgMain::OnOpenChat(wxCommandEvent& event)
{
	wxLaunchDefaultBrowser("https://discord.gg/bvvMJMS");
}

