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
// Connection establishment and management	
//
//-----------------------------------------------------------------------------

#include "network/net_log.h"
#include "network/net_connectionmanager.h"
#include "network/net_interface.h"
#include "network/net_connection.h"

static uint32_t MAX_INCOMING_SIZE_IN_BYTES = 8192;
static uint32_t MAX_OUTGOING_SIZE_IN_BYTES = 8192;

//
// ConnectionManager::ConnectionManager
//
ConnectionManager::ConnectionManager(NetInterface* network_interface, uint16_t max_connections) :
    mNetworkInterface(network_interface),
    mMaxConnections(std::min<uint16_t>(max_connections, MAX_CONNECTIONS)),
    mInitialized(true),
	mConnectionsByAddress(2 * mMaxConnections),
	mConnectionsById(2 * mMaxConnections)
{
}


//
// ConnectionManager::~ConnectionManager
//
ConnectionManager::~ConnectionManager()
{
	closeAllConnections();
}


//
// ConnectionManager::requestConnection
//
// Creates a new Connection object that will request a connection
// from the host at remote_address. Returns the ID of the connection.
//
ConnectionId ConnectionManager::requestConnection(const SocketAddress& remote_address)
{
	Net_LogPrintf(LogChan_Interface, "requesting connection to host %s.", remote_address.getCString());

	Connection* conn = createNewConnection(remote_address);
	conn->openConnection();

	return conn->getConnectionId();
}


//
// ConnectionManager::closeConnection
//
// 
//
void ConnectionManager::closeConnection(const ConnectionId& connection_id)
{
	Connection* conn = findConnection(connection_id);
	if (conn == NULL)
		return;	

	Net_LogPrintf(LogChan_Interface, "closing connection to host %s.", conn->getRemoteAddress().getCString());
	conn->closeConnection();
}


//
// ConnectionManager::closeAllConnections
//
// Iterates through all Connection objects and frees them, updating the
// connection hash tables and freeing all used connection IDs.
//
void ConnectionManager::closeAllConnections()
{
	// free all Connection objects
	ConnectionIdTable::const_iterator it;
	for (it = mConnectionsById.begin(); it != mConnectionsById.end(); ++it)
	{
		Connection* conn = it->second;
		conn->closeConnection();
		delete conn;
	}

	mConnectionsById.clear();
	mConnectionsByAddress.clear();
}


//
// ConnectionManager::isConnected
//
// Returns true if there is a connection that matches connection_id and is
// in a healthy state (not timed-out).
//
bool ConnectionManager::isConnected(const ConnectionId& connection_id) const
{
	const Connection* conn = findConnection(connection_id);
	if (conn != NULL)
		return conn->isConnected();
	return false;
}


//
// ConnectionManager::findConnection
//
// Returns a pointer to the Connection object with the connection ID
// matching connection_id, or returns NULL if it was not found.
//
Connection* ConnectionManager::findConnection(const ConnectionId& connection_id) 
{
	ConnectionIdTable::const_iterator it = mConnectionsById.find(connection_id);
	if (it != mConnectionsById.end())
		return it->second;
	return NULL;
}

const Connection* ConnectionManager::findConnection(const ConnectionId& connection_id) const
{
	ConnectionIdTable::const_iterator it = mConnectionsById.find(connection_id);
	if (it != mConnectionsById.end())
		return it->second;
	return NULL;
}


//
// ConnectionManager::serviceConnections
//
// Checks for incoming packets, creating new connections as appropriate, or
// calling an individual connection's service function,
//
void ConnectionManager::serviceConnections()
{
	// check for incoming packets
	uint8_t buf[MAX_INCOMING_SIZE_IN_BYTES];
	uint16_t size = 0;
	SocketAddress source;

    while (mNetworkInterface->socketRecv(source, buf, size))
	{
		Packet packet = PacketFactory::createPacket(buf, BytesToBits(size));
		processPacket(source, packet);
	}

	// iterate through all of the connections and call their service function
	ConnectionIdTable::const_iterator it;
	for (it = mConnectionsById.begin(); it != mConnectionsById.end(); ++it)
	{
		Connection* conn = it->second;
		conn->service();

		// close timed-out connections
		// they will be freed later after a time-out period
		if (!conn->isTerminated() && conn->isTimedOut())
		{
			Net_LogPrintf(LogChan_Interface, "connection to host %s timed-out.",
					conn->getRemoteAddress().getCString());
			conn->closeConnection();
		}
	}

	// iterate through the connections and delete any that are in the
	// termination state and have finished their time-out period.
	it = mConnectionsById.begin();
	while (it != mConnectionsById.end())
	{
		bool reiterate = false;

		Connection* conn = it->second;
		if (conn->isTerminated() && conn->isTimedOut())
		{
			Net_LogPrintf(LogChan_Interface, "removing terminated connection to host %s.", 
					conn->getRemoteAddress().getCString());
			mConnectionsById.erase(conn->getConnectionId());
			mConnectionsByAddress.erase(conn->getRemoteAddress());
			delete conn;
			reiterate = true;
		}

		// if we remove any items from the hash table while iterating,
		// the iterator becomes invalid and we have to start over.
		if (reiterate)
			it = mConnectionsById.begin();
		else
			++it;
	}
}


//
// ConnectionManager::findConnection
//
// Returns a pointer to the Connection object with the remote socket address
// matching remote_address, or returns NULL if it was not found.
//
Connection* ConnectionManager::findConnection(const SocketAddress& remote_address)
{
	ConnectionAddressTable::const_iterator it = mConnectionsByAddress.find(remote_address);
	if (it != mConnectionsByAddress.end())
		return it->second;	
	return NULL;
}

const Connection* ConnectionManager::findConnection(const SocketAddress& remote_address) const
{
	ConnectionAddressTable::const_iterator it = mConnectionsByAddress.find(remote_address);
	if (it != mConnectionsByAddress.end())
		return it->second;	
	return NULL;
}


//
// ConnectionManager::createNewConnection
//
// Instantiates a new Connection object with the supplied connection ID
// and socket address. The Connection object is added to the ID and address
// lookup hash tables and a pointer to the object is returned.
//
Connection* ConnectionManager::createNewConnection(const SocketAddress& remote_address)
{
	ConnectionId connection_id = mConnectionIdGenerator.generate();

	Connection* conn = new Connection(connection_id, mNetworkInterface, remote_address);
	mConnectionsById.insert(std::pair<ConnectionId, Connection*>(connection_id, conn));
	mConnectionsByAddress.insert(std::pair<SocketAddress, Connection*>(remote_address, conn));
	return conn;
}


//
// ConnectionManager::processPacket
//
// Handles the processing and delivery of incoming packets.
//
void ConnectionManager::processPacket(const SocketAddress& remote_address, Packet& packet)
{
	Connection* conn = findConnection(remote_address);
	if (conn != NULL)
	{
		conn->processPacket(packet);
	}
	else
	{
		if (mNetworkInterface->getHostType() == HOST_SERVER)
		{
			// packet from an unknown host
			Net_LogPrintf(LogChan_Interface, "received packet from an unknown host %s.", remote_address.getCString());

            if (mConnectionsByAddress.size() >= mMaxConnections)
                Net_LogPrintf(LogChan_Interface, "cannot create new connection to host %s, already at a maximum of %d connections",
                        remote_address.getCString(), mMaxConnections);
            else
			    conn = createNewConnection(remote_address);
		}
		else
		{
			// clients shouldn't accept packets from unknown sources
			Net_Warning("received packet from unknown host %s.", remote_address.getCString());
			return;
		}
	}
}