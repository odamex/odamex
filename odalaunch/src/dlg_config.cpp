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
#include <wx/dirdlg.h>
//#include <wx/generic/dirdlgg.h>

#include "main.h"

// Widget ID's
static wxInt32 ID_CHKLISTONSTART = XRCID("ID_CHKLISTONSTART");
static wxInt32 ID_CHKSHOWBLOCKEDSERVERS = XRCID("ID_CHKSHOWBLOCKEDSERVERS");
static wxInt32 ID_BTNADD = XRCID("ID_BTNADD");
static wxInt32 ID_BTNREPLACE = XRCID("ID_BTNREPLACE");
static wxInt32 ID_BTNDELETE = XRCID("ID_BTNDELETE");
static wxInt32 ID_BTNCHOOSEDIR = XRCID("ID_BTNCHOOSEDIR");
static wxInt32 ID_BTNUP = XRCID("ID_BTNUP");
static wxInt32 ID_BTNDOWN = XRCID("ID_BTNDOWN");

static wxInt32 ID_BTNGETENV = XRCID("ID_BTNGETENV");

static wxInt32 ID_BTNCLOSE = XRCID("ID_BTNCLOSE");
static wxInt32 ID_BTNOK = XRCID("ID_BTNOK");

static wxInt32 ID_LSTWADDIR = XRCID("ID_LSTWADDIR");

static wxInt32 ID_TXTWADDIR = XRCID("ID_TXTWADDIR");

// Event table for widgets
BEGIN_EVENT_TABLE(dlgConfig,wxDialog)

	// Button events
	EVT_BUTTON(ID_BTNCHOOSEDIR, dlgConfig::OnChooseDir)
	EVT_BUTTON(ID_BTNADD, dlgConfig::OnAddDir)
	EVT_BUTTON(ID_BTNREPLACE, dlgConfig::OnReplaceDir)
	EVT_BUTTON(ID_BTNDELETE, dlgConfig::OnDeleteDir)
    EVT_BUTTON(ID_BTNUP, dlgConfig::OnUpClick)
    EVT_BUTTON(ID_BTNDOWN, dlgConfig::OnDownClick)

    EVT_BUTTON(ID_BTNGETENV, dlgConfig::OnGetEnvClick)

	EVT_BUTTON(ID_BTNOK, dlgConfig::OnOK)
	EVT_BUTTON(ID_BTNCLOSE, dlgConfig::OnClose)
	// Misc events
	EVT_CHECKBOX(ID_CHKLISTONSTART, dlgConfig::OnCheckedBox)
	EVT_CHECKBOX(ID_CHKSHOWBLOCKEDSERVERS, dlgConfig::OnCheckedBox)
END_EVENT_TABLE()

// Window constructor
dlgConfig::dlgConfig(launchercfg_t *cfg, wxWindow *parent, wxWindowID id)
{
    // Set up the dialog and its widgets
    wxXmlResource::Get()->LoadDialog(this, parent, _T("dlgConfig"));

    MASTER_CHECKBOX = wxStaticCast((*this).FindWindow(ID_CHKLISTONSTART), wxCheckBox);
    BLOCKED_CHECKBOX = wxStaticCast((*this).FindWindow(ID_CHKSHOWBLOCKEDSERVERS), wxCheckBox);

    WAD_LIST = wxStaticCast((*this).FindWindow(ID_LSTWADDIR),wxListBox);

    DIR_BOX = wxStaticCast((*this).FindWindow(ID_TXTWADDIR),wxTextCtrl);

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
    MASTER_CHECKBOX->SetValue(cfg_file->get_list_on_start);
    BLOCKED_CHECKBOX->SetValue(cfg_file->show_blocked_servers);

    // Load wad path list
    WAD_LIST->Clear();

    wxStringTokenizer wadlist(cfg_file->wad_paths, _T(';'));

    UserChangedSetting = 0;

    while (wadlist.HasMoreTokens())
    {
        wxString path = wadlist.GetNextToken();

        #ifdef __WXMSW__
        path.Replace(_T("\\\\"),_T("\\"), true);
        #else
        path.Replace(_T("////"),_T("//"), true);
        #endif

        WAD_LIST->AppendString(path);
    }

    ShowModal();
}

void dlgConfig::OnCheckedBox(wxCommandEvent &event)
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
        cfg_file->get_list_on_start = MASTER_CHECKBOX->GetValue();
        cfg_file->show_blocked_servers = BLOCKED_CHECKBOX->GetValue();

        cfg_file->wad_paths = _T("");

        if (WAD_LIST->GetCount() > 0)
            for (wxUint32 i = 0; i < WAD_LIST->GetCount(); i++)
                cfg_file->wad_paths.Append(WAD_LIST->GetString(i) + _T(';'));

        // Save settings to configuration file
        SaveSettings();
    }
    else
        UserChangedSetting = 0;

    // Close window
    Close();
}

void dlgConfig::OnClose(wxCommandEvent &event)
{
    Close();
}

/*
    WAD Directory stuff
*/

// Add a directory to the listbox
void dlgConfig::OnAddDir(wxCommandEvent &event)
{
    // Check if our path box has a path in it
    if (DIR_BOX->GetLabel().IsEmpty())
    {
        wxDirDialog dirchoose(this, _T("Choose a WAD directory"));

        // User pressed OK?
        if (dirchoose.ShowModal() == wxID_OK)
            DIR_BOX->SetLabel(dirchoose.GetPath());
        else
            return;
    }

    // Check to see if the path exists on the system
    if (wxDirExists(DIR_BOX->GetLabel()))
    {
        // Check if path already exists in box
        if (WAD_LIST->FindString(DIR_BOX->GetLabel()) == wxNOT_FOUND)
        {
            WAD_LIST->Append(DIR_BOX->GetLabel());

            UserChangedSetting = 1;
        }
    }
    else
        wxMessageBox(wxString::Format(_T("Directory %s not found!"), DIR_BOX->GetLabel().c_str()));
}

// Replace a directory in the listbox
void dlgConfig::OnReplaceDir(wxCommandEvent &event)
{
    // Check to see if the path exists on the system
    if (wxDirExists(DIR_BOX->GetLabel()))
    {
        // Get the selected item and replace it, if
        // it is selected.
        wxInt32 i = WAD_LIST->GetSelection();

        if (i != wxNOT_FOUND)
        {
            WAD_LIST->SetString(i,DIR_BOX->GetLabel());

            UserChangedSetting = 1;
        }
        else
            wxMessageBox(_T("Select item to replace!"));
    }
    else
        wxMessageBox(wxString::Format(_T("Directory %s not found!"), DIR_BOX->GetLabel().c_str()));
}

// Delete a directory from the listbox
void dlgConfig::OnDeleteDir(wxCommandEvent &event)
{
    // Get the selected item and delete it, if
    // it is selected.
    wxInt32 i = WAD_LIST->GetSelection();

    if (i != wxNOT_FOUND)
    {
        WAD_LIST->Delete(i);

        UserChangedSetting = 1;
    }
    else
        wxMessageBox(_T("Select an item to delete."));
}

// Choose directory (ellipsis button)
void dlgConfig::OnChooseDir(wxCommandEvent &event)
{
    wxDirDialog dirchoose(this, _T("Choose a WAD directory"));

    // User pressed ok?
    if (dirchoose.ShowModal() == wxID_OK)
        DIR_BOX->SetLabel(dirchoose.GetPath());
}

// Move directory in list up 1 item
void dlgConfig::OnUpClick(wxCommandEvent &event)
{
    // Get the selected item
    wxInt32 i = WAD_LIST->GetSelection();

    if ((i != wxNOT_FOUND) && (i > 0))
    {
        wxString str = WAD_LIST->GetString(i);

        WAD_LIST->Delete(i);

        WAD_LIST->Insert(str, i - 1);

        WAD_LIST->SetSelection(i - 1);

        UserChangedSetting = 1;
    }
}

// Move directory in list down 1 item
void dlgConfig::OnDownClick(wxCommandEvent &event)
{
    // Get the selected item
    wxUint32 i = WAD_LIST->GetSelection();

    if ((i != wxNOT_FOUND) && (i + 1 < WAD_LIST->GetCount()))
    {
        wxString str = WAD_LIST->GetString(i);

        WAD_LIST->Delete(i);

        WAD_LIST->Insert(str, i + 1);

        WAD_LIST->SetSelection(i + 1);

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
                doomwaddir += env_paths[i] + _T(';');
    }

    wxInt32 path_count = 0;

    wxStringTokenizer wadlist(doomwaddir, _T(';'));

    while (wadlist.HasMoreTokens())
    {
        wxString path = wadlist.GetNextToken();

        // make sure the path doesn't already exist in the list box
        if (WAD_LIST->FindString(path) == wxNOT_FOUND)
        {
                WAD_LIST->Append(path);

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
}

// Save settings to configuration file
void dlgConfig::SaveSettings()
{
    ConfigInfo.Write(_T(GETLISTONSTART), cfg_file->get_list_on_start);
	ConfigInfo.Write(_T(SHOWBLOCKEDSERVERS), cfg_file->show_blocked_servers);
	ConfigInfo.Write(_T(DELIMWADPATHS), cfg_file->wad_paths);

	ConfigInfo.Flush();
}
