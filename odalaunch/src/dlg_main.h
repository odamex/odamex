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

#pragma once

#include "odalaunch.h"

#include "lst_players.h"
#include "lst_servers.h"
#include "lst_srvdetails.h"
#include "ctrl_popover.h"

#include "dlg_about.h"
#include "dlg_config.h"
#include "dlg_servers.h"
#include "ctrl_infobar.h"

#include <wx/frame.h>
#include <wx/intl.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/xrc/xmlres.h>
#include <wx/splitter.h>
#include <wx/dynarray.h>
#include <wx/timer.h>
#include <wx/process.h>
#include <wx/srchctrl.h>
#include <wx/url.h>

#include <vector>
#include <memory>
#include <unordered_map>

#include "query_thread.h"
#include "net_packet.h"

#if wxCHECK_VERSION(3, 1, 5)
	#include <wx/webrequest.h>
	#define ODALAUNCH_USE_WEB_REQUEST 1
#else
	#define ODALAUNCH_USE_WEB_REQUEST 0
#endif

// custom event declarations
wxDECLARE_EVENT(wxEVT_THREAD_MONITOR_SIGNAL, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_THREAD_WORKER_SIGNAL, wxCommandEvent);

class dlgServerDetails;

class dlgMain : public wxFrame, wxThreadHelper
{
public:

	dlgMain(wxWindow* parent,wxWindowID id = -1);
	virtual ~dlgMain();

	odalpapi::Server         NullServer;
	std::unique_ptr<odalpapi::Server[]> QServer;
	odalpapi::MasterServer   MServer;

	// Connects to a server (password prompt + launch). Public so the server
	// details dialog can trigger a join. DialogParent owns any dialogs shown.
	void ConnectToServer(const odalpapi::Server& s,
	                     wxWindow* DialogParent = NULL);

	void ApplyServerRefresh(const odalpapi::Server& Refreshed);

protected:
	void OnMenuServers(wxCommandEvent& event);
	void OnManualConnect(wxCommandEvent& event);

	void OnOpenSettingsDialog(wxCommandEvent& event);
	void OnOpenWebsite(wxCommandEvent& event);
	void OnOpenReleases(wxCommandEvent& event);
	void OnOpenForum(wxCommandEvent& event);
	void OnOpenWiki(wxCommandEvent& event);
	void OnOpenChangeLog(wxCommandEvent& event);
	void OnOpenReportBug(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OnOpenChat(wxCommandEvent& event);

	void OnTextSearch(wxCommandEvent& event);

	void OnQuickLaunch(wxCommandEvent& event);
	void OnLaunch(wxCommandEvent& event);
	void OnRefreshAll(wxCommandEvent& event);
	void OnGetList(wxCommandEvent& event);
	void OnRefreshServer(wxCommandEvent& event);
	void OnShowServerFilter(wxCommandEvent& event);

	void OnServerListClick(wxListEvent& event);
	void OnServerListDoubleClick(wxListEvent& event);

	void OnServerListMiddleDown(wxMouseEvent& event);
	void OnViewServerDetails(wxCommandEvent& event);

	// Hover popovers (server info / player list)
	void OnServerListMouseMove(wxMouseEvent& event);
	void OnServerListMouseLeave(wxMouseEvent& event);
	void HideHoverPopovers();

	void OnCheckVersion(wxCommandEvent &event);
	void SendCheckVersionRequest();
	#if ODALAUNCH_USE_WEB_REQUEST
	void OnCheckVersionResponse(wxWebRequestEvent& evt);
	#endif

	void OnShow(wxShowEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnWindowCreate(wxWindowCreateEvent& event);

	void OnTimer(wxTimerEvent& event);
	void OnProcessTerminate(wxProcessEvent& event);

	void OnExit(wxCommandEvent& event);

	void DoGetList(bool IsARTRefresh = false);
	void DoRefreshList(bool IsARTRefresh = false);

	void LoadMasterServers();

	wxInt32 FindServer(wxString);
	wxInt32 GetSelectedServerArrayIndex();

	bool ClientIsRunning() const
	{
		return !m_Processes.empty();
	};

	void LaunchGame(const wxString& Address, const wxString& ODX_Path,
	                const wxString& waddirs, const wxString& Password = "");

	LstOdaServerList* m_LstCtrlServers;

	// Permanent fallback panels, used when frameless popovers are unavailable
	// (wxUSE_POPUPWIN 0). When popovers are available these are collapsed
	// out of the layout and the same data is shown on hover instead.
	LstOdaPlayerList* m_LstCtrlPlayers;
	LstOdaSrvDetails* m_LstOdaSrvDetails;

	#if wxUSE_POPUPWIN
	ServerInfoPopover* m_ServerInfoPopover;
	PlayerListPopover* m_PlayerListPopover;

	// Currently hovered row/column that owns a visible popover (-1 = none)
	long m_HoverItem;
	int  m_HoverColumn;
	#endif

	dlgConfig* config_dlg;
	dlgServers* server_dlg;
	dlgAbout* AboutDialog;

	// Reused across opens so the heavy one-time build is paid only once.
	dlgServerDetails* m_ServerDetailsDlg = nullptr;

	wxPanel* m_PnlServerFilter;
	wxSearchCtrl* m_SrchCtrlGlobal;

	wxStatusBar* m_StatusBar;
	std::unordered_map<int, std::unique_ptr<wxProcess>> m_Processes;

	bool m_UpdateCheckWasAutomatic = false;

	OdaInfoBar *InfoBar;

	wxInt32 TotalPlayers;
	wxInt32 QueriedServers;

	/* ART - Automatic Refresh Timer */
	// Timers interval (in ms)
	int m_RefreshInterval;
	int m_NewListInterval;
	// State of timer usage
	bool m_UseRefreshTimer;
	// This particular refresh was by the timer
	bool m_WasARTRefresh;

	// Timers
	wxTimer* m_TimerRefresh;
	wxTimer* m_TimerNewList;

	/*
	    Our threading system

	    Monitor Thread:
	    ---------------
	    Stays in an infinite loop, continually monitoring the RequestSignal
	    variable, if it encounters a signal change it will run the command
	    and then call the OnMonitorSignal callback function with the
	    appropriate parameters or nothing if it timed out.
	    When I say "infinite loop", its more of a pseudo one obviously,
	    have to terminate it when the user exits ;)

	    Worker Threads:
	    ---------------
	    These get activated by the monitor thread and are controlled by it.
	    They are an array of threads which send signals to the monitor
	    thread to tell it what the status is.

	*/

	// Monitor Thread Command Signals
	// Sends a signal to the monitoring thread to instruct it to carry out
	// a command of some sort (get a list of servers for example)
	enum mtcs_t
	{
		mtcs_none
		,mtcs_getmaster
		,mtcs_getsingleserver
		,mtcs_getservers
		,mtcs_exit       // Shutdown now!

		,mtcs_max
	};

	struct mtcs_struct_t
	{
		mtcs_t Signal;
		wxInt32 Index;
		wxInt32 ServerListIndex;
	};

	// Only set these below if you got a response!
	// [Russell] - iirc, volatile on a struct doesn't work as well as it
	// should
	mtcs_struct_t mtcs_Request;

	// Monitor Thread Return Signals
	// The result of the signal sent above, sent to the callback function
	// below
	enum mtrs_t
	{
		mtrs_master_success
		,mtrs_master_timeout   // Dead

		,mtrs_server_singlesuccess // represents a single selected server
		,mtrs_server_singletimeout

		,mtrs_server_noservers // There are no servers to query!

		,mtrs_servers_querydone // Query of all servers complete

		,mtrs_max
	};

	struct mtrs_struct_t
	{
		mtrs_t Signal;
		wxInt32 Index;
		wxInt32 ServerListIndex;
	};

	mtrs_struct_t mtrs_Result;

	enum wtrs_t
	{
		wtrs_server_success
		,wtrs_server_timeout

		,wtrs_max
	};

	struct wtrs_struct_t
	{
		wtrs_t Signal;
		wxInt32 Index;
		wxInt32 ServerListIndex;
	};

	wtrs_struct_t wtrs_Result;

	// Posts a message from the main thread to the monitor thread
	bool MainThrPostEvent(mtcs_t CommandSignal, wxInt32 Index = -1,
	                      wxInt32 ListIndex = -1);

	// Posts a message from a secondary thread to the main thread
	void MonThrPostEvent(wxEventType EventType, int win_id, mtrs_t Signal,
	                     wxInt32 Index, wxInt32 ListIndex);

	// Various functions for communicating with masters and servers
	bool MonThrGetMasterList();
	void MonThrGetServerList();
	void MonThrGetSingleServer();

	void OnMonitorSignal(wxCommandEvent&);
	void OnWorkerSignal(wxCommandEvent&);
	// Our monitoring thread entry point, from wxThreadHelper
	void* Entry();

	std::vector<std::unique_ptr<QueryThread>> threadVector;

private:

	DECLARE_EVENT_TABLE()
};
