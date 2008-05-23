// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//	Client-side CTF Implementation
//
//-----------------------------------------------------------------------------


#include	"c_dispatch.h"
#include	"cl_main.h"
#include	"w_wad.h"
#include	"z_zone.h"
#include	"v_video.h"
#include	"p_local.h"
#include	"cl_ctf.h"
#include    "st_stuff.h"

flagdata CTFdata[NUMFLAGS];
int TEAMpoints[NUMFLAGS];

static int tintglow = 0;

char *team_names[NUMTEAMS] =
{
	"BLUE", "RED", "GOLD"
};

bool		ctfmode		 = false;
bool		teamplaymode = false;

// denis - this is a lot clearer than doubly nested switches
static mobjtype_t flag_table[NUMFLAGS][NUMFLAGSTATES] =
{
	{MT_BFLG, MT_BDWN, MT_BCAR},
	{MT_RFLG, MT_RDWN, MT_RCAR},
	{MT_GFLG, MT_GDWN, MT_GCAR}
};

EXTERN_CVAR		(screenblocks)
EXTERN_CVAR		(st_scale)

BEGIN_COMMAND	(ctf)
{
	if (ctfmode)	Printf (PRINT_HIGH, "CTF is enabled.\n");
	else			Printf (PRINT_HIGH, "CTF is disabled.\n");
}
END_COMMAND		(ctf)

//
// CTF_Connect
// Receive states of all flags
//
void CTF_Connect()
{
	size_t i;

	// clear player flags client may have imagined
	for(i = 0; i < players.size(); i++)
		for(size_t j = 0; j < NUMFLAGS; j++)
			players[i].flags[j] = false;

	for(i = 0; i < NUMFLAGS; i++)
	{
		CTFdata[i].state = (flag_state_t)MSG_ReadByte();
		byte flagger = MSG_ReadByte();

		if(CTFdata[i].state == flag_carried)
		{
			player_t &player = idplayer(flagger);

			if(validplayer(player))
				CTF_CarryFlag(player, (flag_t)i);
		}
	}
}

//
//	[Toke - CTF] CL_CTFEvent
//	Deals with CTF specific network data
//
void CL_CTFEvent (void)
{
	flag_score_t event = (flag_score_t)MSG_ReadByte();

	if(event == SCORE_NONE) // CTF state refresh
	{
		CTF_Connect();
		return;
	}

	flag_t flag = (flag_t)MSG_ReadByte();
	player_t &player = idplayer(MSG_ReadByte());
	int points = MSG_ReadLong();

	if(validplayer(player))
		player.points = points;

	for(size_t i = 0; i < NUMFLAGS; i++)
		TEAMpoints[i] = MSG_ReadLong ();

	switch(event)
	{
		default:
		case SCORE_NONE:
		case SCORE_REFRESH:
		case SCORE_KILL:
		case SCORE_BETRAYAL:
		case SCORE_CARRIERKILL:
			break;

		case SCORE_GRAB:
		case SCORE_FIRSTGRAB:
			if(validplayer(player))
				CTF_CarryFlag(player, flag);
			break;

		case SCORE_RETURN:
			CTFdata[flag].state = flag_home;
			break;

		case SCORE_CAPTURE:
			if(validplayer(player))
				CTF_CheckFlags(player);
			else
				CTFdata[flag].flagger = 0;
			CTFdata[flag].state = flag_home;
			if(CTFdata[flag].actor)
				CTFdata[flag].actor->Destroy();
			break;

		case SCORE_DROP:
			if(validplayer(player))
				CTF_CheckFlags(player);
			else
				CTFdata[flag].flagger = 0;
			CTFdata[flag].state = flag_dropped;
			if(CTFdata[flag].actor)
				CTFdata[flag].actor->Destroy();
			break;
	}
}

//	CTF_CheckFlags
//																					[Toke - CTF - carry]
//	Checks player for flags
//
void CTF_CheckFlags (player_t &player)
{
	for(size_t i = 0; i < NUMFLAGS; i++)
	{
		if(player.flags[i])
		{
			player.flags[i] = false;
			CTFdata[i].flagger = 0;
		}
	}
}

//
//	CTF_TossFlag
//																					[Toke - CTF - Toss]
//	Player tosses the flag
/* [ML] 04/4/06: Remove flagtossing, too buggy
void CTF_TossFlag (void)
{
	MSG_WriteMarker (&net_buffer, clc_ctfcommand);

	if (CTFdata.BlueScreen)	CTFdata.BlueScreen	= false;
	if (CTFdata.RedScreen)	CTFdata.RedScreen	= false;
	if (CTFdata.GoldScreen)	CTFdata.GoldScreen	= false;
}

BEGIN_COMMAND	(flagtoss)
{
	CTF_TossFlag ();
}
END_COMMAND		(flagtoss)
*/

//
//	[Toke - CTF] CTF_CarryFlag
//	Spawns a flag on a players location and links the flag to the player
//
void CTF_CarryFlag (player_t &player, flag_t flag)
{
	if (!validplayer(player))
		return;

	player.flags[flag] = true;
	CTFdata[flag].flagger = player.id;
	CTFdata[flag].state = flag_carried;

	// spawn visible flags on other players
	if(&player != &consoleplayer())
	{
		AActor *actor = new AActor(0, 0, 0, flag_table[flag][flag_carried]);
		CTFdata[flag].actor = actor->ptr();

		CTF_MoveFlags();
	}
}

//
//	[Toke - CTF] CTF_MoveFlag
//	Moves the flag that is linked to a player
//
void CTF_MoveFlags ()
{
	// denis - flag is now a boolean
	for(size_t i = 0; i < NUMFLAGS; i++)
	{
		if(CTFdata[i].flagger && CTFdata[i].actor)
		{
			player_t &player = idplayer(CTFdata[i].flagger);
			AActor *flag = CTFdata[i].actor;

			if(!player.mo)
			{
				flag->UnlinkFromWorld ();
				return;
			}

			extern fixed_t tmfloorz;
			extern fixed_t tmceilingz;

			unsigned an = player.mo->angle >> ANGLETOFINESHIFT;
			fixed_t x = (player.mo->x + FixedMul (-2*FRACUNIT, finecosine[an]));
			fixed_t y = (player.mo->y + FixedMul (-2*FRACUNIT, finesine[an]));

			P_CheckPosition (player.mo, player.mo->x, player.mo->y);
			flag->UnlinkFromWorld ();

			flag->x = x;
			flag->y = y;
			flag->z = player.mo->z;
			flag->floorz = tmfloorz;
			flag->ceilingz = tmceilingz;

			flag->LinkToWorld ();
		}
	}
}

void TintScreen(int color)
{
	if (screenblocks < 11)
	{
			screen->Clear (0,
						   0,
						   screen->width / 100,
						   screen->height - ST_HEIGHT,
						   color);

			screen->Clear (0,
						   0,
						   screen->width,
						   screen->height / 100,
						   color);

			screen->Clear (screen->width - (screen->width / 100),
						   0,
						   screen->width,
						   screen->height - ST_HEIGHT,
						   color);

			screen->Clear (0,
						   (screen->height - ST_HEIGHT) - (screen->height / 100),
						   screen->width,
						   screen->height - ST_HEIGHT,
						   color);
	}
	else
	{
			screen->Clear (0,
						   0,
						   screen->width / 100,
						   screen->height,
						   color);

			screen->Clear (0,
						   0,
						   screen->width,
						   screen->height / 100,
						   color);

			screen->Clear (screen->width - (screen->width / 100),
						   0,
						   screen->width,
						   screen->height,
						   color);

			screen->Clear (0,
						   (screen->height) - (screen->height / 100),
						   screen->width,
						   screen->height,
						   color);
	}
}

//
//	[Toke - CTF] CTF_RunTics
//	Runs once per gametic when ctf is enabled
//
void CTF_RunTics (void)
{

    // NES - Glowing effect on screen tint.
    if (tintglow < 90)
        tintglow++;
    else
        tintglow = 0;

	// Move the physical clientside flag sprites
	CTF_MoveFlags();
}

//
//	[Toke - CTF - Hud] CTF_DrawHud
//	Draws the CTF Hud, duH
//
void CTF_DrawHud (void)
{
    int tintglowtype;
    bool hasflag = false, hasflags[NUMFLAGS];

	if(!ctfmode)
		return;

	player_t &co = consoleplayer();
	for(size_t i = 0; i < NUMFLAGS; i++)
	{
		hasflags[i] = false;
		if(CTFdata[i].state == flag_carried && CTFdata[i].flagger == co.id)
		{
			hasflag = true;			
			hasflags[i] = true;
		}
	}
	
	if (hasflag) {
		if (tintglow < 15)
			tintglowtype = tintglow;
		else if (tintglow < 30)
			tintglowtype = 30 - tintglow;
		else if (tintglow > 45 && tintglow < 60)
			tintglowtype = tintglow - 45;
		else if (tintglow >= 60 && tintglow < 75)
			tintglowtype = 75 - tintglow;
		else
			tintglowtype = 0;
			
		if (hasflags[0] && hasflags[1]) {
			if (tintglow < 15 || tintglow > 60)
				TintScreen(BestColor (DefaultPalette->basecolors, (int)(255/15)*tintglowtype,
					(int)(255/15)*tintglowtype, 255, DefaultPalette->numcolors));
			else
				TintScreen(BestColor (DefaultPalette->basecolors, 255,
					(int)(255/15)*tintglowtype, (int)(255/15)*tintglowtype, DefaultPalette->numcolors));
		}
		else if (hasflags[0])
			TintScreen(BestColor (DefaultPalette->basecolors, (int)(255/15)*tintglowtype,
				(int)(255/15)*tintglowtype, 255, DefaultPalette->numcolors));		
		else if (hasflags[1])
			TintScreen(BestColor (DefaultPalette->basecolors, 255,
				(int)(255/15)*tintglowtype, (int)(255/15)*tintglowtype, DefaultPalette->numcolors));
		else if (hasflags[2])
			TintScreen(BestColor (DefaultPalette->basecolors, 255,
				255, (int)(255/15)*tintglowtype, DefaultPalette->numcolors));
	}
}


VERSION_CONTROL (cl_ctf_cpp, "$Id$")


