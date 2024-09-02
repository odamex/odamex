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

/******************************************************************************/

std::string AddressCombine(const std::string& address, const uint16_t port)
{
	return fmt::format("{}:{}", address, port);
}

/******************************************************************************/

void AddressSplit(std::string& outIp, uint16_t& outPort, const std::string& address)
{
	outIp = address;
	const size_t colon = outIp.rfind(':');
	const std::string port = outIp.substr(colon + 1);
	outPort = static_cast<uint16_t>(std::stoul(port));
	outIp = outIp.substr(0, colon);
}
