// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: d_netinfo.cpp 1788 2010-08-24 04:42:57Z russellrice $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2010 by The Odamex Team.
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

void D_SetupUserInfo (void) {}
void D_UserInfoChanged (cvar_t *cvar) {} 
void D_DoServerInfoChange (byte **stream) {} 
void D_WriteUserInfoStrings (int i, byte **stream, bool compact) {} 
void D_ReadUserInfoStrings (int i, byte **stream, bool update) {}

VERSION_CONTROL (d_netinfo_cpp, "$Id: d_netinfo.cpp 1788 2010-08-24 04:42:57Z russellrice $")

