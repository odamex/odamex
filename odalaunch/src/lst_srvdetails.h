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
//   List control for handling extra server details 
//
//-----------------------------------------------------------------------------

#ifndef __LST_SRVDETAILS_H__
#define __LST_SRVDETAILS_H__

#include "net_packet.h"

#include <wx/listctrl.h>

class LstOdaSrvDetails : public wxListCtrl
{
    public:
        LstOdaSrvDetails();
        virtual ~LstOdaSrvDetails() { };
        
        void LoadDetailsFromServer(odalpapi::Server &);
        
        //wxEvent *Clone(void);
    protected:
        void ResizeNameValueColumns();

        void InsertLine(const wxString &Name, const wxString &Value);
        void InsertHeader(const wxString &Name, 
                          wxColor NameColor = wxNullColour,
                          wxColor NameBGColor = wxNullColour);
        
        wxColour BGItemAlternator;
        
        wxColour ItemShade;
        wxColour BgColor;
        
        wxColour Header;
        wxColour HeaderText;
        
        DECLARE_DYNAMIC_CLASS(LstOdaSrvDetails)
};

#endif // __LST_SRVDETAILS_H__
