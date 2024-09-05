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

#pragma once

#include "server_cvar_table.h"
#include "server_player_table.h"

#include "FL/Fl_Input.H"
#include "FL/Fl_Window.H"

class ServerWindow final : public Fl_Window
{
protected:
	std::string m_strAddress;
	Fl_Input* m_address = nullptr;
	Fl_Input* m_servername = nullptr;
	Fl_Input* m_gametype = nullptr;
	Fl_Input* m_wads = nullptr;
	Fl_Input* m_map = nullptr;
	Fl_Input* m_players = nullptr;
	ServerPlayerTable* m_playerTable = nullptr;
	ServerCvarTable* m_cvarTable = nullptr;

public:
	ServerWindow() : ServerWindow(640, 480, "Server Info") { }
	ServerWindow(int w, int h, const char* title = 0);

	/**
	 * @brief Set the address of the server window.
	 */
	void setAddress(const std::string& strAddress);

	/**
	 * @brief Refresh all data associated with this server.
	 */
	void refreshInfo();
};
