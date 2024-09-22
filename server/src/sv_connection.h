// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2024 by The Odamex Team.
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
//     Serverside handling of incoming packets.
//
//-----------------------------------------------------------------------------

#pragma once

class player_s;
typedef player_s player_t;

/**
 * @brief Handle incoming connections.
 */
void SV_ConnectClient();

/**
 * @brief Handle second stage of connection.
 */
void SV_ConnectClient2(player_t& player);

/**
 * @brief Handle incoming packets.
 */
void SV_HandleIncomingPackets();
