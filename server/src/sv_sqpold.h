// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2013 by The Odamex Team.
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
//  Old version of the server query protocol, kept for clients and older 
//  launchers
//
//-----------------------------------------------------------------------------


#ifndef __SV_SQPOLD_H__
#define __SV_SQPOLD_H__

void SV_SendServerInfo ();
bool SV_IsValidToken(DWORD token);

#endif // __SV_SQPOLD_H__
