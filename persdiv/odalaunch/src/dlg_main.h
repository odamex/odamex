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

#ifndef DLG_MAIN_H
#define DLG_MAIN_H

#include "lst_players.h"
#include "lst_servers.h"
#include "lst_srvdetails.h"

#include "dlg_about.h"
#include "dlg_config.h"
#include "dlg_servers.h"
#include "frm_odaget.h"

#include <wx/frame.h>
#include <wx/intl.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/xrc/xmlres.h>
#include <wx/splitter.h>
#include "wx/dynarray.h"

#include <vector>

#include "query_thread.h"
#include "net_packet.h"

// custom event declarations
BEGIN_DECLARE_EVENT_TYPES()
DECLARE_EVENT_TYPE(wxEVT_THREAD_MONITOR_SIGNAL, -1)
DECLARE_EVENT_TYPE(wxEVT_THREAD_WORKER_SIGNAL, -1)
END_DECLARE_EVENT_TYPES()

typedef enum
{
     _oda_iav_MIN = 0         // Minimum range
    
    ,_oda_iav_SUCCESS         // Address is valid
    ,_oda_iav_FAILURE         // Unknown error 
    
    ,_oda_iav_emptystr        // Empty address parameter
    ,_oda_iav_interr          // Internal error (regex comp error, bad regex)
    ,_oda_iav_colerr          // Bad or nonexistant substring after colon
    
    ,_oda_iav_NUMERR          // Number of errors (incl min range)
    
    ,_oda_iav_MAX             // Maximum range
} _oda_iav_err_t;


class dlgMain : public wxFrame, wxThreadHelper
{
	public:

		dlgMain(wxWindow* parent,wxWindowID id = -1);
		virtual ~dlgMain();
		
        odalpapi::Server         NullServer;
        odalpapi::Server        *QServer;
        odalpapi::MasterServer   MServer;
        
        launchercfg_t launchercfg_s;
	protected:
        void OnMenuServers(wxCommandEvent& event);
        void OnManualConnect(wxCommandEvent& event);
        
        void OnOpenSettingsDialog(wxCommandEvent& event);
        void OnOpenOdaGet(wxCommandEvent &event);
        void OnOpenWebsite(wxCommandEvent &event);
        void OnOpenForum(wxCommandEvent &event);
        void OnOpenWiki(wxCommandEvent &event);
        void OnOpenChangeLog(wxCommandEvent& event);
        void OnOpenReportBug(wxCommandEvent &event);
		void OnAbout(wxCommandEvent& event);
		
		void OnQuickLaunch(wxCommandEvent &event);
		void OnLaunch(wxCommandEvent &event);
		void OnRefreshAll(wxCommandEvent &event);
		void OnGetList(wxCommandEvent &event);
		void OnRefreshServer(wxCommandEvent& event);
		
		void OnServerListClick(wxListEvent& event);
		void OnServerListDoubleClick(wxListEvent& event);
		
		void OnShow(wxShowEvent &event);
		void OnClose(wxCloseEvent &event);
		void OnWindowCreate(wxWindowCreateEvent &event);
		
		void OnExit(wxCommandEvent& event);
		
		void SetupToolbar();

		wxInt32 FindServer(wxString);
		wxInt32 FindServerInList(wxString);
		wxInt32 GetSelectedServerListIndex();
		wxInt32 GetSelectedServerArrayIndex();

		_oda_iav_err_t IsAddressValid(wxString, wxString &, long &);
		
		void LaunchGame(const wxString &Address, const wxString &ODX_Path, 
            const wxString &waddirs, const wxString &Password = wxT(""));
		
		LstOdaServerList *m_LstCtrlServers;
		LstOdaPlayerList *m_LstCtrlPlayers;
        LstOdaSrvDetails *m_LstOdaSrvDetails;
        
        dlgConfig *config_dlg;
        dlgServers *server_dlg;
        dlgAbout *AboutDialog;
        frmOdaGet *OdaGet;
        
		wxInt32 TotalPlayers;
        wxInt32 QueriedServers;
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
        typedef enum
        {
             mtcs_none
            ,mtcs_getmaster
            ,mtcs_getsingleserver
            ,mtcs_getservers
            ,mtcs_exit       // Shutdown now!
                        
            ,mtcs_max
        } mtcs_t;

        typedef struct
        {
            mtcs_t Signal;
            wxInt32 Index;
            wxInt32 ServerListIndex;
        } mtcs_struct_t;

        // Only set these below if you got a response!
        // [Russell] - iirc, volatile on a struct doesn't work as well as it
        // should
        mtcs_struct_t mtcs_Request;        

        // Monitor Thread Return Signals
        // The result of the signal sent above, sent to the callback function
        // below
        typedef enum
        {
             mtrs_master_success          
            ,mtrs_master_timeout   // Dead
            
            ,mtrs_server_singlesuccess // represents a single selected server
            ,mtrs_server_singletimeout
            
            ,mtrs_server_noservers // There are no servers to query!

            ,mtrs_servers_querydone // Query of all servers complete
            
            ,mtrs_max
        } mtrs_t;

        typedef struct
        {
            mtrs_t Signal;
            wxInt32 Index;
            wxInt32 ServerListIndex;
        } mtrs_struct_t;
        
        mtrs_struct_t mtrs_Result;
        
        typedef enum
        {
             wtrs_server_success
            ,wtrs_server_timeout
            
            ,wtrs_max
        } wtrs_t;
        
        typedef struct
        {
            wtrs_t Signal;
            wxInt32 Index;
            wxInt32 ServerListIndex;
        } wtrs_struct_t;
        
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
        void *Entry();

        std::vector<QueryThread*> threadVector;

	private:

		DECLARE_EVENT_TABLE()
};

#endif
