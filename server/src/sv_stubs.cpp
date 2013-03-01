// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: sv_stubs.cpp $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2012 by The Odamex Team.
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
//	 Serverside function stubs
//
//-----------------------------------------------------------------------------

#include "c_cvars.h"
#include "d_player.h"
#include "doomdef.h"
#include "v_palette.h"

float BaseBlendA;
bool r_underwater;

void D_SetupUserInfo (void) {}
void D_UserInfoChanged (cvar_t *cvar) {} 
void D_DoServerInfoChange (byte **stream) {} 
void D_WriteUserInfoStrings (int i, byte **stream, bool compact) {} 
void D_ReadUserInfoStrings (int i, byte **stream, bool update) {}

std::string V_GetColorStringByName (const char *name) 
{ 
    return ""; 
}

int V_GetColorFromString (const DWORD *palette, const char *colorstring) 
{
    return 0;
}

void PickupMessage(AActor *toucher, const char *message) {}
void WeaponPickupMessage(AActor *toucher, weapontype_t &Weapon) {}

void AM_Stop(void) {}

void RefreshPalettes (void) {}

VERSION_CONTROL (sv_stubs_cpp, "$Id: sv_stubs.cpp $")

