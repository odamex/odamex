// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2021 by The Odamex Team.
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
//  Server window
//
//-----------------------------------------------------------------------------

#include "odalaunch.h"

#include "gui/server_window.h"

#include "FL/fl_draw.H"

/******************************************************************************/

ServerWindow::ServerWindow(int w, int h, const char* title) : Fl_Window(w, h, title)
{
	m_address = new Fl_Input(100, 5, 160, 20);
	m_address->align(FL_ALIGN_LEFT);
	m_address->label("Address");
	m_address->readonly(1);

	m_servername = new Fl_Input(100, 25, 160, 20);
	m_servername->align(FL_ALIGN_LEFT);
	m_servername->label("Server Name");
	m_servername->readonly(1);

	m_gametype = new Fl_Input(100, 45, 160, 20);
	m_gametype->align(FL_ALIGN_LEFT);
	m_gametype->label("Gametype");
	m_gametype->readonly(1);

	m_wads = new Fl_Input(100, 65, 160, 20);
	m_wads->align(FL_ALIGN_LEFT);
	m_wads->label("WADs");
	m_wads->readonly(1);

	m_map = new Fl_Input(100, 85, 160, 20);
	m_map->align(FL_ALIGN_LEFT);
	m_map->label("Map");
	m_map->readonly(1);

	m_players = new Fl_Input(100, 105, 160, 20);
	m_players->align(FL_ALIGN_LEFT);
	m_players->label("Players");
	m_players->readonly(1);

	m_playerTable = new ServerPlayerTable{5, 125, 320, 100};
	m_playerTable->end();

	m_cvarTable = new ServerCvarTable{5, 225, 320, 100};
	m_cvarTable->end();
}

/******************************************************************************/

void ServerWindow::setAddress(const std::string& strAddress)
{
	m_strAddress = strAddress;
	m_playerTable->setAddress(strAddress);
	m_cvarTable->setAddress(strAddress);
}

/******************************************************************************/

void ServerWindow::refreshInfo()
{
	serverRow_s server;
	DB_GetServer(server, m_strAddress);

	m_address->value(m_strAddress.c_str());
	m_servername->value(server.servername.c_str());
	m_gametype->value(server.gametype.c_str());
	m_wads->value(server.Wads().c_str());
	m_map->value(server.map.c_str());
	m_players->value(server.players.c_str());

	m_playerTable->refreshInfo();
	m_cvarTable->refreshInfo();
}
