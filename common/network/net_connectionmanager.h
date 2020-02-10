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

#ifndef __NET_CONNECTION_MANAGER_H__
#define __NET_CONNECTION_MANAGER_H__

#include <list>
#include "hashtable.h"
#include "network/net_type.h"
#include "network/net_socketaddress.h"

class NetInterface;
class Connection;
class Packet;


// ============================================================================
//
// Type Definitions
//
// ============================================================================

static const uint8_t CONNECTION_ID_BITS = 9;		// allows for 0 - 512
typedef SequenceNumber<CONNECTION_ID_BITS> ConnectionId;
typedef IdGenerator<CONNECTION_ID_BITS> ConnectionIdGenerator;

#ifdef CLIENT_APP
static const uint16_t MAX_CONNECTIONS = 1;
#else
static const uint16_t MAX_CONNECTIONS = (1 << CONNECTION_ID_BITS);
#endif

//
// Function object to generate hash keys from SocketAddress objects
//
template <> struct hashfunc<SocketAddress>
{   size_t operator()(const SocketAddress& adr) const { return adr.getIPAddress() ^ adr.getPort(); } };


//
// Function object to generate hash keys from ConnectionId objects
//
template <> struct hashfunc<ConnectionId>
{	size_t	operator()(const ConnectionId& connection_id) const { return connection_id.getInteger(); } };


// ============================================================================
//
// ConnectionManager
//
// ============================================================================

class ConnectionManager
{
public:
    ConnectionManager(NetInterface* network_interface, uint16_t max_connections);
    ~ConnectionManager();

	ConnectionId requestConnection(const SocketAddress& adr);
	void closeConnection(const ConnectionId& connection_id);
	void closeAllConnections();
	bool isConnected(const ConnectionId& connection_id) const;
	Connection* findConnection(const ConnectionId& connection_id);
	const Connection* findConnection(const ConnectionId& connection_id) const;

	void serviceConnections();

	typedef enum
	{
		TERM_RECVTERM,
		TERM_QUIT,
		TERM_TIMEOUT,
		TERM_KICK,
		TERM_BAN,
		TERM_BADPASSWORD,
		TERM_BADHANDSHAKE,
		TERM_PARSEERROR,
		TERM_VERSIONMISMATCH,
		TERM_UNSPECIFIED		// should always be last
	} TermType_t;


private:
    NetInterface*   mNetworkInterface;
    uint16_t        mMaxConnections;

	bool			mInitialized;

	ConnectionIdGenerator		mConnectionIdGenerator;

	typedef OHashTable<SocketAddress, Connection*> ConnectionAddressTable;
	typedef OHashTable<ConnectionId, Connection*> ConnectionIdTable;

	ConnectionAddressTable		mConnectionsByAddress;
	ConnectionIdTable			mConnectionsById;

	struct TerminatedConnection 
	{
		Connection*			mConnection;
		dtime_t				mRemovalTime;
	};
	typedef std::list<TerminatedConnection> TerminatedConnectionList;
	TerminatedConnectionList	mTerminatedConnections;

	Connection* findConnection(const SocketAddress& adr);
	const Connection* findConnection(const SocketAddress& adr) const;
	Connection* createNewConnection(const SocketAddress& remote_address);

	void processDatagram(const SocketAddress& remote_address, BitStream& stream);
};

#endif  // __NET_CONNECTION_MANAGER_H__