// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2010 by The Odamex Team.
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
#include <wx/recguard.h>

#include "main.h"

// Widget ID's
static wxInt32 Id_ChkCtrlGetListOnStart = XRCID("Id_ChkCtrlGetListOnStart");
static wxInt32 Id_ChkCtrlShowBlockedServers = XRCID("Id_ChkCtrlShowBlockedServers");
static wxInt32 Id_DirCtrlChooseWadDir = XRCID("Id_DirCtrlChooseWadDir");
static wxInt32 Id_DirCtrlChooseOdamexPath = XRCID("Id_DirCtrlChooseOdamexPath");

static wxInt32 Id_LstCtrlWadDirectories = XRCID("Id_LstCtrlWadDirectories");

static wxInt32 Id_SpnCtrlMasterTimeout = XRCID("Id_SpnCtrlMasterTimeout");
static wxInt32 Id_SpnCtrlServerTimeout = XRCID("Id_SpnCtrlServerTimeout");
static wxInt32 Id_TxtCtrlExtraCmdLineArgs = XRCID("Id_TxtCtrlExtraCmdLineArgs");

static wxInt32 Id_SpnCtrlPQGood = XRCID("Id_SpnCtrlPQGood");
static wxInt32 Id_SpnCtrlPQPlayable = XRCID("Id_SpnCtrlPQPlayable");
static wxInt32 Id_SpnCtrlPQLaggy = XRCID("Id_SpnCtrlPQLaggy");

static wxInt32 Id_StcBmpPQGood = XRCID("Id_StcBmpPQGood");
static wxInt32 Id_StcBmpPQPlayable = XRCID("Id_StcBmpPQPlayable");
static wxInt32 Id_StcBmpPQLaggy = XRCID("Id_StcBmpPQLaggy");
static wxInt32 Id_StcBmpPQBad = XRCID("Id_StcBmpPQBad");

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
	
	EVT_TEXT(Id_SpnCtrlMasterTimeout, dlgConfig::OnTextChange)
	EVT_TEXT(Id_SpnCtrlServerTimeout, dlgConfig::OnTextChange)
	EVT_TEXT(Id_TxtCtrlExtraCmdLineArgs, dlgConfig::OnTextChange)
	
	EVT_SPINCTRL(Id_SpnCtrlPQGood, dlgConfig::OnSpinValChange)
	EVT_SPINCTRL(Id_SpnCtrlPQPlayable, dlgConfig::OnSpinValChange)
	EVT_SPINCTRL(Id_SpnCtrlPQLaggy, dlgConfig::OnSpinValChange)
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

    m_SpnCtrlMasterTimeout = wxStaticCast(FindWindow(Id_SpnCtrlMasterTimeout), wxSpinCtrl);
    m_SpnCtrlServerTimeout = wxStaticCast(FindWindow(Id_SpnCtrlServerTimeout), wxSpinCtrl);
    m_TxtCtrlExtraCmdLineArgs = wxStaticCast(FindWindow(Id_TxtCtrlExtraCmdLineArgs), wxTextCtrl);

    m_SpnCtrlPQGood = wxStaticCast(FindWindow(Id_SpnCtrlPQGood), wxSpinCtrl);
    m_SpnCtrlPQPlayable = wxStaticCast(FindWindow(Id_SpnCtrlPQPlayable), wxSpinCtrl);
    m_SpnCtrlPQLaggy = wxStaticCast(FindWindow(Id_SpnCtrlPQLaggy), wxSpinCtrl);

    m_StcBmpPQGood = wxStaticCast(FindWindow(Id_StcBmpPQGood), wxStaticBitmap);
    m_StcBmpPQPlayable = wxStaticCast(FindWindow(Id_StcBmpPQPlayable), wxStaticBitmap);
    m_StcBmpPQLaggy = wxStaticCast(FindWindow(Id_StcBmpPQLaggy), wxStaticBitmap);
    m_StcBmpPQBad = wxStaticCast(FindWindow(Id_StcBmpPQBad), wxStaticBitmap);

    // Ping quality icons
    m_StcBmpPQGood->SetBitmap(wxXmlResource::Get()->LoadBitmap(wxT("bullet_green")));
    m_StcBmpPQPlayable->SetBitmap(wxXmlResource::Get()->LoadBitmap(wxT("bullet_orange")));
    m_StcBmpPQLaggy->SetBitmap(wxXmlResource::Get()->LoadBitmap(wxT("bullet_red")));
    m_StcBmpPQBad->SetBitmap(wxXmlResource::Get()->LoadBitmap(wxT("bullet_gray")));

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

    ConfigInfo.Read(wxT(MASTERTIMEOUT), &MasterTimeout, wxT("500"));
    ConfigInfo.Read(wxT(SERVERTIMEOUT), &ServerTimeout, wxT("500"));
    ConfigInfo.Read(wxT(EXTRACMDLINEARGS), &ExtraCmdLineArgs, wxT(""));

    m_SpnCtrlMasterTimeout->SetValue(MasterTimeout);
    m_SpnCtrlServerTimeout->SetValue(ServerTimeout);
    m_TxtCtrlExtraCmdLineArgs->SetValue(ExtraCmdLineArgs);

    wxInt32 PQGood, PQPlayable, PQLaggy;
    
    ConfigInfo.Read(wxT("IconPingQualityGood"), &PQGood, 150);
    ConfigInfo.Read(wxT("IconPingQualityPlayable"), &PQPlayable, 300);
    ConfigInfo.Read(wxT("IconPingQualityLaggy"), &PQLaggy, 350);
    
    m_SpnCtrlPQGood->SetValue(PQGood);
    m_SpnCtrlPQPlayable->SetValue(PQPlayable);
    m_SpnCtrlPQLaggy->SetValue(PQLaggy);

    UserChangedSetting = false;

    ShowModal();
}

void dlgConfig::OnCheckedBox(wxCommandEvent &event)
{
    UserChangedSetting = true;
}

void dlgConfig::OnChooseOdamexPath(wxFileDirPickerEvent &event)
{
    UserChangedSetting = true;
}

// Ping quality spin control changes
void dlgConfig::OnSpinValChange(wxSpinEvent &event)
{
    wxInt32 PQGood, PQPlayable, PQLaggy;

    PQGood = m_SpnCtrlPQGood->GetValue();
    PQPlayable = m_SpnCtrlPQPlayable->GetValue();
    
    // Handle spin values that go beyond a certain range
    // wxWidgets Bug: Double-clicking the down arrow on a spin control will go
    // 1 more below even though we force it not to, we use an event so we 
    // counteract this with using +2 stepping
    if (PQGood >= PQPlayable)
    {
        m_SpnCtrlPQPlayable->SetValue(PQGood + 2);
    }

    PQPlayable = m_SpnCtrlPQPlayable->GetValue();
    PQLaggy = m_SpnCtrlPQLaggy->GetValue();

    if (PQPlayable >= PQLaggy)
    {
        m_SpnCtrlPQLaggy->SetValue(PQPlayable + 2);
    }

    UserChangedSetting = true;
}

// User pressed ok button
void dlgConfig::OnOK(wxCommandEvent &event)
{
    wxMessageDialog msgdlg(this, _T("Save settings?"), _T("Save settings?"),
                           wxYES_NO | wxICON_QUESTION | wxSTAY_ON_TOP);

    if (UserChangedSetting == false)
    {
        Close();
        
        return;
    }

    if (msgdlg.ShowModal() == wxID_YES)
    {
        // reset 'dirty' flag
        UserChangedSetting = false;

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
        UserChangedSetting = false;

    // Close window
    Close();
}

void dlgConfig::OnTextChange(wxCommandEvent &event)
{
    UserChangedSetting = true;
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

            UserChangedSetting = true;
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

        UserChangedSetting = true;
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

        UserChangedSetting = true;
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

        UserChangedSetting = true;
    }
}

// Move directory in list down 1 item
void dlgConfig::OnDownClick(wxCommandEvent &event)
{
    // Get the selected item
    wxInt32 i = m_LstCtrlWadDirectories->GetSelection();

    if ((i != wxNOT_FOUND) && (i + 1 < m_LstCtrlWadDirectories->GetCount()))
    {
        wxString str = m_LstCtrlWadDirectories->GetString(i);

        m_LstCtrlWadDirectories->Delete(i);

        m_LstCtrlWadDirectories->Insert(str, i + 1);

        m_LstCtrlWadDirectories->SetSelection(i + 1);

        UserChangedSetting = true;
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

        UserChangedSetting = true;
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
    ConfigInfo.Write(wxT(MASTERTIMEOUT), m_SpnCtrlMasterTimeout->GetValue());
    ConfigInfo.Write(wxT(SERVERTIMEOUT), m_SpnCtrlServerTimeout->GetValue());
    ConfigInfo.Write(wxT(EXTRACMDLINEARGS), m_TxtCtrlExtraCmdLineArgs->GetValue());
    ConfigInfo.Write(wxT(GETLISTONSTART), cfg_file->get_list_on_start);
	ConfigInfo.Write(wxT(SHOWBLOCKEDSERVERS), cfg_file->show_blocked_servers);
	ConfigInfo.Write(wxT(DELIMWADPATHS), cfg_file->wad_paths);
    ConfigInfo.Write(wxT(ODAMEX_DIRECTORY), cfg_file->odamex_directory);
    ConfigInfo.Write(wxT("IconPingQualityGood"), m_SpnCtrlPQGood->GetValue());
    ConfigInfo.Write(wxT("IconPingQualityPlayable"), m_SpnCtrlPQPlayable->GetValue());
    ConfigInfo.Write(wxT("IconPingQualityLaggy"), m_SpnCtrlPQLaggy->GetValue());

	ConfigInfo.Flush();
}
