// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
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
//	Heads-up displays
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "subprocess.h"

#include "c_dispatch.h"


BEGIN_COMMAND(listen)
{
	const char* command_line[] = {"odasrv.exe", NULL};

	subprocess_s subprocess;
	const int result = subprocess_create(
	    command_line, subprocess_option_inherit_environment, &subprocess);
	if (result != 0)
	{
		// an error occurred!
	}
}
END_COMMAND(listen)
