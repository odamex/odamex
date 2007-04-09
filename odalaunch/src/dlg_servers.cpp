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
//	Custom Servers dialog
//	AUTHOR:	Russell Rice, John D Corrado
//
//-----------------------------------------------------------------------------


#include "dlg_servers.h"

#include <wx/settings.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/msgdlg.h>
#include <wx/wfstream.h>
#include <wx/tokenzr.h>
#include <wx/dirdlg.h>

// Widget ID's
static wxInt32 ID_SRVCHKLST = XRCID("ID_SRVCHKLST");

static wxInt32 ID_BTNSRVADD = XRCID("ID_BTNSRVADD");
static wxInt32 ID_BTNSRVDEL = XRCID("ID_BTNSRVDEL");

static wxInt32 ID_BTNSRVUP = XRCID("ID_BTNSRVUP");
static wxInt32 ID_BTNSRVDWN = XRCID("ID_BTNSRVDWN");

static wxInt32 ID_SRVIPPORT = XRCID("ID_SRVIPPORT");
static wxInt32 ID_CHKSUBST = XRCID("ID_CHKSUBST");
static wxInt32 ID_CHKSUBIPPORT = XRCID("ID_SRVSUBIPPORT");

static wxInt32 ID_BTNSRVOK = XRCID("ID_BTNSRVOK");
static wxInt32 ID_BTNSRVCLOSE = XRCID("ID_BTNSRVCLOSE");

// Event table for widgets
BEGIN_EVENT_TABLE(dlgServers,wxDialog)
	// Window events
    EVT_BUTTON(ID_BTNSRVOK, dlgServers::OnButtonOK)
    EVT_BUTTON(ID_BTNSRVCLOSE, dlgServers::OnButtonClose)

	// Button events
    EVT_BUTTON(ID_BTNSRVADD, dlgServers::OnButtonAddServer)
    EVT_BUTTON(ID_BTNSRVDEL, dlgServers::OnButtonDelServer)
    EVT_BUTTON(ID_BTNSRVUP, dlgServers::OnButtonMoveServerUp)
    EVT_BUTTON(ID_BTNSRVDWN, dlgServers::OnButtonMoveServerDown) 

	// Misc events
    EVT_LISTBOX(ID_SRVCHKLST, dlgServers::OnCheckServerListClick) 
//	EVT_CHECKBOX(ID_CHKSUBST, dlgConfig::OnSubstChecked)
END_EVENT_TABLE()

// Window constructor
dlgServers::dlgServers(wxWindow *parent, wxWindowID id)
{
    // Set up the dialog and its widgets
    wxXmlResource::Get()->LoadDialog(this, parent, _T("dlgServers"));

    SERVER_LIST = wxStaticCast(FindWindow(ID_SRVCHKLST),wxCheckListBox);
    SERVER_IPPORT_BOX = wxStaticCast(FindWindow(ID_SRVIPPORT),wxTextCtrl); 
}

// Window destructor
dlgServers::~dlgServers()
{

}

// OK button
void dlgServers::OnButtonOK(wxCommandEvent &event)
{
    Close();
}

// Close button
void dlgServers::OnButtonClose(wxCommandEvent &event)
{
    Close();
}

// Server Check List Clicked
void dlgServers::OnCheckServerListClick(wxCommandEvent &event)
{
    SERVER_IPPORT_BOX->SetLabel(SERVER_LIST->GetStringSelection());
}

// Add Server button
void dlgServers::OnButtonAddServer(wxCommandEvent &event)
{
    wxString ted_result = _T("");
    wxTextEntryDialog ted(this, 
                            _T("Please enter IP Address and Port"), 
                            _T("Please enter IP Address and Port"),
                            _T("0.0.0.0:0"));
    // Show it
    ted.ShowModal();
    ted_result = ted.GetValue();
    
    if (!ted_result.IsEmpty() && 
        ted_result != _T("0.0.0.0:0") &&
        SERVER_LIST->FindString(ted_result) == wxNOT_FOUND)
    {
        SERVER_LIST->Append(ted_result);
//        UserChangedSetting = 1;
    }
}

// Delete Server button
void dlgServers::OnButtonDelServer(wxCommandEvent &event)
{
    if (SERVER_LIST->GetSelection() != wxNOT_FOUND)
    {
        SERVER_LIST->Delete(SERVER_LIST->GetSelection());
//        UserChangedSetting = 1;
    }
}

// Move Server Up button
void dlgServers::OnButtonMoveServerUp(wxCommandEvent &event)
{
    // Get the selected item
    wxInt32 i = SERVER_LIST->GetSelection();

    if ((i != wxNOT_FOUND) && (i > 0))
    {
        wxString str = SERVER_LIST->GetString(i);

        SERVER_LIST->Delete(i);

        SERVER_LIST->Insert(str, i - 1);

        SERVER_LIST->SetSelection(i - 1);

//        UserChangedSetting = 1;
    }
}

// Move Server Down button
void dlgServers::OnButtonMoveServerDown(wxCommandEvent &event)
{
    // Get the selected item
    wxUint32 i = SERVER_LIST->GetSelection();

    if ((i != wxNOT_FOUND) && (i + 1 < SERVER_LIST->GetCount()))
    {
        wxString str = SERVER_LIST->GetString(i);

        SERVER_LIST->Delete(i);

        SERVER_LIST->Insert(str, i + 1);

        SERVER_LIST->SetSelection(i + 1);

//        UserChangedSetting = 1;
    }
}
