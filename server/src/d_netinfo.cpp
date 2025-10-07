// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2025 by The Odamex Team.
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
//	D_NETINFO
//
//-----------------------------------------------------------------------------


#include "odamex.h"


#include "d_netinf.h"
#include "sv_main.h"
#include "v_textcolors.h"

int D_GenderToInt (const char *gender)
{
	if (!stricmp (gender, "female"))
		return GENDER_FEMALE;
	else if (!stricmp (gender, "cyborg"))
		return GENDER_NEUTER;
	else
		return GENDER_MALE;
}

bool SetServerVar (std::string_view name, const char *value)
{
	cvar_t *dummy;
	cvar_t *var = cvar_t::FindCVar (name, &dummy);

	if (var)
	{
		unsigned oldflags = var->flags();

		var->m_Flags &= ~(CVAR_SERVERINFO|CVAR_LATCH);
		var->Set (value);
		var->m_Flags = oldflags;
		return true;
	}
	return false;
}

void D_SendServerInfoChange (const cvar_t *cvar, const char *value)
{
	SetServerVar(cvar->name().c_str(), value);
	SV_BroadcastPrintFmt("{}{} has been modified to {}!\n", TEXTCOLOR_YELLOW, cvar->name(), value);
	SV_ServerSettingChange ();
}

FArchive &operator<< (FArchive &arc, UserInfo &info)
{
	return arc;
}

FArchive &operator>> (FArchive &arc, UserInfo &info) // removeme
{
	return arc;
}

VERSION_CONTROL (d_netinfo_cpp, "$Id$")
