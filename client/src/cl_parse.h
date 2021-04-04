// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//  Handlers for messages sent from the server.
//
//-----------------------------------------------------------------------------

#ifndef __CL_PARSE_H__
#define __CL_PARSE_H__

#include <string>
#include <vector>

#include "doomtype.h"

enum parseResult_e
{
	PRES_OK,
	PRES_UNKNOWN_HEADER,
	PRES_BAD_DECODE
};

struct Proto
{
	byte header;
	std::string name;
	size_t size;
	std::string data;
	std::string shortdata;
};

typedef std::vector<Proto> Protos;

const Protos& CL_GetTicProtos();
parseResult_e CL_ParseCommand();

#endif // __CL_PARSE_H__
