// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2010 by The Odamex Team.
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
//	Heads-up displays
//
//-----------------------------------------------------------------------------

#include <algorithm>

#include "doomdef.h"
#include "z_zone.h"
#include "m_swap.h"
#include "hu_stuff.h"
#include "w_wad.h"
#include "s_sound.h"
#include "doomstat.h"
#include "st_stuff.h"
#include "gstrings.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "c_cvars.h"
#include "v_text.h"
#include "v_video.h"
#include "cl_main.h"
#include "p_ctf.h"
#include "i_video.h"
#include "i_input.h"

#define QUEUESIZE		128
#define HU_INPUTX		0
#define HU_INPUTY		(0 + (SHORT(hu_font[0]->height) +1))

#define CTFBOARDWIDTH	236
#define CTFBOARDHEIGHT	40

#define DMBOARDWIDTH	368
#define DMBOARDHEIGHT	16

#define TEAMPLAYBORDER	10
#define DMBORDER		20

DCanvas *odacanvas = NULL;
extern	DCanvas *screen;

EXTERN_CVAR (hud_scaletext)
EXTERN_CVAR (sv_fraglimit)
EXTERN_CVAR (sv_timelimit)
EXTERN_CVAR (sv_scorelimit)

int V_TextScaleXAmount();
int V_TextScaleYAmount();

// Chat
void HU_Init (void);
void HU_Drawer (void);
BOOL HU_Responder (event_t *ev);

patch_t *hu_font[HU_FONTSIZE];

void HU_DrawScores (player_t *plyr);
void HU_ConsoleScores (player_t *plyr);

// [Toke - Scores]
void HU_DMScores1 (player_t *player);
void HU_DMScores2 (player_t *player);
void HU_TeamScores1 (player_t *player);
void HU_TeamScores2 (player_t *player);

extern bool HasBehavior;
extern inline int V_StringWidth (const char *str);
size_t P_NumPlayersInGame();
static void ShoveChatStr (std::string str, byte who);

static std::string input_text;
int headsupactive;
BOOL altdown;

EXTERN_CVAR (chatmacro0)
EXTERN_CVAR (chatmacro1)
EXTERN_CVAR (chatmacro2)
EXTERN_CVAR (chatmacro3)
EXTERN_CVAR (chatmacro4)
EXTERN_CVAR (chatmacro5)
EXTERN_CVAR (chatmacro6)
EXTERN_CVAR (chatmacro7)
EXTERN_CVAR (chatmacro8)
EXTERN_CVAR (chatmacro9)

cvar_t *chat_macros[10] =
{
	&chatmacro0,
	&chatmacro1,
	&chatmacro2,
	&chatmacro3,
	&chatmacro4,
	&chatmacro5,
	&chatmacro6,
	&chatmacro7,
	&chatmacro8,
	&chatmacro9
};

//
// HU_Init
//
void HU_Init (void)
{
	int i, j, sub;
	const char *tplate = "STCFN%.3d";
	char buffer[12];

	headsupactive = 0;
	input_text = "";

	// load the heads-up font
	j = HU_FONTSTART;
	sub = 0;

	for (i = 0; i < HU_FONTSIZE; i++)
	{
		sprintf (buffer, tplate, j++ - sub);
		hu_font[i] = W_CachePatch(buffer, PU_STATIC);
	}
}

//
// HU_Responder
//
BOOL HU_Responder (event_t *ev)
{
	unsigned char c;

	if (ev->data1 == KEY_RALT || ev->data1 == KEY_LALT || ev->data1 == KEY_HAT1)
	{
		altdown = (ev->type == ev_keydown);
		return false;
	}
	else if (ev->data1 == KEY_LSHIFT || ev->data1 == KEY_RSHIFT)
	{
		return false;
	}
	else if (  (gamestate != GS_LEVEL && gamestate != GS_INTERMISSION)
		     || ev->type != ev_keydown)
	{
		if (headsupactive)
        {
            return true;
        }

		return false;
	}

	if (!headsupactive)
		return false;

	c = ev->data3;	// [RH] Use localized keymap

	// send a macro
	if (altdown)
	{
		if ((ev->data2 >= '0' && ev->data2 <= '9') || (ev->data2 >= KEY_JOY1 && ev->data2 <= KEY_JOY10))
		{
			if (ev->data2 >= KEY_JOY1 && ev->data2 <= KEY_JOY10)
				ShoveChatStr (chat_macros[ev->data2 - KEY_JOY1]->cstring(), headsupactive - 1);
			else
				ShoveChatStr (chat_macros[ev->data2 - '0']->cstring(), headsupactive - 1);

			I_ResumeMouse();
			headsupactive = 0;
			return true;
		}
	}
	if (ev->data3 == KEY_ENTER)
	{
		ShoveChatStr (input_text, headsupactive - 1);
        I_ResumeMouse();
		headsupactive = 0;
		return true;
	}
	else if (ev->data1 == KEY_ESCAPE || ev->data1 == KEY_JOY2)
	{
        I_ResumeMouse();
		headsupactive = 0;
		return true;
	}
	else if (ev->data1 == KEY_BACKSPACE)
	{
		if (input_text.length())
			input_text.erase(input_text.end() - 1);

		return true;
	}
	else
	{
		if(!c)
			return false;

		if(input_text.length() < MAX_CHATSTR_LEN)
			input_text += c;

		return true;
	}

	return false;
}

EXTERN_CVAR (screenblocks)

EXTERN_CVAR (hud_targetnames)

CVAR_FUNC_IMPL(hud_targetcount)
{
	if (var < 0)
		var.Set((float)0);

	if (var > 64)
		var.Set((float)64);
}

EXTERN_CVAR (sv_allowtargetnames)

// GhostlyDeath -- From Strawberry-Doom
#include <math.h>
int MobjToMobjDistance(AActor *a, AActor *b)
{
	double x1, x2;
	double y1, y2;
	double z1, z2;

	if (a && b)
	{
		x1 = a->x >> FRACBITS;
		x2 = b->x >> FRACBITS;
		y1 = a->y >> FRACBITS;
		y2 = b->y >> FRACBITS;
		z1 = a->z >> FRACBITS;
		z2 = b->z >> FRACBITS;

		return (int)sqrt(
			pow(x2 - x1, 2) +
			pow(y2 - y1, 2) +
			pow(z2 - z1, 2)
			);
	}

	return 0;
}

// GhostlyDeath -- structure for sorting
typedef struct
{
	player_t* PlayPtr;
	int Distance;
	int Color;
} TargetInfo_t;

// GhostlyDeath -- Move this into it's own function!
void HU_DrawTargetNames(void)
{
	int ProposedColor = CR_GREY;
	int TargetX = 0;
	int TargetY = screen->height - ((hu_font[0]->height() + 4) * CleanYfac);
	std::vector<TargetInfo_t> Targets;
	size_t i;

	if(!displayplayer().mo)
		return;

	// TargetY is a special case because we don't want to go crazy over the status bar
	if (screenblocks <= 10 && !consoleplayer().spectator)
		TargetY -= ST_HEIGHT;
	else if (consoleplayer().spectator)
		TargetY -= ((hu_font[0]->height() + 4) * CleanYfac);	// Don't get in the Join messages way!

	// Sometimes the "Other person's name" will get blocked
	if (&(consoleplayer()) != &(displayplayer()))
		TargetY -= ((hu_font[0]->height() + 4) * CleanYfac);

	for (i = 0; i < players.size(); i++)
	{
		/* Check Various things */
		// Spectator?
		if (players[i].spectator)
			continue;

		// Don't get the display player!
		if (&(players[i]) == &(displayplayer()))
			continue;

		/* Now if they are visible... */
		if (players[i].mo && players[i].mo->health > 0)
		{
			// If they are beyond 512 units, ignore
			if (MobjToMobjDistance(displayplayer().mo, players[i].mo) > 512)
				continue;

			// Check to see if the other player is visible
			if (HasBehavior)
			{
                if (!P_CheckSightEdges2(displayplayer().mo, players[i].mo, 0.0))
                    continue;
			}
			else
			{
                if (!P_CheckSightEdges(displayplayer().mo, players[i].mo, 0.0))
                    continue;
			}

			// GhostlyDeath -- Don't draw dead enemies
			if (!consoleplayer().spectator &&
				(players[i].mo->health <= 0))
			{
				if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
				{
					if ((players[i].userinfo.team != displayplayer().userinfo.team) ||
						(displayplayer().userinfo.team == TEAM_NONE) ||
						(players[i].userinfo.team == TEAM_NONE))
							continue;
				}
				else
				{
					if (sv_gametype != GM_COOP)
						continue;
				}
			}

			/* Now we need to figure out if they are infront of us */
			// Taken from r_things.cpp and I have no clue what it does
			fixed_t tr_x, tr_y, gxt, gyt, tx, tz, xscale;
			extern fixed_t FocalLengthX;

			// transform the origin point
			tr_x = players[i].mo->x - viewx;
			tr_y = players[i].mo->y - viewy;

			gxt = FixedMul (tr_x,viewcos);
			gyt = -FixedMul (tr_y,viewsin);

			tz = gxt-gyt;

			// thing is behind view plane?
			if (tz < (FRACUNIT*4))
				continue;

			xscale = FixedDiv (FocalLengthX, tz);

			gxt = -FixedMul (tr_x, viewsin);
			gyt = FixedMul (tr_y, viewcos);
			tx = -(gyt+gxt);

			// too far off the side?
			if (abs(tx)>(tz>>1))
				continue;

			// Are we a friend or foe or are we a spectator ourself?
			if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
			{
				switch (players[i].userinfo.team)
				{
					case TEAM_BLUE: ProposedColor = CR_BLUE; break;
					case TEAM_RED: ProposedColor = CR_RED; break;
					default: ProposedColor = CR_GREY; break;
				}
			}
			else
			{
				if (consoleplayer().spectator)
					ProposedColor = CR_GREY;	// Gray as noone is a friend nor foe
				else
				{
					if (sv_gametype != GM_COOP)
						ProposedColor = CR_RED;
					else
						ProposedColor = CR_GREEN;
				}
			}

			// Ok, make the temporary player info then add it
			TargetInfo_t Tmp = {&players[i], MobjToMobjDistance(displayplayer().mo, players[i].mo), ProposedColor};
			Targets.push_back(Tmp);
		}
	}

	// GhostlyDeath -- Now Sort (hopefully I got my selection sort working!)
	for (i = 0; i < Targets.size(); i++)
	{
		for (size_t j = i + 1; j < Targets.size(); j++)
		{
			if (Targets[j].Distance < Targets[i].Distance)
			{
				player_t* PlayPtr = Targets[i].PlayPtr;
				int Distance = Targets[i].Distance;
				int Color = Targets[i].Color;
				Targets[i].PlayPtr = Targets[j].PlayPtr;
				Targets[i].Distance = Targets[j].Distance;
				Targets[i].Color = Targets[j].Color;
				Targets[j].PlayPtr = PlayPtr;
				Targets[j].Distance = Distance;
				Targets[j].Color = Color;
			}
		}
	}

	// GhostlyDeath -- Now Draw
	for (i = 0; (i < Targets.size()) && (i < hud_targetcount); i++)
	{
		// So "You" (or not) is centered
		if (Targets[i].PlayPtr == &(consoleplayer()))
			TargetX = (screen->width - V_StringWidth ("You")*CleanXfac) >> 1;
		else
			TargetX = (screen->width - V_StringWidth (Targets[i].PlayPtr->userinfo.netname)*CleanXfac) >> 1;

		// Draw the Player's name or You! (Personally, I like the You part - GhostlyDeath)
		if (Targets[i].PlayPtr->mo->health > 0)
			screen->DrawTextClean(Targets[i].Color,
				TargetX,
				TargetY,
				(Targets[i].PlayPtr == &(consoleplayer()) ? "You" : Targets[i].PlayPtr->userinfo.netname));
		else
			screen->DrawTextCleanLuc(Targets[i].Color,
				TargetX,
				TargetY,
				(Targets[i].PlayPtr == &(consoleplayer()) ? "You" : Targets[i].PlayPtr->userinfo.netname));

		TargetY -= ((hu_font[0]->height() + 1) * CleanYfac);
	}
}

EXTERN_CVAR (sv_maxplayers)

//
// HU_Drawer
//
void HU_Drawer (void)
{

	// Draw "Press USE to join" as the bottom layer.
	if ((&consoleplayer())->spectator && (level.time / TICRATE)%2 && gamestate != GS_INTERMISSION)
	{
		setsizeneeded = true;
		int YPos = screen->height - ((hu_font[0]->height() + 4) * CleanYfac);

		if (&consoleplayer() != &displayplayer())
			YPos -= ((hu_font[0]->height() + 4) * CleanYfac);

		if (P_NumPlayersInGame() < sv_maxplayers)
		{
			static const std::string joinmsg("Press USE to join");

			screen->DrawTextClean(CR_GREEN,
					(screen->width - V_StringWidth(joinmsg.c_str())*CleanXfac) >> 1,
					YPos, joinmsg.c_str());
		}
	}

	/* GhostlyDeath -- Cheap Target Names */
	if ((gamestate == GS_LEVEL)						// Must be Playing, allow specs to target always
		&& ((!consoleplayer().spectator && sv_allowtargetnames && hud_targetnames) ||
		 (consoleplayer().spectator && hud_targetnames))
		 )
	{
		HU_DrawTargetNames();
	}

	if (headsupactive)
	{
		static const char *prompt;
		int i, x, c, scalex, y, promptwidth;

		if (hud_scaletext)
		{
			scalex = V_TextScaleXAmount();
			y = (!viewactive ? -30 : -10) * CleanYfac;
		}
		else
		{
			scalex = 1;
			y = (!viewactive ? -30 : -10);
		}

		y += (screen->height == realviewheight && viewactive) ? screen->height : ST_Y;

		if (headsupactive == 2)
			prompt = "Say (TEAM): ";
		else if (headsupactive == 1)
			prompt = "Say: ";

		promptwidth = V_StringWidth (prompt) * scalex;
		x = hu_font['_' - HU_FONTSTART]->width() * scalex * 2 + promptwidth;

		// figure out if the text is wider than the screen->
		// if so, only draw the right-most portion of it.
		for (i = input_text.length() - 1; i >= 0 && x < screen->width; i--)
		{
			c = toupper(input_text[i] & 0x7f) - HU_FONTSTART;
			if (c < 0 || c >= HU_FONTSIZE)
			{
				x += 4 * scalex;
			}
			else
			{
				x += hu_font[c]->width() * scalex;
			}
		}

		if (i >= 0)
			i++;
		else
			i = 0;

		// draw the prompt, text, and cursor
		std::string show_text = input_text;
		show_text += '_';
		if (hud_scaletext)
		{
			screen->DrawTextStretched (	CR_RED, 0, y, prompt, 
										V_TextScaleXAmount(), V_TextScaleYAmount());
			screen->DrawTextStretched (	CR_GREY, promptwidth, y, show_text.c_str() + i,
										V_TextScaleXAmount(), V_TextScaleYAmount());
		}
		else
		{
			screen->DrawText (CR_RED, 0, y, prompt);
			screen->DrawText (CR_GREY, promptwidth, y, show_text.c_str() + i);
		}
	}

	if(multiplayer && consoleplayer().camera && !(demoplayback && democlassic))
    {
        if (((Actions[ACTION_SHOWSCORES]) ||
             ((consoleplayer().camera->health <= 0 && !(&consoleplayer())->spectator) && gamestate != GS_INTERMISSION)))
        {
            HU_DrawScores (&consoleplayer());
        }
    }

	// [csDoom] draw disconnected wire [Toke] Made this 1337er
	// denis - moved to hu_stuff and uncommented
	if (noservermsgs && (gamestate == GS_INTERMISSION || gamestate == GS_LEVEL) )
	{
		patch_t *netlag = W_CachePatch ("NET");
		screen->DrawPatchCleanNoMove (netlag, 50*CleanXfac, CleanYfac);

		// SoM: Not here.
		//screen->Dim ();
	}
}

static void ShoveChatStr (std::string str, byte who)
{
	// Do not send this chat message if the chat string is empty
	if (str.length() == 0)
		return;

	if(str.length() > MAX_CHATSTR_LEN)
		str.resize(MAX_CHATSTR_LEN);

	MSG_WriteMarker (&net_buffer, clc_say);
	MSG_WriteByte (&net_buffer, who);
	MSG_WriteString (&net_buffer, str.c_str());
}

BEGIN_COMMAND (messagemode)
{
	if(!connected)
		return;

	headsupactive = 1;
	C_HideConsole ();
    I_PauseMouse();
	input_text = "";
}
END_COMMAND (messagemode)

BEGIN_COMMAND (say)
{
	if (argc > 1)
	{
		std::string chat = BuildString (argc - 1, (const char **)(argv + 1));
		ShoveChatStr (chat, 0);
	}
}
END_COMMAND (say)

BEGIN_COMMAND (messagemode2)
{
	if(!connected || (sv_gametype != GM_TEAMDM && sv_gametype != GM_CTF && !consoleplayer().spectator))
		return;

	headsupactive = 2;
	C_HideConsole ();
	I_PauseMouse();
	input_text = "";
}
END_COMMAND (messagemode2)

BEGIN_COMMAND (say_team)
{
	if (argc > 1)
	{
		std::string chat = BuildString (argc - 1, (const char **)(argv + 1));
		ShoveChatStr (chat, 1);
	}
}
END_COMMAND (say_team)

static bool STACK_ARGS compare_player_frags (const player_t *arg1, const player_t *arg2)
{
	return arg2->fragcount < arg1->fragcount;
}

static bool STACK_ARGS compare_player_kills (const player_t *arg1, const player_t *arg2)
{
	return arg2->killcount < arg1->killcount;
}

static bool STACK_ARGS compare_player_points (const player_t *arg1, const player_t *arg2)
{
	return arg2->points < arg1->points;
}

EXTERN_CVAR (hud_usehighresboard)

//
// [Toke - Scores] HU_DrawScores
// Decides which scoreboard to draw
//
void HU_DrawScores (player_t *player)
{
	if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
	{
		if (hud_usehighresboard)
		{
			if (screen->width > (CTFBOARDWIDTH * 2)) // If board will fit
				HU_TeamScores2 (player);
			else
				HU_TeamScores1 (player);
		}
		else
			HU_TeamScores1 (player);
	}
	else
	{
		if (hud_usehighresboard && screen->width != 320)
			HU_DMScores2 (player);
		else
			HU_DMScores1 (player);
	}
}

void HU_DisplayTimer(int x, int y, bool scale)
{
	int timeleft, hours, minutes, seconds;
	char str[80];

	if (sv_gametype != GM_COOP && level.timeleft && gamestate == GS_LEVEL)
	{
		timeleft = level.timeleft;

		if (timeleft < 0)
			timeleft = 0;

		hours = timeleft / (TICRATE * 3600);
		timeleft -= hours * TICRATE * 3600;
		minutes = timeleft / (TICRATE * 60);
		timeleft -= minutes * TICRATE * 60;
		seconds = timeleft / TICRATE;

		if (hours)
			sprintf (str, "Level ends in %02d:%02d:%02d", hours, minutes, seconds);
		else
			sprintf (str, "Level ends in %02d:%02d", minutes, seconds);

		if (scale)
			screen->DrawTextClean (CR_GREY, x, y, str);
		else
			screen->DrawText (CR_GREY, x, y, str);
	}
}

//
// [Toke - Scores] HU_DMScores1
//	Draws low-res DM scores
//
void HU_DMScores1 (player_t *player)
{
	char str[80];
	std::vector<player_t *> sortedplayers(players.size());
	int y;
	unsigned int k, i;

	if (player->camera->player)
		player = player->camera->player;

	// Player list sorting
	for (k = 0; k < sortedplayers.size(); k++)
		sortedplayers[k] = &players[k];

	if(sv_gametype != GM_COOP)
		std::sort(sortedplayers.begin(), sortedplayers.end(), compare_player_frags);
	else
		std::sort(sortedplayers.begin(), sortedplayers.end(), compare_player_kills);

	y = 34 * CleanYfac;

	// Scoreboard Identify
	// Dan - Tells which current game mode is being played
	if (sv_gametype != GM_COOP)
		screen->DrawTextClean (CR_GOLD,120 * CleanXfac,4 * CleanYfac,"Deathmatch");
	else
		screen->DrawTextClean (CR_GOLD,120 * CleanXfac,4 * CleanYfac,"Cooperative");

	//	Timelimit display
	HU_DisplayTimer(screen->width/2 - 126/2*CleanXfac, y - 20 * CleanYfac);

	//	Header display
	screen->DrawTextClean (CR_GREY,	16	* CleanXfac,	25	* CleanYfac, "NAME");

	if(sv_gametype != GM_COOP)
		screen->DrawTextClean (CR_GREY,	146	* CleanXfac,	25	* CleanYfac, "FRAG");
	else
		screen->DrawTextClean (CR_GREY,	146	* CleanXfac,	25	* CleanYfac, "KILL");

	if(sv_gametype != GM_COOP)
		screen->DrawTextClean (CR_GREY,	192	* CleanXfac,	25	* CleanYfac, "F/D");
	else
        	screen->DrawTextClean (CR_GREY,	192	* CleanXfac,	25	* CleanYfac, "K/D");

	screen->DrawTextClean (CR_GREY,	221	* CleanXfac,	25	* CleanYfac, "DTH");

	screen->DrawTextClean (CR_GREY,	252	* CleanXfac,	25	* CleanYfac, "PING");

	screen->DrawTextClean (CR_GREY,	287	* CleanXfac,	25	* CleanYfac, "TIME");

	//	Variable display
	for (i = 0; ((i < sortedplayers.size()) && (y < ST_Y - 12 * CleanYfac)); i++)
	{
		int color = sortedplayers[i]->userinfo.color;

		if (sortedplayers[i]->ingame())
		{
			if (screen->is8bit())		// Finds the best solid color based on a players color
				color = BestColor (DefaultPalette->basecolors, RPART(color), GPART(color), BPART(color), DefaultPalette->numcolors);

			// Display Color
			if (!sortedplayers[i]->spectator && sortedplayers[i]->playerstate != PST_CONTACT && sortedplayers[i]->playerstate != PST_DOWNLOAD)
				screen->Clear ((5 * CleanXfac), y, (13 * CleanXfac), y + hu_font[0]->height() * CleanYfac, color);

			// Display Frags or Kills if coop
			if(sv_gametype != GM_COOP)
				sprintf (str, "%d", sortedplayers[i]->fragcount);
			else
				sprintf (str, "%d", sortedplayers[i]->killcount);
			screen->DrawTextClean (sortedplayers[i] == player ? CR_GREEN : CR_BRICK, (177 - V_StringWidth (str)) * CleanXfac, y, str);

			// Display Frags/Deaths or Kills/Deaths ratio
			if(sv_gametype != GM_COOP)
			{
				if (sortedplayers[i]->fragcount <= 0) // Displays a 0.0 when frags are 0 or negative
					sprintf (str, "0.0"); // [deathz0r] Buggy? [anarkavre] just explicitly print as string
				else if (sortedplayers[i]->fragcount >= 1 && sortedplayers[i]->deathcount == 0) // [deathz0r] Do not divide by zero
					sprintf (str, "%4.1f", (float)sortedplayers[i]->fragcount); // [anarkavre] dividing by 1 will just end up fragcount
				else
					sprintf (str, "%4.1f", (float)sortedplayers[i]->fragcount / (float)sortedplayers[i]->deathcount);
			}
			else
			{
				if (sortedplayers[i]->killcount == 0) // [deathz0r] Displays a 0.0 when kills are 0
					sprintf (str, "0.0"); // Buggy?
				else if (sortedplayers[i]->killcount >= 1 && sortedplayers[i]->deathcount == 0) // [deathz0r] Do not divide by zero
					sprintf (str, "%4.1f", (float)sortedplayers[i]->killcount);
				else
					sprintf (str, "%4.1f", (float)sortedplayers[i]->killcount / (float)sortedplayers[i]->deathcount);
			}
			screen->DrawTextClean (sortedplayers[i] == player ? CR_GREEN : CR_BRICK, (214 - V_StringWidth (str)) * CleanXfac, y, str);

			// Display Deaths
			sprintf (str, "%d", sortedplayers[i]->deathcount);
			screen->DrawTextClean (sortedplayers[i] == player ? CR_GREEN : CR_BRICK, (244 - V_StringWidth (str)) * CleanXfac, y, str);

			// Display Ping
			if (sortedplayers[i]->ping < 0 || sortedplayers[i]->ping > 999)
				sprintf(str, "###");
			else
				sprintf (str, "%d", sortedplayers[i]->ping);
			screen->DrawTextClean (sortedplayers[i] == player ? CR_GREEN : CR_BRICK, (279 - V_StringWidth (str)) * CleanXfac, y, str);

			// Display Time
			sprintf (str, "%d", sortedplayers[i]->GameTime / 60);
			screen->DrawTextClean (sortedplayers[i] == player ? CR_GREEN : CR_BRICK, (315 - V_StringWidth (str)) * CleanXfac, y, str);

			// Display Name
			strcpy (str, sortedplayers[i]->userinfo.netname);

			if (sortedplayers[i] != player)
				color = (demoplayback && sortedplayers[i] == &consoleplayer()) ? CR_GOLD : CR_GREY;

			else
				color = CR_GREEN;

			screen->DrawTextClean (color, 16 * CleanXfac, y, str);

			y += 8 * CleanYfac;
		}
	}
}

//
// [Toke - Scores] HU_DMScores2
// Draws high-res DM scores
//
void HU_DMScores2 (player_t *player)
{
	char str[80];
	std::vector<player_t *> sortedplayers(players.size());
	unsigned int i, j, listsize;

	// Board location
	int y = 21;

	int marginx = (screen->width  - DMBOARDWIDTH) / 2;
	int marginy = (screen->height - DMBOARDHEIGHT) / 2;

	int locx = marginx;
	int locy = marginy / 2;

	if (player->camera->player)
		player = player->camera->player;

	// Player list sorting
	for (j = 0; j < sortedplayers.size(); j++)
		sortedplayers[j] = &players[j];

	if(sv_gametype != GM_COOP)
		std::sort(sortedplayers.begin(), sortedplayers.end(), compare_player_frags);
	else
		std::sort(sortedplayers.begin(), sortedplayers.end(), compare_player_kills);

    listsize = sortedplayers.size();

	// Background effect
	OdamexEffect (locx - DMBORDER,
                  locy - DMBORDER,
				  locx + DMBOARDWIDTH  + DMBORDER,
				  locy + DMBOARDHEIGHT + (listsize*10) + DMBORDER);

	//	Timelimit display
	HU_DisplayTimer(locx + 225,locy - DMBORDER+8,false);

	// Scoreboard Identify
    // Dan - Tells which current game mode is being played
    if (sv_gametype != GM_COOP)
        screen->DrawText (CR_GOLD,locx + 135,locy + 0,"Deathmatch");
    else
        screen->DrawText (CR_GOLD,locx + 150,locy + 0,"Cooperative");

	// Player scores header
	screen->DrawText	  (CR_GREY	,locx + 8		,locy + 0	,"PLAYERS:"		);
	sprintf (str, "%d", (unsigned int)sortedplayers.size());
	screen->DrawText	  (CR_GREEN	,locx + 68		,locy + 0	,	str	);

	// Fraglimit
	if (sv_gametype != GM_COOP)
	{
		screen->DrawText	  (CR_GREY	,locx + 266		,locy + 0	,"FRAGLIMIT:"	);
		sprintf (str, "%d", sv_fraglimit.asInt());
		screen->DrawText	  (CR_GREEN	,locx + 336		,locy + 0	,	str	);
	}

	screen->Clear (locx + 8,
				   locy + 8,
				   locx + 368,
				   locy + 10,
				   4);

	screen->DrawText	  (CR_GREY	,locx + 8		,locy + 11	,"NAME"			);
	if(sv_gametype != GM_COOP)
	{
		screen->DrawText	  (CR_GREY	,locx + 130		,locy + 11	,"FRAGS"		);
		screen->DrawText	  (CR_GREY	,locx + 179		,locy + 11	,"FRG/DTH"		);
	}
	else
	{
		screen->DrawText	  (CR_GREY	,locx + 130		,locy + 11	,"KILLS"		);
		screen->DrawText	  (CR_GREY	,locx + 175		,locy + 11	,"KILL/DTH"		);
	}
	screen->DrawText	  (CR_GREY	,locx + 244		,locy + 11	,"DEATHS"		);
	screen->DrawText	  (CR_GREY	,locx + 301		,locy + 11	,"PING"			);
	screen->DrawText	  (CR_GREY	,locx + 339		,locy + 11	,"TIME"			);

	//	Draw player info
	for (i = 0; i < listsize && y < ST_Y - 12 * CleanYfac; i++)
	{
		int color = sortedplayers[i]->userinfo.color;

		if (!sortedplayers[i]->ingame())
			continue;
		// Define text color
		if (sortedplayers[i] != player)
			color = CR_BRICK;
		else
			color = CR_GREEN;



		int blob = sortedplayers[i]->userinfo.color;


		blob = BestColor (DefaultPalette->basecolors,
						  RPART(blob),
						  GPART(blob),
						  BPART(blob),
						  DefaultPalette->numcolors);

		if (!sortedplayers[i]->spectator && sortedplayers[i]->playerstate != PST_CONTACT && sortedplayers[i]->playerstate != PST_DOWNLOAD)
			screen->Clear (locx,
						   locy + y,
						   locx + 7,
						   locy + y + 7,
						   blob);


		// NAME
		strcpy (str, sortedplayers[i]->userinfo.netname);
		screen->DrawText	  (color	,locx + 8							,locy + y	,	str	);

		// [deathz0r] FRAG for deathmatch, KILLS for coop
		if(sv_gametype != GM_COOP)
			sprintf (str, "%d", sortedplayers[i]->fragcount);
		else
			sprintf (str, "%d", sortedplayers[i]->killcount);
		screen->DrawText	  (color	,locx + (166 - V_StringWidth (str))	,locy + y	,	str	);

		// [deathz0r] FRAGS/DEATHS (dm) or KILLS/DEATHS (coop) RATIO
		if(sv_gametype != GM_COOP)
		{
			if (sortedplayers[i]->fragcount <= 0) // Displays a 0.0 when frags are 0 or negative
				sprintf (str, "0.0"); // [deathz0r] Buggy? [anarkavre] just explicitly print as string
			else if (sortedplayers[i]->fragcount >= 1 && sortedplayers[i]->deathcount == 0) // [deathz0r] Do not divide by zero
				sprintf (str, "%5.1f", (float)sortedplayers[i]->fragcount); // [anarkavre] dividing by 1 will just end up fragcount
			else
				sprintf (str, "%5.1f", (float)sortedplayers[i]->fragcount / (float)sortedplayers[i]->deathcount);
		}
		else
		{
			if (sortedplayers[i]->killcount == 0) // [deathz0r] Displays a 0.0 when kills are 0
				sprintf (str, "0.0"); // Buggy?
			else if (sortedplayers[i]->killcount >= 1 && sortedplayers[i]->deathcount == 0) // [deathz0r] Do not divide by zero
				sprintf (str, "%5.1f", (float)sortedplayers[i]->killcount);
			else
				sprintf (str, "%5.1f", (float)sortedplayers[i]->killcount / (float)sortedplayers[i]->deathcount);
		}
		screen->DrawText	  (color	,locx + (231 - V_StringWidth (str))	,locy + y	,	str	);

		// DEATHS
		sprintf (str, "%d", sortedplayers[i]->deathcount);
		screen->DrawText	  (color	,locx + (281 - V_StringWidth (str))	,locy + y	,	str	);

		// PING
		if (sortedplayers[i]->ping < 0 || sortedplayers[i]->ping > 999)
			sprintf(str, "###");
		else
			sprintf (str, "%d", sortedplayers[i]->ping);
		screen->DrawText	  (color	,locx + (326 - V_StringWidth (str))	,locy + y	,	str	);

		// TIME
		sprintf (str, "%d", (sortedplayers[i]->GameTime / 60));
		screen->DrawText	  (color	,locx + (368 - V_StringWidth (str))	,locy + y	,	str	);

		y += 10;
	}
}

//
// [Toke - CTF - Scores] HU_TeamScores1
// Draws low-res CTF scores
//
void HU_TeamScores1 (player_t *player)
{
	int resfix;

	char str[80];
	char frags[80];
	char ping[80];
	char points[80];
	char deaths[80];

	std::vector<player_t *> sortedplayers(players.size());
	int x, y, maxwidth, margin;

	size_t i, j;

	int bluey;
	int redy;

	int bcount = 0;
	int rcount = 0;

	int bfrags = 0;
	int rfrags = 0;

	int bpings = 0;
	int rpings = 0;

	int bpavg = 0;
	int rpavg = 0;

	int bfavg = 0;
	int rfavg = 0;

	int rpoints = 0;
	int bpoints = 0;

	if (player->camera->player)
		player = player->camera->player;

	for (j = 0; j < sortedplayers.size(); j++)
		sortedplayers[j] = &players[j];

	std::sort(sortedplayers.begin(), sortedplayers.end(), sv_gametype == GM_CTF ? compare_player_points : compare_player_frags);

	maxwidth = 60;

	x = (screen->width >> 1) - (((maxwidth + 32 + 32 + 16) * CleanXfac) >> 1);
	margin = x + 40 * CleanXfac;

	resfix = screen->height / 200;

	x = (screen->width >> 1) - (((maxwidth + 32 + 32 + 16) * CleanXfac) >> 1);
	margin = x + 40 * CleanXfac;

	y     = 78 * resfix;

	bluey = 26 * CleanYfac;

	redy  = 98 * CleanYfac;

	// Scoreboard Identify
	// Dan - Tells which current game mode is being played
	if (sv_gametype == GM_CTF)
		screen->DrawTextClean (CR_GOLD,100 * CleanXfac,0 * CleanYfac,"Capture The Flag");

	//	Timelimit display
	HU_DisplayTimer(screen->width/2 - 126/2*CleanXfac, 8 * CleanYfac);

	// Draw team stats header
	screen->DrawTextClean	  (CR_GREY	,	243	* CleanXfac	,	26	* CleanYfac	,	"SCORE:"	);
	if(sv_gametype == GM_CTF)
	{
		screen->DrawTextClean	  (CR_GREY	,	247	* CleanXfac	,	34	* CleanYfac	,	"TOTAL"		);
		screen->DrawTextClean	  (CR_GREY	,	250	* CleanXfac	,	42	* CleanYfac	,	"FRAG:"		);
		screen->DrawTextClean	  (CR_GREY	,	246	* CleanXfac	,	50	* CleanYfac	,	"POINT:"	);
		screen->DrawTextClean	  (CR_GREY	,	247	* CleanXfac	,	66	* CleanYfac	,	"AVERAGE"	);
		screen->DrawTextClean	  (CR_GREY	,	254	* CleanXfac	,	74	* CleanYfac	,	"PING:"		);
	}
	else
	{
		screen->DrawTextClean	  (CR_GREY	,	247	* CleanXfac	,	34	* CleanYfac	,	"AVERAGE"	);
		screen->DrawTextClean	  (CR_GREY	,	250	* CleanXfac	,	42	* CleanYfac	,	"FRAG:"		);
		screen->DrawTextClean	  (CR_GREY	,	254	* CleanXfac	,	50	* CleanYfac	,	"PING:"		);
	}
	screen->DrawTextClean	  (CR_GREY	,	243	* CleanXfac	,	98	* CleanYfac	,	"SCORE:"	);
	if(sv_gametype == GM_CTF)
	{
		screen->DrawTextClean	  (CR_GREY	,	247	* CleanXfac	,	106	* CleanYfac	,	"TOTAL"		);
		screen->DrawTextClean	  (CR_GREY	,	250	* CleanXfac	,	114	* CleanYfac	,	"FRAG:"		);
		screen->DrawTextClean	  (CR_GREY	,	246	* CleanXfac	,	122	* CleanYfac	,	"POINT:"	);
		screen->DrawTextClean	  (CR_GREY	,	247	* CleanXfac	,	138	* CleanYfac	,	"AVERAGE"	);
		screen->DrawTextClean	  (CR_GREY	,	254	* CleanXfac	,	146	* CleanYfac	,	"PING:"		);
	}
	else
	{
		screen->DrawTextClean	  (CR_GREY	,	247	* CleanXfac	,	106	* CleanYfac	,	"AVERAGE"	);
		screen->DrawTextClean	  (CR_GREY	,	250	* CleanXfac	,	114	* CleanYfac	,	"FRAG:"		);
		screen->DrawTextClean	  (CR_GREY	,	254	* CleanXfac	,	122	* CleanYfac	,	"PING:"		);
	}

	// Player scores header
	screen->DrawTextClean	  (CR_GREY	,	10	* CleanXfac	,	18	* CleanYfac	,	"NAME"		);
	if (sv_gametype == GM_CTF) {
        screen->DrawTextClean	  (CR_GREY	,	128	* CleanXfac	,	18	* CleanYfac	,	"POINT"		);
        screen->DrawTextClean	  (CR_GREY	,	171	* CleanXfac	,	18	* CleanYfac	,	"FRAG"		);
	} else {
        screen->DrawTextClean	  (CR_GREY	,	128	* CleanXfac	,	18	* CleanYfac	,	"FRAG"		);
        screen->DrawTextClean	  (CR_GREY	,	165	* CleanXfac	,	18	* CleanYfac	,	"DEATH"		);
	}
	screen->DrawTextClean	  (CR_GREY	,	210	* CleanXfac	,	18	* CleanYfac	,	"PING"		);

	screen->DrawTextClean	  (CR_GREY	,	10	* CleanXfac	,	90	* CleanYfac	,	"NAME"		);
	if (sv_gametype == GM_CTF) {
        screen->DrawTextClean	  (CR_GREY	,	128	* CleanXfac	,	90	* CleanYfac	,	"POINT"		);
        screen->DrawTextClean	  (CR_GREY	,	171	* CleanXfac	,	90	* CleanYfac	,	"FRAG"		);
	} else {
        screen->DrawTextClean	  (CR_GREY	,	128	* CleanXfac	,	90	* CleanYfac	,	"FRAG"		);
        screen->DrawTextClean	  (CR_GREY	,	165	* CleanXfac	,	90	* CleanYfac	,	"DEATH"		);
	}
	screen->DrawTextClean	  (CR_GREY	,	210	* CleanXfac	,	90	* CleanYfac	,	"PING"		);

	for (i = 0; i < sortedplayers.size(); i++)
	{
		int colorblue;
		int colorred;

		if (sortedplayers[i]->ingame())
		{
			sprintf (frags, "%d", sortedplayers[i]->fragcount);

			if (sv_gametype == GM_CTF)
			sprintf (points, "%d", sortedplayers[i]->points);
			else
			sprintf (deaths, "%d", sortedplayers[i]->deathcount);

			if (sortedplayers[i]->ping < 0 || sortedplayers[i]->ping > 999)
				sprintf(ping, "###");
			else
           		sprintf (ping, "%d", (sortedplayers[i]->ping));

			strcpy (str, sortedplayers[i]->userinfo.netname);

			if (sortedplayers[i] != player)
				colorblue = (demoplayback && sortedplayers[i] == &consoleplayer()) ? CR_GOLD : CR_BLUE;

			else
				colorblue = CR_GOLD;

			if (sortedplayers[i] != player)
				colorred = (demoplayback && sortedplayers[i] == &consoleplayer()) ? CR_GOLD : CR_RED;

			else
				colorred = CR_GOLD;

			if (sortedplayers[i]->userinfo.team == TEAM_BLUE)
			{
				int blob = sortedplayers[i]->userinfo.color;
				blob = BestColor (DefaultPalette->basecolors,
								  RPART(blob),
								  GPART(blob),
								  BPART(blob),
								  DefaultPalette->numcolors);

				if (!sortedplayers[i]->spectator && sortedplayers[i]->playerstate != PST_CONTACT && sortedplayers[i]->playerstate != PST_DOWNLOAD)
					screen->Clear (1, bluey, (7 * CleanXfac), bluey + (7 * CleanYfac), blob);

				screen->DrawTextClean (colorblue	,	10	* CleanXfac,			bluey			,			str				);
				if (sv_gametype == GM_CTF) {
                    screen->DrawTextClean (colorblue	,	128	* CleanXfac,			bluey			,			points			);
                    screen->DrawTextClean (colorblue	,	171	* CleanXfac,			bluey			,			frags			);
				} else {
                    screen->DrawTextClean (colorblue	,	128	* CleanXfac,			bluey			,			frags			);
                    screen->DrawTextClean (colorblue	,	165	* CleanXfac,			bluey			,			deaths			);
				}
				screen->DrawTextClean (colorblue	,	210	* CleanXfac,			bluey			,			ping			);

				bfrags = bfrags + sortedplayers[i]->fragcount;
				bpings = bpings + sortedplayers[i]->ping;
				bpoints = bpoints + sortedplayers[i]->points;

				bluey += 8 * CleanYfac;

				bcount++;
			}

			if (sortedplayers[i]->userinfo.team == TEAM_RED)
			{
				int blob = sortedplayers[i]->userinfo.color;
				blob = BestColor (DefaultPalette->basecolors,
								  RPART(blob),
								  GPART(blob),
								  BPART(blob),
								  DefaultPalette->numcolors);

				if (!sortedplayers[i]->spectator)
					screen->Clear (1, redy, (7 * CleanXfac), redy + (7 * CleanYfac), blob);

				screen->DrawTextClean (colorred 	,	10	* CleanXfac,			redy			,			str				);
				if (sv_gametype == GM_CTF) {
                    screen->DrawTextClean (colorred 	,	128	* CleanXfac,			redy			,			points			);
                    screen->DrawTextClean (colorred 	,	171	* CleanXfac,			redy			,			frags			);
				} else {
                    screen->DrawTextClean (colorred 	,	128	* CleanXfac,			redy			,			frags			);
                    screen->DrawTextClean (colorred 	,	165	* CleanXfac,			redy			,			deaths			);
				}
				screen->DrawTextClean (colorred 	,	210	* CleanXfac,			redy			,			ping			);

				rfrags = rfrags + sortedplayers[i]->fragcount;
				rpings = rpings + sortedplayers[i]->ping;
				rpoints = rpoints + sortedplayers[i]->points;

				redy += 8 * CleanYfac;

				rcount++;
			}
		}
	}

	// Blue team score
	sprintf (str, "%d", TEAMpoints[TEAM_BLUE]);
	screen->DrawTextClean	  (CR_BLUE	,	287	* CleanXfac	,	26	* CleanYfac	,	str	);
	// Total blue frags and points
	if(sv_gametype == GM_CTF)
	{
		sprintf (str, "%d", bfrags);
		screen->DrawTextClean	  (CR_BLUE	,	287	* CleanXfac	,	42	* CleanYfac	,	str	);

		sprintf (str, "%d", bpoints);
		screen->DrawTextClean	  (CR_BLUE	,	287	* CleanXfac	,	50	* CleanYfac	,	str	);
	}

	// Red team score
	sprintf (str, "%d", TEAMpoints[TEAM_RED]);
	screen->DrawTextClean	  (CR_RED	,	287	* CleanXfac	,	98	* CleanYfac	,	str	);
	// Total red frags and points
	if(sv_gametype == GM_CTF)
	{
		sprintf (str, "%d", rfrags);
		screen->DrawTextClean	  (CR_RED	,	287	* CleanXfac	,	114	* CleanYfac	,	str	);

		sprintf (str, "%d", rpoints);
		screen->DrawTextClean	  (CR_RED	,	287	* CleanXfac	,	122	* CleanYfac	,	str	);
	}

	if (bcount)
	{
		// Calculate averages
		bfavg = (int)(bfrags / bcount);
		bpavg = (int)(bpings / bcount);

		if(sv_gametype == GM_CTF)
		{
			// Average blue ping
			if (bpavg < 0 || bpavg > 999)
				sprintf(str, "###");
			else
				sprintf (str, "%d", bpavg);
			screen->DrawTextClean	  (CR_BLUE	,	287	* CleanXfac	,	74	* CleanYfac	,	str	);
		}
		else
		{
			// Average blue frags
			sprintf (str, "%d", bfavg);
			screen->DrawTextClean	  (CR_BLUE	,	287	* CleanXfac	,	42	* CleanYfac	,	str	);

			// Average blue ping
			if (bpavg < 0 || bpavg > 999)
				sprintf(str, "###");
			else
				sprintf (str, "%d", bpavg);
			screen->DrawTextClean	  (CR_BLUE	,	287	* CleanXfac	,	50	* CleanYfac	,	str	);
		}

	}

	if (rcount)
	{
		// Calculate averages
		rfavg = (int)(rfrags / rcount);
		rpavg = (int)(rpings / rcount);

		if(sv_gametype == GM_CTF)
		{
			// Average red ping
			if (rpavg < 0 || rpavg > 999)
				sprintf(str, "###");
			else
				sprintf (str, "%d", rpavg);
			screen->DrawTextClean	  (CR_RED	,	287	* CleanXfac	,	146	* CleanYfac	,	str	);
		}

		else
		{
			// Average red frags
			sprintf (str, "%d", rfavg);
			screen->DrawTextClean	  (CR_RED	,	287	* CleanXfac	,	114	* CleanYfac	,	str	);

			// Average red ping
			if (rpavg < 0 || rpavg > 999)
				sprintf(str, "###");
			else
				sprintf (str, "%d", rpavg);
			screen->DrawTextClean	  (CR_RED	,	287	* CleanXfac	,	122	* CleanYfac	,	str	);
		}
	}
}

//
// [Toke - CTF - Scores] HU_TeamScores2
// Draws High-res CTF scores
//
void HU_TeamScores2 (player_t *player)
{
	char str[80];
	unsigned int listsize;

	std::vector<player_t *> sortedplayers(players.size());

	size_t i, j;

	int bcount = 0;
	int rcount = 0;

	int bfrags = 0;
	int rfrags = 0;

	int bpings = 0;
	int rpings = 0;

	int bpavg = 0;
	int rpavg = 0;

	int bfavg = 0;
	int rfavg = 0;

	int bpoints = 0;
	int rpoints = 0;

	if (player->camera->player)
		player = player->camera->player;

	for (j = 0; j < sortedplayers.size(); j++)
	{
		sortedplayers[j] = &players[j];

        if (sortedplayers[j]->userinfo.team == TEAM_BLUE)
            bcount++;

        if (sortedplayers[j]->userinfo.team == TEAM_RED)
            rcount++;
	}

	std::sort(sortedplayers.begin(), sortedplayers.end(), sv_gametype == GM_CTF ? compare_player_points : compare_player_frags);

	listsize = (rcount > bcount ? rcount : bcount);

	// Board locations
	int marginx = (screen->width - (CTFBOARDWIDTH * 2)) / 4;
	int marginy = (screen->height - CTFBOARDHEIGHT) / 4;

	int redy = 40;

	int bluey = 40;

	int blocx = marginx;
	int blocy = marginy;

	int rlocx = (marginx * 3) + CTFBOARDWIDTH;
	int rlocy = marginy;


	//int glocx
	//int glocy

	// Background effect
	OdamexEffect (blocx - TEAMPLAYBORDER,
				  blocy - TEAMPLAYBORDER,
				  rlocx + CTFBOARDWIDTH  + TEAMPLAYBORDER,
				  rlocy + CTFBOARDHEIGHT + (listsize*10) + TEAMPLAYBORDER);

	//	Timelimit display
	HU_DisplayTimer(rlocx + 90,rlocy - TEAMPLAYBORDER+4,false);

	// Player scores header
	// Blue Bar
	screen->Clear (blocx + 8,
				   blocy + 24,
				   blocx + 235,
				   blocy + 30,
				   200);

	// Red Bar
	screen->Clear (rlocx + 8,
				   rlocy + 24,
				   rlocx + 235,
				   rlocy + 30,
				   176);

	// Scoreboard Identify
	// Dan - Tells which current game mode is being played
    if (sv_gametype == GM_CTF)
    {
        strcpy(str, "Capture The Flag");
        screen->DrawText (CR_GOLD,(screen->width/2)-(V_StringWidth(str)/2),blocy + 0,str);
    }

	// BLUE
	screen->DrawText	  (CR_GREY	,blocx + 8	,blocy + 16	,"SCORE:"			);
	if(sv_gametype == GM_CTF)
		screen->DrawText	  (CR_GREY	,blocx + 115	,blocy + 0	,"TOTAL FRAGS:"		);
	screen->DrawText	  (CR_GREY	,blocx + 111	,blocy + 8	,"AVERAGE PING:"		);
	if(sv_gametype != GM_CTF)
		screen->DrawText	  (CR_GREY	,blocx + 100	,rlocy + 16	,"AVERAGE FRAGS:"	);
	else
		screen->DrawText	  (CR_GREY	,blocx + 111	,rlocy + 16	,"TOTAL POINTS:"	);

	screen->DrawText	  (CR_GREY	,blocx + 8	,blocy + 32	,"NAME"				);
	if(sv_gametype == GM_CTF) {
        screen->DrawText	  (CR_GREY	,blocx + 126	,blocy + 32	,"POINT"			);
        screen->DrawText	  (CR_GREY	,blocx + 169	,blocy + 32	,"FRAG"				);
	} else {
        screen->DrawText	  (CR_GREY	,blocx + 126	,blocy + 32	,"FRAG"			);
        screen->DrawText	  (CR_GREY	,blocx + 163	,blocy + 32	,"DEATH"				);
	}
	screen->DrawText	  (CR_GREY	,blocx + 208	,blocy + 32	,"PING"				);


	// RED
	screen->DrawText	  (CR_GREY	,rlocx + 8		,rlocy + 16	,"SCORE:"		);
	if(sv_gametype == GM_CTF)
		screen->DrawText	  (CR_GREY	,rlocx + 115	,rlocy + 0	,"TOTAL FRAGS:"		);
	screen->DrawText	  (CR_GREY	,rlocx + 111	,rlocy + 8	,"AVERAGE PING:"		);
	if(sv_gametype != GM_CTF)
		screen->DrawText	  (CR_GREY	,rlocx + 100	,rlocy + 16	,"AVERAGE FRAGS:"	);
	else
		screen->DrawText	  (CR_GREY	,rlocx + 111	,rlocy + 16	,"TOTAL POINTS:"	);

	screen->DrawText	  (CR_GREY	,rlocx + 8		,rlocy + 32	,"NAME"			);
	if(sv_gametype == GM_CTF) {
        screen->DrawText	  (CR_GREY	,rlocx + 126	,rlocy + 32	,"POINT"			);
        screen->DrawText	  (CR_GREY	,rlocx + 169	,rlocy + 32	,"FRAG"				);
	} else {
        screen->DrawText	  (CR_GREY	,rlocx + 126	,rlocy + 32	,"FRAG"			);
        screen->DrawText	  (CR_GREY	,rlocx + 163	,rlocy + 32	,"DEATH"				);
	}
	screen->DrawText	  (CR_GREY	,rlocx + 208	,rlocy + 32	,"PING"				);


	// Blue team score
	sprintf (str, "%d", TEAMpoints[TEAM_BLUE]);
	screen->DrawText	  (CR_BLUE	,blocx + 52	,blocy + 16	,	str	);

	// Red team score
	sprintf (str, "%d", TEAMpoints[TEAM_RED]);
	screen->DrawText	  (CR_RED	,rlocx + 52	,rlocy + 16	,	str	);

	for (i = 0; i < sortedplayers.size(); i++)
	{
		int colorgrey;
		int colorblue;
		int colorred;

		if (sortedplayers[i]->ingame())
		{
			// GREY
			if (sortedplayers[i] != player)
				colorgrey = (demoplayback && sortedplayers[i] == &consoleplayer()) ? CR_GOLD : CR_GREY;
			else
				colorgrey = CR_GOLD;

			// BLUE
			if (sortedplayers[i] != player)
				colorblue = (demoplayback && sortedplayers[i] == &consoleplayer()) ? CR_GOLD : CR_BLUE;
			else
				colorblue = CR_GOLD;

			// RED
			if (sortedplayers[i] != player)
				colorred = (demoplayback && sortedplayers[i] == &consoleplayer()) ? CR_GOLD : CR_RED;
			else
				colorred = CR_GOLD;

			if (sortedplayers[i]->userinfo.team == TEAM_BLUE)
			{
				int blob = sortedplayers[i]->userinfo.color;

				blob = BestColor (DefaultPalette->basecolors,
								  RPART(blob),
								  GPART(blob),
								  BPART(blob),
								  DefaultPalette->numcolors);

				if (!sortedplayers[i]->spectator && sortedplayers[i]->playerstate != PST_CONTACT && sortedplayers[i]->playerstate != PST_DOWNLOAD)
					screen->Clear (blocx,
								   blocy + bluey,
								   blocx + 7,
								   blocy + bluey + 7,
								   blob);

				//	Draw BLUE team player info

				// NAME
				strcpy (str, sortedplayers[i]->userinfo.netname);
				screen->DrawText	  (colorgrey	,blocx + 8		,blocy + bluey	,	str	);

				if (sv_gametype == GM_CTF) {
                    // POINTS
                    sprintf (str, "%d", sortedplayers[i]->points);
                    screen->DrawText	  (colorblue	,blocx + 126	,blocy + bluey	,	str	);

                    // FRAGS
                    sprintf (str, "%d", sortedplayers[i]->fragcount);
                    screen->DrawText	  (colorgrey	,blocx + 169	,blocy + bluey	,	str	);
                } else {
                    // FRAGS
                    sprintf (str, "%d", sortedplayers[i]->fragcount);
                    screen->DrawText	  (colorblue	,blocx + 126	,blocy + bluey	,	str	);

                    // DEATHS
                    sprintf (str, "%d", sortedplayers[i]->deathcount);
                    screen->DrawText	  (colorgrey	,blocx + 163	,blocy + bluey	,	str	);
                }

				// PING
				sprintf (str, "%d", (sortedplayers[i]->ping));
				screen->DrawText	  (colorgrey	,blocx + 208	,blocy + bluey	,	str	);

				bfrags = bfrags + sortedplayers[i]->fragcount;
				bpings = bpings + sortedplayers[i]->ping;
				if(sv_gametype == GM_CTF)
					bpoints = bpoints + sortedplayers[i]->points;

				bluey += 10;

				//bcount++;
			}

			if (sortedplayers[i]->userinfo.team == TEAM_RED)
			{

				int blob = sortedplayers[i]->userinfo.color;

				blob = BestColor (DefaultPalette->basecolors,
								  RPART(blob),
								  GPART(blob),
								  BPART(blob),
								  DefaultPalette->numcolors);

				if (!sortedplayers[i]->spectator)
					screen->Clear (rlocx,
								   rlocy + redy,
								   rlocx + 7,
								   rlocy + redy + 7,
								   blob);

				//	Draw RED team player info

				// NAME
				strcpy (str, sortedplayers[i]->userinfo.netname);
				screen->DrawText	  (colorgrey	,rlocx + 8		,rlocy + redy	,	str	);

				if (sv_gametype == GM_CTF) {
                    // POINTS
                    sprintf (str, "%d", sortedplayers[i]->points);
                    screen->DrawText	  (colorred	,rlocx + 126	,rlocy + redy	,	str	);

                    // FRAGS
                    sprintf (str, "%d", sortedplayers[i]->fragcount);
                    screen->DrawText	  (colorgrey	,rlocx + 169	,rlocy + redy	,	str	);
				} else {
                    // FRAGS
                    sprintf (str, "%d", sortedplayers[i]->fragcount);
                    screen->DrawText	  (colorred	,rlocx + 126	,rlocy + redy	,	str	);

                    // DEATHS
                    sprintf (str, "%d", sortedplayers[i]->deathcount);
                    screen->DrawText	  (colorgrey	,rlocx + 163	,rlocy + redy	,	str	);
				}

				// PING
				sprintf (str, "%d", (sortedplayers[i]->ping));
				screen->DrawText	  (colorgrey	,rlocx + 208	,rlocy + redy	,	str	);

				rfrags = rfrags + sortedplayers[i]->fragcount;
				rpings = rpings + sortedplayers[i]->ping;
				if(sv_gametype == GM_CTF)
					rpoints = rpoints + sortedplayers[i]->points;

				redy += 10;

				//rcount++;
			}
		}
	}
	// [deathz0r] Todo - Move these so that they always display even if no players exist on team
	if (bcount)
	{
		bfavg = (int)(bfrags / bcount);
		bpavg = (int)(bpings / bcount);

		if (bpavg < 0 || bpavg > 999)
			sprintf(str, "###");
		else
			sprintf (str, "%d", bpavg);
		screen->DrawText	  (CR_BLUE	,blocx + 203		,blocy + 8	,	str			);

		if(sv_gametype != GM_CTF)
			sprintf (str, "%d", bfavg);
		else
			sprintf (str, "%d", bpoints);
		screen->DrawText	  (CR_BLUE	,blocx + 203		,blocy + 16	,	str			);

		// TOTAL FRAGS (ctf only)
		if(sv_gametype == GM_CTF)
		{
			sprintf (str, "%d", bfrags);
			screen->DrawText	  (CR_BLUE	,blocx + 203		,blocy + 0	,	str			);
		}
	}

	if (rcount)
	{
		rfavg = (int)(rfrags / rcount);
		rpavg = (int)(rpings / rcount);

		if (rpavg < 0 || rpavg > 999)
			sprintf(str, "###");
		else
			sprintf (str, "%d", rpavg);
		screen->DrawText	  (CR_RED	,rlocx + 203		,rlocy + 8	,	str			);

		if(sv_gametype != GM_CTF)
			sprintf (str, "%d", rfavg);
		else
			sprintf (str, "%d", rpoints);
		screen->DrawText	  (CR_RED	,rlocx + 203		,rlocy + 16	,	str			);

		// TOTAL FRAGS (ctf only)
		if(sv_gametype == GM_CTF)
		{
			sprintf (str, "%d", rfrags);
			screen->DrawText	  (CR_RED	,rlocx + 203		,rlocy + 0	,	str			);
		}
	}
}

//
// [Toke] OdamexEffect
// Draws the 50% reduction in brightness effect
//
void OdamexEffect (int xa, int ya, int xb, int yb)
{
	static int odawidth, odaheight;
	static DCanvas *odacanvas = NULL;

	if (odawidth != xb - xa || odaheight != yb - ya)
	{
		if (odacanvas)
		{
			I_FreeScreen(odacanvas);
			odacanvas = NULL;
		}
	}

	odawidth  = xb - xa;
	odaheight = yb - ya;

	if (xa < 0 || ya < 0 || xb > screen->width || yb > screen->height)
		return;

	if (!odacanvas)
		odacanvas = I_AllocateScreen((xb - xa), (yb - ya), 8);

	screen->CopyRect(xa, ya, (xb - xa), (yb - ya), 0, 0, odacanvas);
	odacanvas->Dim ();
	odacanvas->Blit(0, 0, (xb - xa), (yb - ya), screen, xa, ya, (xb - xa), (yb - ya));
}

//
// HU_ConsoleScores
// Draws scoreboard to console.
//
void HU_ConsoleScores (player_t *player)
{
    char str[80];
    std::vector<player_t *> sortedplayers(players.size());
    int j;
    unsigned int i;

    C_ToggleConsole(); // One of these at each end prevents the following from
                       // drawing on the screen itself.

    // Player list sorting
	for (i = 0; i < sortedplayers.size(); i++)
		sortedplayers[i] = &players[i];

    if (sv_gametype == GM_CTF) {
        std::sort(sortedplayers.begin(), sortedplayers.end(), compare_player_points);

        Printf_Bold("\n--------------------------------------\n");
        Printf_Bold("           CAPTURE THE FLAG\n");

        if (sv_scorelimit)
            sprintf (str, "Scorelimit: %-6d", sv_scorelimit.asInt());
        else
            sprintf (str, "Scorelimit: N/A   ");

        Printf_Bold("%s  ", str);

        if (sv_timelimit)
            sprintf (str, "Timelimit: %-7d", sv_timelimit.asInt());
        else
            sprintf (str, "Timelimit: N/A");

        Printf_Bold("%18s\n", str);

        for (j = 0; j < 2; j++) {
            if (j == 0)
                Printf_Bold("\n-----------------------------BLUE TEAM\n");
            else
                Printf_Bold("\n------------------------------RED TEAM\n");
            Printf_Bold("Name            Points Caps Frags Time\n");
            Printf_Bold("--------------------------------------\n");

            for (i = 0; i < sortedplayers.size(); i++) {
                if (sortedplayers[i]->userinfo.team == j && !sortedplayers[i]->spectator
                && sortedplayers[i]->playerstate != PST_CONTACT && sortedplayers[i]->playerstate != PST_DOWNLOAD) {
                    if (sortedplayers[i] != player)
                        Printf(PRINT_HIGH, "%-15s %-6d N/A  %-5d %4d\n",
                            sortedplayers[i]->userinfo.netname,
                            sortedplayers[i]->points,
                            //sortedplayers[i]->captures,
                            sortedplayers[i]->fragcount,
                            sortedplayers[i]->GameTime / 60);

                    else
                        Printf_Bold("%-15s %-6d N/A  %-5d %4d\n",
                            player->userinfo.netname,
                            player->points,
                            //player->captures,
                            player->fragcount,
                            player->GameTime / 60);
                }
            }
        }

        Printf_Bold("\n----------------------------SPECTATORS\n");
            for (i = 0; i < sortedplayers.size(); i++) {
                if (sortedplayers[i]->spectator) {
                    if (sortedplayers[i] != player)
                        Printf(PRINT_HIGH, "%-15s\n", sortedplayers[i]->userinfo.netname);
                    else
                        Printf_Bold("%-15s\n", player->userinfo.netname);
                }
            }
    } else if (sv_gametype == GM_TEAMDM) {
        std::sort(sortedplayers.begin(), sortedplayers.end(), compare_player_frags);

        Printf_Bold("\n--------------------------------------\n");
        Printf_Bold("           TEAM DEATHMATCH\n");

        if (sv_fraglimit)
            sprintf (str, "Fraglimit: %-7d", sv_fraglimit.asInt());
        else
            sprintf (str, "Fraglimit: N/A    ");

        Printf_Bold("%s  ", str);

        if (sv_timelimit)
            sprintf (str, "Timelimit: %-7d", sv_timelimit.asInt());
        else
            sprintf (str, "Timelimit: N/A");

        Printf_Bold("%18s\n", str);

        for (j = 0; j < 2; j++) {
            if (j == 0)
                Printf_Bold("\n-----------------------------BLUE TEAM\n");
            else
                Printf_Bold("\n------------------------------RED TEAM\n");
            Printf_Bold("Name            Frags Deaths  K/D Time\n");
            Printf_Bold("--------------------------------------\n");

            for (i = 0; i < sortedplayers.size(); i++) {
                if (sortedplayers[i]->userinfo.team == j && !sortedplayers[i]->spectator
                && sortedplayers[i]->playerstate != PST_CONTACT && sortedplayers[i]->playerstate != PST_DOWNLOAD) {
                    if (sortedplayers[i]->fragcount <= 0) // Copied from HU_DMScores1.
                        sprintf (str, "0.0");
                    else if (sortedplayers[i]->fragcount >= 1 && sortedplayers[i]->deathcount == 0)
                        sprintf (str, "%2.1f", (float)sortedplayers[i]->fragcount);
                    else
                        sprintf (str, "%2.1f", (float)sortedplayers[i]->fragcount / (float)sortedplayers[i]->deathcount);

                    if (sortedplayers[i] != player)
                        Printf(PRINT_HIGH, "%-15s %-5d %-6d %4s %4d\n",
                            sortedplayers[i]->userinfo.netname,
                            sortedplayers[i]->fragcount,
                            sortedplayers[i]->deathcount,
                            str,
                            sortedplayers[i]->GameTime / 60);

                    else
                        Printf_Bold("%-15s %-5d %-6d %4s %4d\n",
                            player->userinfo.netname,
                            player->fragcount,
                            player->deathcount,
                            str,
                            player->GameTime / 60);
                }
            }
        }

        Printf_Bold("\n----------------------------SPECTATORS\n");
            for (i = 0; i < sortedplayers.size(); i++) {
                if (sortedplayers[i]->spectator) {
                    if (sortedplayers[i] != player)
                        Printf(PRINT_HIGH, "%-15s\n", sortedplayers[i]->userinfo.netname);
                    else
                        Printf_Bold("%-15s\n", player->userinfo.netname);
                }
            }
    } else if (sv_gametype == GM_DM) {
        std::sort(sortedplayers.begin(), sortedplayers.end(), compare_player_frags);

        Printf_Bold("\n--------------------------------------\n");
        Printf_Bold("              DEATHMATCH\n");

        if (sv_fraglimit)
            sprintf (str, "Fraglimit: %-7d", sv_fraglimit.asInt());
        else
            sprintf (str, "Fraglimit: N/A    ");

        Printf_Bold("%s  ", str);

        if (sv_timelimit)
            sprintf (str, "Timelimit: %-7d", sv_timelimit.asInt());
        else
            sprintf (str, "Timelimit: N/A");

        Printf_Bold("%18s\n", str);

        Printf_Bold("Name            Frags Deaths  K/D Time\n");
        Printf_Bold("--------------------------------------\n");

        for (i = 0; i < sortedplayers.size(); i++) {
        	if (!sortedplayers[i]->spectator
        	&& sortedplayers[i]->playerstate != PST_CONTACT && sortedplayers[i]->playerstate != PST_DOWNLOAD) {
				if (sortedplayers[i]->fragcount <= 0) // Copied from HU_DMScores1.
					sprintf (str, "0.0");
				else if (sortedplayers[i]->fragcount >= 1 && sortedplayers[i]->deathcount == 0)
					sprintf (str, "%2.1f", (float)sortedplayers[i]->fragcount);
				else
					sprintf (str, "%2.1f", (float)sortedplayers[i]->fragcount / (float)sortedplayers[i]->deathcount);

				if (sortedplayers[i] != player)
					Printf(PRINT_HIGH, "%-15s %-5d %-6d %4s %4d\n",
						sortedplayers[i]->userinfo.netname,
						sortedplayers[i]->fragcount,
						sortedplayers[i]->deathcount,
						str,
						sortedplayers[i]->GameTime / 60);

				else
					Printf_Bold("%-15s %-5d %-6d %4s %4d\n",
						player->userinfo.netname,
						player->fragcount,
						player->deathcount,
						str,
						player->GameTime / 60);
        	}
        }

        Printf_Bold("\n----------------------------SPECTATORS\n");
            for (i = 0; i < sortedplayers.size(); i++) {
                if (sortedplayers[i]->spectator) {
                    if (sortedplayers[i] != player)
                        Printf(PRINT_HIGH, "%-15s\n", sortedplayers[i]->userinfo.netname);
                    else
                        Printf_Bold("%-15s\n", player->userinfo.netname);
                }
            }
    } else if (multiplayer) {
        std::sort(sortedplayers.begin(), sortedplayers.end(), compare_player_kills);

        Printf_Bold("\n--------------------------------------\n");
        Printf_Bold("             COOPERATIVE\n");
        Printf_Bold("Name            Kills Deaths  K/D Time\n");
        Printf_Bold("--------------------------------------\n");

        for (i = 0; i < sortedplayers.size(); i++) {
        	if (!sortedplayers[i]->spectator
        	&& sortedplayers[i]->playerstate != PST_CONTACT && sortedplayers[i]->playerstate != PST_DOWNLOAD) {
				if (sortedplayers[i]->killcount <= 0) // Copied from HU_DMScores1.
					sprintf (str, "0.0");
				else if (sortedplayers[i]->killcount >= 1 && sortedplayers[i]->deathcount == 0)
					sprintf (str, "%2.1f", (float)sortedplayers[i]->killcount);
				else
					sprintf (str, "%2.1f", (float)sortedplayers[i]->killcount / (float)sortedplayers[i]->deathcount);

				if (sortedplayers[i] != player)
					Printf(PRINT_HIGH, "%-15s %-5d %-6d %4s %4d\n",
						sortedplayers[i]->userinfo.netname,
						sortedplayers[i]->killcount,
						sortedplayers[i]->deathcount,
						str,
						sortedplayers[i]->GameTime / 60);

				else
					Printf_Bold("%-15s %-5d %-6d %4s %4d\n",
						player->userinfo.netname,
						player->killcount,
						player->deathcount,
						str,
						player->GameTime / 60);
			}
        }

        Printf_Bold("\n----------------------------SPECTATORS\n");
            for (i = 0; i < sortedplayers.size(); i++) {
                if (sortedplayers[i]->spectator) {
                    if (sortedplayers[i] != player)
                        Printf(PRINT_HIGH, "%-15s\n", sortedplayers[i]->userinfo.netname);
                    else
                        Printf_Bold("%-15s\n", player->userinfo.netname);
                }
            }
    } else {
        Printf (PRINT_HIGH, "This command is only used for multiplayer games.");
    }

    Printf (PRINT_HIGH, "\n");

    C_ToggleConsole();
}

BEGIN_COMMAND (displayscores)
{
    HU_ConsoleScores(&consoleplayer());
}
END_COMMAND (displayscores)

VERSION_CONTROL (hu_stuff_cpp, "$Id$")


