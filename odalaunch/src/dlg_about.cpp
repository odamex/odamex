// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: dlg_config.cpp 1648 2010-07-11 02:50:26Z russellrice $
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
//	About Dialog
//
//-----------------------------------------------------------------------------

#include "dlg_about.h"

#include "net_packet.h"

#include <wx/xrc/xmlres.h>
#include <wx/version.h> 

#define _ODA_COPYRIGHT_ "Copyright (C) 2006-2010 by The Odamex Team."

static wxInt32 Id_StcTxtCopyright = XRCID("Id_StcTxtCopyright");
static wxInt32 Id_StcTxtVersion = XRCID("Id_StcTxtVersion");
static wxInt32 Id_TxtCtrlDevelopers = XRCID("Id_TxtCtrlDevelopers");
static wxInt32 Id_StcTxtWxVer = XRCID("Id_StcTxtWxVer");

dlgAbout::dlgAbout(wxWindow* parent, wxWindowID id)
{
    wxString Version, wxWidgetsVersion;
    
    wxXmlResource::Get()->LoadDialog(this, parent, _T("dlgAbout"));
    
    m_StcTxtCopyright = wxDynamicCast(FindWindow(Id_StcTxtCopyright), 
        wxStaticText);
    
    m_StcTxtVersion = wxDynamicCast(FindWindow(Id_StcTxtVersion), 
        wxStaticText);
    
    m_StcTxtWxVer = wxDynamicCast(FindWindow(Id_StcTxtWxVer), wxStaticText);
        
    m_TxtCtrlDevelopers = wxDynamicCast(FindWindow(Id_TxtCtrlDevelopers), 
        wxTextCtrl);
       
    // Set (protocol) version info on desired text control
    Version = wxString::Format(
        wxT("Version %d.%d.%d - Protocol Version %d"), 
        VERSIONMAJOR(VERSION), VERSIONMINOR(VERSION), VERSIONPATCH(VERSION), 
        PROTOCOL_VERSION);
        
    m_StcTxtVersion->SetLabel(Version);
    
    // Set the copyright and year
    m_StcTxtCopyright->SetLabel(wxT(_ODA_COPYRIGHT_));
    
    // Set launcher built version
    wxWidgetsVersion = wxString::Format(
        wxT(", Version %d.%d.%d-%d"), 
        wxMAJOR_VERSION, wxMINOR_VERSION, wxRELEASE_NUMBER, 
        wxSUBRELEASE_NUMBER);
    
    m_StcTxtWxVer->SetLabel(wxWidgetsVersion);
    
}
