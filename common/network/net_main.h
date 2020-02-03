// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2015 by The Odamex Team.
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
//  
// Global Network Functions 
//
//-----------------------------------------------------------------------------

#ifndef __NET_MAIN_H__
#define __NET_MAIN_H__

#include "network/net_socketaddress.h"
#include "network/net_interface.h"

void Net_InitNetworkInterface(uint16_t desired_port);
void Net_CloseNetworkInterface();
NetInterface* Net_GetNetworkInterface();
void Net_ServiceConnections();

void Net_OpenConnection(const std::string& address_string);
void Net_ReOpenConnection();
void Net_CloseConnection();
bool Net_IsConnected();

#endif	// __NET_MAIN_H__