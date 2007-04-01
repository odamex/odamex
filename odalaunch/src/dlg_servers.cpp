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
//	EVT_CHECKBOX(ID_CHKSUBST, dlgConfig::OnSubstChecked)
END_EVENT_TABLE()

// Window constructor
dlgServers::dlgServers(wxWindow *parent, wxWindowID id)
{
    // Set up the dialog and its widgets
    wxXmlResource::Get()->LoadDialog(this, parent, _T("dlgServers"));

//    WAD_LIST = wxStaticCast((*this).FindWindow(ID_LSTWADDIR),wxListBox);

//    DIR_BOX = wxStaticCast((*this).FindWindow(ID_TXTWADDIR),wxTextCtrl); 
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

// Add Server button
void dlgServers::OnButtonAddServer(wxCommandEvent &event)
{
    
}

// Delete Server button
void dlgServers::OnButtonDelServer(wxCommandEvent &event)
{
    
}

// Move Server Up button
void dlgServers::OnButtonMoveServerUp(wxCommandEvent &event)
{
    
}

// Move Server Down button
void dlgServers::OnButtonMoveServerDown(wxCommandEvent &event)
{
    
}
