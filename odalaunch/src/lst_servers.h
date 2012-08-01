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
//  Server list control class
//
//-----------------------------------------------------------------------------

#ifndef __LST_SERVERS_H__
#define __LST_SERVERS_H__

#include "net_packet.h"
#include "lst_custom.h"

typedef enum
{
    serverlist_field_attr
    ,serverlist_field_name
    ,serverlist_field_ping
    ,serverlist_field_players
    ,serverlist_field_wads
    ,serverlist_field_map
    ,serverlist_field_type
    ,serverlist_field_iwad
    ,serverlist_field_address
    
    ,max_serverlist_fields
} serverlist_fields_t;

class LstOdaServerList : public wxAdvancedListCtrl
{
    public:
        LstOdaServerList();
        virtual ~LstOdaServerList();

        void AddServerToList(const odalpapi::Server &s, wxInt32 index, bool insert = true);

    protected:
        
        void ClearItemCells(long item);
        
        void OnOpenContextMenu(wxContextMenuEvent& event);

        void OnCopyAddress(wxCommandEvent& event);

        void SetupServerListColumns();

        void OnCreateControl(wxWindowCreateEvent &event);

        DECLARE_DYNAMIC_CLASS(LstOdaServerList)

    private:

        wxMenu *m_mnuPopup;

        DECLARE_EVENT_TABLE()
};

#endif // __LST_SERVERS_H__
