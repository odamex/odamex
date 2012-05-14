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

#ifndef __DLG_ABOUT_H__
#define __DLG_ABOUT_H__

#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>

class dlgAbout : public wxDialog
{
    public:
    
        dlgAbout(wxWindow* parent, wxWindowID id = -1);
		virtual ~dlgAbout() { }
    
    protected:
        
        void OnTxtCtrlUrlClick(wxTextUrlEvent &event);
        
        wxStaticText *m_StcTxtCopyright;
        wxStaticText *m_StcTxtVersion;
        wxStaticText *m_StcTxtWxVer;
        wxTextCtrl *m_TxtCtrlDevelopers;
        
	private:

		DECLARE_EVENT_TABLE()
};

#endif // __DLG_ABOUT_H__
