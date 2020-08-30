// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
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
//		Functions and data used for 'Capture The Flag' mode.
//
// ----------------------------------------------------------------------------


#include "sv_main.h"
#include "m_random.h"
#include "p_ctf.h"
#include "i_system.h"
#include "g_levelstate.h"
#include "p_unlag.h"
#include "m_wdlstats.h"

bool G_CheckSpot (player_t &player, mapthing2_t *mthing);

extern int shotclock;

EXTERN_CVAR (sv_teamsinplay)
EXTERN_CVAR (sv_scorelimit)
EXTERN_CVAR (ctf_manualreturn)
EXTERN_CVAR (ctf_flagathometoscore)
EXTERN_CVAR (ctf_flagtimeout)

// denis - this is a lot clearer than doubly nested switches
static mobjtype_t flag_table[NUMTEAMS][NUMFLAGSTATES] =
{
	{MT_BFLG, MT_BDWN, MT_BCAR},
	{MT_RFLG, MT_RDWN, MT_RCAR},
	{MT_GFLG, MT_GDWN, MT_GCAR}
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
	0, // DROP
	1 // MANUALRETURN
};

//
//	[Toke - CTF] SV_CTFEvent
//	Sends CTF events to player
//
void SV_CTFEvent (team_t f, flag_score_t event, player_t &who)
{
	if(event == SCORE_NONE)
		return;

	if(validplayer(who) && ::levelstate.checkScoreChange())
		who.points += ctf_points[event];

	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		client_t *cl = &(it->client);

		MSG_WriteMarker (&cl->reliablebuf, svc_ctfevent);
		MSG_WriteByte (&cl->reliablebuf, event);
		MSG_WriteByte (&cl->reliablebuf, f);
		if (who.userinfo.team == TEAM_NONE)
			MSG_WriteByte(&cl->reliablebuf, f);
		else
			MSG_WriteByte(&cl->reliablebuf, who.userinfo.team);

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

		for(size_t j = 0; j < NUMTEAMS; j++)
			MSG_WriteLong (&cl->reliablebuf, GetTeamInfo((team_t)j)->Points);
	}
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

	for(size_t i = 0; i < NUMTEAMS; i++)
	{
		TeamInfo* teamInfo = GetTeamInfo((team_t)i);
		MSG_WriteByte (&cl->reliablebuf, teamInfo->FlagData.state);
		MSG_WriteByte (&cl->reliablebuf, teamInfo->FlagData.flagger);
	}

	// send team scores to the client
	SV_CTFEvent ((team_t)0, SCORE_REFRESH, player);
}

//
//	[Toke - CTF] SV_FlagGrab
//	Event of a player picking up a flag
//
ItemEquipVal SV_FlagGrab (player_t &player, team_t f, bool firstgrab)
{
	// Manual return allows you to carry your flag and an opponents flag
	if (f != player.userinfo.team)
	{
		for (int i = 0; i < NUMTEAMS; i++)
		{
			// Already carrying an enemy flag, can't pick up more than one
			if ((team_t)i != player.userinfo.team && player.flags[i])
				return IEV_NotEquipped;
		}
	}

	TeamInfo* teamInfo = GetTeamInfo(f);

	player.flags[f] = true;
	teamInfo->FlagData.flagger = player.id;
	teamInfo->FlagData.state = flag_carried;
	teamInfo->FlagData.pickup_time = I_MSTime();

	if (player.userinfo.team != f){
		if (firstgrab) {
			teamInfo->FlagData.firstgrab = true;
			SV_BroadcastPrintf (PRINT_HIGH, "%s has taken the %s flag\n", player.userinfo.netname.c_str(), teamInfo->ColorStringUpper.c_str());
			SV_CTFEvent (f, SCORE_FIRSTGRAB, player);
			M_LogWDLEvent(WDL_EVENT_TOUCH, &player, NULL, 0, 0, 0);
		} else {
			teamInfo->FlagData.firstgrab = false;
			SV_BroadcastPrintf (PRINT_HIGH, "%s picked up the %s flag\n", player.userinfo.netname.c_str(), teamInfo->ColorStringUpper.c_str());
			SV_CTFEvent (f, SCORE_GRAB, player);
			M_LogWDLEvent(WDL_EVENT_PICKUPTOUCH, &player, NULL, 0, 0, 0);
		}
	} else {
		SV_BroadcastPrintf (PRINT_HIGH, "%s is recovering the %s flag\n", player.userinfo.netname.c_str(), teamInfo->ColorStringUpper.c_str());
		SV_CTFEvent (f, SCORE_MANUALRETURN, player);
	}

	return IEV_EquipRemove;
}

//
//	[Toke - CTF] SV_FlagReturn
//	Returns the flag to its socket
//
void SV_FlagReturn (player_t &player, team_t f)
{
	SV_CTFEvent (f, SCORE_RETURN, player);

	CTF_SpawnFlag (f);

	SV_BroadcastPrintf (PRINT_HIGH, "%s has returned the %s flag\n", player.userinfo.netname.c_str(), GetTeamInfo(f)->ColorStringUpper.c_str());
	M_LogWDLEvent(WDL_EVENT_RETURNFLAG, &player, NULL, 0, 0, 0);
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

extern void P_GiveTeamPoints(player_t* player, int num);

//
//	[Toke - CTF] SV_FlagScore
//	Event of a player capturing the flag
//
void SV_FlagScore (player_t &player, team_t f)
{
	P_GiveTeamPoints(&player, 1);

	SV_CTFEvent (f, SCORE_CAPTURE, player);

	TeamInfo* teamInfo = GetTeamInfo(f);
	int time_held = I_MSTime() - teamInfo->FlagData.pickup_time;

	SV_BroadcastPrintf(PRINT_HIGH, "%s has captured the %s flag (held for %s)\n", player.userinfo.netname.c_str(), GetTeamInfo(f)->ColorStringUpper.c_str(), CTF_TimeMSG(time_held));
	if (teamInfo->FlagData.firstgrab)
		M_LogWDLEvent(WDL_EVENT_CAPTURE, &player, NULL, 0, 0, 0);
	else
		M_LogWDLEvent(WDL_EVENT_PICKUPCAPTURE, &player, NULL, 0, 0, 0);

	player.flags[f] = false; // take scoring player's flag
	teamInfo->FlagData.flagger = 0;
	teamInfo->FlagData.firstgrab = false;

	CTF_SpawnFlag(f);

	// checks to see if a team won a game
	if (GetTeamInfo(player.userinfo.team)->Points >= sv_scorelimit &&
	    sv_scorelimit != 0 && ::levelstate.checkEndGame())
	{
		SV_BroadcastPrintf(PRINT_HIGH, "Score limit reached. %s team wins!\n",
		                   GetTeamInfo(player.userinfo.team)->ColorStringUpper.c_str());
		M_CommitWDLLog();
		::levelstate.endGame();
	}
}

//
// SV_FlagTouch
// Event of a player touching a flag, called from P_TouchSpecial
//
ItemEquipVal SV_FlagTouch (player_t &player, team_t f, bool firstgrab)
{
	if (shotclock)
		return IEV_NotEquipped;

	if(player.userinfo.team == f)
	{
		if(GetTeamInfo(f)->FlagData.state == flag_home) // Do nothing.
		{
			return IEV_NotEquipped;
		}
		else // Returning team flag.
		{
			if (ctf_manualreturn)
			{
				return SV_FlagGrab(player, f, firstgrab);	
			}
			else
				SV_FlagReturn(player, f);
		}
	}
	else // Grabbing enemy flag.
	{
		return SV_FlagGrab(player, f, firstgrab);
	}

	// returning IEV_EquipRemove should make P_TouchSpecial destroy the touched flag
	return IEV_EquipRemove;
}

//
// SV_SocketTouch
// Event of a player touching a socket, called from P_TouchSpecial
//
void SV_SocketTouch (player_t &player, team_t f)
{
	if (shotclock)
		return;

	TeamInfo* teamInfo = GetTeamInfo(f);

	// Returning team's flag manually.
	if (player.userinfo.team == f && player.flags[f]) {
		player.flags[f] = false;
		teamInfo->FlagData.flagger = 0;
		teamInfo->FlagData.firstgrab = false;
		SV_FlagReturn(player, f);
	}

	// Scoring with enemy flag.
	for (size_t i = 0; i < NUMTEAMS; i++)
	{
		if (player.userinfo.team == f && player.userinfo.team != (team_t)i &&
			player.flags[i] && (!ctf_flagathometoscore || teamInfo->FlagData.state == flag_home))
		{
			SV_FlagScore(player, (team_t)i);
		}
	}
}

//
//	[Toke - CTF] SV_FlagDrop
//	Event of a player dropping a flag
//
void SV_FlagDrop (player_t &player, team_t f)
{
	if (shotclock)
		return;

	SV_CTFEvent (f, SCORE_DROP, player);

	TeamInfo* teamInfo = GetTeamInfo(f);
	int time_held = I_MSTime() - teamInfo->FlagData.pickup_time;

	SV_BroadcastPrintf (PRINT_HIGH, "%s has dropped the %s flag (held for %s)\n", player.userinfo.netname.c_str(), GetTeamInfo(f)->ColorStringUpper.c_str(), CTF_TimeMSG(time_held));

	player.flags[f] = false; // take ex-carrier's flag
	teamInfo->FlagData.flagger = 0;
	teamInfo->FlagData.firstgrab = false;

	fixed_t x, y, z;
	Unlag::getInstance().getCurrentPlayerPosition(player.id, x, y, z);
	CTF_SpawnDroppedFlag(f, x, y, z);
}

//
//	[Toke - CTF] CTF_RunTics
//	Runs once per gametic when ctf is enabled
//
void CTF_RunTics (void)
{
	if (shotclock || gamestate != GS_LEVEL)
		return;

	for(size_t i = 0; i < NUMTEAMS; i++)
	{
		TeamInfo* teamInfo = GetTeamInfo((team_t)i);

		if(teamInfo->FlagData.state != flag_dropped)
			continue;

		if (!ctf_flagtimeout)
			continue;

		if(teamInfo->FlagData.timeout--)
			continue;

		if(teamInfo->FlagData.actor)
			teamInfo->FlagData.actor->Destroy();

		SV_CTFEvent (teamInfo->Team, SCORE_RETURN, idplayer(0));

		SV_BroadcastPrintf (PRINT_HIGH, "%s flag returned.\n", teamInfo->ColorStringUpper.c_str());

		CTF_SpawnFlag(teamInfo->Team);
	}
}

//
//	[Toke - CTF] CTF_SpawnFlag
//	Spawns a flag of the designated type in the designated location
//
void CTF_SpawnFlag (team_t f)
{
	TeamInfo* teamInfo = GetTeamInfo(f);

	AActor *flag = new AActor (teamInfo->FlagData.x, teamInfo->FlagData.y, teamInfo->FlagData.z, flag_table[f][flag_home]);

	SV_SpawnMobj(flag);

	teamInfo->FlagData.actor = flag->ptr();
	teamInfo->FlagData.state = flag_home;
	teamInfo->FlagData.flagger = 0;
	teamInfo->FlagData.firstgrab = false;
}

//
// CTF_SpawnDroppedFlag
// Spawns a dropped flag
//
void CTF_SpawnDroppedFlag (team_t f, int x, int y, int z)
{
	TeamInfo* teamInfo = GetTeamInfo(f);

	AActor *flag = new AActor (x, y, z, flag_table[f][flag_dropped]);

	SV_SpawnMobj(flag);

	teamInfo->FlagData.actor = flag->ptr();
	teamInfo->FlagData.state = flag_dropped;
	teamInfo->FlagData.timeout = (size_t)(ctf_flagtimeout * TICRATE);
	teamInfo->FlagData.flagger = 0;
	teamInfo->FlagData.firstgrab = false;
}

//
//	[Toke - CTF] CTF_CheckFlags
//	Checks to see if player has any flags in his inventory, if so it spawns that flag on players location
//
void CTF_CheckFlags (player_t &player)
{
	for (size_t i = 0; i < NUMTEAMS; i++)
	{
		if (player.flags[i])
			SV_FlagDrop(player, (team_t)i);
	}
}

//
//	[Toke - CTF] CTF_RememberFlagPos
//	Remembers the position of flag sockets
//
void CTF_RememberFlagPos (mapthing2_t *mthing)
{
	for (int iTeam = 0; iTeam < NUMTEAMS; iTeam++)
	{
		TeamInfo* teamInfo = GetTeamInfo((team_t)iTeam);
		if (mthing->type == teamInfo->FlagThingNum)
		{
			teamInfo->FlagData.x = mthing->x << FRACBITS;
			teamInfo->FlagData.y = mthing->y << FRACBITS;
			teamInfo->FlagData.z = mthing->z << FRACBITS;

			teamInfo->FlagData.flaglocated = true;
			break;
		}
	}
}

FArchive &operator<< (FArchive &arc, flagdata &flag)
{
	return arc;
}

FArchive &operator>> (FArchive &arc, flagdata &flag)
{
	return arc;
}

VERSION_CONTROL (sv_ctf_cpp, "$Id$")
