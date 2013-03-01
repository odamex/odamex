// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2012 by The Odamex Team.
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

#include	"c_console.h"
#include	"c_dispatch.h"
#include	"cl_main.h"
#include	"w_wad.h"
#include	"z_zone.h"
#include	"v_video.h"
#include	"p_local.h"
#include	"p_inter.h"
#include	"p_ctf.h"
#include	"p_mobj.h"
#include    "st_stuff.h"
#include	"s_sound.h"
#include	"v_text.h"

flagdata CTFdata[NUMFLAGS];
int TEAMpoints[NUMFLAGS];

static int tintglow = 0;

const char *team_names[NUMTEAMS + 2] =
{
	"BLUE", "RED", "", ""
};

// denis - this is a lot clearer than doubly nested switches
static mobjtype_t flag_table[NUMFLAGS][NUMFLAGSTATES] =
{
	{MT_BFLG, MT_BDWN, MT_BCAR},
	{MT_RFLG, MT_RDWN, MT_RCAR}
};

EXTERN_CVAR		(screenblocks)
EXTERN_CVAR		(st_scale)
EXTERN_CVAR		(hud_gamemsgtype)

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
		CTFdata[i].flagger = 0;
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
		case SCORE_MANUALRETURN:
			if(validplayer(player))
			{
				CTF_CarryFlag(player, flag);
				if (player.id == displayplayer().id)
					player.bonuscount = BONUSADD;
			}
			break;

		case SCORE_CAPTURE:
			if (validplayer(player))
			{
				player.flags[flag] = 0;
			}

			CTFdata[flag].flagger = 0;
			CTFdata[flag].state = flag_home;
			if(CTFdata[flag].actor)
				CTFdata[flag].actor->Destroy();
			break;

		case SCORE_RETURN:
			if (validplayer(player))
			{
				player.flags[flag] = 0;
			}

			CTFdata[flag].flagger = 0;
			CTFdata[flag].state = flag_home;
			if(CTFdata[flag].actor)
				CTFdata[flag].actor->Destroy();
			break;

		case SCORE_DROP:
			if (validplayer(player))
			{
				player.flags[flag] = 0;
			}

			CTFdata[flag].flagger = 0;
			CTFdata[flag].state = flag_dropped;
			if(CTFdata[flag].actor)
				CTFdata[flag].actor->Destroy();
			break;
	}

	// [AM] Play CTF sound, moved from server.
	CTF_Sound(flag, event);

	// [AM] Show CTF message.
	CTF_Message(flag, event);
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

	AActor *actor = new AActor(0, 0, 0, flag_table[flag][flag_carried]);
	CTFdata[flag].actor = actor->ptr();

	CTF_MoveFlags();
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

			if (!validplayer(player) || !player.mo)
			{
				// [SL] 2012-12-13 - Remove a flag if it's being carried but
				// there's not a valid player carrying it (should not happen)
				CTFdata[i].flagger = 0;
				CTFdata[i].state = flag_home;
				if(CTFdata[i].actor)
					CTFdata[i].actor->Destroy();
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
			if(CTFdata[i].actor)
				CTFdata[i].actor->Destroy();
		}
	}
}

void TintScreen(int color)
{
	// draw border around the screen excluding the status bar
	// NOTE: status bar is not currently drawn when spectating
	if (screenblocks < 11 && !consoleplayer().spectator)
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

	// if there's no status bar, draw border around the full screen
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

	// Don't draw the flag the display player is carrying as it blocks the view.
	for (size_t flag = 0; flag < NUMFLAGS; flag++)
	{
		if (!CTFdata[flag].actor)
			continue;

		if (CTFdata[flag].flagger == displayplayer().id && 
			CTFdata[flag].state == flag_carried)
		{
			CTFdata[flag].actor->flags2 |= MF2_DONTDRAW;
		}
		else
		{
			CTFdata[flag].actor->flags2 &= ~MF2_DONTDRAW;
		}
	}
}

//
//	[Toke - CTF - Hud] CTF_DrawHud
//	Draws the CTF Hud, duH
//
void CTF_DrawHud (void)
{
    int tintglowtype;
    bool hasflag = false, hasflags[NUMFLAGS];

	if(sv_gametype != GM_CTF)
		return;

	player_t &player = displayplayer();
	for(size_t i = 0; i < NUMFLAGS; i++)
	{
		hasflags[i] = false;
		if(CTFdata[i].state == flag_carried && CTFdata[i].flagger == player.id)
		{
			hasflag = true;
			hasflags[i] = true;
		}
	}

	if (hasflag) {
		palette_t *pal = GetDefaultPalette();

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
				TintScreen(BestColor(pal->basecolors, (int)(255/15)*tintglowtype,
					(int)(255/15)*tintglowtype, 255, pal->numcolors));
			else
				TintScreen(BestColor(pal->basecolors, 255,
					(int)(255/15)*tintglowtype, (int)(255/15)*tintglowtype, pal->numcolors));
		}
		else if (hasflags[0])
			TintScreen(BestColor(pal->basecolors, (int)(255/15)*tintglowtype,
				(int)(255/15)*tintglowtype, 255, pal->numcolors));
		else if (hasflags[1])
			TintScreen(BestColor(pal->basecolors, 255,
				(int)(255/15)*tintglowtype, (int)(255/15)*tintglowtype, pal->numcolors));
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

// [AM] Clientside CTF sounds.
// 0: Team-agnostic SFX
// 1: Own team announcer
// 2: Enemy team announcer
// 3: Blue team announcer
// 4: Red team announcer
static const char *flag_sound[NUM_CTF_SCORE][6] = {
	{"", "", "", "", "", ""}, // NONE
	{"", "", "", "", "", ""}, // REFRESH
	{"", "", "", "", "", ""}, // KILL
	{"", "", "", "", "", ""}, // BETRAYAL
	{"ctf/your/flag/take", "ctf/enemy/flag/take", "vox/your/flag/take", "vox/enemy/flag/take", "vox/blue/flag/take", "vox/red/flag/take"}, // GRAB
	{"ctf/your/flag/take", "ctf/enemy/flag/take", "vox/your/flag/take", "vox/enemy/flag/take", "vox/blue/flag/take", "vox/red/flag/take"}, // FIRSTGRAB
	{"", "", "", "", "", ""}, // CARRIERKILL
	{"ctf/your/flag/return", "ctf/enemy/flag/return", "vox/your/flag/return", "vox/enemy/flag/return", "vox/blue/flag/return", "vox/red/flag/return"}, // RETURN
	{"ctf/enemy/score", "ctf/your/score", "vox/enemy/score", "vox/your/score", "vox/red/score", "vox/blue/score"}, // CAPTURE
	{"ctf/your/flag/drop", "ctf/enemy/flag/drop", "vox/your/flag/drop", "vox/enemy/flag/drop", "vox/blue/flag/drop", "vox/red/flag/drop"}, // DROP
	{"ctf/your/flag/manualreturn", "ctf/enemy/flag/manualreturn", "vox/your/flag/manualreturn", "vox/enemy/flag/manualreturn", "vox/blue/flag/manualreturn", "vox/red/flag/manualreturn"}, // MANUALRETURN
};

EXTERN_CVAR(snd_voxtype)
EXTERN_CVAR(snd_gamesfx)

// [AM] Play appropriate sounds for CTF events.
void CTF_Sound(flag_t flag, flag_score_t event) {
	if (flag >= NUMFLAGS) {
		// Invalid team
		return;
	}

	if (event >= NUM_CTF_SCORE) {
		// Invalid CTF event
		return;
	}

	if (strcmp(flag_sound[event][0], "") == 0) {
		// No logical sound for this event
		return;
	}

	// Play sound effect
	if (snd_gamesfx) {
		if (consoleplayer().spectator || consoleplayer().userinfo.team != (team_t)flag) {
			// Enemy flag is being evented
			if (S_FindSound(flag_sound[event][1]) != -1) {
				S_Sound(CHAN_GAMEINFO, flag_sound[event][1], 1, ATTN_NONE);
			}
		} else {
			// Your flag is being evented
			if (S_FindSound(flag_sound[event][0]) != -1) {
				S_Sound(CHAN_GAMEINFO, flag_sound[event][0], 1, ATTN_NONE);
			}
		}
	}

	// Play announcer sound
	switch (snd_voxtype.asInt()) {
	case 2:
		// Possessive (yours/theirs)
		if (!consoleplayer().spectator) {
			if (consoleplayer().userinfo.team != (team_t)flag) {
				// Enemy flag is being evented
				if (S_FindSound(flag_sound[event][3]) != -1) {
					S_Sound(CHAN_ANNOUNCER, flag_sound[event][3], 1, ATTN_NONE);
					break;
				}
			} else {
				// Your flag is being evented
				if (S_FindSound(flag_sound[event][2]) != -1) {
					S_Sound(CHAN_ANNOUNCER, flag_sound[event][2], 1, ATTN_NONE);
					break;
				}
			}
		}
		// fallthrough
	case 1:
		// Team colors (red/blue)
		if (S_FindSound(flag_sound[event][4 + flag]) != -1) {
			if (consoleplayer().userinfo.team != (team_t)flag && !consoleplayer().spectator) {
				S_Sound(CHAN_ANNOUNCER, flag_sound[event][4 + flag], 1, ATTN_NONE);
			} else {
				S_Sound(CHAN_ANNOUNCER, flag_sound[event][4 + flag], 1, ATTN_NONE);
			}
			break;
		}
		// fallthrough
	default:
		break;
	}
}

static const char* flag_message[NUM_CTF_SCORE][4] = {
	{"", "", "", ""}, // NONE
	{"", "", "", ""}, // REFRESH
	{"", "", "", ""}, // KILL
	{"", "", "", ""}, // BETRAYAL
	{"Your Flag Taken", "Enemy Flag Taken", "Blue Flag Taken", "Red Flag Taken"}, // GRAB
	{"Your Flag Taken", "Enemy Flag Taken", "Blue Flag Taken", "Red Flag Taken"}, // FIRSTGRAB
	{"", "", "", ""}, // CARRIERKILL
	{"Your Flag Returned", "Enemy Flag Returned", "Blue Flag Returned", "Red Flag Returned"}, // RETURN
	{"Enemy Team Scores", "Your Team Scores", "Red Team Scores", "Blue Team Scores"}, // CAPTURE
	{"Your Flag Dropped", "Enemy Flag Dropped", "Blue Flag Dropped", "Red Flag Dropped"}, // DROP
	{"Your Flag Returning", "Enemy Flag Returning", "Blue Flag Returning", "Red Flag Returning"} // MANUALRETURN*/
};

void CTF_Message(flag_t flag, flag_score_t event) {
	// Invalid team
	if (flag >= NUMFLAGS)
		return;

	// Invalid CTF event
	if (event >= NUM_CTF_SCORE)
		return;

	// No message for this event
	if (strcmp(flag_sound[event][0], "") == 0)
		return;

	int color = CR_GREY;

	// Show message 
	switch (hud_gamemsgtype.asInt()) {
	case 2:
		// Possessive (yours/theirs)
		if (!consoleplayer().spectator) {
			if (consoleplayer().userinfo.team != (team_t)flag) {
				// Enemy flag is being evented
				if (event == SCORE_GRAB || event == SCORE_FIRSTGRAB || event == SCORE_CAPTURE)
					color = CR_GREEN;
				else
					color = CR_BRICK;
				C_GMidPrint(flag_message[event][1], color, 0);
			} else {
				// Friendly flag is being evented
				if (event == SCORE_GRAB || event == SCORE_FIRSTGRAB || event == SCORE_CAPTURE)
					color = CR_BRICK;
				else
					color = CR_GREEN;
				C_GMidPrint(flag_message[event][0], color, 0);
			}
			break;
		}
		// fallthrough
	case 1:
		// Team colors (red/blue)
		if (event == SCORE_CAPTURE) {
			if (flag == it_blueflag)
				color = CR_RED;
			else
				color = CR_BLUE;
		} else {
			if (flag == it_blueflag)
				color = CR_BLUE;
			else
				color = CR_RED;
		}
		
		C_GMidPrint(flag_message[event][2 + flag], color, 0);
		break;
	default:
		break;
	}
}

VERSION_CONTROL (cl_ctf_cpp, "$Id$")
