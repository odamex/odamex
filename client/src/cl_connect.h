// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
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
//  Serverside connection sequence.
//
//-----------------------------------------------------------------------------

#pragma once

enum netQuitReason_e
{
	NQ_SILENT,     // Don't print a message.
	NQ_DISCONNECT, // Generic message for "typical" forced disconnects initiated by the
	               // client.
	NQ_ABORT,      // Connection attempt was aborted
	NQ_PROTO,      // Encountered something unexpected in the protocol
};

/**
 * @brief Do what a launcher does...
 * @author denis
 */
void CL_RequestConnectInfo();

/**
 * @brief Process server info and switch to the right wads...
 * @author denis
 */
bool CL_PrepareConnect();

/**
 * @brief Connecting to a server...
 */
bool CL_Connect();

/**
 * @brief Tick connection sequence.
 */
void CL_TickConnecting();

/**
 * @brief Reconnect to the server.
 */
void CL_Reconnect();

/**
 * @brief Disconnect from the server.
 */
#define CL_QuitNetGame(reason) CL_QuitNetGame2(reason, __FILE__, __LINE__)
void CL_QuitNetGame2(const netQuitReason_e reason, const char* file, const int line);

/**
 * @brief Quit the network game while attempting to download a file.
 *
 * @param missing_file Missing file to attempt to download.
 */
void CL_QuitAndTryDownload(const OWantFile& missing_file);
