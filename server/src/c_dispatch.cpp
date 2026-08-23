// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	Argument processing (?)
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "c_cvars.h"
#include "c_dispatch.h"
#include "sv_main.h"

void C_RunCVarScriptHook(const cvar_t& var, bool resend)
{
	if (!var.str().empty())
	{
		AddCommandString(var.str());
		// Make sure any cvar modifications from the script make it to clients
		if (resend)
		{
			cvar_t::UnlatchCVars();
			SV_ServerSettingChange(true);
		}
	}
}

VERSION_CONTROL (c_dispatch_cpp, "$Id$")