// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2024 by The Odamex Team.
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


#include "odamex.h"

#include "c_console.h"
#include "c_dispatch.h"
#include "cl_main.h"
#include "w_wad.h"
#include "z_zone.h"
#include "i_video.h"
#include "v_video.h"
#include "p_local.h"
#include "p_inter.h"
#include "p_ctf.h"
#include "st_stuff.h"
#include "s_sound.h"
#include "v_text.h"
#include "g_gametype.h"

static int tintglow = 0;

// denis - this is a lot clearer than doubly nested switches
static mobjtype_t flag_table[NUMTEAMS][NUMFLAGSTATES] =
{
	{MT_BFLG, MT_BDWN, MT_BCAR},
	{MT_RFLG, MT_RDWN, MT_RCAR},
	{MT_GFLG, MT_GDWN, MT_GCAR}
};

EXTERN_CVAR (screenblocks)
EXTERN_CVAR (hud_gamemsgtype)
EXTERN_CVAR (hud_heldflag)
EXTERN_CVAR (hud_heldflag_flash)
EXTERN_CVAR (sv_teamsinplay)

//
// CTF_Connect
// Receive states of all flags
//
void CTF_Connect()
{

}

//	CTF_CheckFlags
//																					[Toke - CTF - carry]
//	Checks player for flags
//
void CTF_CheckFlags (player_t &player)
{
	for(size_t i = 0; i < NUMTEAMS; i++)
	{
		if(player.flags[i])
		{
			player.flags[i] = false;
			GetTeamInfo((team_t)i)->FlagData.flagger = 0;
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
void CTF_CarryFlag (player_t &player, team_t flag)
{
	if (!validplayer(player))
		return;

	TeamInfo* teamInfo = GetTeamInfo(flag);

	player.flags[flag] = true;
	teamInfo->FlagData.flagger = player.id;
	teamInfo->FlagData.state = flag_carried;

	AActor *actor = new AActor(0, 0, 0, flag_table[flag][flag_carried]);
	teamInfo->FlagData.actor = actor->ptr();

	CTF_MoveFlags();
}

//
//	[Toke - CTF] CTF_MoveFlag
//	Moves the flag that is linked to a player
//
void CTF_MoveFlags ()
{
	// denis - flag is now a boolean
	for(size_t i = 0; i < NUMTEAMS; i++)
	{
		TeamInfo* teamInfo = GetTeamInfo((team_t)i);

		if(teamInfo->FlagData.flagger && teamInfo->FlagData.actor)
		{
			player_t &player = idplayer(teamInfo->FlagData.flagger);
			AActor *flag = teamInfo->FlagData.actor;

			if (!validplayer(player) || !player.mo)
			{
				// [SL] 2012-12-13 - Remove a flag if it's being carried but
				// there's not a valid player carrying it (should not happen)
				teamInfo->FlagData.flagger = 0;
				teamInfo->FlagData.state = flag_home;
				if(teamInfo->FlagData.actor)
					teamInfo->FlagData.actor->Destroy();
				continue;
			}

			unsigned an = player.mo->angle >> ANGLETOFINESHIFT;
			fixed_t x = (player.mo->x + FixedMul (-2*FRACUNIT, finecosine[an]));
			fixed_t y = (player.mo->y + FixedMul (-2*FRACUNIT, finesine[an]));

			CL_MoveThing(flag, x, y, player.mo->z);
		}
		else
		{
			// [AM] The flag isn't actually being held by anybody, so if
			// anything is in CTFdata[i].actor it's a ghost and should
			// be cleaned up.
			if(teamInfo->FlagData.actor)
				teamInfo->FlagData.actor->Destroy();
		}
	}
}

static void TintScreen(argb_t color)
{
	int surface_width = I_GetSurfaceWidth(), surface_height = I_GetSurfaceHeight();
	int thickness = std::min(surface_height / 100, surface_width / 100);

	// draw border around the screen excluding the status bar
	// NOTE: status bar is not currently drawn when spectating
	if (R_StatusBarVisible())
		surface_height = ST_StatusBarY(surface_width, surface_height);

	if (hud_heldflag == 1)
	{
		screen->Clear(0, 0, thickness, surface_height, color);
		screen->Clear(0, 0, surface_width, thickness, color);
		screen->Clear(surface_width - thickness, 0, surface_width, surface_height, color);
	}
	if (hud_heldflag > 0)
	{
		screen->Clear (0, surface_height - thickness, surface_width, surface_height, color);
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

	// Don't draw the flag the display player is carrying as it blocks the view.
	for (size_t flag = 0; flag < NUMTEAMS; flag++)
	{
		TeamInfo* teamInfo = GetTeamInfo((team_t)flag);

		if (!teamInfo->FlagData.actor)
			continue;

		if (teamInfo->FlagData.flagger == displayplayer().id &&
			teamInfo->FlagData.state == flag_carried)
		{
			teamInfo->FlagData.actor->flags2 |= MF2_DONTDRAW;
		}
		else
		{
			teamInfo->FlagData.actor->flags2 &= ~MF2_DONTDRAW;
		}
	}
}

//
//	[Toke - CTF - Hud] CTF_DrawHud
//	Draws the CTF Hud, duH
//
void CTF_DrawHud (void)
{
    int tintglowtype = 0;
	team_t yourFlag = NUMTEAMS;
	team_t enemyFlag = NUMTEAMS;

	if(sv_gametype != GM_CTF)
		return;

	player_t &player = displayplayer();
	for(size_t i = 0; i < NUMTEAMS; i++)
	{
		TeamInfo* teamInfo = GetTeamInfo((team_t)i);

		if(teamInfo->FlagData.state == flag_carried && teamInfo->FlagData.flagger == player.id)
		{
			if (teamInfo->Team == player.userinfo.team)
				yourFlag = teamInfo->Team;
			else
				enemyFlag = teamInfo->Team;
		}
	}

	if ((yourFlag != NUMTEAMS || enemyFlag != NUMTEAMS) && hud_heldflag > 0)
	{
		if (hud_heldflag_flash == 1)
		{
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
		}

		argb_t tintColor = 0;
		if (yourFlag != NUMTEAMS && enemyFlag != NUMTEAMS)
		{
			if (tintglow < 15 || tintglow > 60)
				tintColor = GetTeamInfo(yourFlag)->Color;
			else
				tintColor = GetTeamInfo(enemyFlag)->Color;
		}
		else if (enemyFlag != NUMTEAMS)
		{
			tintColor = GetTeamInfo(enemyFlag)->Color;
		}
		else if (yourFlag != NUMTEAMS)
		{
			tintColor = GetTeamInfo(yourFlag)->Color;
		}

		if (tintColor != 0)
		{
			if (tintColor.getr() != 255)
				tintColor.setr(17 * tintglowtype);
			if (tintColor.getg() != 255)
				tintColor.setg(17 * tintglowtype);
			if (tintColor.getb() != 255)
				tintColor.setb(17 * tintglowtype);
			TintScreen(tintColor);
		}
	}
}

FArchive &operator<< (FArchive &arc, flagdata &flag)
{
	int netid = flag.actor ? flag.actor->netid : 0;
	
	arc << flag.flaglocated
		<< netid
		<< flag.flagger
		<< flag.pickup_time
		<< flag.x << flag.y << flag.z
		<< flag.timeout
		<< static_cast<byte>(flag.state)
		<< flag.sb_tick;
		
	arc << 0;

	return arc;
}

FArchive &operator>> (FArchive &arc, flagdata &flag)
{
	int netid;
	byte state;
	int dummy;
	
	arc >> flag.flaglocated
		>> netid
		>> flag.flagger
		>> flag.pickup_time
		>> flag.x >> flag.y >> flag.z
		>> flag.timeout
		>> state
		>> flag.sb_tick;
		
	arc >> dummy;
	
	flag.state = static_cast<flag_state_t>(state);
	flag.actor = AActor::AActorPtr();

	return arc;
}

// 0: Your sound effect
// 1: Enemy sound effect
// 2: Your announcer
// 3: Enemy announcer
// 4: Blue team announcer
// 5: Red team announcer
// 6: Green team announcer
static const char *flag_sound[NUM_CTF_SCORE][7] = {
	{"", "", "", "", "", ""}, // NONE
	{"", "", "", "", "", ""}, // REFRESH
	{"", "", "", "", "", ""}, // KILL
	{"", "", "", "", "", ""}, // BETRAYAL
	{"ctf/your/flag/take", "ctf/enemy/flag/take", "vox/your/flag/take", "vox/enemy/flag/take", "vox/blue/flag/take", "vox/red/flag/take", "vox/green/flag/take"}, // GRAB
	{"ctf/your/flag/take", "ctf/enemy/flag/take", "vox/your/flag/take", "vox/enemy/flag/take", "vox/blue/flag/take", "vox/red/flag/take", "vox/green/flag/take"}, // FIRSTGRAB
	{"", "", "", "", "", ""}, // CARRIERKILL
	{"ctf/your/flag/return", "ctf/enemy/flag/return", "vox/your/flag/return", "vox/enemy/flag/return", "vox/blue/flag/return", "vox/red/flag/return", "vox/green/flag/return"}, // RETURN
	{"ctf/your/score", "ctf/enemy/score", "vox/your/score", "vox/enemy/score", "vox/blue/score", "vox/red/score", "vox/green/score"}, // CAPTURE
	{"ctf/your/flag/drop", "ctf/enemy/flag/drop", "vox/your/flag/drop", "vox/enemy/flag/drop", "vox/blue/flag/drop", "vox/red/flag/drop", "vox/green/flag/drop"}, // DROP
	{"ctf/your/flag/manualreturn", "ctf/enemy/flag/manualreturn", "vox/your/flag/manualreturn", "vox/enemy/flag/manualreturn", "vox/blue/flag/manualreturn", "vox/red/flag/manualreturn", "vox/green/flag/manualreturn"}, // MANUALRETURN
};

EXTERN_CVAR(snd_voxtype)
EXTERN_CVAR(snd_gamesfx)

enum PossessiveType
{
	Yours,
	Theirs
};

// If two teams in play then the event is possessive
// For more than two teams the event will only be possessive for your team events only
bool IsPossesiveEvent(team_t playerTeam, team_t flag, team_t team, flag_score_t ev)
{
	if (sv_teamsinplay.asInt() == 2)
		return true;

	if (ev == SCORE_CAPTURE && team == playerTeam)
		return true;

	return flag == playerTeam;
}

// [AM] Play appropriate sounds for CTF events.
void CTF_Sound(team_t flag, team_t team, flag_score_t ev)
{
	if (flag >= NUMTEAMS || team >= NUMTEAMS || ev >= NUM_CTF_SCORE || strcmp(flag_sound[ev][0], "") == 0)
		return;

	// Play sound effect
	if (snd_gamesfx)
	{
		PossessiveType sound = Theirs;
		bool nonScoreEvent = ev != SCORE_CAPTURE;
		bool yourFlag = consoleplayer().userinfo.team == flag;
		bool yourTeam = consoleplayer().userinfo.team == team;

		if (nonScoreEvent && yourFlag)
			sound = Yours;
		else if (ev == SCORE_CAPTURE && yourTeam)
			sound = Yours;

		// Do not play sound if enemy is grabbing another enemy's flag etc
		if ((ev == SCORE_RETURN || !(nonScoreEvent && !yourTeam && !yourFlag)) && S_FindSound(flag_sound[ev][sound]) != -1)
			S_Sound(CHAN_GAMEINFO, flag_sound[ev][sound], 1, ATTN_NONE);
	}

	// Play announcer sound
	switch (snd_voxtype.asInt())
	{
	case 2:
		// Possessive (yours/theirs)
		if (!consoleplayer().spectator)
		{
			team_t playerTeam = consoleplayer().userinfo.team;
			PossessiveType sound = Theirs;

			if (ev == SCORE_CAPTURE && playerTeam == team)
				sound = Yours;
			else if ((ev == SCORE_RETURN || ev == SCORE_MANUALRETURN || ev == SCORE_DROP || ev == SCORE_GRAB || ev == SCORE_FIRSTGRAB) && playerTeam == flag)
				sound = Yours;

			if (IsPossesiveEvent(playerTeam, flag, team, ev) && S_FindSound(flag_sound[ev][2 + sound]))
			{
				S_Sound(CHAN_ANNOUNCER, flag_sound[ev][2 + sound], 1, ATTN_NONE);
				break;
			}
		}
		// fallthrough
	case 1:
	{
		int sound = flag;
		if (ev == SCORE_CAPTURE)
			sound = team;

		if (S_FindSound(flag_sound[ev][4 + sound]) != -1) 
			S_Sound(CHAN_ANNOUNCER, flag_sound[ev][4 + sound], 1, ATTN_NONE);
		break;
	}
		// fallthrough
	default:
		break;
	}
}

static const char* flag_message[NUM_CTF_SCORE][5] = {
	{"", "", "", "", ""}, // NONE
	{"", "", "", "", ""}, // REFRESH
	{"", "", "", "", ""}, // KILL
	{"", "", "", "", ""}, // BETRAYAL
	{"Your Flag Taken", "Enemy Flag Taken", "Blue Flag Taken", "Red Flag Taken", "Green Flag Taken"}, // GRAB
	{"Your Flag Taken", "Enemy Flag Taken", "Blue Flag Taken", "Red Flag Taken",  "Green Flag Taken"}, // FIRSTGRAB
	{"", "", "", "", ""}, // CARRIERKILL
	{"Your Flag Returned", "Enemy Flag Returned", "Blue Flag Returned", "Red Flag Returned", "Green Flag Returned"}, // RETURN
	{"Your Team Scores", "Enemy Team Scores", "Blue Team Scores", "Red Team Scores" , "Green Team Scores"}, // CAPTURE
	{"Your Flag Dropped", "Enemy Flag Dropped", "Blue Flag Dropped", "Red Flag Dropped", "Green Flag Dropped"}, // DROP
	{"Your Flag Returning", "Enemy Flag Returning", "Blue Flag Returning", "Red Flag Returning", "Green Flag Returning"} // MANUALRETURN*/
};

void CTF_Message(team_t flag, team_t team, flag_score_t ev)
{
	if (flag >= NUMTEAMS || ev >= NUM_CTF_SCORE || strcmp(flag_sound[ev][0], "") == 0)
		return;

	int color = CR_GREY;

	switch (hud_gamemsgtype.asInt())
	{
	case 2:
		// Possessive (yours/theirs)
		if (!consoleplayer().spectator)
		{
			PossessiveType msg = Theirs;
			color = CR_BRICK;

			team_t playerTeam = consoleplayer().userinfo.team;

			if (ev == SCORE_RETURN)
			{
				if (playerTeam == flag)
				{
					msg = Yours;
					color = CR_GREEN;
				}
				else
				{
					msg = Theirs;
					color = CR_BRICK;
				}
			}
			else if (ev == SCORE_CAPTURE || ev == SCORE_MANUALRETURN)
			{
				if (playerTeam == team)
				{
					msg = Yours;
					color = CR_GREEN;
				}
				else
				{
					msg = Theirs;
					color = CR_BRICK;
				}
			}
			else if (ev == SCORE_DROP)
			{
				if (playerTeam == flag)
				{
					msg = Yours;
					color = CR_BRICK;
				}
				else
				{
					msg = Theirs;
					color = CR_GREEN;
				}
			}
			else if (ev == SCORE_GRAB || ev == SCORE_FIRSTGRAB)
			{
				if (playerTeam == flag)
				{
					msg = Yours;
					color = CR_BRICK;
				}
				else
				{
					msg = Theirs;
					if (playerTeam == team)
						color = CR_GREEN;
					else
						color = CR_BRICK;
				}
			}

			if (IsPossesiveEvent(playerTeam, flag, team, ev))
			{
				C_GMidPrint(flag_message[ev][msg], color, 0);
				break;
			}
		}
		// fallthrough
	case 1:
		if (ev == SCORE_CAPTURE)
			C_GMidPrint(flag_message[ev][2 + team], V_GetTextColor(GetTeamInfo(team)->TextColor.c_str()), 0);
		else
			C_GMidPrint(flag_message[ev][2 + flag], V_GetTextColor(GetTeamInfo(flag)->TextColor.c_str()), 0);
		break;
	default:
		break;
	}
}

VERSION_CONTROL (cl_ctf_cpp, "$Id$")
