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
//	AUTHOR:	Russell Rice, John D Corrado
//
//-----------------------------------------------------------------------------


#include "net_packet.h"
#include "dlg_config.h"
#include "oda_defs.h"
#include "plat_utils.h"

#include <wx/settings.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/msgdlg.h>
#include <wx/wfstream.h>
#include <wx/tokenzr.h>
#include <wx/recguard.h>

#include "main.h"

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

    EVT_DIRPICKER_CHANGED(XRCID("Id_DirCtrlChooseOdamexPath"), dlgConfig::OnChooseOdamexPath)

	// Misc events
	EVT_CHECKBOX(XRCID("Id_ChkCtrlGetListOnStart"), dlgConfig::OnCheckedBox)
	EVT_CHECKBOX(XRCID("Id_ChkCtrlShowBlockedServers"), dlgConfig::OnCheckedBox)
	EVT_CHECKBOX(XRCID("Id_ChkCtrlEnableBroadcasts"), dlgConfig::OnCheckedBox)

	EVT_TEXT(XRCID("Id_SpnCtrlMasterTimeout"), dlgConfig::OnTextChange)
	EVT_TEXT(XRCID("Id_SpnCtrlServerTimeout"), dlgConfig::OnTextChange)
	EVT_TEXT(XRCID("Id_SpnCtrlRetry"), dlgConfig::OnTextChange)
	EVT_TEXT(XRCID("Id_TxtCtrlExtraCmdLineArgs"), dlgConfig::OnTextChange)

	EVT_SPINCTRL(XRCID("Id_SpnCtrlPQGood"), dlgConfig::OnSpinValChange)
	EVT_SPINCTRL(XRCID("Id_SpnCtrlPQPlayable"), dlgConfig::OnSpinValChange)
	EVT_SPINCTRL(XRCID("Id_SpnCtrlPQLaggy"), dlgConfig::OnSpinValChange)

	EVT_LISTBOX_DCLICK(XRCID("Id_LstCtrlWadDirectories"), dlgConfig::OnReplaceDir)
END_EVENT_TABLE()

// Window constructor
dlgConfig::dlgConfig(wxWindow *parent, wxWindowID id)
{
    // Set up the dialog and its widgets
    wxXmlResource::Get()->LoadDialog(this, parent, _T("dlgConfig"));

    m_ChkCtrlGetListOnStart = XRCCTRL(*this, "Id_ChkCtrlGetListOnStart", wxCheckBox);
    m_ChkCtrlShowBlockedServers = XRCCTRL(*this, "Id_ChkCtrlShowBlockedServers", wxCheckBox);
    m_ChkCtrlEnableBroadcasts = XRCCTRL(*this, "Id_ChkCtrlEnableBroadcasts", wxCheckBox);

    m_LstCtrlWadDirectories = XRCCTRL(*this, "Id_LstCtrlWadDirectories", wxListBox);

    m_DirCtrlChooseOdamexPath = XRCCTRL(*this, "Id_DirCtrlChooseOdamexPath", wxDirPickerCtrl);

    m_SpnCtrlMasterTimeout = XRCCTRL(*this, "Id_SpnCtrlMasterTimeout", wxSpinCtrl);
    m_SpnCtrlServerTimeout = XRCCTRL(*this, "Id_SpnCtrlServerTimeout", wxSpinCtrl);
    m_SpnCtrlRetry = XRCCTRL(*this, "Id_SpnCtrlRetry", wxSpinCtrl);
    m_TxtCtrlExtraCmdLineArgs = XRCCTRL(*this, "Id_TxtCtrlExtraCmdLineArgs", wxTextCtrl);

    m_SpnCtrlPQGood = XRCCTRL(*this, "Id_SpnCtrlPQGood", wxSpinCtrl);
    m_SpnCtrlPQPlayable = XRCCTRL(*this, "Id_SpnCtrlPQPlayable", wxSpinCtrl);
    m_SpnCtrlPQLaggy = XRCCTRL(*this, "Id_SpnCtrlPQLaggy", wxSpinCtrl);

    m_StcBmpPQGood = XRCCTRL(*this, "Id_StcBmpPQGood", wxStaticBitmap);
    m_StcBmpPQPlayable = XRCCTRL(*this, "Id_StcBmpPQPlayable", wxStaticBitmap);
    m_StcBmpPQLaggy = XRCCTRL(*this, "Id_StcBmpPQLaggy", wxStaticBitmap);
    m_StcBmpPQBad = XRCCTRL(*this, "Id_StcBmpPQBad", wxStaticBitmap);

    // Ping quality icons
    m_StcBmpPQGood->SetBitmap(wxXmlResource::Get()->LoadBitmap(wxT("bullet_green")));
    m_StcBmpPQPlayable->SetBitmap(wxXmlResource::Get()->LoadBitmap(wxT("bullet_orange")));
    m_StcBmpPQLaggy->SetBitmap(wxXmlResource::Get()->LoadBitmap(wxT("bullet_red")));
    m_StcBmpPQBad->SetBitmap(wxXmlResource::Get()->LoadBitmap(wxT("bullet_gray")));

    // Allow $ in directory names
    ConfigInfo.SetExpandEnvVars(false);
}

// Window destructor
dlgConfig::~dlgConfig()
{

}

void dlgConfig::Show()
{
    LoadSettings();
    
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

    wxSpinCtrl *SpinControl = wxDynamicCast(event.GetEventObject(), wxSpinCtrl);

    if (!SpinControl)
        return;


    if (SpinControl == m_SpnCtrlPQGood)
        PQGood = m_SpnCtrlPQGood->GetValue();

    if (SpinControl == m_SpnCtrlPQPlayable)
        PQPlayable = m_SpnCtrlPQPlayable->GetValue();

    // Handle spin values that go beyond a certain range
    // wxWidgets Bug: Double-clicking the down arrow on a spin control will go
    // 1 more below even though we force it not to, we use an event so we
    // counteract this with using +2 stepping
    if (PQGood >= PQPlayable)
    {
        if (SpinControl == m_SpnCtrlPQPlayable)
            m_SpnCtrlPQPlayable->SetValue(PQGood + 2);
    }

    if (SpinControl == m_SpnCtrlPQPlayable)
        PQPlayable = m_SpnCtrlPQPlayable->GetValue();
    if (SpinControl == m_SpnCtrlPQLaggy)
        PQLaggy = m_SpnCtrlPQLaggy->GetValue();

    if (PQPlayable >= PQLaggy)
    {
        if (SpinControl == m_SpnCtrlPQLaggy)
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
    wxString WadDirectory;

    wxDirDialog ChooseWadDialog(this,
        wxT("Select a directory containing WAD files"));

    if (ChooseWadDialog.ShowModal() != wxID_OK)
        return;

    WadDirectory = ChooseWadDialog.GetPath();

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
        wxMessageBox(wxString::Format(_T("Directory %s not found"),
                WadDirectory.c_str()));
}

// Replace a directory in the listbox
void dlgConfig::OnReplaceDir(wxCommandEvent &event)
{
    wxInt32 i = m_LstCtrlWadDirectories->GetSelection();

    wxString WadDirectory;

    if (i == wxNOT_FOUND)
    {
        wxMessageBox(_T("Select a directory from the list to replace"));

        return;
    }

    WadDirectory = m_LstCtrlWadDirectories->GetStringSelection();

    wxDirDialog ChooseWadDialog(this,
                                wxT("Replace selected directory with.."),
                                WadDirectory);

    if (ChooseWadDialog.ShowModal() != wxID_OK)
        return;

    WadDirectory = ChooseWadDialog.GetPath();

    // Check to see if the path exists on the system
    if (wxDirExists(WadDirectory))
    {
        // Get the selected item and replace it
        m_LstCtrlWadDirectories->SetString(i, WadDirectory);

        UserChangedSetting = true;
    }
    else
        wxMessageBox(wxString::Format(_T("Directory %s not found"),
                WadDirectory.c_str()));
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
        wxMessageBox(_T("Select a directory from the list to delete"));
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
        wxMessageBox(_T("Environment variables import successful"));

        UserChangedSetting = true;
    }
    else
        wxMessageBox(_T("Environment variables contains paths that have been already imported."));

}

// Load settings from configuration file
void dlgConfig::LoadSettings()
{
    bool UseBroadcast;
    bool GetListOnStart, ShowBlockedServers;
    int MasterTimeout, ServerTimeout, RetryCount;
    wxString DelimWadPaths, OdamexDirectory, ExtraCmdLineArgs;
    wxInt32 PQGood, PQPlayable, PQLaggy;

    ConfigInfo.Read(wxT(USEBROADCAST), &UseBroadcast, ODA_QRYUSEBROADCAST);
    ConfigInfo.Read(wxT(GETLISTONSTART), &GetListOnStart, ODA_UIGETLISTONSTART);
    ConfigInfo.Read(wxT(SHOWBLOCKEDSERVERS), &ShowBlockedServers, 
                    ODA_UISHOWBLOCKEDSERVERS);
    ConfigInfo.Read(wxT(DELIMWADPATHS), &DelimWadPaths, OdaGetDataDir());
    ConfigInfo.Read(wxT(ODAMEX_DIRECTORY), &OdamexDirectory, OdaGetInstallDir());
    ConfigInfo.Read(wxT(MASTERTIMEOUT), &MasterTimeout, ODA_QRYMASTERTIMEOUT);
    ConfigInfo.Read(wxT(SERVERTIMEOUT), &ServerTimeout, ODA_QRYSERVERTIMEOUT);
    ConfigInfo.Read(wxT(RETRYCOUNT), &RetryCount, ODA_QRYGSRETRYCOUNT);
    ConfigInfo.Read(wxT(EXTRACMDLINEARGS), &ExtraCmdLineArgs, wxT(""));
    ConfigInfo.Read(wxT(ICONPINGQGOOD), &PQGood, ODA_UIPINGQUALITYGOOD);
    ConfigInfo.Read(wxT(ICONPINGQPLAYABLE), &PQPlayable, 
        ODA_UIPINGQUALITYPLAYABLE);
    ConfigInfo.Read(wxT(ICONPINGQLAGGY), &PQLaggy, ODA_UIPINGQUALITYLAGGY);
        
    m_ChkCtrlEnableBroadcasts->SetValue(UseBroadcast);
    m_ChkCtrlGetListOnStart->SetValue(GetListOnStart);
    m_ChkCtrlShowBlockedServers->SetValue(ShowBlockedServers);
    m_DirCtrlChooseOdamexPath->SetPath(OdamexDirectory);

    // Load wad path list
    m_LstCtrlWadDirectories->Clear();

    wxStringTokenizer wadlist(DelimWadPaths, _T(PATH_DELIMITER));

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
    
    m_SpnCtrlMasterTimeout->SetValue(MasterTimeout);
    m_SpnCtrlServerTimeout->SetValue(ServerTimeout);
    m_SpnCtrlRetry->SetValue(RetryCount);
    m_TxtCtrlExtraCmdLineArgs->SetValue(ExtraCmdLineArgs);

    m_SpnCtrlPQGood->SetValue(PQGood);
    m_SpnCtrlPQPlayable->SetValue(PQPlayable);
    m_SpnCtrlPQLaggy->SetValue(PQLaggy);

}

// Save settings to configuration file
void dlgConfig::SaveSettings()
{
    wxString DelimWadPaths;
    
    for (unsigned int i = 0; i < m_LstCtrlWadDirectories->GetCount(); i++)
        DelimWadPaths.Append(m_LstCtrlWadDirectories->GetString(i) + wxT(PATH_DELIMITER));
   
    ConfigInfo.Write(wxT(MASTERTIMEOUT), m_SpnCtrlMasterTimeout->GetValue());
    ConfigInfo.Write(wxT(SERVERTIMEOUT), m_SpnCtrlServerTimeout->GetValue());
    ConfigInfo.Write(wxT(RETRYCOUNT), m_SpnCtrlRetry->GetValue());
    ConfigInfo.Write(wxT(EXTRACMDLINEARGS), m_TxtCtrlExtraCmdLineArgs->GetValue());
    ConfigInfo.Write(wxT(GETLISTONSTART), m_ChkCtrlGetListOnStart->GetValue());
	ConfigInfo.Write(wxT(SHOWBLOCKEDSERVERS), m_ChkCtrlShowBlockedServers->GetValue());
	ConfigInfo.Write(wxT(DELIMWADPATHS), DelimWadPaths);
    ConfigInfo.Write(wxT(ODAMEX_DIRECTORY), m_DirCtrlChooseOdamexPath->GetPath());
    ConfigInfo.Write(wxT(ICONPINGQGOOD), m_SpnCtrlPQGood->GetValue());
    ConfigInfo.Write(wxT(ICONPINGQPLAYABLE), m_SpnCtrlPQPlayable->GetValue());
    ConfigInfo.Write(wxT(ICONPINGQLAGGY), m_SpnCtrlPQLaggy->GetValue());
    ConfigInfo.Write(wxT(USEBROADCAST), m_ChkCtrlEnableBroadcasts->GetValue());

	ConfigInfo.Flush();
}
