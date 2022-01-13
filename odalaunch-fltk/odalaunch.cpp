// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2021 by The Odamex Team.
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
//  Global defines.
//
//-----------------------------------------------------------------------------

#include "odalaunch.h"

#if defined(USE_MS_SNPRINTF)

int __cdecl ms_snprintf(char* buffer, size_t n, const char* format, ...)
{
	int retval;
	va_list argptr;

	va_start(argptr, format);
	retval = _vsnprintf(buffer, n, format, argptr);
	va_end(argptr);
	return retval;
}

int __cdecl ms_vsnprintf(char* s, size_t n, const char* format, va_list arg)
{
	return _vsnprintf(s, n, format, arg);
}

#endif

std::string AddressCombine(const char* address, uint16_t port)
{
	char buffer[128];
	snprintf(buffer, ARRAY_LENGTH(buffer), "%s:%u", address, port);
	return buffer;
}

std::string AddressCombine(const char* address, const char* port)
{
	char buffer[128];
	snprintf(buffer, ARRAY_LENGTH(buffer), "%s:%s", address, port);
	return buffer;
}

void AddressSplit(const char* address, std::string& outIp, uint16_t& outPort)
{
	outIp = address;
	const size_t colon = outIp.rfind(':');
	const std::string port = outIp.substr(colon + 1);
	outPort = static_cast<uint16_t>(atoi(port.c_str()));
	outIp = outIp.substr(0, colon);
}
