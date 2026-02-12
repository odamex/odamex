// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by The Odamex Team.
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
//
// DESCRIPTION:
//	System interface, URL parsing helpers.
//
//-----------------------------------------------------------------------------

#pragma once

#include <stddef.h>

#ifdef __cplusplus
#include <string>
#include <string_view>

struct OdamexUrlParts
{
	// Optional credentials parsed from userinfo before '@'.
	std::string username;
	std::string password;
	// Host[:port] portion used for -connect.
	std::string hostport;
};

// Parse an odamex:// URL into username/password and host:port.
OdamexUrlParts I_ParseOdamexUrlParts(std::string_view url);
// Extract host[:port] from an odamex:// URL, or return empty on failure.
std::string I_ParseOdamexUrl(std::string_view url);

#endif
