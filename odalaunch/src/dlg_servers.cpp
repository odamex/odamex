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

static wxInt32 ID_BTNCLOSE = XRCID("ID_BTNCLOSE");
static wxInt32 ID_BTNOK = XRCID("ID_BTNOK");

// Event table for widgets
BEGIN_EVENT_TABLE(dlgServers,wxDialog)
	// Window events
//	EVT_CLOSE(dlgConfig::OnCloseWindow)
//	EVT_BUTTON(ID_BTNOK, dlgServers::OnOK)
//	EVT_BUTTON(ID_BTNCLOSE, dlgServers::OnClose)

	// Button events
//  EVT_BUTTON(ID_BTNSRVADD, dlgConfig::OnAddServer)
//  EVT_BUTTON(ID_BTNSRVDEL, dlgConfig::OnDelServer)
//  EVT_BUTTON(ID_BTNSRVUP, dlgConfig::OnUpServer)
//  EVT_BUTTON(ID_BTNSRVDWN, dlgConfig::OnDownServer) 

	// Misc events
//	EVT_CHECKBOX(ID_CHKSUBST, dlgConfig::OnSubstChecked)
END_EVENT_TABLE()

// Window constructor
dlgServers::dlgServers(wxWindow *parent, wxWindowID id)
{
    // Set up the dialog and its widgets
    wxXmlResource::Get()->LoadDialog(this, parent, _T("dlgServers"));

//    MASTER_CHECKBOX = wxStaticCast((*this).FindWindow(ID_CHKLISTONSTART), wxCheckBox);
//    BLOCKED_CHECKBOX = wxStaticCast((*this).FindWindow(ID_CHKSHOWBLOCKEDSERVERS), wxCheckBox);

//    ADD_BUTTON = wxStaticCast((*this).FindWindow(ID_BTNADD),wxButton);
//    REPLACE_BUTTON = wxStaticCast((*this).FindWindow(ID_BTNREPLACE),wxButton);
//    DELETE_BUTTON = wxStaticCast((*this).FindWindow(ID_BTNDELETE),wxButton);
//    CHOOSEDIR_BUTTON = wxStaticCast((*this).FindWindow(ID_BTNCHOOSEDIR),wxButton);
//    UP_BUTTON = wxStaticCast((*this).FindWindow(ID_BTNUP),wxButton);
//    DOWN_BUTTON = wxStaticCast((*this).FindWindow(ID_BTNDOWN),wxButton);

//    GETENV_BUTTON = wxStaticCast((*this).FindWindow(ID_BTNGETENV),wxButton);

//    CLOSE_BUTTON = wxStaticCast((*this).FindWindow(ID_BTNCLOSE),wxButton);
//    OK_BUTTON = wxStaticCast((*this).FindWindow(ID_BTNOK),wxButton);

//    WAD_LIST = wxStaticCast((*this).FindWindow(ID_LSTWADDIR),wxListBox);

//    DIR_BOX = wxStaticCast((*this).FindWindow(ID_TXTWADDIR),wxTextCtrl); 
}

// Window destructor
dlgServers::~dlgServers()
{

}

