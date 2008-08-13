// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2008 by The Odamex Team.
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

// configuration file structure
struct launchercfg_t
{
    wxInt32     get_list_on_start;
    wxInt32     show_blocked_servers;
    wxString    wad_paths;
    wxString    odamex_directory;
};

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

// config file value names
#define GETLISTONSTART      "GET_LIST_ON_START"
#define SHOWBLOCKEDSERVERS  "SHOW_BLOCKED_SERVERS"
#define DELIMWADPATHS       "DELIMITED_WAD_PATHS"
#define ODAMEX_DIRECTORY    "ODAMEX_DIRECTORY"

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
        
        wxCheckBox *m_ChkCtrlGetListOnStart;
        wxCheckBox *m_ChkCtrlShowBlockedServers;

        wxListBox *m_LstCtrlWadDirectories;

        wxDirPickerCtrl *m_DirCtrlChooseWadDir;

        wxDirPickerCtrl *m_DirCtrlChooseOdamexPath;

        wxTextCtrl *m_TxtCtrlMasterTimeout, *m_TxtCtrlServerTimeout;

        wxFileConfig ConfigInfo;

        launchercfg_t *cfg_file;
        
        wxInt32 UserChangedSetting;

	private:

		DECLARE_EVENT_TABLE()
};

#endif
