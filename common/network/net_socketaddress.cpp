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

#include "m_ostring.h"

#include "network/net_platform.h"
#include "network/net_socketaddress.h"


// ============================================================================
//
// SocketAddress Implementation
//
// ============================================================================

SocketAddress::SocketAddress() :
	mIP(0), mPort(0), mIsSockDirty(true), mIsStringDirty(true)
{
}

SocketAddress::SocketAddress(uint8_t oct1, uint8_t oct2, uint8_t oct3, uint8_t oct4, uint16_t port) :
	mIP((oct1 << 24) | (oct2 << 16) | (oct3 << 8) | oct4),
	mPort(port), mIsSockDirty(true), mIsStringDirty(true)
{
}

SocketAddress::SocketAddress(const SocketAddress& other) :
	mIP(other.mIP), mPort(other.mPort),
	mIsSockDirty(true), mIsStringDirty(true)
{
}

SocketAddress::SocketAddress(const OString& stradr) :
	mIP(0), mPort(0),
	mIsSockDirty(true), mIsStringDirty(true)
{
	char copy[256];
	strncpy(copy, stradr.c_str(), sizeof(copy) - 1);
	copy[sizeof(copy) - 1] = 0;

	// strip off a trailing :port if present
	for (char* colon = copy; *colon; colon++)
	{
		if (*colon == ':')
		{
			*colon = 0;
			mPort = atoi(colon + 1);
		}
	}

	// determine which interface IP address to bind to based on ip_string
	struct addrinfo hints;
	struct addrinfo* ai;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(copy, NULL, &hints, &ai) == 0)
	{
		struct sockaddr_in* addr = (struct sockaddr_in*)(ai->ai_addr);
		mIP = ntohl(addr->sin_addr.s_addr);
	}
}

SocketAddress::SocketAddress(const struct sockaddr_in& sadr)
{
	mIP = ntohl(sadr.sin_addr.s_addr);
	mPort = ntohs(sadr.sin_port);
	mIsSockDirty = mIsStringDirty = true;
}

SocketAddress& SocketAddress::operator=(const SocketAddress& other)
{
	if (&other != this)
	{
		mIP = other.mIP;
		mPort = other.mPort;
		mIsSockDirty = mIsStringDirty = true;
	}
   
	return *this;
}

bool SocketAddress::operator==(const SocketAddress& other) const
{
	return mIP == other.mIP && mPort == other.mPort;
}

bool SocketAddress::operator!=(const SocketAddress& other) const
{
	return !(operator==(other));
}

bool SocketAddress::operator<(const SocketAddress& other) const
{
	return (mIP < other.mIP) || (mIP == other.mIP && mPort < other.mPort);
}

bool SocketAddress::operator<=(const SocketAddress& other) const
{
	return (mIP < other.mIP) || (mIP == other.mIP && mPort <= other.mPort);
}

bool SocketAddress::operator>(const SocketAddress& other) const
{
	return !(operator<=(other));
}

bool SocketAddress::operator>=(const SocketAddress& other) const
{
	return !(operator<(other));
}

void SocketAddress::setIPAddress(uint32_t ip)
{
	mIP = ip;
	mIsSockDirty = mIsStringDirty = true;
}

void SocketAddress::setIPAddress(uint8_t oct1, uint8_t oct2, uint8_t oct3, uint8_t oct4)
{
	mIP = (oct1 << 24) | (oct2 << 16) | (oct3 << 8) | (oct4);
	mIsSockDirty = mIsStringDirty = true;
}

void SocketAddress::setPort(uint16_t port)
{
	mPort = port;
	mIsSockDirty = mIsStringDirty = true;
}

uint32_t SocketAddress::getIPAddress() const
{
	return mIP;
}

uint16_t SocketAddress::getPort() const
{
	return mPort;
}

void SocketAddress::clear()
{
	mIP = mPort = 0;
	mIsSockDirty = mIsStringDirty = true;
}

bool SocketAddress::isValid() const
{
	return mIP > 0 && mPort > 0;
}

const struct sockaddr_in& SocketAddress::getSockAddrIn() const
{
	if (mIsSockDirty)
	{
		memset(&mSockAddrIn, 0, sizeof(mSockAddrIn));
		mSockAddrIn.sin_family = AF_INET;
		mSockAddrIn.sin_addr.s_addr = htonl(mIP);
		mSockAddrIn.sin_port = htons(mPort);
		mIsSockDirty = false;
	}

	return mSockAddrIn;
}

const OString& SocketAddress::getString() const
{
	if (mIsStringDirty)
	{
		char buf[22];
		sprintf(buf, "%u.%u.%u.%u:%u",
				(mIP >> 24) & 0xFF,
				(mIP >> 16) & 0xFF,
				(mIP >> 8 ) & 0xFF,
				(mIP      ) & 0xFF,
				mPort);

		mString = buf;
		mIsStringDirty = false;
	}

	return mString;
}

const char* SocketAddress::getCString() const
{
	return getString().c_str();
}

VERSION_CONTROL (net_socketaddress_cpp, "$Id$")

