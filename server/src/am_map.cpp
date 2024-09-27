// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//  AutoMap module.
//
//-----------------------------------------------------------------------------

#include "odamex.h"
#include "am_map.h"

am_default_colors_t AutomapDefaultColors;
am_colors_t AutomapDefaultCurrentColors;
int am_cheating = 0;

bool automapactive = false;

bool AM_ClassicAutomapVisible()
{
	return automapactive && !viewactive;
}

bool AM_OverlayAutomapVisible()
{
	return automapactive && viewactive;
}

void AM_SetBaseColorDoom()
{
	
}

void AM_SetBaseColorRaven()
{
	
}

void AM_SetBaseColorStrife()
{
	
}

void AM_Start()
{
	
}

BOOL AM_Responder(event_t* ev)
{
	return false;
}

void AM_Drawer()
{
	
}

VERSION_CONTROL(am_map_cpp, "$Id$")
