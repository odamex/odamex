// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2006-2007 by The Odamex Team.
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
//	C_CMDS
//
//-----------------------------------------------------------------------------


#include "version.h"
#include "c_console.h"
#include "c_dispatch.h"

#include "i_system.h"

#include "doomstat.h"
#include "dstrings.h"
#include "s_sound.h"
#include "g_game.h"
#include "d_items.h"
#include "p_inter.h"
#include "z_zone.h"
#include "w_wad.h"
#include "g_level.h"
#include "gi.h"
#include "r_defs.h"
#include "d_player.h"
#include "r_main.h"
#include "sv_main.h"

extern FILE *Logfile;

BEGIN_COMMAND (rquit)
{
	SV_SendReconnectSignal();
	exit (0);
}
END_COMMAND (rquit)

BEGIN_COMMAND (quit)
{
	exit (0);
}
END_COMMAND (quit)

BEGIN_COMMAND (idclev)
{
	if ((argc > 1) && (*(argv[1] + 2) == 0) && *(argv[1] + 1) && *argv[1])
	{
		int epsd, map;
		char buf[2];
		char *mapname;

		buf[0] = argv[1][0] - '0';
		buf[1] = argv[1][1] - '0';

		if (gameinfo.flags & GI_MAPxx)
		{
			epsd = 1;
			map = buf[0]*10 + buf[1];
		}
		else
		{
			epsd = buf[0];
			map = buf[1];
		}

		// Catch invalid maps.
		mapname = CalcMapName (epsd, map);
		if (W_CheckNumForName (mapname) == -1)
			return;

		// So be it.
		Printf (PRINT_HIGH, "%s\n", STSTR_CLEV);

		unnatural_level_progression = true;
      	G_DeferedInitNew (mapname);
	}
}
END_COMMAND (idclev)

BEGIN_COMMAND (changemap)
{
	if (argc > 1)
	{
		// [Dash|RD] -- We can make a safe assumption that the user might not specify
		//		the whole lumpname for the level, and might opt for just the
		//		number. This makes sense, so why isn't there any code for it?
		if (W_CheckNumForName (argv[1]) == -1)
		{ // The map name isn't valid, so lets try to make some assumptions for the user.
			char mapname[32];

			// If argc is 2, we assume Doom 2/Final Doom. If it's 3, Ultimate Doom.
			if ( argc == 2 )
				sprintf( mapname, "MAP%02i", atoi( argv[1] ) );
			else if ( argc == 3 )
			{
				sprintf( mapname, "E%iM%i", atoi( argv[1] ), atoi( argv[2] ) );
			}

			if (W_CheckNumForName (mapname) == -1)
			{ // Still no luck, oh well.
				Printf (PRINT_HIGH, "Map %s not found\n", argv[1]);
			}
			else
			{ // Success
//				Net_WriteByte (DEM_CHANGEMAP);
//				Net_WriteString (mapname);
			}

		}
		else
		{
//			Net_WriteByte (DEM_CHANGEMAP);
//			Net_WriteString (argv[1]);
		}
	}
}
END_COMMAND (changemap)

BEGIN_COMMAND (gameversion)
{
	Printf (PRINT_HIGH, "%d.%d : " __DATE__ "\n", VERSION / 100, VERSION % 100);
}
END_COMMAND (gameversion)

BEGIN_COMMAND (dumpheap)
{
	int lo = PU_STATIC, hi = PU_CACHE;

	if (argc >= 2) {
		lo = atoi (argv[1]);
		if (argc >= 3) {
			hi = atoi (argv[2]);
		}
	}

	Z_DumpHeap (lo, hi);
}
END_COMMAND (dumpheap)

BEGIN_COMMAND (logfile)
{
	time_t clock;
	char *timestr;

	time (&clock);
	timestr = asctime (localtime (&clock));

	if (Logfile)
	{
		Printf (PRINT_HIGH, "Log stopped: %s\n", timestr);
		fclose (Logfile);
		Logfile = NULL;
	}

	if (argc >= 2)
	{
		if ( (Logfile = fopen (argv[1], "w")) )
		{
			Printf (PRINT_HIGH, "Log started: %s\n", timestr);
		}
		else
		{
			Printf (PRINT_HIGH, "Could not start log\n");
		}
	}
}
END_COMMAND (logfile)

