// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: dlg_config.cpp 1648 2010-07-11 02:50:26Z russellrice $
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
//	About Dialog
//
//-----------------------------------------------------------------------------

#include "dlg_about.h"

#include "net_packet.h"

#include <wx/event.h>
#include <wx/utils.h>
#include <wx/version.h>
#include <wx/xrc/xmlres.h>

#define _ODA_COPYRIGHT_ "Copyright (C) 2006-2013 The Odamex Team"

BEGIN_EVENT_TABLE(dlgAbout, wxDialog)
    EVT_TEXT_URL(XRCID("Id_TxtCtrlDevelopers"), dlgAbout::OnTxtCtrlUrlClick)
END_EVENT_TABLE()

dlgAbout::dlgAbout(wxWindow* parent, wxWindowID id)
{
    wxString Text, Version, wxWidgetsVersion;

    wxXmlResource::Get()->LoadDialog(this, parent, _T("dlgAbout"));

    m_StcTxtCopyright = XRCCTRL(*this, "Id_StcTxtCopyright",
        wxStaticText);

    m_StcTxtVersion = XRCCTRL(*this, "Id_StcTxtVersion",
        wxStaticText);

    m_StcTxtWxVer = XRCCTRL(*this, "Id_StcTxtWxVer", wxStaticText);

    m_TxtCtrlDevelopers = XRCCTRL(*this, "Id_TxtCtrlDevelopers",
        wxTextCtrl);

    // wxWidgets Bug: wxTE_AUTO_URL appears to get set AFTER SetValue() has been
    // called, this causes urls to not get recognized (msw XRC handler problem?)
    Text = m_TxtCtrlDevelopers->GetValue();
    m_TxtCtrlDevelopers->SetValue(wxT(""));
    m_TxtCtrlDevelopers->WriteText(Text);

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

// wxTextCtrl doesn't provide a handler for urls, so we use an almost
// undocumented event handler provided by wx
void dlgAbout::OnTxtCtrlUrlClick(wxTextUrlEvent &event)
{
    wxString URL;
    wxTextCtrl *Control;
    wxMouseEvent MouseEvent;

    MouseEvent = event.GetMouseEvent();

    if (MouseEvent.LeftDown())
    {
        Control = wxDynamicCast(event.GetEventObject(), wxTextCtrl);

        URL = Control->GetRange(event.GetURLStart(), event.GetURLEnd());

        wxLaunchDefaultBrowser(URL);

        event.Skip();
    }
}
