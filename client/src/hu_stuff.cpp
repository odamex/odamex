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
#include "dstrings.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "c_cvars.h"
#include "v_text.h"
#include "v_video.h"
#include "cl_main.h"
#include "cl_ctf.h"
#include "i_video.h"

#define QUEUESIZE		128
#define MESSAGESIZE		256
#define HU_INPUTX		0
#define HU_INPUTY		(0 + (SHORT(hu_font[0]->height) +1))

#define CTFBOARDWIDTH	236
#define CTFBOARDHEIGHT	103

#define DMBOARDWIDTH	368
#define DMBOARDHEIGHT	100

#define TEAMPLAYBORDER	10
#define DMBORDER		20

DCanvas *odacanvas = NULL;
extern	DCanvas *screen;

EXTERN_CVAR (con_scaletext)
EXTERN_CVAR (fraglimit)
EXTERN_CVAR (timelimit)
EXTERN_CVAR (scorelimit)
EXTERN_CVAR (teamplay)

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

void OdamexEffect (int xa, int ya, int xb, int yb);

extern inline int V_StringWidth (const char *str);

static void ShoveChatStr (std::string str, byte who);

static std::string input_text;
int headsupactive;
BOOL altdown;

CVAR (chatmacro0, HUSTR_CHATMACRO0, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro1, HUSTR_CHATMACRO1, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro2, HUSTR_CHATMACRO2, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro3, HUSTR_CHATMACRO3, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro4, HUSTR_CHATMACRO4, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro5, HUSTR_CHATMACRO5, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro6, HUSTR_CHATMACRO6, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro7, HUSTR_CHATMACRO7, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro8, HUSTR_CHATMACRO8, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro9, HUSTR_CHATMACRO9, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)

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
	char *tplate, buffer[12];

	headsupactive = 0;
	input_text = "";

	// load the heads-up font
	j = HU_FONTSTART;

	tplate = "STCFN%.3d";
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

	if (ev->data1 == KEY_RALT || ev->data1 == KEY_LALT)
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
		return false;
	}

	if (!headsupactive)
		return false;

	c = ev->data3;	// [RH] Use localized keymap

	// send a macro
	if (altdown)
	{
		if (ev->data2 >= '0' && ev->data2 <= '9')
		{
			ShoveChatStr (chat_macros[ev->data2 - '0']->cstring(), headsupactive - 1);
			headsupactive = 0;
			return true;
		}
	}
	if (ev->data3 == KEY_ENTER)
	{
		ShoveChatStr (input_text, headsupactive - 1);
		headsupactive = 0;
		return true;
	}
	else if (ev->data1 == KEY_ESCAPE)
	{
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

		if(input_text.length() < 1024)
			input_text += c;

		return true;
	}

	return false;
}

//
// HU_Drawer
//
void HU_Drawer (void)
{
	if (headsupactive)
	{
		static const char *prompt = "Say: ";
		int i, x, c, scalex, y, promptwidth;

		if (con_scaletext)
		{
			scalex = CleanXfac;
			y = (!viewactive ? -30 : -10) * CleanYfac;
		}
		else
		{
			scalex = 1;
			y = (!viewactive ? -30 : -10);
		}

		y += (screen->height == realviewheight && viewactive) ? screen->height : ST_Y;

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
		if (con_scaletext)
		{
			screen->DrawTextClean (CR_RED, 0, y, prompt);
			screen->DrawTextClean (CR_GREY, promptwidth, y, show_text.c_str() + i);
		}
		else
		{
			screen->DrawText (CR_RED, 0, y, prompt);
			screen->DrawText (CR_GREY, promptwidth, y, show_text.c_str() + i);
		}
	}

	if(multiplayer && consoleplayer().camera && !(demoplayback && democlassic))
	if (((Actions[ACTION_SHOWSCORES]) ||
		 consoleplayer().camera->health <= 0) && gamestate != GS_INTERMISSION)
	{
		HU_DrawScores (&consoleplayer());
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

	if(str.length() > MESSAGESIZE)
		str.resize(MESSAGESIZE);

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
	if(!connected)
		return;

	headsupactive = 2;
	C_HideConsole ();
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

CVAR (usehighresboard, "1",	CVAR_ARCHIVE)

#define CTFBOARDWIDTH	236
#define CTFBOARDHEIGHT	103

//
// [Toke - Scores] HU_DrawScores
// Decides which scoreboard to draw
//
void HU_DrawScores (player_t *player)
{
	if (ctfmode || teamplay)
	{
		if (usehighresboard)
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
		if (usehighresboard && screen->width != 320)
			HU_DMScores2 (player);
		else
			HU_DMScores1 (player);
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

	if(deathmatch)
		std::sort(sortedplayers.begin(), sortedplayers.end(), compare_player_frags);
	else
		std::sort(sortedplayers.begin(), sortedplayers.end(), compare_player_kills);

	y = 34 * CleanYfac;

	//	Timelimit display
/*
	if (deathmatch && timelimit && gamestate == GS_LEVEL)
	{
		int timeleft = (int)(timelimit * TICRATE * 60) - level.time;
		int hours, minutes, seconds;

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

		screen->DrawTextClean (CR_GREY, screen->width/2 - V_StringWidth (str)/2*CleanXfac, y - 12 * CleanYfac, str);
	}
*/

	// Scoreboard Identify
	// Dan - Tells which current game mode is being played
	if (deathmatch)
		screen->DrawText (CR_GOLD,120 * CleanXfac,10 * CleanYfac,"Deathmatch");
	else
		screen->DrawText (CR_GOLD,120 * CleanXfac,10 * CleanYfac,"Cooperative");

	//	Header display
	screen->DrawTextClean (CR_GREY,	16	* CleanXfac,	25	* CleanYfac, "NAME");

	if(deathmatch)
		screen->DrawTextClean (CR_GREY,	146	* CleanXfac,	25	* CleanYfac, "FRAG");
	else
		screen->DrawTextClean (CR_GREY,	146	* CleanXfac,	25	* CleanYfac, "KILL");

	if(deathmatch)
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
			if (screen->is8bit)		// Finds the best solid color based on a players color
				color = BestColor (DefaultPalette->basecolors, RPART(color), GPART(color), BPART(color), DefaultPalette->numcolors);

			// Display Color
			screen->Clear ((5 * CleanXfac), y, (13 * CleanXfac), y + hu_font[0]->height() * CleanYfac, color);

			// Display Frags or Kills if coop
			if(deathmatch)
				sprintf (str, "%d", sortedplayers[i]->fragcount);
			else
				sprintf (str, "%d", sortedplayers[i]->killcount);
			screen->DrawTextClean (sortedplayers[i] == player ? CR_GREEN : CR_BRICK, (177 - V_StringWidth (str)) * CleanXfac, y, str);

			// Display Frags/Deaths or Kills/Deaths ratio
			if(deathmatch)
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
	unsigned int i, j;

	if (player->camera->player)
		player = player->camera->player;

	// Player list sorting
	for (j = 0; j < sortedplayers.size(); j++)
		sortedplayers[j] = &players[j];

	if(deathmatch)
		std::sort(sortedplayers.begin(), sortedplayers.end(), compare_player_frags);
	else
		std::sort(sortedplayers.begin(), sortedplayers.end(), compare_player_kills);

	//	Timelimit display
/*
	if (deathmatch && timelimit && gamestate == GS_LEVEL)
	{
		int timeleft = (int)(timelimit * TICRATE * 60) - level.time;
		int hours, minutes, seconds;

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

		screen->DrawTextClean (CR_GREY, screen->width/2 - V_StringWidth (str)/2*CleanXfac, y - 12 * CleanYfac, str);
	}
*/



	// Board location
	int marginx = (screen->width  - DMBOARDWIDTH)	/ 2;
	int marginy = (screen->height - DMBOARDHEIGHT)	/ 2;

	int y = 21;


	int locx = marginx;
	int locy = marginy / 2;

	// Background effect
	OdamexEffect (locx - DMBORDER,
				  locy - DMBORDER,
				  locx + DMBOARDWIDTH  + DMBORDER,
				  locy + DMBOARDHEIGHT + DMBORDER);

	// Scoreboard Identify
    // Dan - Tells which current game mode is being played
    if (deathmatch)
        screen->DrawText (CR_GOLD,locx + 135,locy + 0,"Deathmatch");
    else
        screen->DrawText (CR_GOLD,locx + 150,locy + 0,"Cooperative");

	// Player scores header
	screen->DrawText	  (CR_GREY	,locx + 8		,locy + 0	,"PLAYERS:"		);
	sprintf (str, "%d", (unsigned int)sortedplayers.size());
	screen->DrawText	  (CR_GREEN	,locx + 68		,locy + 0	,	str	);

	// Fraglimit
	if (deathmatch)
	{
		screen->DrawText	  (CR_GREY	,locx + 266		,locy + 0	,"FRAGLIMIT:"	);
		sprintf (str, "%d", (int)fraglimit);
		screen->DrawText	  (CR_GREEN	,locx + 336		,locy + 0	,	str	);
	}

	screen->Clear (locx + 8,
				   locy + 8,
				   locx + 368,
				   locy + 10,
				   4);

	screen->DrawText	  (CR_GREY	,locx + 8		,locy + 11	,"NAME"			);
	if(deathmatch)
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


	i = sortedplayers.size();

	//	Draw player info
	for (i = 0; i < sortedplayers.size() && y < ST_Y - 12 * CleanYfac; i++)
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


		screen->Clear (locx,
					   locy + y,
					   locx + 7,
					   locy + y + 7,
					   blob);


		// NAME
		strcpy (str, sortedplayers[i]->userinfo.netname);
		screen->DrawText	  (color	,locx + 8							,locy + y	,	str	);

		// [deathz0r] FRAG for deathmatch, KILLS for coop
		if(deathmatch)
			sprintf (str, "%d", sortedplayers[i]->fragcount);
		else
			sprintf (str, "%d", sortedplayers[i]->killcount);
		screen->DrawText	  (color	,locx + (166 - V_StringWidth (str))	,locy + y	,	str	);

		// [deathz0r] FRAGS/DEATHS (dm) or KILLS/DEATHS (coop) RATIO
		if(deathmatch)
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

	std::sort(sortedplayers.begin(), sortedplayers.end(), ctfmode ? compare_player_points : compare_player_frags);

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
	if (ctfmode)
		screen->DrawText (CR_GOLD,100 * CleanXfac,0 * CleanYfac,"Capture The Flag");

	// Draw team stats header
	screen->DrawTextClean	  (CR_GREY	,	243	* CleanXfac	,	26	* CleanYfac	,	"SCORE:"	);
	if(ctfmode)
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
	if(ctfmode)
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
	if (ctfmode) {
        screen->DrawTextClean	  (CR_GREY	,	128	* CleanXfac	,	18	* CleanYfac	,	"POINT"		);
        screen->DrawTextClean	  (CR_GREY	,	171	* CleanXfac	,	18	* CleanYfac	,	"FRAG"		);
	} else {
        screen->DrawTextClean	  (CR_GREY	,	128	* CleanXfac	,	18	* CleanYfac	,	"FRAG"		);
        screen->DrawTextClean	  (CR_GREY	,	165	* CleanXfac	,	18	* CleanYfac	,	"DEATH"		);
	}
	screen->DrawTextClean	  (CR_GREY	,	210	* CleanXfac	,	18	* CleanYfac	,	"PING"		);

	screen->DrawTextClean	  (CR_GREY	,	10	* CleanXfac	,	90	* CleanYfac	,	"NAME"		);
	if (ctfmode) {
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

			if (ctfmode)
			sprintf (points, "%d", sortedplayers[i]->points);
			else
			sprintf (deaths, "%d", sortedplayers[i]->deathcount);

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

				screen->Clear (1, bluey, (7 * CleanXfac), bluey + (7 * CleanYfac), blob);

				screen->DrawTextClean (colorblue	,	10	* CleanXfac,			bluey			,			str				);
				if (ctfmode) {
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

				// Total blue frags and points
				if(ctfmode)
				{
					sprintf (str, "%d", bfrags);
					screen->DrawTextClean	  (CR_BLUE	,	287	* CleanXfac	,	42	* CleanYfac	,	str	);

					sprintf (str, "%d", bpoints);
					screen->DrawTextClean	  (CR_BLUE	,	287	* CleanXfac	,	50	* CleanYfac	,	str	);
				}

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

				screen->Clear (1, redy, (7 * CleanXfac), redy + (7 * CleanYfac), blob);

				screen->DrawTextClean (colorred 	,	10	* CleanXfac,			redy			,			str				);
				if (ctfmode) {
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

				// Total red frags and points
				if(ctfmode)
				{
					sprintf (str, "%d", rfrags);
					screen->DrawTextClean	  (CR_RED	,	287	* CleanXfac	,	114	* CleanYfac	,	str	);

					sprintf (str, "%d", rpoints);
					screen->DrawTextClean	  (CR_RED	,	287	* CleanXfac	,	122	* CleanYfac	,	str	);
				}

				redy += 8 * CleanYfac;

				rcount++;
			}
		}
	}

	// Blue team score
	sprintf (str, "%d", TEAMpoints[TEAM_BLUE]);
	screen->DrawTextClean	  (CR_BLUE	,	287	* CleanXfac	,	26	* CleanYfac	,	str	);

	// Red team score
	sprintf (str, "%d", TEAMpoints[TEAM_RED]);
	screen->DrawTextClean	  (CR_RED	,	287	* CleanXfac	,	98	* CleanYfac	,	str	);


	if (bcount)
	{
		// Calculate averages
		bfavg = (int)(bfrags / bcount);
		bpavg = (int)(bpings / bcount);

		if(ctfmode)
		{
			// Average blue ping
			sprintf (str, "%d", bpavg);
			screen->DrawTextClean	  (CR_BLUE	,	287	* CleanXfac	,	74	* CleanYfac	,	str	);
		}
		else
		{
			// Average blue frags
			sprintf (str, "%d", bfavg);
			screen->DrawTextClean	  (CR_BLUE	,	287	* CleanXfac	,	42	* CleanYfac	,	str	);

			// Average blue ping
			sprintf (str, "%d", bpavg);
			screen->DrawTextClean	  (CR_BLUE	,	287	* CleanXfac	,	50	* CleanYfac	,	str	);
		}

	}

	if (rcount)
	{
		// Calculate averages
		rfavg = (int)(rfrags / rcount);
		rpavg = (int)(rpings / rcount);

		if(ctfmode)
		{
			// Average red ping
			sprintf (str, "%d", rpavg);
			screen->DrawTextClean	  (CR_RED	,	287	* CleanXfac	,	146	* CleanYfac	,	str	);
		}

		else
		{
			// Average red frags
			sprintf (str, "%d", rfavg);
			screen->DrawTextClean	  (CR_RED	,	287	* CleanXfac	,	114	* CleanYfac	,	str	);

			// Average red ping
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
		sortedplayers[j] = &players[j];

	std::sort(sortedplayers.begin(), sortedplayers.end(), ctfmode ? compare_player_points : compare_player_frags);

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
				  rlocy + CTFBOARDHEIGHT + TEAMPLAYBORDER);

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

	// Gold Bar
//	screen->Clear (glocx + 8,
//				   glocy + 24,
//				   glocx + 235,
//				   glocx + 30,
//				   231);

	// Scoreboard Identify
	// Dan - Tells which current game mode is being played
    if (ctfmode)
        screen->DrawText (CR_GOLD,blocx + 275,blocy + 0,"Capture The Flag");

	// BLUE
	screen->DrawText	  (CR_GREY	,blocx + 8	,blocy + 16	,"SCORE:"			);
	if(ctfmode)
		screen->DrawText	  (CR_GREY	,blocx + 115	,blocy + 0	,"TOTAL FRAGS:"		);
	screen->DrawText	  (CR_GREY	,blocx + 111	,blocy + 8	,"AVERAGE PING:"		);
	if(!ctfmode)
		screen->DrawText	  (CR_GREY	,blocx + 100	,rlocy + 16	,"AVERAGE FRAGS:"	);
	else
		screen->DrawText	  (CR_GREY	,blocx + 111	,rlocy + 16	,"TOTAL POINTS:"	);

	screen->DrawText	  (CR_GREY	,blocx + 8	,blocy + 32	,"NAME"				);
	if(ctfmode) {
        screen->DrawText	  (CR_GREY	,blocx + 126	,blocy + 32	,"POINT"			);
        screen->DrawText	  (CR_GREY	,blocx + 169	,blocy + 32	,"FRAG"				);
	} else {
        screen->DrawText	  (CR_GREY	,blocx + 126	,blocy + 32	,"FRAG"			);
        screen->DrawText	  (CR_GREY	,blocx + 163	,blocy + 32	,"DEATH"				);
	}
	screen->DrawText	  (CR_GREY	,blocx + 208	,blocy + 32	,"PING"				);


	// RED
	screen->DrawText	  (CR_GREY	,rlocx + 8		,rlocy + 16	,"SCORE:"		);
	if(ctfmode)
		screen->DrawText	  (CR_GREY	,rlocx + 115	,rlocy + 0	,"TOTAL FRAGS:"		);
	screen->DrawText	  (CR_GREY	,rlocx + 111	,rlocy + 8	,"AVERAGE PING:"		);
	if(!ctfmode)
		screen->DrawText	  (CR_GREY	,rlocx + 100	,rlocy + 16	,"AVERAGE FRAGS:"	);
	else
		screen->DrawText	  (CR_GREY	,rlocx + 111	,rlocy + 16	,"TOTAL POINTS:"	);

	screen->DrawText	  (CR_GREY	,rlocx + 8		,rlocy + 32	,"NAME"			);
	if(ctfmode) {
        screen->DrawText	  (CR_GREY	,rlocx + 126	,rlocy + 32	,"POINT"			);
        screen->DrawText	  (CR_GREY	,rlocx + 169	,rlocy + 32	,"FRAG"				);
	} else {
        screen->DrawText	  (CR_GREY	,rlocx + 126	,rlocy + 32	,"FRAG"			);
        screen->DrawText	  (CR_GREY	,rlocx + 163	,rlocy + 32	,"DEATH"				);
	}
	screen->DrawText	  (CR_GREY	,rlocx + 208	,rlocy + 32	,"PING"				);


	// GOLD
/*	screen->DrawText	  (CR_GREY	,glocx + 8		,glocy + 16	,"SCORE:"			);

	screen->DrawText	  (CR_GREY	,glocx + 115	,glocy + 0	,"TOTAL FRAGS:"		);
	screen->DrawText	  (CR_GREY	,glocx + 111	,glocy + 8	,"AVERAGE PING:"	);
	screen->DrawText	  (CR_GREY	,glocx + 100	,glocy + 16	,"AVERAGE FRAGS:"	);

	screen->DrawText	  (CR_GREY	,glocx + 8		,glocy + 32	,"NAME"				);	if (ctfmode)
	screen->DrawText	  (CR_GREY	,glocx + 126	,glocy + 32	,"POINT"			);
	screen->DrawText	  (CR_GREY	,glocx + 169	,glocy + 32	,"FRAG"				);
	screen->DrawText	  (CR_GREY	,glocx + 208	,glocy + 32	,"PING"				);*/


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

				screen->Clear (blocx,
							   blocy + bluey,
							   blocx + 7,
							   blocy + bluey + 7,
							   blob);

				//	Draw BLUE team player info

				// NAME
				strcpy (str, sortedplayers[i]->userinfo.netname);
				screen->DrawText	  (colorgrey	,blocx + 8		,blocy + bluey	,	str	);

				if (ctfmode) {
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
				if(ctfmode)
					bpoints = bpoints + sortedplayers[i]->points;

				// TOTAL FRAGS (ctf only)
				if(ctfmode)
				{
					sprintf (str, "%d", bfrags);
					screen->DrawText	  (CR_BLUE	,blocx + 203		,blocy + 0	,	str			);
				}

				bluey += 10;

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

				screen->Clear (rlocx,
							   rlocy + redy,
							   rlocx + 7,
							   rlocy + redy + 7,
							   blob);

				//	Draw RED team player info

				// NAME
				strcpy (str, sortedplayers[i]->userinfo.netname);
				screen->DrawText	  (colorgrey	,rlocx + 8		,rlocy + redy	,	str	);

				if (ctfmode) {
                    // POINTS
                    sprintf (str, "%d", sortedplayers[i]->points);
                    screen->DrawText	  (colorblue	,rlocx + 126	,rlocy + redy	,	str	);

                    // FRAGS
                    sprintf (str, "%d", sortedplayers[i]->fragcount);
                    screen->DrawText	  (colorgrey	,rlocx + 169	,rlocy + redy	,	str	);
				} else {
                    // FRAGS
                    sprintf (str, "%d", sortedplayers[i]->fragcount);
                    screen->DrawText	  (colorblue	,rlocx + 126	,rlocy + redy	,	str	);

                    // DEATHS
                    sprintf (str, "%d", sortedplayers[i]->deathcount);
                    screen->DrawText	  (colorgrey	,rlocx + 163	,rlocy + redy	,	str	);
				}

				// PING
				sprintf (str, "%d", (sortedplayers[i]->ping));
				screen->DrawText	  (colorgrey	,rlocx + 208	,rlocy + redy	,	str	);

				rfrags = rfrags + sortedplayers[i]->fragcount;
				rpings = rpings + sortedplayers[i]->ping;
				if(ctfmode)
					rpoints = rpoints + sortedplayers[i]->points;

				// TOTAL FRAGS (ctf only)
				if(ctfmode)
				{
					sprintf (str, "%d", rfrags);
					screen->DrawText	  (CR_RED	,rlocx + 203		,rlocy + 0	,	str			);
				}

				redy += 10;

				rcount++;
			}
		}
	}
	// [deathz0r] Todo - Move these so that they always display even if no players exist on team
	if (bcount)
	{
		bfavg = (int)(bfrags / bcount);
		bpavg = (int)(bpings / bcount);

		sprintf (str, "%d", bpavg);
		screen->DrawText	  (CR_BLUE	,blocx + 203		,blocy + 8	,	str			);

		if(!ctfmode)
			sprintf (str, "%d", bfavg);
		else
			sprintf (str, "%d", bpoints);
		screen->DrawText	  (CR_BLUE	,blocx + 203		,blocy + 16	,	str			);
	}

	if (rcount)
	{
		rfavg = (int)(rfrags / rcount);
		rpavg = (int)(rpings / rcount);

		sprintf (str, "%d", rpavg);
		screen->DrawText	  (CR_RED	,rlocx + 203		,rlocy + 8	,	str			);

		if(!ctfmode)
			sprintf (str, "%d", rfavg);
		else
			sprintf (str, "%d", rpoints);
		screen->DrawText	  (CR_RED	,rlocx + 203		,rlocy + 16	,	str			);
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

    if (ctfmode) {
        std::sort(sortedplayers.begin(), sortedplayers.end(), compare_player_points);

        Printf_Bold("\n--------------------------------------\n");
        Printf_Bold("           CAPTURE THE FLAG\n");

        if (scorelimit)
            sprintf (str, "Scorelimit: %-6d", (int)scorelimit);
        else
            sprintf (str, "Scorelimit: N/A   ");

        Printf_Bold("%s  ", str);

        if (timelimit)
            sprintf (str, "Timelimit: %-7d", (int)timelimit);
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
                if (sortedplayers[i]->userinfo.team == j) {
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
    } else if (teamplay) {
        std::sort(sortedplayers.begin(), sortedplayers.end(), compare_player_frags);

        Printf_Bold("\n--------------------------------------\n");
        Printf_Bold("           TEAM DEATHMATCH\n");

        if (fraglimit)
            sprintf (str, "Fraglimit: %-7d", (int)fraglimit);
        else
            sprintf (str, "Fraglimit: N/A    ");

        Printf_Bold("%s  ", str);

        if (timelimit)
            sprintf (str, "Timelimit: %-7d", (int)timelimit);
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
                if (sortedplayers[i]->userinfo.team == j) {
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
    } else if (deathmatch) {
        std::sort(sortedplayers.begin(), sortedplayers.end(), compare_player_frags);

        Printf_Bold("\n--------------------------------------\n");
        Printf_Bold("              DEATHMATCH\n");

        if (fraglimit)
            sprintf (str, "Fraglimit: %-7d", (int)fraglimit);
        else
            sprintf (str, "Fraglimit: N/A    ");

        Printf_Bold("%s  ", str);

        if (timelimit)
            sprintf (str, "Timelimit: %-7d", (int)timelimit);
        else
            sprintf (str, "Timelimit: N/A");

        Printf_Bold("%18s\n", str);

        Printf_Bold("Name            Frags Deaths  K/D Time\n");
        Printf_Bold("--------------------------------------\n");

        for (i = 0; i < sortedplayers.size(); i++) {
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
    } else if (multiplayer) {
        std::sort(sortedplayers.begin(), sortedplayers.end(), compare_player_kills);

        Printf_Bold("\n--------------------------------------\n");
        Printf_Bold("             COOPERATIVE\n");
        Printf_Bold("Name            Kills Deaths  K/D Time\n");
        Printf_Bold("--------------------------------------\n");

        for (i = 0; i < sortedplayers.size(); i++) {
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

