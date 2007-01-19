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

#ifndef LAUNCHER_H
#define LAUNCHER_H

#include "lst_custom.h"

#include "dlg_config.h"

#include <wx/frame.h>
#include <wx/intl.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/xrc/xmlres.h>
#include <wx/splitter.h>
//#include <wx/listctrl.h>
#include <wx/aboutdlg.h>

#include "net_packet.h"

class dlgMain : public wxFrame
{
	public:

		dlgMain(wxWindow* parent,wxWindowID id = -1);
		virtual ~dlgMain();
		
        Server          *QServer;
        MasterServer    *MServer;
        
        launchercfg_t launchercfg_s;
	protected:

        void OnOpenSettingsDialog(wxCommandEvent& event);
        void OnOpenWebsite(wxCommandEvent &event);
        void OnOpenForum(wxCommandEvent &event);
        void OnOpenWiki(wxCommandEvent &event);
        void OnOpenChangeLog(wxCommandEvent& event);
        void OnOpenReportBug(wxCommandEvent &event);
		void OnAbout(wxCommandEvent& event);
        void OnQuit(wxCloseEvent& event);
		
		void OnLaunch(wxCommandEvent &event);
		void OnRefreshAll(wxCommandEvent &event);
		void OnGetList(wxCommandEvent &event);
		void OnExitClick(wxCommandEvent& event);
		void OnRefreshServer(wxCommandEvent& event);
		
		void OnServerListClick(wxListEvent& event);
		void OnServerListDoubleClick(wxListEvent& event);
		
		void OnComboSelectMaster(wxCommandEvent& event);
		
		wxSplitterWindow *SPLITTER_WINDOW;
		wxAdvancedListCtrl *SERVER_LIST;
		wxAdvancedListCtrl *PLAYER_LIST;
		wxStatusBar *status_bar;
        
        dlgConfig *config_dlg;
        
        wxAboutDialogInfo AboutDlg;
        
		wxInt32 totalPlayers;

	private:

		DECLARE_EVENT_TABLE()
};

#endif
