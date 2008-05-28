// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2008 by The Odamex Team.
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
//		Functions and data used for 'Capture The Flag' mode.
//
// ----------------------------------------------------------------------------


#include "sv_main.h"
#include "m_random.h"
#include "sv_ctf.h"
#include "s_sound.h"
#include "i_system.h"

bool G_CheckSpot (player_t &player, mapthing2_t *mthing);

EXTERN_CVAR (usectf)
extern int shotclock;

CVAR(ctf_manualreturn, "0", CVAR_ARCHIVE)
CVAR(ctf_flagathometoscore, "1", CVAR_ARCHIVE)
CVAR(ctf_flagtimeout, "600", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE) // Flag timeout in gametics

flagdata CTFdata[NUMFLAGS];
int TEAMpoints[NUMFLAGS];
bool TEAMenabled[NUMFLAGS];

bool ctfmode = false;

// denis - this is a lot clearer than doubly nested switches
static mobjtype_t flag_table[NUMFLAGS][NUMFLAGSTATES] =
{
	{MT_BFLG, MT_BDWN, MT_BCAR},
	{MT_RFLG, MT_RDWN, MT_RCAR},
	{MT_GFLG, MT_GDWN, MT_GCAR}
};

char *team_names[NUMTEAMS + 2] =
{
	"BLUE", "RED", "GOLD",
	"", ""
};

static int ctf_points[NUM_CTF_SCORE] =
{
	0, // NONE
	0, // REFRESH
	1, // KILL
	-1, // BETRAYAL
	1, // GRAB
	3, // FIRSTGRAB
	3, // CARRIERKILL
	2, // RETURN
	10, // CAPTURE
	0 // DROP
};

//
//	[Toke - CTF] CTF_Load
//	Loads CTF mode
//
void CTF_Load (void)
{
	if (!usectf) return;

	for(size_t i = 0; i < NUMFLAGS; i++)
		TEAMpoints[i] = 0;

	ctfmode = true;
}

//
//	[Toke - CTF] CTF_Unload
//	Unloads CTF mode
//
void CTF_Unload (void)
{
	ctfmode = false;
}

//
//	[Toke - CTF] SV_CTFEvent
//	Sends CTF events to player
//
void SV_CTFEvent (flag_t f, flag_score_t event, player_t &who)
{
	if(event == SCORE_NONE)
		return;

	if(validplayer(who))
		who.points += ctf_points[event];

	for (size_t i = 0; i < players.size(); ++i)
	{
		if (!players[i].ingame())
			continue;

		client_t *cl = &players[i].client;

		MSG_WriteMarker (&cl->reliablebuf, svc_ctfevent);
		MSG_WriteByte (&cl->reliablebuf, event);
		MSG_WriteByte (&cl->reliablebuf, f);

		if(validplayer(who))
		{
			MSG_WriteByte (&cl->reliablebuf, who.id);
			MSG_WriteLong (&cl->reliablebuf, who.points);
		}
		else
		{
			MSG_WriteByte (&cl->reliablebuf, 0);
			MSG_WriteLong (&cl->reliablebuf, 0);
		}

		for(size_t j = 0; j < NUMFLAGS; j++)
			MSG_WriteLong (&cl->reliablebuf, TEAMpoints[j]);
	}

	CTF_Sound (f, event);
}

//
// CTF_Connect
// Send states of all flags to new player
//
void CTF_Connect(player_t &player)
{
	client_t *cl = &player.client;

	MSG_WriteMarker (&cl->reliablebuf, svc_ctfevent);
	MSG_WriteByte (&cl->reliablebuf, SCORE_NONE);

	for(size_t i = 0; i < NUMFLAGS; i++)
	{
		MSG_WriteByte (&cl->reliablebuf, CTFdata[i].state);
		MSG_WriteByte (&cl->reliablebuf, CTFdata[i].flagger);
	}

	// send team scores to the client
	SV_CTFEvent ((flag_t)0, SCORE_REFRESH, player);
}

//
//	[Toke - CTF] SV_FlagGrab
//	Event of a player picking up a flag
//
void SV_FlagGrab (player_t &player, flag_t f, bool firstgrab)
{
	player.flags[f] = true;
	CTFdata[f].flagger = player.id;
	CTFdata[f].state = flag_carried;
	CTFdata[f].pickup_time = I_MSTime();

	if (player.userinfo.team != (team_t)f) {
		if (firstgrab) {
			SV_BroadcastPrintf (PRINT_HIGH, "%s has taken the %s flag\n", player.userinfo.netname, team_names[f]);
			SV_CTFEvent (f, SCORE_FIRSTGRAB, player);
		} else {
			SV_BroadcastPrintf (PRINT_HIGH, "%s picked up the %s flag\n", player.userinfo.netname, team_names[f]);
			SV_CTFEvent (f, SCORE_GRAB, player);
		}
	} else {
		SV_BroadcastPrintf (PRINT_HIGH, "%s is recovering the %s flag\n", player.userinfo.netname, team_names[f]);
		SV_CTFEvent (f, SCORE_GRAB, player);
	}
}

//
//	[Toke - CTF] SV_FlagReturn
//	Returns the flag to its socket
//
void SV_FlagReturn (player_t &player, flag_t f)
{
	SV_CTFEvent (f, SCORE_RETURN, player);

	CTF_SpawnFlag (f);

	SV_BroadcastPrintf (PRINT_HIGH, "%s has returned the %s flag\n", player.userinfo.netname, team_names[f]);
}

//
// CTF_TimeMSG
// denis - tells players how long someone held onto a flag
//
static const char *CTF_TimeMSG(unsigned int milliseconds)
{
	static char msg[64];

	milliseconds /= 10;
	int ms = milliseconds%100;
	milliseconds -= ms;
	milliseconds /= 100;
	int se = milliseconds%60;
	milliseconds -= se;
	milliseconds /= 60;
	int mi = milliseconds;

	sprintf((char *)&msg, "%d:%.2d.%.2d", mi, se, ms);

	return msg;
}

//
//	[Toke - CTF] SV_FlagScore
//	Event of a player capturing the flag
//
void SV_FlagScore (player_t &player, flag_t f)
{
	TEAMpoints[player.userinfo.team]++;

	SV_CTFEvent (f, SCORE_CAPTURE, player);

	int time_held = I_MSTime() - CTFdata[f].pickup_time;

	SV_BroadcastPrintf (PRINT_HIGH, "%s has captured the %s flag (held for %s)\n", player.userinfo.netname, team_names[f], CTF_TimeMSG(time_held));

	player.flags[f] = false; // take scoring player's flag
	CTFdata[f].flagger = 0;

	CTF_SpawnFlag(f);

	// checks to see if a team won a game
	if(TEAMpoints[player.userinfo.team] >= scorelimit)
	{
		SV_BroadcastPrintf (PRINT_HIGH, "Score limit reached. %s team wins!\n", team_names[player.userinfo.team]);
		shotclock = TICRATE*2;
	}
}

//
// SV_FlagTouch
// Event of a player touching a flag, called from P_TouchSpecial
//
bool SV_FlagTouch (player_t &player, flag_t f, bool firstgrab)
{
	if (shotclock)
		return false;

	if(player.userinfo.team == (team_t)f)
	{
		if(CTFdata[f].state == flag_home)
		{
			// score?
			for(size_t i = 0; i < NUMFLAGS; i++)
				if(player.userinfo.team != (team_t)i && player.flags[i] && ctf_flagathometoscore)
					SV_FlagScore(player, (flag_t)i);

			return false;
		}
		else
		{
			if (ctf_manualreturn)
				SV_FlagGrab(player, f, firstgrab);
			else
				SV_FlagReturn(player, f);
		}
	}
	else
	{
		SV_FlagGrab(player, f, firstgrab);
	}

	// returning true should make P_TouchSpecial destroy the touched flag
	return true;
}

//
// SV_SocketTouch
// Event of a player touching a socket, called from P_TouchSpecial
//
void SV_SocketTouch (player_t &player, flag_t f)
{
	if (shotclock)
		return;

	if (player.userinfo.team == (team_t)f && player.flags[f]) {
		player.flags[f] = false; // take ex-carrier's flag
		CTFdata[f].flagger = 0;
		SV_FlagReturn(player, f);
	}
	
	for(size_t i = 0; i < NUMFLAGS; i++)
		if(player.userinfo.team == (team_t)f && player.userinfo.team != (team_t)i &&
			player.flags[i] && !ctf_flagathometoscore)
			SV_FlagScore(player, (flag_t)i);
}

//
//	[Toke - CTF] SV_FlagDrop
//	Event of a player dropping a flag
//
void SV_FlagDrop (player_t &player, flag_t f)
{
	if (shotclock)
		return;

	SV_CTFEvent (f, SCORE_DROP, player);

	int time_held = I_MSTime() - CTFdata[f].pickup_time;

	SV_BroadcastPrintf (PRINT_HIGH, "%s has dropped the %s flag (held for %s)\n", player.userinfo.netname, team_names[f], CTF_TimeMSG(time_held));

	player.flags[f] = false; // take ex-carrier's flag
	CTFdata[f].flagger = 0;

	CTF_SpawnDroppedFlag (f,
				   player.mo->x,
				   player.mo->y,
				   player.mo->z);
}

//
//	[Toke - CTF] SV_FlagSetup
//	Sets up flags at the start of each map
//
void SV_FlagSetup (void)
{
	for(size_t i = 0; i < NUMFLAGS; i++)
		if(TEAMenabled[i])
			CTF_SpawnFlag((flag_t)i);
}

//
//	[Toke - CTF] CTF_RunTics
//	Runs once per gametic when ctf is enabled
//
void CTF_RunTics (void)
{
	if (shotclock)
		return;

	for(size_t i = 0; i < NUMFLAGS; i++)
	{
		flagdata *data = &CTFdata[i];

		if(data->state != flag_dropped)
			continue;

		if(data->timeout--)
			continue;

		if(data->actor)
			data->actor->Destroy();

		SV_CTFEvent ((flag_t)i, SCORE_RETURN, idplayer(0));

		SV_BroadcastPrintf (PRINT_HIGH, "%s flag returned.\n", team_names[i]);

		CTF_SpawnFlag((flag_t)i);
	}
}

//
//	[Toke - CTF] CTF_SpawnFlag
//	Spawns a flag of the designated type in the designated location
//
void CTF_SpawnFlag (flag_t f)
{
	flagdata *data = &CTFdata[f];

	AActor *flag = new AActor (data->x, data->y, data->z, flag_table[f][flag_home]);

	SV_SpawnMobj(flag);

	data->actor = flag->ptr();
	data->state = flag_home;
	data->flagger = 0;
}

//
// CTF_SpawnDroppedFlag
// Spawns a dropped flag
//
void CTF_SpawnDroppedFlag (flag_t f, int x, int y, int z)
{
	flagdata *data = &CTFdata[f];

	AActor *flag = new AActor (x, y, z, flag_table[f][flag_dropped]);

	SV_SpawnMobj(flag);

	data->actor = flag->ptr();
	data->state = flag_dropped;
	data->timeout = ctf_flagtimeout;
	data->flagger = 0;
}

//
//	[Toke - CTF] CTF_CheckFlags
//	Checks to see if player has any flags in his inventory, if so it spawns that flag on players location
//
void CTF_CheckFlags (player_t &player)
{
	for(size_t i = 0; i < NUMFLAGS; i++)
		if(player.flags[i])
			SV_FlagDrop (player, (flag_t)i);
}

//
//	[Toke - CTF] CTF_RememberFlagPos
//	Remembers the position of flag sockets
//
void CTF_RememberFlagPos (mapthing2_t *mthing)
{
	if (!ctfmode)
		return;

	flag_t f;

	switch(mthing->type)
	{
		case ID_BLUE_FLAG: f = it_blueflag; break;
		case ID_RED_FLAG: f = it_redflag; break;
		case ID_GOLD_FLAG: f = it_goldflag; break;
		default:
			return;
	}

	flagdata *data = &CTFdata[f];

	data->x = mthing->x << FRACBITS;
	data->y = mthing->y << FRACBITS;
	data->z = mthing->z << FRACBITS;
}

//
//	[Toke - CTF] CTF_SelectTeamPlaySpot
//	Randomly selects a team spawn point
//
mapthing2_t *CTF_SelectTeamPlaySpot (player_t &player, int selections)
{
	int i, j;

	for (j = 0; j < MaxBlueTeamStarts; j++)
	{
		i = M_Random () % selections;
		if (player.userinfo.team == TEAM_BLUE)
		{
			if (G_CheckSpot (player, &blueteamstarts[i]) )
			{
				return &blueteamstarts[i];
			}
		}

		if (player.userinfo.team == TEAM_RED)
		{
			if (G_CheckSpot (player, &redteamstarts[i]) )
			{
				return &redteamstarts[i];
			}
		}

		if (player.userinfo.team == TEAM_GOLD)
		{
			if (G_CheckSpot (player, &goldteamstarts[i]) )
			{
				return &goldteamstarts[i];
			}
		}
	}

	return &blueteamstarts[0];
}

// sounds played differ depending on your team, [0] for event on own team, [1] for others
static const char *flag_sound[NUM_CTF_SCORE][2] =
{
	{"", ""}, // NONE
	{"", ""}, // REFRESH
	{"", ""}, // KILL
	{"", ""}, // BETRAYAL
	{"ctf/f-flaggrab", "ctf/e-flaggrab"}, // GRAB
	{"ctf/f-flaggrab", "ctf/e-flaggrab"}, // FIRSTGRAB
	{"", ""}, // CARRIERKILL
	{"ctf/f-flagreturn", "ctf/e-flagreturn"}, // RETURN
	{"ctf/f-flagscore", "ctf/e-flagscore"}, // CAPTURE
	{"ctf/f-flagdrop", "ctf/e-flagdrop"}, // DROP
};

//
//	[Toke - CTF] CTF_Sound
//	Plays the flag event sounds
//
void CTF_Sound (flag_t f, flag_score_t event)
{
	for(size_t i = 0; i < NUMTEAMS; i++)
	{
		const char *snd = flag_sound[event][f == (flag_t)i];
		if(snd && *snd)
			SV_SoundTeam (CHAN_VOICE, snd, ATTN_NONE, i);
	}
}


VERSION_CONTROL (sv_ctf_cpp, "$Id$")

