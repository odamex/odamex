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
//     Clientside handling of incoming packets.
//
//-----------------------------------------------------------------------------

#pragma once

/**
 * @brief Process server info and switch to the right wads...
 * 
 * @author denis
 * 
 * @return True if the server info was parsed and successfully handled.
 */
bool CL_HandleServerInfo();

/**
 * @brief Handle the initial packet, containing a full update.
 * 
 * @return True if the packet was parsed correctly.
 */
bool CL_HandleFirstPacket();

/**
 * @brief Handle incoming packets.
 */
void CL_HandleIncomingPackets();

/**
 * @brief Send client updates to server.
 */
void CL_UpdateServer();
