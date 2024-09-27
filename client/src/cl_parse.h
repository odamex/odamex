// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2024 by The Odamex Team.
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
//  Handlers for messages sent from the server.
//
//-----------------------------------------------------------------------------

#pragma once



enum parseError_e
{
	PERR_OK,
	PERR_UNKNOWN_HEADER,
	PERR_UNKNOWN_MESSAGE,
	PERR_BAD_DECODE
};

struct Proto
{
	byte header;
	std::string name;
	size_t size;
	std::string data;
};

typedef std::vector<Proto> Protos;

const Protos& CL_GetTicProtos();
parseError_e CL_ParseCommand();
