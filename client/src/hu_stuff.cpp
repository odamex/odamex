// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2015 by The Odamex Team.
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
#include <sstream>

#include "doomdef.h"
#include "z_zone.h"
#include "m_swap.h"
#include "hu_stuff.h"
#include "w_wad.h"
#include "s_sound.h"
#include "doomstat.h"
#include "st_stuff.h"
#include "gstrings.h"
#include "c_bind.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "c_cvars.h"
#include "v_text.h"

#include "cl_main.h"
#include "p_ctf.h"
#include "i_video.h"
#include "i_input.h"
#include "cl_netgraph.h"
#include "hu_mousegraph.h"
#include "am_map.h"

#include "hu_drawers.h"
#include "hu_elements.h"

#include "v_video.h"

#define QUEUESIZE		128
#define HU_INPUTX		0
#define HU_INPUTY		(0 + (LESHORT(hu_font[0]->height) +1))

#define CTFBOARDWIDTH	236
#define CTFBOARDHEIGHT	40

#define DMBOARDWIDTH	368
#define DMBOARDHEIGHT	16

#define TEAMPLAYBORDER	10
#define DMBORDER		20

DCanvas *odacanvas = NULL;
extern DCanvas *screen;
extern byte *Ranges;

EXTERN_CVAR(hud_fullhudtype)
EXTERN_CVAR(hud_scaletext)
EXTERN_CVAR(sv_fraglimit)
EXTERN_CVAR(sv_timelimit)
EXTERN_CVAR(sv_scorelimit)
EXTERN_CVAR(cl_netgraph)
EXTERN_CVAR(hud_mousegraph)
EXTERN_CVAR(hud_show_scoreboard_ondeath)
EXTERN_CVAR(hud_targetcount)
EXTERN_CVAR(sv_maxplayers)
EXTERN_CVAR(noisedebug)
EXTERN_CVAR(screenblocks)
EXTERN_CVAR(idmypos)


static int crosshair_lump;

static void HU_InitCrosshair();
static byte crosshair_trans[256];

static int crosshair_color_custom = 0xb0;
CVAR_FUNC_IMPL (hud_crosshaircolor)
{
	argb_t color = V_GetColorFromString(hud_crosshaircolor);
	crosshair_color_custom = V_BestColor(V_GetDefaultPalette()->basecolors, color);
}

EXTERN_CVAR (hud_crosshairhealth)
EXTERN_CVAR (hud_crosshairdim)
EXTERN_CVAR (hud_crosshairscale)

CVAR_FUNC_IMPL(hud_crosshair)
{
	HU_InitCrosshair();
}

int V_TextScaleXAmount();
int V_TextScaleYAmount();

void HU_Init();
void HU_Drawer();
BOOL HU_Responder(event_t *ev);

patch_t *hu_font[HU_FONTSIZE];
patch_t* sbline;

void HU_DrawScores (player_t *plyr);
void HU_ConsoleScores (player_t *plyr);

extern bool HasBehavior;
extern inline int V_StringWidth(const char *str);
size_t P_NumPlayersInGame();
static void ShoveChatStr(std::string str, byte who);

static std::string input_text;

HUDChat chat;

HUDChat::EChatMode HUDChat::GetStatus()
{
	return status;
}

void HUDChat::SetStatus(EChatMode newstatus)
{
	status = newstatus;
}

void HUDChat::Unset()
{
	status = INACTIVE;
}

//
// Draws the "CHAT:" on the screen, making you able to type text,
// and chat with other players.
//
void HUDChat::DrawPrompt() 
{
	int surface_width = I_GetSurfaceWidth(), surface_height = I_GetSurfaceHeight();

	// Set up text scaling
	int scaledxfac = hud_scaletext ? V_TextScaleXAmount() : CleanXfac;
	int scaledyfac = hud_scaletext ? V_TextScaleYAmount() : CleanYfac;

	// Determine what Y height to display the chat prompt at.
	// * I_GetSurfaceHeight() is the "actual" screen height.
	// * viewactive is false if you have a fullscreen automap or
	//   intermission on-screen.
	// * ST_Y is the current Y height of the status bar.

	int y;
	if (!viewactive && gamestate != GS_INTERMISSION)
	{
		// Fullscreen automap is visible
		y = ST_StatusBarY(surface_width, surface_height) - (20 * scaledyfac);
	}
	else if (viewactive && R_StatusBarVisible())
	{
		// Status bar is visible
		y = ST_StatusBarY(surface_width, surface_height) - (10 * scaledyfac);
	}
	else
	{
		// Must be fullscreen HUD or intermission
		y = surface_height - (10 * scaledyfac);
	}

	static const char* prompt;
	if (chat.GetStatus() == HUDChat::TEAM)
		prompt = "Say (TEAM): ";
	else if (chat.GetStatus() == HUDChat::NORMAL)
		prompt = "Say: ";

	int promptwidth = V_StringWidth(prompt) * scaledxfac;
	int x = hu_font['_' - HU_FONTSTART]->width() * scaledxfac * 2 + promptwidth;

	// figure out if the text is wider than the screen->
	// if so, only draw the right-most portion of it.
	int i;
	for (i = input_text.length() - 1; i >= 0 && x < I_GetSurfaceWidth(); i--)
	{
		int c = toupper(input_text[i] & 0x7f) - HU_FONTSTART;
		if (c < 0 || c >= HU_FONTSIZE)
			x += 4 * scaledxfac;
		else
			x += hu_font[c]->width() * scaledxfac;
	}

	if (i >= 0)
		i++;
	else
		i = 0;

	// draw the prompt, text, and cursor
	std::string show_text = input_text;
	show_text += '_';
	screen->DrawTextStretched(CR_RED, 0, y, prompt,
		scaledxfac, scaledyfac);
	screen->DrawTextStretched(CR_GREY, promptwidth, y, show_text.c_str() + i,
		scaledxfac, scaledyfac);
}

BOOL altdown;

void ST_voteDraw (int y);

NetGraph netgraph(10, 100);

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
void HU_Init()
{
	const char *tplate = "STCFN%.3d";
	char buffer[12];

	chat.Unset();
	input_text.clear();

	// load the heads-up font
	int j = HU_FONTSTART;
	int sub = 0;

	for (int i = 0; i < HU_FONTSIZE; i++)
	{
		sprintf(buffer, tplate, j++ - sub);
		hu_font[i] = wads.CachePatch(buffer, PU_STATIC);
	}

	// Load the status bar line
	sbline = wads.CachePatch("SBLINE", PU_STATIC);

	HU_InitCrosshair();
}


//
// HU_Shutdown
//
// Frees any memory allocated specifically for the HUD.
//
void STACK_ARGS HU_Shutdown()
{
	Z_Free(sbline);
	sbline = NULL;
}


//
// HU_Ticker
//
// Perform maintence for the HUD system once per gametic.
//
void HU_Ticker()
{
	// verify the chat mode status is valid
	if (ConsoleState != c_up || menuactive || (gamestate != GS_LEVEL && gamestate != GS_INTERMISSION))
		chat.Unset();
}

void HU_ReleaseKeyStates()
{
	altdown = false;
}

//
// HU_Responder
//
// Chat mode text entry
//
BOOL HU_Responder(event_t *ev)
{
	if (ev->data1 == KEY_RALT || ev->data1 == KEY_LALT || ev->data1 == KEY_HAT1)
	{
		altdown = (ev->type == ev_keydown);
		return false;
	}
	else if (ev->data1 == KEY_LSHIFT || ev->data1 == KEY_RSHIFT)
	{
		return false;
	}
	else if ((gamestate != GS_LEVEL && gamestate != GS_INTERMISSION) || ev->type != ev_keydown)
	{
		if (chat.GetStatus() != HUDChat::INACTIVE)
            return true;

		return false;
	}

	if (chat.GetStatus() == HUDChat::INACTIVE)
		return false;

	if (altdown)
	{
		// send a macro
		if (ev->data2 >= KEY_JOY1 && ev->data2 <= KEY_JOY10)
		{
			ShoveChatStr(chat_macros[ev->data2 - KEY_JOY1]->cstring(), chat.GetStatus()- 1);
			chat.Unset();
			return true;
		}
		else if (ev->data1 >= '0' && ev->data1 <= '9')
		{
			ShoveChatStr(chat_macros[ev->data1 - '0']->cstring(), chat.GetStatus() - 1);
			chat.Unset();
			return true;
		}
	}
	if (ev->data1 == KEY_ENTER || ev->data1 == KEYP_ENTER)
	{
		ShoveChatStr(input_text, chat.GetStatus() - 1);
		chat.Unset();
		return true;
	}
	else if (ev->data1 == KEY_ESCAPE || ev->data1 == KEY_JOY2)
	{
		chat.Unset();
		return true;
	}
	else if (ev->data1 == KEY_BACKSPACE)
	{
		if (!input_text.empty())
			input_text.erase(input_text.end() - 1);
		return true;
	}

	int textkey = ev->data2;	// [RH] Use localized keymap
	if (textkey < ' ' || textkey > '~')		// ASCII only please
		return false;

	if (input_text.length() < MAX_CHATSTR_LEN)
		input_text += (char)textkey;

	return true;
}


static void HU_InitCrosshair()
{
	int xhairnum = hud_crosshair.asInt();

	if (xhairnum)
	{
		char xhairname[16];
		int xhair;

		sprintf(xhairname, "XHAIR%d", xhairnum);

		if ((xhair = wads.CheckNumForName(xhairname)) == -1)
			xhair = wads.CheckNumForName("XHAIR1");

		if (xhair != -1)
			crosshair_lump = xhair;
	}

	// set up translation table for the crosshair's color
	// initialize to default colors
	for (size_t i = 0; i < 256; i++)
		crosshair_trans[i] = i;
}


//
// HU_DrawCrosshair
//
static void HU_DrawCrosshair()
{
	if (gamestate != GS_LEVEL)
		return;

	if (!camera)
		return;

	// Don't draw the crosshair in chasecam mode
	if (camera->player && (camera->player->cheats & CF_CHASECAM))
		return;

    // Don't draw the crosshair when automap is visible.
	if (AM_ClassicAutomapVisible() || AM_OverlayAutomapVisible())
        return;

	// Don't draw the crosshair in spectator mode
	if (camera->player && camera->player->spectator)
		return;

	if (hud_crosshair && crosshair_lump)
	{
		static const byte crosshair_color = 0xB0;
		if (hud_crosshairhealth)
		{
			byte health_colors[4] = { 0xB0, 0xDF, 0xE7, 0x77 };

			if (camera->health > 75)
				crosshair_trans[crosshair_color] = health_colors[3];
			else if (camera->health > 50)
				crosshair_trans[crosshair_color] = health_colors[2];
			else if (camera->health > 25)
				crosshair_trans[crosshair_color] = health_colors[1];
			else
				crosshair_trans[crosshair_color] = health_colors[0];
		}
		else
			crosshair_trans[crosshair_color] = crosshair_color_custom;

		V_ColorMap = translationref_t(crosshair_trans);

		int x = I_GetSurfaceWidth() / 2;
		int y = I_GetSurfaceHeight() / 2;

		if (R_StatusBarVisible())
			y = ST_StatusBarY(I_GetSurfaceWidth(), I_GetSurfaceHeight()) / 2;

		if (hud_crosshairdim && hud_crosshairscale)
			screen->DrawTranslatedLucentPatchCleanNoMove(wads.CachePatch(crosshair_lump), x, y);
        else if (hud_crosshairscale)
			screen->DrawTranslatedPatchCleanNoMove(wads.CachePatch(crosshair_lump), x, y);
        else if (hud_crosshairdim)
			screen->DrawTranslatedLucentPatch(wads.CachePatch(crosshair_lump), x, y);
		else
			screen->DrawTranslatedPatch (wads.CachePatch (crosshair_lump), x, y);
	}
}



//
// HU_Drawer
//
void HU_Drawer()
{
	if (noisedebug)
		S_NoiseDebug();

	if (gamestate == GS_LEVEL)
	{
		bool spechud = consoleplayer().spectator && consoleplayer_id == displayplayer_id;

		if ((viewactive && !R_StatusBarVisible()) || spechud)
		{
			if (screenblocks < 12)
			{
				if (spechud)
					hud::SpectatorHUD();
				else if (hud_fullhudtype >= 1)
					hud::OdamexHUD();
				else
					hud::ZDoomHUD();
			}
		}
		else
		{
			hud::DoomHUD();
		}
	}

	// [csDoom] draw disconnected wire [Toke] Made this 1337er
	// denis - moved to hu_stuff and uncommented
	if (noservermsgs && (gamestate == GS_INTERMISSION || gamestate == GS_LEVEL))
		screen->DrawPatchCleanNoMove(wads.CachePatch("NET"), 50 * CleanXfac, 1 * CleanYfac);

	if (cl_netgraph)
		netgraph.draw();

	if (hud_mousegraph)
		mousegraph.draw(hud_mousegraph);

	if (idmypos && gamestate == GS_LEVEL)
		Printf (PRINT_HIGH, "ang=%d;x,y,z=(%d,%d,%d)\n",
				displayplayer().camera->angle/FRACUNIT,
				displayplayer().camera->x/FRACUNIT,
				displayplayer().camera->y/FRACUNIT,
				displayplayer().camera->z/FRACUNIT);

	// Draw Netdemo info
	hud::drawNetdemo();

	// [AM] Voting HUD!
	ST_voteDraw(11 * CleanYfac);

	if (multiplayer && consoleplayer().camera && !(demoplayback && democlassic))
	{
		if (gamestate != GS_INTERMISSION && 
			(Actions[ACTION_SHOWSCORES]) 
			|| (hud_show_scoreboard_ondeath && displayplayer().health <= 0 && !displayplayer().spectator) )
		{
			HU_DrawScores(&displayplayer());
		}
	}

	if (gamestate == GS_LEVEL)
		HU_DrawCrosshair();

	if (chat.GetStatus() != HUDChat::INACTIVE)
		chat.DrawPrompt();
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

static void ShovePrivMsg(byte pid, std::string str)
{
	// Do not send this chat message if the chat string is empty
	if (str.length() == 0)
		return;

	if (str.length() > MAX_CHATSTR_LEN)
		str.resize(MAX_CHATSTR_LEN);

	MSG_WriteMarker(&net_buffer, clc_privmsg);
	MSG_WriteByte(&net_buffer, pid);
	MSG_WriteString(&net_buffer, str.c_str());
}

BEGIN_COMMAND (messagemode)
{
	if(!connected)
		return;

	chat.SetStatus(HUDChat::NORMAL);
	C_HideConsole ();
	input_text.clear();
	C_ReleaseKeys();
}
END_COMMAND (messagemode)

BEGIN_COMMAND (say)
{
	if (argc > 1)
	{
		std::string chat = C_ArgCombine(argc - 1, (const char **)(argv + 1));
		ShoveChatStr(chat, 0);
	}
}
END_COMMAND (say)

BEGIN_COMMAND (messagemode2)
{
	if(!connected || (sv_gametype != GM_TEAMDM && sv_gametype != GM_CTF && !consoleplayer().spectator))
		return;

	chat.SetStatus(HUDChat::TEAM);
	C_HideConsole ();
	input_text.clear();
	C_ReleaseKeys();
}
END_COMMAND (messagemode2)

BEGIN_COMMAND (say_team)
{
	if (argc > 1)
	{
		std::string chat = C_ArgCombine(argc - 1, (const char **)(argv + 1));
		ShoveChatStr(chat, 1);
	}
}
END_COMMAND (say_team)

BEGIN_COMMAND (say_to)
{
	if (argc > 2)
	{
		player_t &player = nameplayer(argv[1]);
		if (!validplayer(player))
		{
			Printf(PRINT_HIGH, "%s isn't the name of anybody on the server.\n", argv[1]);
			return;
		}

		std::string chat = C_ArgCombine(argc - 2, (const char **)(argv + 2));
		ShovePrivMsg(player.id, chat);
	}
}
END_COMMAND (say_to)

EXTERN_CVAR(hud_scalescoreboard)

EXTERN_CVAR(sv_gametype)
EXTERN_CVAR(sv_maxplayers)
EXTERN_CVAR(sv_hostname)

namespace hud {

// [AM] Draw scoreboard header
void drawHeader(player_t *player, int y) {
	int color;
	std::ostringstream buffer;
	std::string str;

	// Center
	if (sv_gametype == GM_COOP) {
		str = "COOPERATIVE";
	} else if (sv_gametype == GM_DM && sv_maxplayers == 2) {
		str = "DUEL";
	} else if (sv_gametype == GM_DM) {
		str = "DEATHMATCH";
	} else if (sv_gametype == GM_TEAMDM) {
		str = "TEAM DEATHMATCH";
	} else if (sv_gametype == GM_CTF) {
		str = "CAPTURE THE FLAG";
	}

	hud::DrawText(0, y, hud_scalescoreboard,
	              hud::X_CENTER, hud::Y_MIDDLE,
	              hud::X_CENTER, hud::Y_TOP,
	              str.c_str(), CR_GOLD, true);

	brokenlines_t *hostname = V_BreakLines(192, sv_hostname.cstring());
	for (size_t i = 0; i < 2 && hostname[i].width > 0; i++)
	{
		hud::DrawText(0, y + 8 * (i + 1), hud_scalescoreboard,
					  hud::X_CENTER, hud::Y_MIDDLE,
					  hud::X_CENTER, hud::Y_TOP,
					  hostname[i].string, CR_GREY, true);
	}
	V_FreeBrokenLines(hostname);

	// Left
	hud::DrawText(-236, y, hud_scalescoreboard,
	              hud::X_CENTER, hud::Y_MIDDLE,
	              hud::X_LEFT, hud::Y_TOP,
	              "CLIENTS: ", CR_GREY, true);
	hud::DrawText(-236 + V_StringWidth("CLIENTS: "), y, hud_scalescoreboard,
	              hud::X_CENTER, hud::Y_MIDDLE,
	              hud::X_LEFT, hud::Y_TOP,
	              hud::ClientsSplit().c_str(), CR_GREEN, true);
	if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF) {
		hud::DrawText(-236, y + 8, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_LEFT, hud::Y_TOP,
		              "BLUE PLAYERS: ", CR_GREY, true);
		hud::DrawText(-236 + V_StringWidth("BLUE PLAYERS: "), y + 8, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_LEFT, hud::Y_TOP,
		              TeamPlayers(color, TEAM_BLUE).c_str(), CR_GREEN, true);
		hud::DrawText(-236, y + 16, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_LEFT, hud::Y_TOP,
		              "RED PLAYERS: ", CR_GREY, true);
		hud::DrawText(-236 + V_StringWidth("RED PLAYERS: "), y + 16, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_LEFT, hud::Y_TOP,
		              TeamPlayers(color, TEAM_RED).c_str(), CR_GREEN, true);
	} else {
		hud::DrawText(-236, y + 8, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_LEFT, hud::Y_TOP,
		              "PLAYERS: ", CR_GREY, true);
		hud::DrawText(-236 + V_StringWidth("PLAYERS: "), y + 8, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_LEFT, hud::Y_TOP,
		              hud::PlayersSplit().c_str(), CR_GREEN, true);
	}

	// Right
	std::string timer, fraglimit, scorelimit;
	if (gamestate == GS_INTERMISSION)
		timer = hud::IntermissionTimer();
	else
		timer = hud::Timer(color);

	if (timer.empty())
		timer = "N/A";

	buffer.clear();
	if (sv_fraglimit.asInt() == 0)
		buffer.str("N/A");
	else
	{
		buffer.str("");
		buffer << sv_fraglimit.asInt();
	}
	fraglimit = buffer.str();

	buffer.clear();
	if (sv_scorelimit.asInt() == 0)
		buffer.str("N/A");
	else
	{
		buffer.str("");
		buffer << sv_scorelimit.asInt();
	}
	scorelimit = buffer.str();

	int rw = V_StringWidth("00:00");
	if (sv_timelimit.asInt() == 0 && gamestate != GS_INTERMISSION)
		rw = V_StringWidth("N/A");
	else if (timer.size() > 5)
		rw = V_StringWidth("00:00:00");

	if (V_StringWidth(fraglimit.c_str()) > rw)
		rw = V_StringWidth(fraglimit.c_str());

	if (V_StringWidth(scorelimit.c_str()) > rw)
		rw = V_StringWidth(scorelimit.c_str());

	hud::DrawText(236 - rw, y, hud_scalescoreboard,
	              hud::X_CENTER, hud::Y_MIDDLE,
	              hud::X_LEFT, hud::Y_TOP,
	              timer.c_str(), CR_GREEN, true);
	hud::DrawText(236 - rw, y + 8, hud_scalescoreboard,
	              hud::X_CENTER, hud::Y_MIDDLE,
	              hud::X_LEFT, hud::Y_TOP,
	              fraglimit.c_str(), CR_GREEN, true);
	hud::DrawText(236 - rw, y + 16, hud_scalescoreboard,
	              hud::X_CENTER, hud::Y_MIDDLE,
	              hud::X_LEFT, hud::Y_TOP,
	              scorelimit.c_str(), CR_GREEN, true);

	hud::DrawText(236 - rw, y, hud_scalescoreboard,
	              hud::X_CENTER, hud::Y_MIDDLE,
	              hud::X_RIGHT, hud::Y_TOP,
	              "TIME LEFT: ", CR_GREY, true);
	hud::DrawText(236 - rw, y + 8, hud_scalescoreboard,
	              hud::X_CENTER, hud::Y_MIDDLE,
	              hud::X_RIGHT, hud::Y_TOP,
	              "FRAGLIMIT: ", CR_GREY, true);
	hud::DrawText(236 - rw, y + 16, hud_scalescoreboard,
	              hud::X_CENTER, hud::Y_MIDDLE,
	              hud::X_RIGHT, hud::Y_TOP,
	              "SCORELIMIT: ", CR_GREY, true);

	// Line
	for (short xi = -236 + 1;xi < 236;xi += 2) {
		hud::DrawTranslatedPatch(xi, y + 24, hud_scalescoreboard,
		                         hud::X_CENTER, hud::Y_MIDDLE,
		                         hud::X_CENTER, hud::Y_TOP,
		                         sbline, Ranges + CR_GREY * 256, true);
	}
}

// [AM] Draw scores for teamless gametypes.
void drawScores(player_t *player, int y, byte extra_rows) {
	std::string str;

	// Colum headers
	hud::DrawText(-227, y, hud_scalescoreboard,
	              hud::X_CENTER, hud::Y_MIDDLE,
	              hud::X_LEFT, hud::Y_TOP,
	              "Name", CR_GREY, true);
	if (sv_gametype != GM_COOP) {
		hud::DrawText(44, y, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_RIGHT, hud::Y_TOP,
		              "FRG", CR_GREY, true);
		hud::DrawText(92, y, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_RIGHT, hud::Y_TOP,
		              "DTH", CR_GREY, true);
		hud::DrawText(140, y, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_RIGHT, hud::Y_TOP,
		              "K/D", CR_GREY, true);
	} else {
		hud::DrawText(92, y, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_RIGHT, hud::Y_TOP,
		              "KIL", CR_GREY, true);
		hud::DrawText(140, y, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_RIGHT, hud::Y_TOP,
		              "DTH", CR_GREY, true);
	}
	hud::DrawText(188, y, hud_scalescoreboard,
	              hud::X_CENTER, hud::Y_MIDDLE,
	              hud::X_RIGHT, hud::Y_TOP,
	              "MIN", CR_GREY, true);
	hud::DrawText(236, y, hud_scalescoreboard,
	              hud::X_CENTER, hud::Y_MIDDLE,
	              hud::X_RIGHT, hud::Y_TOP,
	              "PNG", CR_GREY, true);

	// Line
	for (short xi = -236 + 1;xi < 236;xi += 2) {
		hud::DrawTranslatedPatch(xi, y + 8, hud_scalescoreboard,
		                         hud::X_CENTER, hud::Y_MIDDLE,
		                         hud::X_CENTER, hud::Y_TOP,
		                         sbline, Ranges + CR_GREY * 256, true);
	}

	// Ingame Players
	byte limit = extra_rows + 4;
	hud::EAPlayerColors(-236, y + 11, 7, 7, hud_scalescoreboard,
	                    hud::X_CENTER, hud::Y_MIDDLE,
	                    hud::X_LEFT, hud::Y_TOP,
	                    1, limit);
	hud::EAPlayerNames(-227, y + 11, hud_scalescoreboard,
	                   hud::X_CENTER, hud::Y_MIDDLE,
	                   hud::X_LEFT, hud::Y_TOP,
	                   1, limit, true);
	if (sv_gametype != GM_COOP) {
		hud::EAPlayerFrags(44, y + 11, hud_scalescoreboard,
		                   hud::X_CENTER, hud::Y_MIDDLE,
		                   hud::X_RIGHT, hud::Y_TOP,
		                   1, limit, true);
		hud::EAPlayerDeaths(92, y + 11, hud_scalescoreboard,
		                   hud::X_CENTER, hud::Y_MIDDLE,
		                   hud::X_RIGHT, hud::Y_TOP,
		                   1, limit, true);
		hud::EAPlayerKD(140, y + 11, hud_scalescoreboard,
		                hud::X_CENTER, hud::Y_MIDDLE,
		                hud::X_RIGHT, hud::Y_TOP,
		                1, limit, true);
	} else {
		hud::EAPlayerKills(92, y + 11, hud_scalescoreboard,
		                   hud::X_CENTER, hud::Y_MIDDLE,
		                   hud::X_RIGHT, hud::Y_TOP,
		                   1, limit, true);
		hud::EAPlayerDeaths(140, y + 11, hud_scalescoreboard,
		                   hud::X_CENTER, hud::Y_MIDDLE,
		                   hud::X_RIGHT, hud::Y_TOP,
		                   1, limit, true);
	}
	hud::EAPlayerTimes(188, y + 11, hud_scalescoreboard,
	                   hud::X_CENTER, hud::Y_MIDDLE,
	                   hud::X_RIGHT, hud::Y_TOP,
	                   1, limit, true);
	hud::EAPlayerPings(236, y + 11, hud_scalescoreboard,
	                   hud::X_CENTER, hud::Y_MIDDLE,
	                   hud::X_RIGHT, hud::Y_TOP,
	                   1, limit, true);
}

// [AM] Draw scores for team gametypes.
void drawTeamScores(player_t *player, int y, byte extra_rows) {
	int color;
	std::ostringstream buffer;
	std::string str;
	static short tx[2] = {-236, 4};

	for (byte i = 0;i < 2;i++) {
		// Column headers
		hud::DrawText(tx[i] + 9, y, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_LEFT, hud::Y_TOP,
		              "Name", CR_GREY, true);
		if (sv_gametype == GM_CTF) {
			hud::DrawText(tx[i] + 168, y, hud_scalescoreboard,
			              hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP,
			              "PTS", CR_GREY, true);
			hud::DrawText(tx[i] + 200, y, hud_scalescoreboard,
			              hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP,
			              "FRG", CR_GREY, true);
		} else {
			hud::DrawText(tx[i] + 164, y, hud_scalescoreboard,
			              hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP,
			              "FRG", CR_GREY, true);
			hud::DrawText(tx[i] + 200, y, hud_scalescoreboard,
			              hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP,
			              "K/D", CR_GREY, true);
		}
		hud::DrawText(tx[i] + 232, y, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_RIGHT, hud::Y_TOP,
		              "PNG", CR_GREY, true);

		// Line
		if (i == TEAM_BLUE) {
			color = CR_BLUE;
		} else {
			color = CR_RED;
		}

		for (short xi = tx[i];xi < tx[i] + 232;xi += 2) {
			hud::DrawTranslatedPatch(xi, y + 8, hud_scalescoreboard,
			                         hud::X_CENTER, hud::Y_MIDDLE,
			                         hud::X_LEFT, hud::Y_TOP,
			                         sbline, Ranges + color * 256, true);
			hud::DrawTranslatedPatch(xi, y + 19, hud_scalescoreboard,
			                         hud::X_CENTER, hud::Y_MIDDLE,
			                         hud::X_LEFT, hud::Y_TOP,
			                         sbline, Ranges + color * 256, true);
		}

		// Team Info
		str = hud::TeamName(color, i);
		hud::DrawText(tx[i] + 9, y + 11, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_LEFT, hud::Y_TOP,
		              str.c_str(), color, true);
		if (sv_gametype == GM_CTF) {
			hud::DrawText(tx[i] + 168, y + 11, hud_scalescoreboard,
			              hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP,
			              hud::TeamPoints(color, i).c_str(), color, true);
			str = hud::TeamFrags(color, i);
			hud::DrawText(tx[i] + 200, y + 11, hud_scalescoreboard,
			              hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP,
			              str.c_str(), color, true);
		} else {
			hud::DrawText(tx[i] + 164, y + 11, hud_scalescoreboard,
			              hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP,
			              hud::TeamPoints(color, i).c_str(), color, true);
			str = hud::TeamKD(color, i);
			hud::DrawText(tx[i] + 200, y + 11, hud_scalescoreboard,
			              hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP,
			              str.c_str(), color, true);
		}
		str = hud::TeamPing(color, i);
		hud::DrawText(tx[i] + 232, y + 11, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_RIGHT, hud::Y_TOP,
		              str.c_str(), color, true);

		// Ingame Players
		byte limit = extra_rows + 4;
		hud::EATeamPlayerColors(tx[i], y + 22, 7, 7, hud_scalescoreboard,
		                        hud::X_CENTER, hud::Y_MIDDLE,
		                        hud::X_LEFT, hud::Y_TOP,
		                        1, limit, i);
		hud::EATeamPlayerNames(tx[i] + 9, y + 22, hud_scalescoreboard,
		                       hud::X_CENTER, hud::Y_MIDDLE,
		                       hud::X_LEFT, hud::Y_TOP,
		                       1, limit, i, true);
		if (sv_gametype == GM_CTF) {
			hud::EATeamPlayerPoints(tx[i] + 168, y + 22, hud_scalescoreboard,
			                        hud::X_CENTER, hud::Y_MIDDLE,
			                        hud::X_RIGHT, hud::Y_TOP,
			                        1, limit, i, true);
			hud::EATeamPlayerFrags(tx[i] + 200, y + 22, hud_scalescoreboard,
			                       hud::X_CENTER, hud::Y_MIDDLE,
			                       hud::X_RIGHT, hud::Y_TOP,
			                       1, limit, i, true);
		} else {
			hud::EATeamPlayerFrags(tx[i] + 164, y + 22, hud_scalescoreboard,
			                       hud::X_CENTER, hud::Y_MIDDLE,
			                       hud::X_RIGHT, hud::Y_TOP,
			                       1, limit, i, true);
			hud::EATeamPlayerKD(tx[i] + 200, y + 22, hud_scalescoreboard,
			                    hud::X_CENTER, hud::Y_MIDDLE,
			                    hud::X_RIGHT, hud::Y_TOP,
			                    1, limit, i, true);
		}
		hud::EATeamPlayerPings(tx[i] + 232, y + 22, hud_scalescoreboard,
		                       hud::X_CENTER, hud::Y_MIDDLE,
		                       hud::X_RIGHT, hud::Y_TOP,
		                       1, limit, i, true);
	}
}

// [AM] Draw spectators.
void drawSpectators(player_t *player, int y, byte extra_rows) {
	static short tx[3] = {-236, -79, 78};

	// Line
	for (short xi = -236 + 1;xi < 236;xi += 2) {
		hud::DrawTranslatedPatch(xi, y, hud_scalescoreboard,
		                         hud::X_CENTER, hud::Y_MIDDLE,
		                         hud::X_CENTER, hud::Y_TOP,
		                         sbline, Ranges + CR_GREY * 256, true);
	}

	byte specs = hud::CountSpectators();
	byte skip = 0;
	byte limit;
	for (byte i = 0;i < 3;i++) {
		limit = (specs + 2 - i) / 3;
		if (extra_rows + 1 < limit) {
			limit = extra_rows + 1;
		}
		hud::EASpectatorNames(tx[i], y + 3, hud_scalescoreboard,
		                      hud::X_CENTER, hud::Y_MIDDLE,
		                      hud::X_LEFT, hud::Y_TOP,
		                      1, skip, limit, true);
		hud::EASpectatorPings(tx[i] + 124, y + 3, hud_scalescoreboard,
		                      hud::X_CENTER, hud::Y_MIDDLE,
		                      hud::X_LEFT, hud::Y_TOP,
		                      1, skip, limit, true);
		skip += limit;
	}
}

// [AM] Draw the scoreboard
void Scoreboard(player_t *player) {
	// Calculate height
	int height, y;
	byte extra_spec_rows = 0;
	byte extra_player_rows = 0;

	// Reset playerID to self if ever happening to spy.
	if (gamestate == GS_INTERMISSION)
		displayplayer_id = consoleplayer_id;

	if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF) {
		height = 99;

		// Team scoreboard was designed for 4 players on a team.  If
		// there are more, increase the height.
		byte blue = hud::CountTeamPlayers(TEAM_BLUE);
		byte red = hud::CountTeamPlayers(TEAM_RED);
		if (blue > 4 || red > 4) {
			extra_player_rows += (blue > red) ? (blue - 4) : (red - 4);
		}
	} else {
		height = 88;

		// Normal scoreboard was designed for 4 players.  If there are
		// more, increase the height.
		byte players = P_NumPlayersInGame();
		if (players > 4) {
			extra_player_rows += players - 4;
		}
	}

	// If there are more than 3 spectators, increase the height.
	byte specs = hud::CountSpectators();
	if (specs > 3) {
		extra_spec_rows += ((specs + 2) / 3) - 1;
	}

	height += (extra_player_rows + extra_spec_rows) * 8;

	// 368 is our max height.
	while (height > 368) {
		// We have too many rows.  Start removing extra spectator rows
		// to make the scoreboard fit.
		if (extra_spec_rows == 0) {
			break;
		}
		extra_spec_rows -= 1;
		height -= 8;
	}

	while (height > 368) {
		// We still have too many rows.  Start removing extra player
		// rows to make the scoreboard fit.
		if (extra_player_rows == 0) {
			break;
		}
		extra_player_rows -= 1;
		height -= 8;
	}

	// Starting Y position, measured from the center up.
	y = -(height / 2);
	if (y > -96) {
		// Scoreboard starts slightly off-center, then grows to be centered.
		// -96 makes it even with where the default lowres TDM begins.
		// For the record, the lowres DM scoreboard begins at -54.
		y = -96;
	}

	// Dim the background
	hud::Dim(0, y, 480, height, hud_scalescoreboard,
	         hud::X_CENTER, hud::Y_MIDDLE,
	         hud::X_CENTER, hud::Y_TOP);

	hud::drawHeader(player, y + 4);
	if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF) {
		hud::drawTeamScores(player, y + 31, extra_player_rows);
	} else {
		hud::drawScores(player, y + 31, extra_player_rows);
	}
	hud::drawSpectators(player, y + (height - 14 - (extra_spec_rows * 8)),
	                    extra_spec_rows);
}

// [AM] Draw the low-resolution scoreboard header.
void drawLowHeader(player_t *player, int y) {
	std::string str;

	// Center
	if (sv_gametype == GM_COOP) {
		str = "COOPERATIVE";
	} else if (sv_gametype == GM_DM && sv_maxplayers == 2) {
		str = "DUEL";
	} else if (sv_gametype == GM_DM) {
		str = "DEATHMATCH";
	} else if (sv_gametype == GM_TEAMDM) {
		str = "TEAM DEATHMATCH";
	} else if (sv_gametype == GM_CTF) {
		str = "CAPTURE THE FLAG";
	}

	hud::DrawText(0, y, hud_scalescoreboard,
	              hud::X_CENTER, hud::Y_MIDDLE,
	              hud::X_CENTER, hud::Y_TOP,
	              str.c_str(), CR_GOLD, true);

	// Line
	for (short xi = -146 + 1;xi < 146;xi += 2) {
		hud::DrawTranslatedPatch(xi, y + 8, hud_scalescoreboard,
		                         hud::X_CENTER, hud::Y_MIDDLE,
		                         hud::X_CENTER, hud::Y_TOP,
		                         sbline, Ranges + CR_GREY * 256, true);
	}
}

// [AM] Draw low-resolution teamless gametype scores.
void drawLowScores(player_t *player, int y, byte extra_rows) {
	std::string str;

	// Colum headers
	hud::DrawText(-137, y, hud_scalescoreboard,
	              hud::X_CENTER, hud::Y_MIDDLE,
	              hud::X_LEFT, hud::Y_TOP,
	              "Name", CR_GREY, true);
	if (sv_gametype != GM_COOP) {
		hud::DrawText(22, y, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_RIGHT, hud::Y_TOP,
		              "FRG", CR_GREY, true);
		hud::DrawText(50, y, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_RIGHT, hud::Y_TOP,
		              "DTH", CR_GREY, true);
		hud::DrawText(90, y, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_RIGHT, hud::Y_TOP,
		              "K/D", CR_GREY, true);
	} else {
		hud::DrawText(62, y, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_RIGHT, hud::Y_TOP,
		              "KIL", CR_GREY, true);
		hud::DrawText(90, y, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_RIGHT, hud::Y_TOP,
		              "DTH", CR_GREY, true);
	}
	hud::DrawText(118, y, hud_scalescoreboard,
	              hud::X_CENTER, hud::Y_MIDDLE,
	              hud::X_RIGHT, hud::Y_TOP,
	              "MIN", CR_GREY, true);
	hud::DrawText(146, y, hud_scalescoreboard,
	              hud::X_CENTER, hud::Y_MIDDLE,
	              hud::X_RIGHT, hud::Y_TOP,
	              "PNG", CR_GREY, true);

	// Line
	for (short xi = -146 + 1;xi < 146;xi += 2) {
		hud::DrawTranslatedPatch(xi, y + 8, hud_scalescoreboard,
		                         hud::X_CENTER, hud::Y_MIDDLE,
		                         hud::X_CENTER, hud::Y_TOP,
		                         sbline, Ranges + CR_GREY * 256, true);
	}

	// Ingame Players
	byte limit = extra_rows + 4;
	hud::EAPlayerColors(-146, y + 11, 7, 7, hud_scalescoreboard,
	                    hud::X_CENTER, hud::Y_MIDDLE,
	                    hud::X_LEFT, hud::Y_TOP,
	                    1, limit);
	hud::EAPlayerNames(-137, y + 11, hud_scalescoreboard,
	                   hud::X_CENTER, hud::Y_MIDDLE,
	                   hud::X_LEFT, hud::Y_TOP,
	                   1, limit, true);
	if (sv_gametype != GM_COOP) {
		// NOTE: If we ever get true kill counts, we don't have
		//       enough room on this scoreboard to show frags, kills,
		//       deaths and K/D.
		hud::EAPlayerFrags(22, y + 11, hud_scalescoreboard,
		                   hud::X_CENTER, hud::Y_MIDDLE,
		                   hud::X_RIGHT, hud::Y_TOP,
		                   1, limit, true);
		hud::EAPlayerDeaths(50, y + 11, hud_scalescoreboard,
		                   hud::X_CENTER, hud::Y_MIDDLE,
		                   hud::X_RIGHT, hud::Y_TOP,
		                   1, limit, true);
		hud::EAPlayerKD(90, y + 11, hud_scalescoreboard,
		                hud::X_CENTER, hud::Y_MIDDLE,
		                hud::X_RIGHT, hud::Y_TOP,
		                1, limit, true);
	} else {
		hud::EAPlayerKills(62, y + 11, hud_scalescoreboard,
		                   hud::X_CENTER, hud::Y_MIDDLE,
		                   hud::X_RIGHT, hud::Y_TOP,
		                   1, limit, true);
		hud::EAPlayerDeaths(90, y + 11, hud_scalescoreboard,
		                   hud::X_CENTER, hud::Y_MIDDLE,
		                   hud::X_RIGHT, hud::Y_TOP,
		                   1, limit, true);
	}
	hud::EAPlayerTimes(118, y + 11, hud_scalescoreboard,
	                   hud::X_CENTER, hud::Y_MIDDLE,
	                   hud::X_RIGHT, hud::Y_TOP,
	                   1, limit, true);
	hud::EAPlayerPings(146, y + 11, hud_scalescoreboard,
	                   hud::X_CENTER, hud::Y_MIDDLE,
	                   hud::X_RIGHT, hud::Y_TOP,
	                   1, limit, true);
}

// [AM] Draw low-resolution team gametype scores.
void drawLowTeamScores(player_t *player, int y, byte extra_rows) {
	int color;
	std::string str;

	// Column headers
	hud::DrawText(-137, y, hud_scalescoreboard,
	              hud::X_CENTER, hud::Y_MIDDLE,
	              hud::X_LEFT, hud::Y_TOP,
	              "Name", CR_GREY, true);
	if (sv_gametype == GM_CTF) {
		hud::DrawText(34, y, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_RIGHT, hud::Y_TOP,
		              "PPL", CR_GREY, true);
		hud::DrawText(62, y, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_RIGHT, hud::Y_TOP,
		              "PTS", CR_GREY, true);
		hud::DrawText(90, y, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_RIGHT, hud::Y_TOP,
		              "FRG", CR_GREY, true);
	} else {
		hud::DrawText(22, y, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_RIGHT, hud::Y_TOP,
		              "PPL", CR_GREY, true);
		hud::DrawText(50, y, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_RIGHT, hud::Y_TOP,
		              "FRG", CR_GREY, true);
		hud::DrawText(90, y, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_RIGHT, hud::Y_TOP,
		              "K/D", CR_GREY, true);
	}
	hud::DrawText(118, y, hud_scalescoreboard,
	              hud::X_CENTER, hud::Y_MIDDLE,
	              hud::X_RIGHT, hud::Y_TOP,
	              "MIN", CR_GREY, true);
	hud::DrawText(146, y, hud_scalescoreboard,
	              hud::X_CENTER, hud::Y_MIDDLE,
	              hud::X_RIGHT, hud::Y_TOP,
	              "PNG", CR_GREY, true);

	// Since we are doing blue-over-red scores, we need to measure how
	// far down we want to start for the red team.
	byte blue_size = hud::CountTeamPlayers(TEAM_BLUE);
	if (blue_size < 4) {
		blue_size = 4;
	} else if (blue_size > extra_rows + 4) {
		blue_size = extra_rows + 4;
	}

	short ty[2] = {8, short(blue_size * 8 + 22) };

	for (byte i = 0;i < 2;i++) {
		// Line
		if (i == TEAM_BLUE) {
			color = CR_BLUE;
		} else {
			color = CR_RED;
		}

		for (short xi = -146 + 1;xi < 146;xi += 2) {
			hud::DrawTranslatedPatch(xi, y + ty[i], hud_scalescoreboard,
			                         hud::X_CENTER, hud::Y_MIDDLE,
			                         hud::X_CENTER, hud::Y_TOP,
			                         sbline, Ranges + color * 256, true);
			hud::DrawTranslatedPatch(xi, y + ty[i] + 11, hud_scalescoreboard,
			                         hud::X_CENTER, hud::Y_MIDDLE,
			                         hud::X_CENTER, hud::Y_TOP,
			                         sbline, Ranges + color * 256, true);
		}

		// Team Info
		str = hud::TeamName(color, i);
		hud::DrawText(-137, y + ty[i] + 3, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_LEFT, hud::Y_TOP,
		              str.c_str(), color, true);
		if (sv_gametype == GM_CTF) {
			hud::DrawText(34, y + ty[i] + 3, hud_scalescoreboard,
			              hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP,
			              TeamPlayers(color, i).c_str(), color, true);
			hud::DrawText(62, y + ty[i] + 3, hud_scalescoreboard,
			              hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP,
			              hud::TeamPoints(color, i).c_str(), color, true);
			str = hud::TeamFrags(color, i);
			hud::DrawText(90 , y + ty[i] + 3, hud_scalescoreboard,
			              hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP,
			              str.c_str(), color, true);
		} else {
			hud::DrawText(22, y + ty[i] + 3, hud_scalescoreboard,
			              hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP,
			              TeamPlayers(color, i).c_str(), color, true);
			hud::DrawText(50, y + ty[i] + 3, hud_scalescoreboard,
			              hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP,
			              hud::TeamPoints(color, i).c_str(), color, true);
			str = hud::TeamKD(color, i);
			hud::DrawText(90, y + ty[i] + 3, hud_scalescoreboard,
			              hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP,
			              str.c_str(), color, true);
		}
		str = hud::TeamPing(color, i);
		hud::DrawText(146, y + ty[i] + 3, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_RIGHT, hud::Y_TOP,
		              str.c_str(), color, true);

		// Ingame Players
		byte limit = extra_rows + 4;
		hud::EATeamPlayerColors(-146, y + ty[i] + 14, 7, 7, hud_scalescoreboard,
		                        hud::X_CENTER, hud::Y_MIDDLE,
		                        hud::X_LEFT, hud::Y_TOP,
		                        1, limit, i);
		hud::EATeamPlayerNames(-137, y + ty[i] + 14, hud_scalescoreboard,
		                       hud::X_CENTER, hud::Y_MIDDLE,
		                       hud::X_LEFT, hud::Y_TOP,
		                       1, limit, i, true);
		if (sv_gametype == GM_CTF) {
			hud::EATeamPlayerPoints(62, y + ty[i] + 14, hud_scalescoreboard,
			                        hud::X_CENTER, hud::Y_MIDDLE,
			                        hud::X_RIGHT, hud::Y_TOP,
			                        1, limit, i, true);
			hud::EATeamPlayerFrags(90, y + ty[i] + 14, hud_scalescoreboard,
			                       hud::X_CENTER, hud::Y_MIDDLE,
			                       hud::X_RIGHT, hud::Y_TOP,
			                       1, limit, i, true);
		} else {
			hud::EATeamPlayerFrags(50, y + ty[i] + 14, hud_scalescoreboard,
			                       hud::X_CENTER, hud::Y_MIDDLE,
			                       hud::X_RIGHT, hud::Y_TOP,
			                       1, limit, i, true);
			hud::EATeamPlayerKD(90, y + ty[i] + 14, hud_scalescoreboard,
			                    hud::X_CENTER, hud::Y_MIDDLE,
			                    hud::X_RIGHT, hud::Y_TOP,
			                    1, limit, i, true);
		}
		hud::EATeamPlayerTimes(118, y + ty[i] + 14, hud_scalescoreboard,
		                       hud::X_CENTER, hud::Y_MIDDLE,
		                       hud::X_RIGHT, hud::Y_TOP,
		                       1, limit, i, true);
		hud::EATeamPlayerPings(146, y + ty[i] + 14, hud_scalescoreboard,
		                       hud::X_CENTER, hud::Y_MIDDLE,
		                       hud::X_RIGHT, hud::Y_TOP,
		                       1, limit, i, true);
	}
}

// [AM] Draw low-resolution spectators.
void drawLowSpectators(player_t *player, int y, byte extra_rows) {
	static short tx[2] = {-146, 1};

	// Line
	for (short xi = -146 + 1;xi < 146;xi += 2) {
		hud::DrawTranslatedPatch(xi, y, hud_scalescoreboard,
		                         hud::X_CENTER, hud::Y_MIDDLE,
		                         hud::X_CENTER, hud::Y_TOP,
		                         sbline, Ranges + CR_GREY * 256, true);
	}

	byte specs = hud::CountSpectators();
	byte limit;
	byte skip = 0;
	for (byte i = 0;i < 2;i++) {
		limit = (specs + 1 - i) / 2;
		if (extra_rows + 1 < limit) {
			limit = extra_rows + 1;
		}
		hud::EASpectatorNames(tx[i], y + 3, hud_scalescoreboard,
		                      hud::X_CENTER, hud::Y_MIDDLE,
		                      hud::X_LEFT, hud::Y_TOP,
		                      1, skip, limit, true);
		hud::EASpectatorPings(tx[i] + 121, y + 3, hud_scalescoreboard,
		                      hud::X_CENTER, hud::Y_MIDDLE,
		                      hud::X_LEFT, hud::Y_TOP,
		                      1, skip, limit, true);
		skip += limit;
	}
}

// [AM] Draw the low-resolution scoreboard.
void LowScoreboard(player_t *player) {
	int height, y;
	byte extra_spec_rows = 0;
	byte extra_player_rows = 0;

	if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF) {
		height = 129;

		// Team scoreboard was designed for 4 players on a team.  If
		// there are more (per team), increase the height.
		byte blue = hud::CountTeamPlayers(TEAM_BLUE);
		byte red = hud::CountTeamPlayers(TEAM_RED);
		if (blue > 4) {
			extra_player_rows += blue - 4;
		}
		if (red > 4) {
			extra_player_rows += red - 4;
		}
	} else {
		height = 72;

		// Normal scoreboard was designed for 4 players.  If there are
		// more, increase the height.
		byte players = P_NumPlayersInGame();
		if (players > 4) {
			extra_player_rows += players - 4;
		}
	}

	// If there are more than 2 spectators, increase the height.
	byte specs = hud::CountSpectators();
	if (specs > 2) {
		extra_spec_rows += ((specs + 1) / 2) - 1;
	}

	height += (extra_player_rows + extra_spec_rows) * 8;

	// 180 is our max height.
	while (height > 180) {
		// We have too many rows.  Start removing extra spectator rows
		// to make the scoreboard fit.
		if (extra_spec_rows == 0) {
			break;
		}
		extra_spec_rows -= 1;
		height -= 8;
	}

	while (height > 180) {
		// We still have too many rows.  Start removing extra player
		// rows to make the scoreboard fit.
		if (extra_player_rows == 0) {
			break;
		}
		extra_player_rows -= 1;
		if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF) {
			// Removing one player row sometimes means removing two
			// lines on the low resolution team scoreboard.
			if (extra_player_rows + 4 < hud::CountTeamPlayers(TEAM_BLUE)) {
				height -= 8;
			}
			if (extra_player_rows + 4 < hud::CountTeamPlayers(TEAM_RED)) {
				height -= 8;
			}
		} else {
			height -= 8;
		}
	}

	// Starting Y position, measured from the center up.
	y = -(height / 2);
	if (y > -64) {
		// Scoreboard starts slightly off-center, then grows to be centered.
		// -96 makes it even with where the default lowres TDM begins.
		// For the record, the lowres DM scoreboard begins at -54.
		y = -64;
	}

	// Dim the background.
	hud::Dim(0, y, 300, height, hud_scalescoreboard,
	         hud::X_CENTER, hud::Y_MIDDLE,
	         hud::X_CENTER, hud::Y_TOP);

	hud::drawLowHeader(player, y + 4);
	if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF) {
		hud::drawLowTeamScores(player, y + 15,
		                       extra_player_rows);
	} else {
		hud::drawLowScores(player, y + 15,
		                   extra_player_rows);
	}
	hud::drawLowSpectators(player, y + (height - 14 - (extra_spec_rows * 8)),
	                       extra_spec_rows);
}

}

void HU_DrawScores(player_t *player) {
	// We need at least 480 scaled horizontal units for our "high resolution"
	// scoreboard to fit.
	if (hud::XSize(hud_scalescoreboard) >= 480) {
		hud::Scoreboard(player);
	} else {
		hud::LowScoreboard(player);
	}
}

//
// [Toke] OdamexEffect
// Draws the 50% reduction in brightness effect
//
void OdamexEffect (int xa, int ya, int xb, int yb)
{
	if (xa < 0 || ya < 0 || xb > I_GetSurfaceWidth() || yb > I_GetSurfaceHeight())
		return;

	screen->Dim(xa, ya, xb - xa, yb - ya);
}


//
// Comparison functors for sorting vectors of players
//
struct compare_player_frags
{
	bool operator()(const player_t* arg1, const player_t* arg2) const
	{
		return arg2->fragcount < arg1->fragcount;
	}
};

struct compare_player_kills
{
	bool operator()(const player_t* arg1, const player_t* arg2) const
	{
		return arg2->killcount < arg1->killcount;
	}
};

struct compare_player_points
{
	bool operator()(const player_t* arg1, const player_t* arg2) const
	{
		return arg2->points < arg1->points;
	}
};

struct compare_player_names
{
	bool operator()(const player_t* arg1, const player_t* arg2) const
	{
		return arg1->userinfo.netname < arg2->userinfo.netname;
	}
};


static float HU_CalculateKillDeathRatio(const player_t* player)
{
	if (player->killcount <= 0)	// Copied from HU_DMScores1.
		return 0.0f;
	else if (player->killcount >= 1 && player->deathcount == 0)
		return float(player->killcount);
	else
		return float(player->killcount) / float(player->deathcount);
}

static float HU_CalculateFragDeathRatio(const player_t* player)
{
	if (player->fragcount <= 0)	// Copied from HU_DMScores1.
		return 0.0f;
	else if (player->fragcount >= 1 && player->deathcount == 0)
		return float(player->fragcount);
	else
		return float(player->fragcount) / float(player->deathcount);
}

//
// HU_ConsoleScores
// Draws scoreboard to console.
//
// [AM] This should be cleaned up and commonized, because it's useful on the
//      client and server.
//
void HU_ConsoleScores(player_t *player)
{
	char str[1024];

	typedef std::list<const player_t*> PlayerPtrList;
	PlayerPtrList sortedplayers;
	PlayerPtrList sortedspectators;

	for (Players::const_iterator it = players.begin(); it != players.end(); ++it)
		if (it->ingame() && !it->spectator)
			sortedplayers.push_back(&*it);
		else
			sortedspectators.push_back(&*it);

	// One of these at each end prevents the following from
	// drawing on the screen itself.
	C_ToggleConsole();

	if (sv_gametype == GM_CTF)
	{
		compare_player_points comparison_functor;
		sortedplayers.sort(comparison_functor);

		Printf_Bold("\n--------------------------------------\n");
		Printf_Bold("           CAPTURE THE FLAG\n");

		if (sv_scorelimit)
			sprintf(str, "Scorelimit: %-6d", sv_scorelimit.asInt());
		else
			sprintf(str, "Scorelimit: N/A   ");

		Printf_Bold("%s  ", str);

		if (sv_timelimit)
			sprintf(str, "Timelimit: %-7d", sv_timelimit.asInt());
		else
			sprintf(str, "Timelimit: N/A");

		Printf_Bold("%18s\n", str);

		for (int team_num = 0; team_num < NUMTEAMS; team_num++)
		{
			if (team_num == TEAM_BLUE)
				Printf_Bold("\n-----------------------------BLUE TEAM\n");
			else if (team_num == TEAM_RED)
				Printf_Bold("\n------------------------------RED TEAM\n");
			else		// shouldn't happen
				Printf_Bold("\n--------------------------UNKNOWN TEAM\n");

			Printf_Bold("Name            Points Caps Frags Time\n");
			Printf_Bold("--------------------------------------\n");

			for (PlayerPtrList::const_iterator it = sortedplayers.begin(); it != sortedplayers.end(); ++it)
			{
				const player_t* itplayer = *it;
				if (itplayer->userinfo.team == team_num)
				{
					sprintf(str, "%-15s %-6d N/A  %-5d %4d\n",
							itplayer->userinfo.netname.c_str(),
							itplayer->points,
							//itplayer->captures,
							itplayer->fragcount,
							itplayer->GameTime / 60);

					if (itplayer == player)
						Printf_Bold(str);
					else
						Printf(PRINT_HIGH, str);
				}
			}
		}
	}

	else if (sv_gametype == GM_TEAMDM)
	{
		compare_player_frags comparison_functor;
		sortedplayers.sort(comparison_functor);

		Printf_Bold("\n--------------------------------------\n");
		Printf_Bold("           TEAM DEATHMATCH\n");

		if (sv_fraglimit)
			sprintf(str, "Fraglimit: %-7d", sv_fraglimit.asInt());
		else
			sprintf(str, "Fraglimit: N/A    ");

		Printf_Bold("%s  ", str);

		if (sv_timelimit)
			sprintf(str, "Timelimit: %-7d", sv_timelimit.asInt());
		else
			sprintf(str, "Timelimit: N/A");

		Printf_Bold("%18s\n", str);

		for (int team_num = 0; team_num < NUMTEAMS; team_num++)
		{
			if (team_num == TEAM_BLUE)
				Printf_Bold("\n-----------------------------BLUE TEAM\n");
			else if (team_num == TEAM_RED)
				Printf_Bold("\n------------------------------RED TEAM\n");
			else		// shouldn't happen
				Printf_Bold("\n--------------------------UNKNOWN TEAM\n");

			Printf_Bold("Name            Frags Deaths  K/D Time\n");
			Printf_Bold("--------------------------------------\n");

			for (PlayerPtrList::const_iterator it = sortedplayers.begin(); it != sortedplayers.end(); ++it)
			{
				const player_t* itplayer = *it;
				if (itplayer->userinfo.team == team_num)
				{
					sprintf(str, "%-15s %-5d %-6d %2.1f %4d\n",
							itplayer->userinfo.netname.c_str(),
							itplayer->fragcount,
							itplayer->deathcount,
							HU_CalculateFragDeathRatio(itplayer),
							itplayer->GameTime / 60);

					if (itplayer == player)
						Printf_Bold(str);
					else
						Printf(PRINT_HIGH, str);
				}
			}
		}
	}

	else if (sv_gametype == GM_DM)
	{
		compare_player_frags comparison_functor;
		sortedplayers.sort(comparison_functor);

		Printf_Bold("\n--------------------------------------\n");
		Printf_Bold("              DEATHMATCH\n");

		if (sv_fraglimit)
			sprintf(str, "Fraglimit: %-7d", sv_fraglimit.asInt());
		else
			sprintf(str, "Fraglimit: N/A    ");

		Printf_Bold("%s  ", str);

		if (sv_timelimit)
			sprintf(str, "Timelimit: %-7d", sv_timelimit.asInt());
		else
			sprintf(str, "Timelimit: N/A");

		Printf_Bold("%18s\n", str);

		Printf_Bold("Name            Frags Deaths  K/D Time\n");
		Printf_Bold("--------------------------------------\n");

		for (PlayerPtrList::const_iterator it = sortedplayers.begin(); it != sortedplayers.end(); ++it)
		{
			const player_t* itplayer = *it;
			sprintf(str, "%-15s %-5d %-6d %2.1f %4d\n",
					itplayer->userinfo.netname.c_str(),
					itplayer->fragcount,
					itplayer->deathcount,
					HU_CalculateFragDeathRatio(itplayer),
					itplayer->GameTime / 60);

			if (itplayer == player)
				Printf_Bold(str);
			else
				Printf(PRINT_HIGH, str);
		}

	}

	else if (sv_gametype == GM_COOP)
	{
		compare_player_kills comparison_functor;
		sortedplayers.sort(comparison_functor);

		Printf_Bold("\n--------------------------------------\n");
		Printf_Bold("             COOPERATIVE\n");
		Printf_Bold("Name            Kills Deaths  K/D Time\n");
		Printf_Bold("--------------------------------------\n");

		for (PlayerPtrList::const_iterator it = sortedplayers.begin(); it != sortedplayers.end(); ++it)
		{
			const player_t* itplayer = *it;
			sprintf(str,"%-15s %-5d %-6d %2.1f %4d\n",
					itplayer->userinfo.netname.c_str(),
					itplayer->killcount,
					itplayer->deathcount,
					HU_CalculateKillDeathRatio(itplayer),
					itplayer->GameTime / 60);

			if (itplayer == player)
				Printf_Bold(str);
			else
				Printf(PRINT_HIGH, str);
		}
	}

	if (!sortedspectators.empty())
	{
		compare_player_names comparison_functor;
		sortedspectators.sort(comparison_functor);

		Printf_Bold("\n----------------------------SPECTATORS\n");

		for (PlayerPtrList::const_iterator it = sortedspectators.begin(); it != sortedspectators.end(); ++it)
		{
			const player_t* itplayer = *it;
			sprintf(str, "%-15s\n", itplayer->userinfo.netname.c_str());
			if (itplayer == player)
				Printf_Bold(str);
			else
				Printf(PRINT_HIGH, str);
		}
	}

	Printf(PRINT_HIGH, "\n");

	C_ToggleConsole();
}

BEGIN_COMMAND (displayscores)
{
	if (multiplayer)
	    HU_ConsoleScores(&consoleplayer());
	else
		Printf(PRINT_HIGH, "This command is only used for multiplayer games.");
}
END_COMMAND (displayscores)

VERSION_CONTROL (hu_stuff_cpp, "$Id$")


