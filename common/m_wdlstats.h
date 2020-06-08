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
//     Defines needed by the WDL stats logger.
//
//-----------------------------------------------------------------------------

#ifndef __WDLSTATS_H__
#define __WDLSTATS_H__

enum WDLEvents {
	WDL_DAMAGE,
	WDL_CARRIERDAMAGE,
	WDL_KILL,
	WDL_CARRIERKILL,
	WDL_ENVIRODAMAGEUNUSED,
	WDL_ENVIRODAMAGE,
	WDL_ENVIROKILL,
	WDL_ENVIROCARRIERKILL,
	WDL_TOUCH,
	WDL_PICKUPTOUCH,
	WDL_CAPTURE,
	WDL_PICKUPCAPTURE,
	WDL_ASSIST,
	WDL_RETURNFLAG,
	WDL_ITEMPICKUP,
	WDL_ACCURACY,
};

void P_StartWDLLog();
void P_LogWDLEvent(WDLEvents event, int arg0, int arg1, int arg2);
void P_CommitWDLLog();

#endif
