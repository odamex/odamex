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

#include "i_url.h"

#include <string_view>

OdamexUrlParts I_ParseOdamexUrlParts(std::string_view url)
{
	static constexpr std::string_view protocol = "odamex://";
	static constexpr std::string_view::size_type protocolSize = protocol.size();
	OdamexUrlParts parts;

	// Not a valid URL if it's empty or doesn't start with the expected protocol.
	if (url.empty())
	{
		return OdamexUrlParts{};
	}

	// Check for the protocol prefix.
	if (url.size() < protocolSize || url.compare(0, protocolSize, protocol) != 0)
	{
		return OdamexUrlParts{};
	}

	std::string_view uri = url.substr(protocolSize);
	const size_t slash = uri.find('/');

	// If there's a slash, we only care about the part before it for -connect.
	if (slash == 0)
	{
		return OdamexUrlParts{};
	}
	else if (slash != std::string_view::npos)
	{
		uri = uri.substr(0, slash);
	}

	const size_t at = uri.find('@');

	// If there's no '@', the entire URI is treated as host:port.
	if (at == std::string_view::npos)
	{
		parts.hostport = std::string(uri);
		return parts.hostport.empty() ? OdamexUrlParts{} : parts;
	}

	// If '@' is at the start or end, it's invalid.
	if (at == 0 || at + 1 >= uri.size())
	{
		return OdamexUrlParts{};
	}

	const std::string_view userinfo = uri.substr(0, at);
	parts.hostport = std::string(uri.substr(at + 1));

	// If the host:port portion is empty, it's invalid.
	if (parts.hostport.empty())
	{
		return OdamexUrlParts{};
	}

	const size_t colon = userinfo.find(':');

	if (colon == std::string_view::npos)
	{
		// password-only form
		parts.password = std::string(userinfo);
		return parts;
	}

	// username:password form
	parts.username = std::string(userinfo.substr(0, colon));
	parts.password = std::string(userinfo.substr(colon + 1));
	return parts;
}

std::string I_ParseOdamexUrl(std::string_view url)
{
	// Only return the host:port for now.
	return I_ParseOdamexUrlParts(url).hostport;
}
