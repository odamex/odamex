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
//	Config dialog
//  AUTHOR:	Russell Rice, John D Corrado
//
//-----------------------------------------------------------------------------


#ifndef DLG_CONFIG_H
#define DLG_CONFIG_H

#include <wx/dialog.h>
#include <wx/intl.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/xrc/xmlres.h>
#include <wx/listctrl.h>
#include <wx/fileconf.h>
#include <wx/checkbox.h>
#include <wx/listbox.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/filepicker.h>
#include <wx/spinctrl.h>
#include <wx/statbmp.h>

// config file value names
#define GETLISTONSTART      "GET_LIST_ON_START"
#define SHOWBLOCKEDSERVERS  "SHOW_BLOCKED_SERVERS"
#define DELIMWADPATHS       "DELIMITED_WAD_PATHS"
#define ODAMEX_DIRECTORY    "ODAMEX_DIRECTORY"
#define EXTRACMDLINEARGS    "ExtraCommandLineArguments"
#define MASTERTIMEOUT       "MasterTimeout"
#define SERVERTIMEOUT       "ServerTimeout"
#define USEBROADCAST        "UseBroadcast"
#define RETRYCOUNT          "RetryCount"

#ifdef __WXMSW__
#define PATH_DELIMITER ';'
#else
#define PATH_DELIMITER ':'
#endif

// configuration file structure
struct launchercfg_t
{
    bool     get_list_on_start;
    bool     show_blocked_servers;
    wxString    wad_paths;
    wxString    odamex_directory;
};

// a more dynamic way of adding environment variables, even if they are
// hardcoded.
#define NUM_ENVVARS 2
const wxString env_vars[NUM_ENVVARS] = { _T("DOOMWADDIR"), _T("DOOMWADPATH") };

class dlgConfig: public wxDialog
{
	public:

		dlgConfig(launchercfg_t *cfg, wxWindow* parent, wxWindowID id = -1);
		virtual ~dlgConfig();

        void LoadSettings();
        void SaveSettings();

        void Show();

	protected:
        void OnOK(wxCommandEvent &event);
        
        void OnChooseDir(wxFileDirPickerEvent &event);
        void OnAddDir(wxCommandEvent &event);
        void OnReplaceDir(wxCommandEvent &event);
        void OnDeleteDir(wxCommandEvent &event);
        
        void OnUpClick(wxCommandEvent &event);
        void OnDownClick(wxCommandEvent &event);
        
        void OnGetEnvClick(wxCommandEvent &event);
        
        void OnCheckedBox(wxCommandEvent &event);
        
        void OnChooseOdamexPath(wxFileDirPickerEvent &event);
        
        void OnTextChange(wxCommandEvent &event);

        void OnSpinValChange(wxSpinEvent &event);
        
        wxCheckBox *m_ChkCtrlGetListOnStart;
        wxCheckBox *m_ChkCtrlShowBlockedServers;
        wxCheckBox *m_ChkCtrlEnableBroadcasts;

        wxListBox *m_LstCtrlWadDirectories;

        wxDirPickerCtrl *m_DirCtrlChooseOdamexPath;

        wxSpinCtrl *m_SpnCtrlMasterTimeout;
        wxSpinCtrl *m_SpnCtrlServerTimeout;
        wxSpinCtrl *m_SpnCtrlRetry;
        wxTextCtrl *m_TxtCtrlExtraCmdLineArgs;

        wxSpinCtrl *m_SpnCtrlPQGood;
        wxSpinCtrl *m_SpnCtrlPQPlayable;
        wxSpinCtrl *m_SpnCtrlPQLaggy;

        wxStaticBitmap *m_StcBmpPQGood;
        wxStaticBitmap *m_StcBmpPQPlayable;
        wxStaticBitmap *m_StcBmpPQLaggy;
        wxStaticBitmap *m_StcBmpPQBad;

        wxFileConfig ConfigInfo;

        launchercfg_t *cfg_file;
        
        bool UserChangedSetting;

	private:

		DECLARE_EVENT_TABLE()
};

#endif
