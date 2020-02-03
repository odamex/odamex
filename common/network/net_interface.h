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
// Abstraction for low-level network mangement 
//
//-----------------------------------------------------------------------------

#ifndef __NET_INTERFACE_H__
#define __NET_INTERFACE_H__

#include "hashtable.h"
#include "m_ostring.h" 

#include "network/net_platform.h"
#include "network/net_socketaddress.h"

class UPNPInterface;

// ============================================================================
//
// NetInterface Interface
//
// 
// ============================================================================

typedef enum
{
	HOST_CLIENT,
	HOST_SERVER
} HostType_t;


class NetInterface
{
public:
	NetInterface();
	~NetInterface();

	bool openInterface(const OString& address, uint16_t desired_port);
	bool closeInterface();
	bool setReceivingBufferSize(uint32_t size);
	bool setSendingBufferSize(uint32_t size);

	HostType_t getHostType() const;
	const SocketAddress& getLocalAddress() const;

	bool socketSend(const SocketAddress& dest, const uint8_t* databuf, uint16_t size) const;
	bool socketRecv(SocketAddress& source, uint8_t* data, uint16_t& size) const;

private:
	// ------------------------------------------------------------------------
	// Low-level socket interaction
	// ------------------------------------------------------------------------

	void openSocket(uint32_t desired_ip, uint16_t desired_port);
	void closeSocket();
	bool isSocketValid() const;
	void assignLocalAddress();

	HostType_t				mHostType;
	bool					mInitialized;
	SocketAddress			mLocalAddress;
	SOCKET					mSocket;
	UPNPInterface*			mUPNPInterface;
};


#endif	// __NET_INTERFACE_H__
