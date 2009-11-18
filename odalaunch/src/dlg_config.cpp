// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2009 by The Odamex Team.
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
//	AUTHOR:	Russell Rice, John D Corrado
//
//-----------------------------------------------------------------------------


#include "net_packet.h"
#include "dlg_config.h"

#include <wx/settings.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/msgdlg.h>
#include <wx/wfstream.h>
#include <wx/tokenzr.h>

#include "main.h"

// Widget ID's
static wxInt32 Id_ChkCtrlGetListOnStart = XRCID("Id_ChkCtrlGetListOnStart");
static wxInt32 Id_ChkCtrlShowBlockedServers = XRCID("Id_ChkCtrlShowBlockedServers");
static wxInt32 Id_DirCtrlChooseWadDir = XRCID("Id_DirCtrlChooseWadDir");
static wxInt32 Id_DirCtrlChooseOdamexPath = XRCID("Id_DirCtrlChooseOdamexPath");

static wxInt32 Id_LstCtrlWadDirectories = XRCID("Id_LstCtrlWadDirectories");

static wxInt32 Id_TxtCtrlMasterTimeout = XRCID("Id_TxtCtrlMasterTimeout");
static wxInt32 Id_TxtCtrlServerTimeout = XRCID("Id_TxtCtrlServerTimeout");
static wxInt32 Id_TxtCtrlExtraCmdLineArgs = XRCID("Id_TxtCtrlExtraCmdLineArgs");

// Event table for widgets
BEGIN_EVENT_TABLE(dlgConfig,wxDialog)

	// Button events
	EVT_BUTTON(XRCID("Id_BtnCtrlAddDir"), dlgConfig::OnAddDir)
	EVT_BUTTON(XRCID("Id_BtnCtrlReplaceDir"), dlgConfig::OnReplaceDir)
	EVT_BUTTON(XRCID("Id_BtnCtrlDeleteDir"), dlgConfig::OnDeleteDir)
    EVT_BUTTON(XRCID("Id_BtnCtrlMoveDirUp"), dlgConfig::OnUpClick)
    EVT_BUTTON(XRCID("Id_BtnCtrlMoveDirDown"), dlgConfig::OnDownClick)

    EVT_BUTTON(XRCID("Id_BtnCtrlGetEnvironment"), dlgConfig::OnGetEnvClick)

	EVT_BUTTON(wxID_OK, dlgConfig::OnOK)

    EVT_DIRPICKER_CHANGED(Id_DirCtrlChooseOdamexPath, dlgConfig::OnChooseOdamexPath)

	// Misc events
	EVT_CHECKBOX(Id_ChkCtrlGetListOnStart, dlgConfig::OnCheckedBox)
	EVT_CHECKBOX(Id_ChkCtrlShowBlockedServers, dlgConfig::OnCheckedBox)
	
	EVT_TEXT(Id_TxtCtrlMasterTimeout, dlgConfig::OnTextChange)
	EVT_TEXT(Id_TxtCtrlServerTimeout, dlgConfig::OnTextChange)
	EVT_TEXT(Id_TxtCtrlExtraCmdLineArgs, dlgConfig::OnTextChange)
END_EVENT_TABLE()

// Window constructor
dlgConfig::dlgConfig(launchercfg_t *cfg, wxWindow *parent, wxWindowID id)
{
    // Set up the dialog and its widgets
    wxXmlResource::Get()->LoadDialog(this, parent, _T("dlgConfig"));

    m_ChkCtrlGetListOnStart = wxStaticCast(FindWindow(Id_ChkCtrlGetListOnStart), wxCheckBox);
    m_ChkCtrlShowBlockedServers = wxStaticCast(FindWindow(Id_ChkCtrlShowBlockedServers), wxCheckBox);

    m_LstCtrlWadDirectories = wxStaticCast(FindWindow(Id_LstCtrlWadDirectories), wxListBox);

    m_DirCtrlChooseWadDir = wxStaticCast(FindWindow(Id_DirCtrlChooseWadDir), wxDirPickerCtrl);
    m_DirCtrlChooseOdamexPath = wxStaticCast(FindWindow(Id_DirCtrlChooseOdamexPath), wxDirPickerCtrl);

    m_TxtCtrlMasterTimeout = wxStaticCast(FindWindow(Id_TxtCtrlMasterTimeout), wxTextCtrl);
    m_TxtCtrlServerTimeout = wxStaticCast(FindWindow(Id_TxtCtrlServerTimeout), wxTextCtrl);
    m_TxtCtrlExtraCmdLineArgs = wxStaticCast(FindWindow(Id_TxtCtrlExtraCmdLineArgs), wxTextCtrl);

    // Load current configuration from global configuration structure
    cfg_file = cfg;

    LoadSettings();
}

// Window destructor
dlgConfig::~dlgConfig()
{

}

void dlgConfig::Show()
{
    m_ChkCtrlGetListOnStart->SetValue(cfg_file->get_list_on_start);
    m_ChkCtrlShowBlockedServers->SetValue(cfg_file->show_blocked_servers);

    // Load wad path list
    m_LstCtrlWadDirectories->Clear();

    wxStringTokenizer wadlist(cfg_file->wad_paths, _T(PATH_DELIMITER));

    while (wadlist.HasMoreTokens())
    {
        wxString path = wadlist.GetNextToken();

        #ifdef __WXMSW__
        path.Replace(_T("\\\\"),_T("\\"), true);
        #else
        path.Replace(_T("////"),_T("//"), true);
        #endif

        m_LstCtrlWadDirectories->AppendString(path);
    }

    m_DirCtrlChooseOdamexPath->SetPath(cfg_file->odamex_directory);

    wxString MasterTimeout, ServerTimeout, ExtraCmdLineArgs;

    ConfigInfo.Read(_T("MasterTimeout"), &MasterTimeout, _T("500"));
    ConfigInfo.Read(_T("ServerTimeout"), &ServerTimeout, _T("500"));
    ConfigInfo.Read(wxT("ExtraCommandLineArguments"), &ExtraCmdLineArgs, wxT(""));

    m_TxtCtrlMasterTimeout->SetValue(MasterTimeout);
    m_TxtCtrlServerTimeout->SetValue(ServerTimeout);
    m_TxtCtrlExtraCmdLineArgs->SetValue(ExtraCmdLineArgs);

    UserChangedSetting = 0;

    ShowModal();
}

void dlgConfig::OnCheckedBox(wxCommandEvent &event)
{
    UserChangedSetting = 1;
}

void dlgConfig::OnChooseOdamexPath(wxFileDirPickerEvent &event)
{
    UserChangedSetting = 1;
}

// User pressed ok button
void dlgConfig::OnOK(wxCommandEvent &event)
{
    wxMessageDialog msgdlg(this, _T("Save settings?"), _T("Save settings?"),
                           wxYES_NO | wxICON_QUESTION | wxSTAY_ON_TOP);

    if (UserChangedSetting == 1)
    if (msgdlg.ShowModal() == wxID_YES)
    {
        // reset 'dirty' flag
        UserChangedSetting = 0;

        // Store data into global launcher configuration structure
        cfg_file->get_list_on_start = m_ChkCtrlGetListOnStart->GetValue();
        cfg_file->show_blocked_servers = m_ChkCtrlShowBlockedServers->GetValue();

        cfg_file->wad_paths = _T("");

        if (m_LstCtrlWadDirectories->GetCount() > 0)
            for (wxUint32 i = 0; i < m_LstCtrlWadDirectories->GetCount(); i++)
                cfg_file->wad_paths.Append(m_LstCtrlWadDirectories->GetString(i) + _T(PATH_DELIMITER));

        cfg_file->odamex_directory = m_DirCtrlChooseOdamexPath->GetPath();

        // Save settings to configuration file
        SaveSettings();
    }
    else
        UserChangedSetting = 0;

    // Close window
    Close();
}

void dlgConfig::OnTextChange(wxCommandEvent &event)
{
    UserChangedSetting = 1;
}

/*
    WAD Directory stuff
*/

// Add a directory to the listbox
void dlgConfig::OnAddDir(wxCommandEvent &event)
{    
    wxString WadDirectory = m_DirCtrlChooseWadDir->GetPath();
    
    if (WadDirectory == wxT(""))
    {
        wxDirDialog ChooseWadDialog(this, 
                                    wxT("Select a folder containing WAD files"));
                                    
        
        if (ChooseWadDialog.ShowModal() != wxID_OK)
            return;
            
        WadDirectory = ChooseWadDialog.GetPath();
    }
    
    // Check to see if the path exists on the system
    if (wxDirExists(WadDirectory))
    {
        // Check if path already exists in box
        if (m_LstCtrlWadDirectories->FindString(WadDirectory) == wxNOT_FOUND)
        {
            m_LstCtrlWadDirectories->Append(WadDirectory);

            UserChangedSetting = 1;
        }
    }
    else
        wxMessageBox(wxString::Format(_T("Directory %s not found!"), WadDirectory.c_str()));
}

// Replace a directory in the listbox
void dlgConfig::OnReplaceDir(wxCommandEvent &event)
{
    wxInt32 i = m_LstCtrlWadDirectories->GetSelection();
    wxString WadDirectory = m_DirCtrlChooseWadDir->GetPath();

    if (i == wxNOT_FOUND)
    {
        wxMessageBox(_T("Select an directory from the list to replace!"));
        
        return;
    }
    
    if (WadDirectory == wxT(""))
    {
        wxDirDialog ChooseWadDialog(this, 
                                    wxT("Select a folder containing WAD files"));
                                    
        if (ChooseWadDialog.ShowModal() != wxID_OK)
            return;
            
        WadDirectory = ChooseWadDialog.GetPath();
    }
    
    // Check to see if the path exists on the system
    if (wxDirExists(WadDirectory))
    {
        // Get the selected item and replace it
        m_LstCtrlWadDirectories->SetString(i, WadDirectory);

        UserChangedSetting = 1;
    }
    else
        wxMessageBox(wxString::Format(_T("Directory %s not found!"), WadDirectory.c_str()));
}

// Delete a directory from the listbox
void dlgConfig::OnDeleteDir(wxCommandEvent &event)
{
    // Get the selected item and delete it, if
    // it is selected.
    wxInt32 i = m_LstCtrlWadDirectories->GetSelection();

    if (i != wxNOT_FOUND)
    {
        m_LstCtrlWadDirectories->Delete(i);

        UserChangedSetting = 1;
    }
    else
        wxMessageBox(_T("Select an item to delete."));
}

// Move directory in list up 1 item
void dlgConfig::OnUpClick(wxCommandEvent &event)
{
    // Get the selected item
    wxInt32 i = m_LstCtrlWadDirectories->GetSelection();

    if ((i != wxNOT_FOUND) && (i > 0))
    {
        wxString str = m_LstCtrlWadDirectories->GetString(i);

        m_LstCtrlWadDirectories->Delete(i);

        m_LstCtrlWadDirectories->Insert(str, i - 1);

        m_LstCtrlWadDirectories->SetSelection(i - 1);

        UserChangedSetting = 1;
    }
}

// Move directory in list down 1 item
void dlgConfig::OnDownClick(wxCommandEvent &event)
{
    // Get the selected item
    wxUint32 i = m_LstCtrlWadDirectories->GetSelection();

    if ((i != wxNOT_FOUND) && (i + 1 < m_LstCtrlWadDirectories->GetCount()))
    {
        wxString str = m_LstCtrlWadDirectories->GetString(i);

        m_LstCtrlWadDirectories->Delete(i);

        m_LstCtrlWadDirectories->Insert(str, i + 1);

        m_LstCtrlWadDirectories->SetSelection(i + 1);

        UserChangedSetting = 1;
    }
}

// Get the environment variables
void dlgConfig::OnGetEnvClick(wxCommandEvent &event)
{
    wxString doomwaddir = _T("");
    wxString env_paths[NUM_ENVVARS];
    wxInt32 i = 0;

    // create a small list of paths
    for (i = 0; i < NUM_ENVVARS; i++)
    {
        // only add paths if the variable exists and path isn't blank
        if (wxGetEnv(env_vars[i], &env_paths[i]))
            if (!env_paths[i].IsEmpty())
                doomwaddir += env_paths[i] + _T(PATH_DELIMITER);
    }

    wxInt32 path_count = 0;

    wxStringTokenizer wadlist(doomwaddir, _T(PATH_DELIMITER));

    while (wadlist.HasMoreTokens())
    {
        wxString path = wadlist.GetNextToken();

        // make sure the path doesn't already exist in the list box
        if (m_LstCtrlWadDirectories->FindString(path) == wxNOT_FOUND)
        {
                m_LstCtrlWadDirectories->Append(path);

                path_count++;
        }
    }

    if (path_count)
    {
        wxMessageBox(_T("Environment variables import successful!"));

        UserChangedSetting = 1;
    }
    else
        wxMessageBox(_T("Environment variables contains paths that have been already imported."));

}

// Load settings from configuration file
void dlgConfig::LoadSettings()
{
    ConfigInfo.Read(_T(GETLISTONSTART), &cfg_file->get_list_on_start, 1);
    ConfigInfo.Read(_T(SHOWBLOCKEDSERVERS), &cfg_file->show_blocked_servers, cfg_file->show_blocked_servers);
	cfg_file->wad_paths = ConfigInfo.Read(_T(DELIMWADPATHS), cfg_file->wad_paths);
	cfg_file->odamex_directory = ConfigInfo.Read(_T(ODAMEX_DIRECTORY), cfg_file->odamex_directory);
}

// Save settings to configuration file
void dlgConfig::SaveSettings()
{
    ConfigInfo.Write(_T("MasterTimeout"), m_TxtCtrlMasterTimeout->GetValue());
    ConfigInfo.Write(_T("ServerTimeout"), m_TxtCtrlServerTimeout->GetValue());
    ConfigInfo.Write(wxT("ExtraCommandLineArguments"), m_TxtCtrlExtraCmdLineArgs->GetValue());
    ConfigInfo.Write(_T(GETLISTONSTART), cfg_file->get_list_on_start);
	ConfigInfo.Write(_T(SHOWBLOCKEDSERVERS), cfg_file->show_blocked_servers);
	ConfigInfo.Write(_T(DELIMWADPATHS), cfg_file->wad_paths);
    ConfigInfo.Write(_T(ODAMEX_DIRECTORY), cfg_file->odamex_directory);

	ConfigInfo.Flush();
}
