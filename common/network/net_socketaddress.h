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
//  Network socket address class
//
//-----------------------------------------------------------------------------

#ifndef __NET_SOCKETADDRESS_H__
#define __NET_SOCKETADDRESS_H__

#include "doomdef.h"
#include "doomtype.h"
#include "m_ostring.h"

#include "network/net_platform.h"


// ============================================================================
//
// SocketAddress Interface
//
// ============================================================================

class SocketAddress
{
public:
	SocketAddress();
	SocketAddress(uint8_t oct1, uint8_t oct2, uint8_t oct3, uint8_t oct4, uint16_t port);
	SocketAddress(const SocketAddress& other);
	SocketAddress(const OString& stradr);
	SocketAddress(const struct sockaddr_in& sadr);

	SocketAddress& operator=(const SocketAddress& other);
	bool operator==(const SocketAddress& other) const;
	bool operator!=(const SocketAddress& other) const;
	bool operator<(const SocketAddress& other) const;
	bool operator<=(const SocketAddress& other) const;
	bool operator>(const SocketAddress& other) const;
	bool operator>=(const SocketAddress& other) const;

	void setIPAddress(uint32_t ip);
	void setIPAddress(uint8_t oct1, uint8_t oct2, uint8_t oct3, uint8_t oct4);
	void setPort(uint16_t port);

	uint32_t getIPAddress() const;
	uint16_t getPort() const;

	void clear();
	bool isValid() const;

	const struct sockaddr_in& getSockAddrIn() const;
	const OString& getString() const;
	const char* getCString() const;

private:
	uint32_t    mIP;
	uint16_t    mPort;

	mutable bool					mIsSockDirty;
	mutable struct sockaddr_in		mSockAddrIn;
	mutable bool					mIsStringDirty;
	mutable OString 				mString;
};

#endif	// __NET_SOCKETADDRESS_H__

