// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//   Handles parsing all ONCRINFO lumps.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "w_wad.h"
#include "oscanner.h"
#include "gstrings.h"
#include "g_announcer.h"

void G_ParseOncrInfo()
{
	int lump = -1;

	AnnouncerManager::getInstance().reset();

	// No ONCRINFO? Load defaults and continue.
	if (W_FindLump("ONCRINFO", lump) == -1)
	{
		AnnouncerManager::getInstance().loadAnnouncerDefaults();
		return;
	}

	while ((lump = W_FindLump("ONCRINFO", lump)) != -1)
	{
		//ParseOncrInfo(lump, "ONCRINFO");
	}
}
