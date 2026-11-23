// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2026 by The Odamex Team.
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
//  Utility functions
//
// AUTHORS:
//  Russell Rice (russell at odamex dot net)
//  Michael Wood (mwoodj at knology dot net)
//
//-----------------------------------------------------------------------------

#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <WinSock2.h>
#include <Windows.h>
#else
#include <sys/time.h>
#endif

#ifdef max
	#undef max
#endif

#include <limits>

#include "net_utils.h"

namespace odalpapi
{

#if defined(_WIN32)

/* FILETIME of Jan 1 1970 00:00:00. */
static const unsigned __int64 epoch = ((unsigned __int64)116444736000000000ULL);

int gettimeofday(struct timeval* tp, ...)
{
	FILETIME file_time;
	SYSTEMTIME system_time;
	ULARGE_INTEGER ularge;

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	ularge.LowPart = file_time.dwLowDateTime;
	ularge.HighPart = file_time.dwHighDateTime;

	tp->tv_sec = (long)((ularge.QuadPart - epoch) / 10000000L);
	tp->tv_usec = (long)(system_time.wMilliseconds * 1000);

	return 0;
}

#endif

// GetMillisNow()

// denis - use this unless you want your program
// to get confused every 49 days due to DWORD limit
uint64_t _UnwrapTime(uint32_t now32)
{
	static uint64_t last = 0;
	uint64_t now = now32;
	static uint64_t max = std::numeric_limits<uint32_t>::max();

	if(now < last%max)
		last += (max-(last%max)) + now;
	else
		last = now;

	return last;
}

int32_t _Millis()
{
	struct timeval tp;

	gettimeofday(&tp, (struct timezone*)NULL);

	return (tp.tv_sec * 1000 + (tp.tv_usec / 1000));
}

uint64_t GetMillisNow()
{
	return _UnwrapTime(_Millis());
}

int32_t OdaAddrToComponents(const std::string& HostPort, std::string& AddrOut,
                            uint16_t& PortOut)
{
	if(HostPort.empty())
		return 1;

	std::string host;
	std::string portText;
	bool hasPort = false;

	if(HostPort[0] == '[')
	{
		// Brackets make the port separator unambiguous for IPv6 literals.
		const size_t closeBracket = HostPort.find(']');
		if(closeBracket == std::string::npos || closeBracket == 1)
			return 2;

		host = HostPort.substr(1, closeBracket - 1);
		const std::string suffix = HostPort.substr(closeBracket + 1);
		if(!suffix.empty())
		{
			if(suffix[0] != ':' || suffix.size() == 1)
				return 2;
			portText = suffix.substr(1);
			hasPort = true;
		}
	}
	else
	{
		const size_t firstColon = HostPort.find(':');
		const size_t lastColon = HostPort.rfind(':');

		if(firstColon != std::string::npos && firstColon == lastColon)
		{
			// IPv4/hostname with an explicit port.
			host = HostPort.substr(0, firstColon);
			portText = HostPort.substr(firstColon + 1);
			hasPort = true;
		}
		else
		{
			// Multiple colons are treated as an unbracketed IPv6 literal.
			// To specify an IPv6 port, use [address]:port.
			host = HostPort;
		}
	}

	if(host.empty())
		return 1;

	if(hasPort)
	{
		if(portText.empty())
			return 2;

		for(std::string::const_iterator it = portText.begin();
		    it != portText.end(); ++it)
		{
			if(*it < '0' || *it > '9')
				return 2;
		}

		char* end = NULL;
		const unsigned long port = std::strtoul(portText.c_str(), &end, 10);
		if(!end || *end != '\0' || port > 65535)
			return 2;

		PortOut = static_cast<uint16_t>(port);
	}

	AddrOut = host;
	return 0;
}

} // namespace

// ???
// ---
