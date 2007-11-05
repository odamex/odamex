// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//		DOOM Network game communication and protocol,
//		all OS independent parts.
//
//-----------------------------------------------------------------------------

#include "version.h"
#include "m_alloc.h"
#include "m_menu.h"
#include "m_random.h"
#include "i_system.h"
#include "i_video.h"
#include "i_net.h"
#include "g_game.h"
#include "doomdef.h"
#include "doomstat.h"
#include "c_console.h"
#include "d_netinf.h"
#include "cmdlib.h"
#include "s_sound.h"
#include "m_cheat.h"
#include "p_local.h"
#include "c_dispatch.h"
#include "gi.h"
#include "cl_main.h"

extern byte		*demo_p;		// [RH] Special "ticcmds" get recorded in demos

void CL_RememberSkin(void);

//
// NETWORKING
//
// gametic is the tic about to (or currently being) run
// maketic is the tick that hasn't had control made for it yet
// nettics[] has the maketics for all players 
//
// a gametic cannot be run until nettics[] > gametic for all players
//
int 			maketic;
int 			lastnettic;
int 			skiptics;
int 			ticdup; 		

void D_ProcessEvents (void); 
void G_BuildTiccmd (ticcmd_t *cmd); 
void D_DoAdvanceDemo (void);

//
// NetUpdate
// Builds ticcmds for console player,
// sends out a packet
//
void NetUpdate (void)
{
    // check time
	static QWORD gametime;

    QWORD nowtime = I_GetTime ();
    QWORD newtics = nowtime - gametime;
    gametime = nowtime;

    if (newtics <= 0)       // nothing new to update
       return;
	   
    for (QWORD i = 0; i < newtics; i++) // denis - todo - this cycles needlessly at startup? but a trivial fix creates input delay? potential endless loops?
    {
        I_StartTic ();
        D_ProcessEvents ();
		G_BuildTiccmd (&consoleplayer().netcmds[maketic%BACKUPTICS]);
		
        maketic++;
    }
}

//
// D_CheckNetGame
// Works out player numbers among the net participants
//
void D_CheckNetGame (void)
{
    CL_InitNetwork ();

    D_SetupUserInfo();

    ticdup = 1;
}


//
// TryRunTics
//
extern	BOOL	 advancedemo;
int canceltics = 0;


void TryRunTics (void)
{
	// get real tics
	static QWORD oldentertics;

	QWORD entertic = I_WaitForTic (oldentertics);
	QWORD realtics = entertic - oldentertics;
	oldentertics = entertic;
	
	DObject::BeginFrame ();
	
	NetUpdate ();  // check for new console commands

	// run the realtics tics
	while (realtics--)
	{
		if(canceltics && canceltics--)
			continue;

		if (advancedemo)
			D_DoAdvanceDemo ();
		
		C_Ticker ();
		M_Ticker ();
		G_Ticker ();
		gametic++;
	}
	
	DObject::EndFrame ();
}


VERSION_CONTROL (d_net_cpp, "$Id$")

