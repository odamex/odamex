// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	Command library (?)
//
//-----------------------------------------------------------------------------


#include <time.h>

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
#include "cl_main.h"

extern FILE *Logfile;

BEGIN_COMMAND (toggleconsole)
{
	C_ToggleConsole();
}
END_COMMAND (toggleconsole)

EXTERN_CVAR (sv_cheats)

bool CheckCheatmode (void)
{
	if (((skill == sk_nightmare) || netgame || deathmatch) && !sv_cheats)
	{
		Printf (PRINT_HIGH, "You must run the server with '+set sv_cheats 1' to enable this command.\n");
		return true;
	}
	else
	{
		return false;
	}
}

BEGIN_COMMAND (quit)
{
	exit (0);
}
END_COMMAND (quit)

/*
==================
Cmd_God

Sets client to godmode

argv(0) god
==================
*/
BEGIN_COMMAND (god)
{
	if (CheckCheatmode ())
		return;

	consoleplayer().cheats ^= CF_GODMODE;

	MSG_WriteMarker(&net_buffer, clc_cheat);
	MSG_WriteByte(&net_buffer, consoleplayer().cheats);
}
END_COMMAND (god)

BEGIN_COMMAND (notarget)
{
	if (CheckCheatmode ())
		return;

	consoleplayer().cheats ^= CF_NOTARGET;

	MSG_WriteMarker(&net_buffer, clc_cheat);
	MSG_WriteByte(&net_buffer, consoleplayer().cheats);
}
END_COMMAND (notarget)

BEGIN_COMMAND (fly)
{
	if (CheckCheatmode ())
		return;

	consoleplayer().cheats ^= CF_FLY;

	MSG_WriteMarker(&net_buffer, clc_cheat);
	MSG_WriteByte(&net_buffer, consoleplayer().cheats);
}
END_COMMAND (fly)

/*
==================
Cmd_Noclip

argv(0) noclip
==================
*/
BEGIN_COMMAND (noclip)
{
	if (CheckCheatmode ())
		return;

	consoleplayer().cheats ^= CF_NOCLIP;

	MSG_WriteMarker(&net_buffer, clc_cheat);
	MSG_WriteByte(&net_buffer, consoleplayer().cheats);
}
END_COMMAND (noclip)

EXTERN_CVAR (chasedemo)

BEGIN_COMMAND (chase)
{
	if (demoplayback)
	{
		size_t i;

		if (chasedemo)
		{
			chasedemo.Set (0.0f);
			for (i = 0; i < players.size(); i++)
				players[i].cheats &= ~CF_CHASECAM;
		}
		else
		{
			chasedemo.Set (1.0f);
			for (i = 0; i < players.size(); i++)
				players[i].cheats |= CF_CHASECAM;
		}
	}
	else
	{
		if (deathmatch && CheckCheatmode ())
			return;

		consoleplayer().cheats ^= CF_NOCLIP;

		MSG_WriteMarker(&net_buffer, clc_cheat);
		MSG_WriteByte(&net_buffer, consoleplayer().cheats);
	}
}
END_COMMAND (chase)

BEGIN_COMMAND (idmus)
{
	level_info_t *info;
	char *map;
	int l;

	if (argc > 1)
	{
		if (gameinfo.flags & GI_MAPxx)
		{
			l = atoi (argv[1]);
			if (l <= 99)
				map = CalcMapName (0, l);
			else
			{
				Printf (PRINT_HIGH, "%s\n", STSTR_NOMUS);
				return;
			}
		}
		else
		{
			map = CalcMapName (argv[1][0] - '0', argv[1][1] - '0');
		}

		if ( (info = FindLevelInfo (map)) )
		{
			if (info->music[0])
			{
				S_ChangeMusic (std::string(info->music, 8), 1);
				Printf (PRINT_HIGH, "%s\n", STSTR_MUS);
			}
		} else
			Printf (PRINT_HIGH, "%s\n", STSTR_NOMUS);
	}
}
END_COMMAND (idmus)

BEGIN_COMMAND (give)
{
	if (CheckCheatmode ())
		return;

	if (argc < 2)
		return;
	
	std::string name = BuildString (argc - 1, (const char **)(argv + 1));
	if (name.length())
	{
		//Net_WriteByte (DEM_GIVECHEAT);
		//Net_WriteString (name.c_str());
		// todo
	}
}
END_COMMAND (give)

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

BEGIN_COMMAND (fov)
{
	if(!connected || !m_Instigator || !m_Instigator->player)
	{
		Printf (PRINT_HIGH, "cannot change fov: not in game\n");
		return;
	}
		
	if (argc != 2)
		Printf (PRINT_HIGH, "fov is %g\n", m_Instigator->player->fov);
	else if (sv_cheats)
		m_Instigator->player->fov = atof (argv[1]);
    else
        Printf (PRINT_HIGH, "You must run the server with '+set sv_cheats 1' to enable this command.\n");
}
END_COMMAND (fov)

BEGIN_COMMAND (spectate)
{
	MSG_WriteMarker(&net_buffer, clc_spectate);
	MSG_WriteByte(&net_buffer, true);
}
END_COMMAND (spectate)

BEGIN_COMMAND (joingame)
{
	MSG_WriteMarker(&net_buffer, clc_spectate);
	MSG_WriteByte(&net_buffer, false);
}
END_COMMAND (joingame)


VERSION_CONTROL (c_cmds_cpp, "$Id$")

