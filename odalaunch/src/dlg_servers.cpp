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
//	Custom Servers dialog
//	AUTHOR:	Russell Rice, John D Corrado
//
//-----------------------------------------------------------------------------

#include <wx/settings.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/msgdlg.h>
#include <wx/wfstream.h>
#include <wx/tokenzr.h>
#include <wx/dirdlg.h>

#include "dlg_servers.h"
#include "str_utils.h"

using namespace odalpapi;

// Widget ID's
static wxInt32 ID_BTNADDSERVER = XRCID("ID_BTNADDSERVER");
static wxInt32 ID_BTNREPLACESERVER = XRCID("ID_BTNREPLACESERVER");
static wxInt32 ID_BTNDELETESERVER = XRCID("ID_BTNDELETESERVER");

static wxInt32 ID_BTNMOVEUP = XRCID("ID_MOVEUP");
static wxInt32 ID_BTNMOVEDOWN = XRCID("ID_MOVEDOWN");

// Event table for widgets
BEGIN_EVENT_TABLE(dlgServers,wxDialog)
	// Button events
    EVT_BUTTON(ID_BTNADDSERVER, dlgServers::OnButtonAddServer)
    EVT_BUTTON(ID_BTNREPLACESERVER, dlgServers::OnButtonReplaceServer)
    EVT_BUTTON(ID_BTNDELETESERVER, dlgServers::OnButtonDeleteServer)

    EVT_BUTTON(ID_BTNMOVEUP, dlgServers::OnButtonMoveServerUp)
    EVT_BUTTON(ID_BTNMOVEDOWN, dlgServers::OnButtonMoveServerDown)

    EVT_BUTTON(wxID_OK, dlgServers::OnButtonOK)

	// Misc events
	EVT_CHECKBOX(XRCID("ID_CHKSUBSTITUTE"), dlgServers::OnSubstChecked)

	EVT_LISTBOX(XRCID("ID_SERVERLIST"), dlgServers::OnServerList)
END_EVENT_TABLE()

// Window constructor
dlgServers::dlgServers(MasterServer *ms, wxWindow *parent, wxWindowID id)
{
    // Set up the dialog and its widgets
    wxXmlResource::Get()->LoadDialog(this, parent, _T("dlgServers"));

    SERVER_LIST = XRCCTRL(*this, "ID_SERVERLIST", wxListBox);
    CHECK_SUBSTITUTE = XRCCTRL(*this, "ID_CHKSUBSTITUTE", wxCheckBox);
    TEXT_SUBSTITUTE = XRCCTRL(*this, "ID_TXTSUBIPPORT", wxTextCtrl);

    MServer = ms;

    LoadSettings();

    LoadServersIn();
}

// Window destructor
dlgServers::~dlgServers()
{
    // clean up client data.
    if (SERVER_LIST->GetCount())
    for (size_t i = 0; i < SERVER_LIST->GetCount(); i++)
    {
        CustomServer_t *cs = (CustomServer_t *)SERVER_LIST->GetClientData(i);

        delete cs;
    }
}

// Triggers a wxEVT_COMMAND_CHECKBOX_CLICKED event when used. wxWidgets doesn't
// do this by default (stupid)
void dlgServers::ChkSetValueEx(wxInt32 XrcId, wxCheckBox *CheckBox, bool checked)
{
    CheckBox->SetValue(checked);

    wxCommandEvent Event(wxEVT_COMMAND_CHECKBOX_CLICKED, XrcId );
    wxPostEvent(this, Event);
}

// OK button
void dlgServers::OnButtonOK(wxCommandEvent &event)
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

        LoadServersIn();

        SaveSettings();
    }
    else
        UserChangedSetting = false;

    Close();
}

// Close button
void dlgServers::OnButtonClose(wxCommandEvent &event)
{
    Close();
}

// User clicks on a server address in the list
void dlgServers::OnServerList(wxCommandEvent &event)
{
    wxInt32 i = SERVER_LIST->GetSelection();

    if (i == wxNOT_FOUND)
         return;

    // get values from stored data
    CustomServer_t *cs = (CustomServer_t *)SERVER_LIST->GetClientData(i);

//    ChkSetValueEx(ID_CHKSUBSTITUTE, CHECK_SUBSTITUTE, cs->Subst.Enabled);
//    TEXT_SUBSTITUTE->SetLabel(cs->Subst.Address);
}

void dlgServers::OnSubstChecked(wxCommandEvent &event)
{
//    TEXT_SUBSTITUTE->Enable(CHECK_SUBSTITUTE->IsChecked());
}

// Add Server button
void dlgServers::OnButtonAddServer(wxCommandEvent &event)
{
    wxString tedaddr_res;
    wxUint16 tedport_res;

    wxTextEntryDialog tedAddress(this,
                            _T("Please enter an IP Address"),
                            _T("Please enter an IP Address"),
                            _T("127.0.0.1"));

    wxTextEntryDialog tedPort(this,
                            _T("Please enter a Port number"),
                            _T("Please enter a Port number"),
                            _T("10666"));

    // Show it
    tedAddress.ShowModal();
    tedaddr_res = tedAddress.GetValue();

    tedPort.ShowModal();
    tedport_res = wxAtoi(tedPort.GetValue().c_str());

    // Make a 0.0.0.0:0 address string
    wxString addr_portfmt = wxString::Format(_T("%s:%d"),
                                             tedaddr_res.c_str(),
                                             tedport_res);

    if (!tedaddr_res.IsEmpty() &&
        tedport_res != 0 &&
        SERVER_LIST->FindString(addr_portfmt) == wxNOT_FOUND)
    {
        CustomServer_t *cs = new CustomServer_t;

        cs->Address = tedaddr_res;
        cs->Port = tedport_res;

        SERVER_LIST->Append(addr_portfmt, (void *)(cs));

        UserChangedSetting = true;
    }
}

void dlgServers::OnButtonReplaceServer(wxCommandEvent &event)
{
    wxString tedaddr_res;
    wxUint16 tedport_res;

    wxInt32 i = SERVER_LIST->GetSelection();

    if (i == wxNOT_FOUND)
    {
         wxMessageBox(_T("Select an item to replace!"));
         return;
    }

    CustomServer_t *cs = (CustomServer_t *)SERVER_LIST->GetClientData(i);

    wxTextEntryDialog tedAddress(this,
                            _T("Please enter an IP Address"),
                            _T("Please enter an IP Address"),
                            cs->Address);

    wxTextEntryDialog tedPort(this,
                            _T("Please enter a Port number"),
                            _T("Please enter a Port number"),
                            wxString::Format(_T("%d"), cs->Port));

    // Show it
    tedAddress.ShowModal();
    tedaddr_res = tedAddress.GetValue();

    tedPort.ShowModal();
    tedport_res = wxAtoi(tedPort.GetValue().c_str());

    // Make a 0.0.0.0:0 address string
    wxString addr_portfmt = wxString::Format(_T("%s:%d"),
                                             tedaddr_res.c_str(),
                                             tedport_res);

    if (!tedaddr_res.IsEmpty() && tedport_res != 0)
    {
        SERVER_LIST->SetString(i, addr_portfmt);
        SERVER_LIST->SetClientData(i, (void *)(cs));

        UserChangedSetting = true;
    }
}

// Delete Server button
void dlgServers::OnButtonDeleteServer(wxCommandEvent &event)
{
    wxInt32 i = SERVER_LIST->GetSelection();

    if (i != wxNOT_FOUND)
    {
        CustomServer_t *cs = (CustomServer_t *)SERVER_LIST->GetClientData(i);

        delete cs;

        SERVER_LIST->Delete(i);

        UserChangedSetting = true;
    }
}

// Move Server Up button
void dlgServers::OnButtonMoveServerUp(wxCommandEvent &event)
{
    // Get the selected item
    wxInt32 i = SERVER_LIST->GetSelection();

    if ((i != wxNOT_FOUND) && (i > 0))
    {
        void *cd = SERVER_LIST->GetClientData(i);
        wxString str = SERVER_LIST->GetString(i);

        SERVER_LIST->Delete(i);

        SERVER_LIST->Insert(str, i - 1, cd);

        SERVER_LIST->SetSelection(i - 1);

        UserChangedSetting = true;
    }
}

// Move Server Down button
void dlgServers::OnButtonMoveServerDown(wxCommandEvent &event)
{
    // Get the selected item
    wxInt32 i = SERVER_LIST->GetSelection();

    if ((i != wxNOT_FOUND) && (i + 1 < SERVER_LIST->GetCount()))
    {
        void *cd = SERVER_LIST->GetClientData(i);
        wxString str = SERVER_LIST->GetString(i);

        SERVER_LIST->Delete(i);

        SERVER_LIST->Insert(str, i + 1, cd);

        SERVER_LIST->SetSelection(i + 1);

        UserChangedSetting = true;
    }
}

void dlgServers::SaveSettings()
{
    wxFileConfig fc;

    fc.SetPath(_T("/CustomServers"));
    fc.Write(_T("NumberOfServers"), (wxInt32)SERVER_LIST->GetCount());

    for (wxInt32 i = 0; i < SERVER_LIST->GetCount(); i++)
    {
        // Jump to/create path for current server to be written
        fc.SetPath(wxString::Format(_T("%d"), i));

        CustomServer_t *cs = (CustomServer_t *)SERVER_LIST->GetClientData(i);

        fc.Write(_T("Address"), cs->Address);
        fc.Write(_T("Port"), cs->Port);

        fc.SetPath(_T("Substitute"));

        fc.Write(_T("Enabled"), cs->Subst.Enabled);
        fc.Write(_T("Address"), cs->Subst.Address);
        fc.Write(_T("Port"), cs->Subst.Port);

        fc.SetPath(_T("../"));

        // Traverse back down to "/CustomServers"
        fc.SetPath(_T("../"));
    }

    // Save settings to configuration file
    fc.Flush();
}

void dlgServers::LoadSettings()
{
    wxFileConfig fc;

    fc.SetPath(_T("/CustomServers"));

    wxInt32 NumberOfServers = fc.Read(_T("NumberOfServers"), 0L);

    for (wxInt32 i = 0; i < NumberOfServers; i++)
    {
        // Jump to/create path for current server to be written
        fc.SetPath(wxString::Format(_T("%d"), i));

        CustomServer_t *cs = new CustomServer_t;

        cs->Address = fc.Read(_T("Address"), _T(""));
        cs->Port = fc.Read(_T("Port"), 0L);

        fc.SetPath(_T("Substitute"));

        fc.Read(_T("Enabled"), cs->Subst.Enabled);
        cs->Subst.Address = fc.Read(_T("Address"), _T(""));
        cs->Subst.Port = fc.Read(_T("Port"), 0L);

        SERVER_LIST->Append(wxString::Format(_T("%s:%d"),
                                             cs->Address.c_str(),
                                             cs->Port), (void *)(cs));

        fc.SetPath(_T("../"));

        // Traverse back down to "/CustomServers"
        fc.SetPath(_T("../"));
    }
}

void dlgServers::LoadServersIn()
{
    MServer->DeleteAllCustomServers();

    if (SERVER_LIST->GetCount())
    for (size_t i = 0; i < SERVER_LIST->GetCount(); i++)
    {
        CustomServer_t *cs = (CustomServer_t *)SERVER_LIST->GetClientData(i);

        MServer->AddCustomServer(wxstr_tostdstr(cs->Address), cs->Port);
    }
}
