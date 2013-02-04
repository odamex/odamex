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
//  Player list control class
//
//-----------------------------------------------------------------------------

#ifndef __LST_PLAYERS_H__
#define __LST_PLAYERS_H__

#include "net_packet.h"
#include "lst_custom.h"

class LstOdaPlayerList : public wxAdvancedListCtrl
{
    public:
        LstOdaPlayerList() { };
        virtual ~LstOdaPlayerList();

        void AddPlayersToList(const odalpapi::Server &s);

    protected:

        void SetupPlayerListColumns();        
        void OnCreateControl(wxWindowCreateEvent &event);
        
        DECLARE_DYNAMIC_CLASS(LstOdaPlayerList)

    private:
        DECLARE_EVENT_TABLE()
};

#endif // __LST_PLAYERS_H__
