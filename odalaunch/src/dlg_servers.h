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
//  AUTHOR:	Russell Rice, John D Corrado
//
//-----------------------------------------------------------------------------


#ifndef DLG_SERVERS_H
#define DLG_SERVERS_H

#include <wx/dialog.h>
#include <wx/intl.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/xrc/xmlres.h>
#include <wx/listctrl.h>
#include <wx/fileconf.h>
#include <wx/checkbox.h>
#include <wx/listbox.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/checklst.h>
#include <wx/textdlg.h>

#include "net_packet.h"

#include <vector>

typedef struct
{
    bool     Enabled;
    wxString Address;
    wxUint16 Port;
} CS_Subst_t;

typedef struct
{
    wxString Address;
    wxUint16 Port;
    
    CS_Subst_t Subst;
} CustomServer_t;

class dlgServers: public wxDialog
{
	public:

		dlgServers(odalpapi::MasterServer *ms, wxWindow* parent, wxWindowID id = -1);
		virtual ~dlgServers();

        CustomServer_t GetCustomServer(wxUint32);

	protected:

        void OnServerList(wxCommandEvent &event);
        void OnSubstChecked(wxCommandEvent &event);

        void OnButtonOK(wxCommandEvent &event);
        void OnButtonClose(wxCommandEvent &event);
        
        void OnButtonAddServer(wxCommandEvent &event);
        void OnButtonReplaceServer(wxCommandEvent &event);
        void OnButtonDeleteServer(wxCommandEvent &event);
        
        void OnButtonMoveServerUp(wxCommandEvent &event);
        void OnButtonMoveServerDown(wxCommandEvent &event);

        void ChkSetValueEx(wxInt32 XrcId, wxCheckBox *CheckBox, bool checked);

        wxFileConfig ConfigInfo;

        wxListBox *SERVER_LIST;
        wxTextCtrl *TEXT_SUBSTITUTE;
        wxCheckBox *CHECK_SUBSTITUTE;
                
		wxButton *ADD_SERVER_BUTTON;
		wxButton *DEL_SERVER_BUTTON;
        wxButton *UP_SERVER_BUTTON;
        wxButton *DOWN_SERVER_BUTTON;

		wxButton *CLOSE_BUTTON;
		wxButton *OK_BUTTON;

        void SaveSettings();   
        void LoadSettings();
        
        void LoadServersIn();

        bool UserChangedSetting;

        odalpapi::MasterServer *MServer;

	private:

		DECLARE_EVENT_TABLE()
};

#endif
