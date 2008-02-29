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

#ifndef DLG_MAIN_H
#define DLG_MAIN_H

#include "lst_custom.h"

#include "dlg_config.h"
#include "dlg_servers.h"

#include <wx/frame.h>
#include <wx/intl.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/xrc/xmlres.h>
#include <wx/splitter.h>
#include "wx/dynarray.h"

#include "net_packet.h"

#if ODL_ENABLE_THREADS
// custom event declarations
BEGIN_DECLARE_EVENT_TYPES()
DECLARE_EVENT_TYPE(wxEVT_THREAD_MONITOR_SIGNAL, -1)
END_DECLARE_EVENT_TYPES()

// Defines a new wxArray
WX_DEFINE_ARRAY_PTR(wxThread *, wxArrayThread);
#endif

class dlgMain : public wxFrame
#if ODL_ENABLE_THREADS
, wxThreadHelper
#endif
{
	public:

		dlgMain(wxWindow* parent,wxWindowID id = -1);
		virtual ~dlgMain();
		
        Server          *QServer;
        MasterServer    *MServer;
        
        launchercfg_t launchercfg_s;
	protected:
        void OnMenuServers(wxCommandEvent& event);
        void OnManualConnect(wxCommandEvent& event);
        
        void OnOpenSettingsDialog(wxCommandEvent& event);
        void OnOpenWebsite(wxCommandEvent &event);
        void OnOpenForum(wxCommandEvent &event);
        void OnOpenWiki(wxCommandEvent &event);
        void OnOpenChangeLog(wxCommandEvent& event);
        void OnOpenReportBug(wxCommandEvent &event);
		void OnAbout(wxCommandEvent& event);
        void OnQuit(wxCloseEvent& event);
		
		void OnQuickLaunch(wxCommandEvent &event);
		void OnLaunch(wxCommandEvent &event);
		void OnRefreshAll(wxCommandEvent &event);
		void OnGetList(wxCommandEvent &event);
		void OnExitClick(wxCommandEvent& event);
		void OnRefreshServer(wxCommandEvent& event);
		
		void OnServerListClick(wxListEvent& event);
		void OnServerListDoubleClick(wxListEvent& event);
		void OnServerListRightClick(wxListEvent& event);
		
		void OnComboSelectMaster(wxCommandEvent& event);
		
		wxInt32 FindServer(wxString Address);
		
		wxAdvancedListCtrl *SERVER_LIST;
		wxAdvancedListCtrl *PLAYER_LIST;
		wxStatusBar *status_bar;
        
        dlgConfig *config_dlg;
        dlgServers *server_dlg;
        
		wxInt32 totalPlayers;
#if ODL_ENABLE_THREADS
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
            ,mtcs_getserver 
            ,mtcs_getservers
                        
            ,mtcs_max
        } mtcs_t;

        typedef struct
        {
            mtcs_t Signal;
            
            wxString Address;
            wxInt16  Port;
        } mtcs_struct_t;

        // Only set these below if you got a response!
        // [Russell] - iirc, volatile on a struct doesn't work as well as it
        // should
        volatile mtcs_struct_t mtcs_Request;        

        // Monitor Thread Return Signals
        // The result of the signal sent above, sent to the callback function
        // below
        typedef enum
        {
             mtrs_master_success
            ,mtrs_server_success // (Server)
            
            ,mtrs_master_timeout   // Dead
            ,mtrs_server_timeout
                        
            ,mtrs_max
        } mtrs_t;
        
        void OnMonitorSignal(wxCommandEvent&);
        
        // Worker Thread Return Signals
        // Essentially the same as above.
        typedef enum
        {
            wtrs_gotserver // (Server)
            
            ,wtrs_timeout   // Dead
                        
            ,wtrs_max
        } wtrs_t;
                       
        // Our monitoring thread entry point, from wxThreadHelper
        void *Entry();

        // Our worker threads
        wxArrayThread m_workerthreads;
#endif
        // Misc thread-related crap
	private:

		DECLARE_EVENT_TABLE()
};

#define EVT_THREAD_MONITOR_SIGNAL(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( \
        wxEVT_THREAD_MONITOR_SIGNAL, id, wxID_ANY, \
        (wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent( wxCommandEventFunction, &fn ), \
        (wxObject *) NULL \
    ),

#endif
