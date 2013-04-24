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
//	SV_MASTER
//
//-----------------------------------------------------------------------------


#ifndef __SVMASTER_H__
#define __SVMASTER_H__

#include "doomtype.h"
#include "doomstat.h"
#include "d_player.h"
#include "p_local.h"
#include "sv_main.h"
#include "c_console.h"

bool SV_AddMaster (const char *masterip);
void SV_InitMasters();
bool SV_AddMaster(const char *masterip);
void SV_ListMasters ();
bool SV_RemoveMaster (const char *masterip);
void SV_UpdateMasterServers(void);
void SV_UpdateMaster(void);
void SV_ArchiveMasters(FILE *fp);

#endif
