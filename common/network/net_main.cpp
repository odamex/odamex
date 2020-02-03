
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

#include "i_system.h"

#include "network/net_log.h"
#include "network/net_interface.h"
#include "network/net_connectionmanager.h"

static const uint16_t DEFAULT_SERVER_PORT = 10666;
static const uint16_t DEFAULT_CLIENT_PORT = 10667;

static NetInterface network_interface;
static ConnectionManager* connection_manager;
static SocketAddress remote_address;
static ConnectionId client_connection_id;


//
// Net_InitNetworkInterface
//
void Net_InitNetworkInterface(uint16_t desired_port)
{
    if (desired_port == 0)
    #ifdef CLIENT_APP
        desired_port = DEFAULT_CLIENT_PORT;
    #else
        desired_port = DEFAULT_SERVER_PORT;
    #endif

    std::string host("0.0.0.0");        // bind to all interfaces
    if (!network_interface.openInterface(host, desired_port))
		I_FatalError("Unable to initialize network interface");

    connection_manager = new ConnectionManager(&network_interface, 256);
    remote_address.clear();
}


//
// Net_CloseNetworkInterface
//
void Net_CloseNetworkInterface()
{
    remote_address.clear();
    delete connection_manager;
    connection_manager = NULL;

	if (!network_interface.closeInterface())
		I_FatalError("Unable to de-initialize network interface");
}


//
// Net_GetNetworkInterface
//
NetInterface Net_GetNetworkInterface()
{
    return network_interface;
}


//
// Net_ServiceConnections
//
void Net_ServiceConnections()
{
    connection_manager->serviceConnections();
}


//
// Net_OpenConnection
//
// Opens a new connection to the remote host given by socket address string.
// This should only be used by the client application.
// Note that this requires CS_OpenNetInterface to be called prior.
//
void Net_OpenConnection(const std::string& address_string)
{
    assert(connection_manager != NULL);
    assert(network_interface.getHostType() == HOST_CLIENT);

    remote_address = SocketAddress(address_string);
    if (remote_address.getPort() == 0)
        remote_address.setPort(DEFAULT_SERVER_PORT);
    if (remote_address.getIPAddress() == 0)
        Net_Warning("Net_OpenConnection: could not resolve host %s", address_string.c_str());
    else
        client_connection_id = connection_manager->requestConnection(remote_address);
}


//
// Net_CloseConnection
//
// Closes the connection to the remote host.
// This should only be used by the client application.
//
void Net_CloseConnection()
{
    assert(connection_manager != NULL);
    assert(network_interface.getHostType() == HOST_CLIENT);

    if (connection_manager->isConnected(client_connection_id))
        connection_manager->closeConnection(client_connection_id);
}


//
// Net_ReOpenConnection
//
void Net_ReOpenConnection()
{
    assert(connection_manager != NULL);
    assert(network_interface.getHostType() == HOST_CLIENT);

    Net_CloseConnection();
    Net_OpenConnection(remote_address.getString());
}


//
// Net_IsConnected
//
// Returns true if the client application is currently connected to a remote
// server.
//
bool Net_IsConnected()
{
    assert(connection_manager != NULL);
    assert(network_interface.getHostType() == HOST_CLIENT);

    return connection_manager->isConnected(client_connection_id);
}