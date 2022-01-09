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
//  Server database management.
//
//-----------------------------------------------------------------------------

#pragma once

#include "odalaunch.h"

#include <string>
#include <vector>

struct serverRow_t
{
	std::string address;
	std::string servername;
	std::string gametype;
	std::string wads;
	std::string map;
	std::string players;
	std::string ping;
};
typedef std::vector<serverRow_t> serverRows_t;

bool DB_Init();
void DB_AddServer(const std::string& address, const uint16_t port);
void DB_GetServerList(serverRows_t& rows);
void DB_DeInit();
