// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Heads-up displays
//
//-----------------------------------------------------------------------------

#include <algorithm>
#include <iterator>
#include <sstream>

#include "doomdef.h"
#include "z_zone.h"
#include "hu_stuff.h"
#include "w_wad.h"
#include "s_sound.h"
#include "doomstat.h"
#include "st_stuff.h"
#include "c_bind.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "c_cvars.h"
#include "v_text.h"
#include "g_gametype.h"

#include "cl_main.h"
#include "p_ctf.h"
#include "i_video.h"
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
EXTERN_CVAR(sv_teamsinplay)
EXTERN_CVAR(g_lives)

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

patch_t* sbline;

void HU_DrawScores (player_t *plyr);
void HU_ConsoleScores (player_t *plyr);

// [Toke - Scores]
void HU_DMScores1 (player_t *player);
void HU_DMScores2 (player_t *player);
void HU_TeamScores1 (player_t *player);
void HU_TeamScores2 (player_t *player);

extern bool HasBehavior;
extern inline int V_StringWidth(const char *str);
size_t P_NumPlayersInGame();
static void ShoveChatStr(std::string str, byte who);

static std::string input_text;
static chatmode_t chatmode;

static const int DefaultTeamHeight = 56;

chatmode_t HU_ChatMode()
{
	return chatmode;
}

void HU_SetChatMode()
{
	chatmode = CHAT_NORMAL;
}

void HU_SetTeamChatMode()
{
	chatmode = CHAT_TEAM;
}

void HU_UnsetChatMode()
{
	chatmode = CHAT_INACTIVE;
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

static const int HiResolutionWidth = 480;

//
// HU_Init
//
void HU_Init()
{
	char buffer[12];

	HU_UnsetChatMode();
	input_text.clear();

	V_TextInit();

	// Load the status bar line
	sbline = W_CachePatch("SBLINE", PU_STATIC);

	HU_InitCrosshair();
}


//
// HU_Shutdown
//
// Frees any memory allocated specifically for the HUD.
//
void STACK_ARGS HU_Shutdown()
{
	Z_Discard(&::sbline);

	V_TextShutdown();
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
		HU_UnsetChatMode();
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
	if (ev->data1 == OKEY_RALT || ev->data1 == OKEY_LALT || ev->data1 == OKEY_HAT1)
	{
		altdown = (ev->type == ev_keydown);
		return false;
	}
	else if (ev->data1 == OKEY_LSHIFT || ev->data1 == OKEY_RSHIFT)
	{
		return false;
	}
	else if ((gamestate != GS_LEVEL && gamestate != GS_INTERMISSION) || ev->type != ev_keydown)
	{
		if (HU_ChatMode() != CHAT_INACTIVE)
            return true;

		return false;
	}

	if (HU_ChatMode() == CHAT_INACTIVE)
		return false;

	if (altdown)
	{
		// send a macro
		if (ev->data1 >= OKEY_JOY1 && ev->data1 <= OKEY_JOY10)
		{
			ShoveChatStr(chat_macros[ev->data1 - OKEY_JOY1]->cstring(), HU_ChatMode()- 1);
			HU_UnsetChatMode();
			return true;
		}
		else if (ev->data1 >= '0' && ev->data1 <= '9')
		{
			ShoveChatStr(chat_macros[ev->data1 - '0']->cstring(), HU_ChatMode() - 1);
			HU_UnsetChatMode();
			return true;
		}
	}
	if (ev->data1 == OKEY_ENTER || ev->data1 == OKEYP_ENTER)
	{
		ShoveChatStr(input_text, HU_ChatMode() - 1);
		HU_UnsetChatMode();
		return true;
	}
	else if (ev->data1 == OKEY_ESCAPE || ev->data1 == OKEY_JOY2)
	{
		HU_UnsetChatMode();
		return true;
	}
	else if (ev->data1 == OKEY_BACKSPACE)
	{
		if (!input_text.empty())
			input_text.erase(input_text.end() - 1);
		return true;
	}

	int textkey = ev->data3;	// [RH] Use localized keymap
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

		if ((xhair = W_CheckNumForName(xhairname)) == -1)
			xhair = W_CheckNumForName("XHAIR1");

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
			if (camera->health > 75)
			{
				crosshair_trans[crosshair_color] =
				    V_BestColor(V_GetDefaultPalette()->basecolors, 0x00, 0xFF, 0x00);
			}
			else if (camera->health > 50)
			{
				crosshair_trans[crosshair_color] =
				    V_BestColor(V_GetDefaultPalette()->basecolors, 0xFF, 0xFF, 0x00);
			}
			else if (camera->health > 25)
			{
				crosshair_trans[crosshair_color] =
				    V_BestColor(V_GetDefaultPalette()->basecolors, 0xFF, 0x7F, 0x00);
			}
			else
			{
				crosshair_trans[crosshair_color] =
				    V_BestColor(V_GetDefaultPalette()->basecolors, 0xFF, 0x00, 0x00);
			}
		}
		else
		{
			crosshair_trans[crosshair_color] = crosshair_color_custom;
		}

		V_ColorMap = translationref_t(crosshair_trans);

		int x = I_GetSurfaceWidth() / 2;
		int y = I_GetSurfaceHeight() / 2;

		if (R_StatusBarVisible())
			y = ST_StatusBarY(I_GetSurfaceWidth(), I_GetSurfaceHeight()) / 2;

		if (hud_crosshairdim && hud_crosshairscale)
			screen->DrawTranslatedLucentPatchCleanNoMove(W_CachePatch(crosshair_lump), x, y);
        else if (hud_crosshairscale)
			screen->DrawTranslatedPatchCleanNoMove(W_CachePatch(crosshair_lump), x, y);
        else if (hud_crosshairdim)
			screen->DrawTranslatedLucentPatch(W_CachePatch(crosshair_lump), x, y);
		else
			screen->DrawTranslatedPatch (W_CachePatch (crosshair_lump), x, y);
	}
}


static void HU_DrawChatPrompt()
{
	// Don't draw the chat prompt without a valid font.
	if (::hu_font[0] == NULL)
		return;

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
	if (HU_ChatMode() == CHAT_TEAM)
		prompt = "Say (TEAM): ";
	else if (HU_ChatMode() == CHAT_NORMAL)
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

		hud::LevelStateHUD();
	}

	// [csDoom] draw disconnected wire [Toke] Made this 1337er
	// denis - moved to hu_stuff and uncommented
	if (noservermsgs && (gamestate == GS_INTERMISSION || gamestate == GS_LEVEL))
		screen->DrawPatchCleanNoMove(W_CachePatch("NET"), 50 * CleanXfac, 1 * CleanYfac);

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

	if (consoleplayer().camera && !(demoplayback))
	{
		if ((gamestate != GS_INTERMISSION && Actions[ACTION_SHOWSCORES]) ||
		    (::multiplayer && hud_show_scoreboard_ondeath &&
		     displayplayer().health <= 0 && !displayplayer().spectator))
		{
			HU_DrawScores(&displayplayer());
		}
	}

	if (gamestate == GS_LEVEL)
		HU_DrawCrosshair();

	if (HU_ChatMode() != CHAT_INACTIVE)
		HU_DrawChatPrompt();
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

	HU_SetChatMode();
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

	HU_SetTeamChatMode();
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
EXTERN_CVAR(hud_timer)

EXTERN_CVAR(sv_gametype)
EXTERN_CVAR(sv_maxplayers)
EXTERN_CVAR(sv_hostname)
EXTERN_CVAR(g_winlimit)
EXTERN_CVAR(g_roundlimit)
EXTERN_CVAR(g_rounds)

namespace hud {

static int GetLongestTeamWidth()
{
	int color;
	int longest = 0;
	std::string longestTeamName;
	for (int i = 0; i < sv_teamsinplay; i++)
	{
		std::string name = TeamName(color, i);
		int length = name.length();
		if (length > longest)
		{
			longest = length;
			longestTeamName = name;
		}
	}

	if (g_rounds)
		longestTeamName.append(" WINS: ");
	else if (sv_gametype == GM_TEAMDM)
		longestTeamName.append(" FRAGS: ");
	else
		longestTeamName.append(" POINTS: ");
	return V_StringWidth(longestTeamName.c_str());
}

// [AM] Draw scoreboard header
void drawHeader(player_t *player, int y)
{
	int color;
	std::string str = G_GametypeName();

	hud::DrawText(0, y, hud_scalescoreboard,
	              hud::X_CENTER, hud::Y_MIDDLE,
	              hud::X_CENTER, hud::Y_TOP,
	              str.c_str(), CR_GOLD, true);

	brokenlines_t* hostname = NULL;
	if (::multiplayer)
	{
		hostname = V_BreakLines(192, sv_hostname.cstring());
	}
	else
	{
		hostname = V_BreakLines(192, "Odamex " DOTVERSIONSTR " - Offline");
	}

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

	int yOffset = 0;
	if (G_IsTeamGame()) 
	{
		int xOffset = GetLongestTeamWidth();
		
		for (int i = 0; i < sv_teamsinplay; i++)
		{
			// Display wins for round-based gamemodes, otherwise points.
			std::string displayName = TeamName(color, i);
			if (g_rounds)
				displayName.append(" WINS: ");
			else if (sv_gametype == GM_TEAMDM)
				displayName.append(" FRAGS: ");
			else
				displayName.append(" POINTS: ");

			yOffset += 8;
			hud::DrawText(-236, y + yOffset, hud_scalescoreboard, hud::X_CENTER,
			              hud::Y_MIDDLE, hud::X_LEFT, hud::Y_TOP, displayName.c_str(),
			              CR_GREY, true);

			std::string points;
			if (g_rounds)
			{
				StrFormat(points, "%d", GetTeamInfo((team_t)i)->RoundWins);
			}
			else
			{
				StrFormat(points, "%d", GetTeamInfo((team_t)i)->Points);
			}

			hud::DrawText(-236 + xOffset, y + yOffset, hud_scalescoreboard, hud::X_CENTER,
			              hud::Y_MIDDLE, hud::X_LEFT, hud::Y_TOP, points.c_str(),
			              CR_GREEN, true);
		}
	} 
	else
	{
		hud::DrawText(-236, y + 8, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_LEFT, hud::Y_TOP,
		              "PLAYERS: ", CR_GREY, true);
		hud::DrawText(-236 + V_StringWidth("PLAYERS: "), y + 8, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_LEFT, hud::Y_TOP,
		              hud::PlayersSplit().c_str(), CR_GREEN, true);
		yOffset += 16;
	}

	// Right
	std::string timer, fraglimit, scorelimit;
	StringTokens names, values;

	if (::gamestate == GS_INTERMISSION)
	{
		names.push_back("NEXT MAP IN: ");
		timer = hud::IntermissionTimer();
		values.push_back(timer);
	}
	else if (sv_timelimit > 0.0)
	{
		// Timelimit.
		if (hud_timer == 2)
			names.push_back("TIME: ");
		else
			names.push_back("TIME LEFT: ");

		timer = hud::Timer();
		values.push_back(timer);
	}

	// Winlimit.
	if (g_winlimit > 0.0 && G_UsesWinlimit())
	{
		StrFormat(str, "%d", g_winlimit.asInt());
		names.push_back("WIN LIMIT: ");
		values.push_back(str);
	}

	// Roundlimit.
	if (g_roundlimit > 0.0 && G_UsesRoundlimit())
	{
		StrFormat(str, "%d", g_roundlimit.asInt());
		names.push_back("ROUND LIMIT: ");
		values.push_back(str);
	}

	// Scorelimit.
	if (sv_scorelimit > 0.0 && G_UsesScorelimit())
	{
		StrFormat(str, "%d", sv_scorelimit.asInt());
		names.push_back("SCORE LIMIT: ");
		values.push_back(str);
	}

	// Fraglimit
	if (sv_fraglimit > 0.0 && G_UsesFraglimit())
	{
		StrFormat(str, "%d", sv_fraglimit.asInt());
		names.push_back("FRAG LIMIT: ");
		values.push_back(str);
	}

	int rw = V_StringWidth("00:00");
	if (sv_timelimit.asInt() == 0 && gamestate != GS_INTERMISSION)
		rw = V_StringWidth("N/A");
	else if (timer.size() > 5)
		rw = V_StringWidth("00:00:00");

	StringTokens::const_iterator it;
	for (it = values.begin(); it != values.end(); ++it)
		rw = std::max(V_StringWidth(it->c_str()), rw);

	for (size_t i = 0; i < values.size() && i < 3; i++)
	{
		int yoff = i * 8;
		hud::DrawText(236 - rw, y + yoff, hud_scalescoreboard, hud::X_CENTER,
		              hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, names.at(i).c_str(),
		              CR_GREY, true);
		hud::DrawText(236 - rw, y + yoff, hud_scalescoreboard, hud::X_CENTER,
		              hud::Y_MIDDLE, hud::X_LEFT, hud::Y_TOP, values.at(i).c_str(),
		              CR_GREEN, true);
	}

	// Line
	for (short xi = -236 + 1;xi < 236;xi += 2) {
		hud::DrawTranslatedPatch(xi, y + yOffset + 8, hud_scalescoreboard,
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

	if (sv_gametype == GM_COOP)
	{
		if (g_lives)
		{
			hud::DrawText(92, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "KILLS", CR_GREY, true);
			hud::DrawText(140, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "LIVES", CR_GREY, true);
		}
		else
		{
			hud::DrawText(92, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "KILLS", CR_GREY, true);
			hud::DrawText(140, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "DETHS", CR_GREY, true);
		}
	}
	else
	{
		if (g_lives)
		{
			hud::DrawText(44, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "WINS", CR_GREY, true);
			hud::DrawText(140, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "LIVES", CR_GREY, true);
			hud::DrawText(92, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "FRAGS", CR_GREY, true);
		}
		else
		{
			hud::DrawText(44, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "FRAGS", CR_GREY, true);
			hud::DrawText(92, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "DETHS", CR_GREY, true);
			hud::DrawText(140, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "K/D", CR_GREY, true);
		}
	}

	hud::DrawText(188, y, hud_scalescoreboard,
	              hud::X_CENTER, hud::Y_MIDDLE,
	              hud::X_RIGHT, hud::Y_TOP,
	              "MINS", CR_GREY, true);
	hud::DrawText(236, y, hud_scalescoreboard,
	              hud::X_CENTER, hud::Y_MIDDLE,
	              hud::X_RIGHT, hud::Y_TOP,
	              "PING", CR_GREY, true);

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

	if (sv_gametype == GM_COOP)
	{
		if (g_lives)
		{
			hud::EAPlayerKills(92, y + 11, hud_scalescoreboard, hud::X_CENTER,
			                   hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, 1, limit, true);
			hud::EAPlayerLives(140, y + 11, hud_scalescoreboard, hud::X_CENTER,
			                   hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, 1, limit, true);
		}
		else
		{
			hud::EAPlayerKills(92, y + 11, hud_scalescoreboard, hud::X_CENTER,
			                   hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, 1, limit, true);
			hud::EAPlayerDeaths(140, y + 11, hud_scalescoreboard, hud::X_CENTER,
			                    hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, 1, limit, true);
		}
	}
	else
	{
		if (g_lives)
		{
			hud::EAPlayerRoundWins(44, y + 11, hud_scalescoreboard, hud::X_CENTER,
			                       hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, 1, limit,
			                       true);
			hud::EAPlayerLives(140, y + 11, hud_scalescoreboard, hud::X_CENTER,
			                   hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, 1, limit, true);
			hud::EAPlayerFrags(92, y + 11, hud_scalescoreboard, hud::X_CENTER,
			                   hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, 1, limit, true);
		}
		else
		{
			hud::EAPlayerFrags(44, y + 11, hud_scalescoreboard, hud::X_CENTER,
			                   hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, 1, limit, true);
			hud::EAPlayerDeaths(92, y + 11, hud_scalescoreboard, hud::X_CENTER,
			                    hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, 1, limit, true);
			hud::EAPlayerKD(140, y + 11, hud_scalescoreboard, hud::X_CENTER,
			                hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, 1, limit, true);
		}
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
void drawTeamScores(player_t *player, int& y, byte extra_rows) {
	int color;
	std::ostringstream buffer;
	std::string str;
	static short tx[2] = {-236, 4};

	int teams = sv_teamsinplay.asInt();

	for (int i = 0; i < teams + 1; i++)
	{
		int xOffset = tx[i % 2];
		int row = i / 2; // Set of two teams per row
		int yOffset = (row * DefaultTeamHeight) + (row * extra_rows * 8);

		// Add spacing between team rows
		if (row > 0)
			yOffset += 8 * row;

		// Adjust y with next row yOffset so spectators draw correctly
		if (i == teams)
		{
			if (i % 2 != 0)
				row++;
			y += (row * DefaultTeamHeight) + (row * extra_rows * 8);
			// Account for extra spacing between rows after the first
			if (row > 1)
				y += row * 8;
			break;
		}

		// Column headers
		hud::DrawText(xOffset + 9, yOffset + y, hud_scalescoreboard,
			hud::X_CENTER, hud::Y_MIDDLE,
			hud::X_LEFT, hud::Y_TOP,
			"Name", CR_GREY, true);
		if (sv_gametype == GM_CTF)
		{
			if (g_lives)
			{
				hud::DrawText(xOffset + 168, yOffset + y, hud_scalescoreboard,
				              hud::X_CENTER, hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP,
				              "PTS", CR_GREY, true);
				hud::DrawText(xOffset + 200, yOffset + y, hud_scalescoreboard,
				              hud::X_CENTER, hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP,
				              "LIV", CR_GREY, true);
			}
			else
			{
				hud::DrawText(xOffset + 168, yOffset + y, hud_scalescoreboard,
				              hud::X_CENTER, hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP,
				              "PTS", CR_GREY, true);
				hud::DrawText(xOffset + 200, yOffset + y, hud_scalescoreboard,
				              hud::X_CENTER, hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP,
				              "FRG", CR_GREY, true);
			}
		}
		else
		{
			if (g_lives)
			{
				hud::DrawText(xOffset + 164, yOffset + y, hud_scalescoreboard,
				              hud::X_CENTER, hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP,
				              "FRG", CR_GREY, true);
				hud::DrawText(xOffset + 200, y, hud_scalescoreboard, hud::X_CENTER,
				              hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, "LIV", CR_GREY,
				              true);
			}
			else
			{
				hud::DrawText(xOffset + 164, yOffset + y, hud_scalescoreboard,
				              hud::X_CENTER, hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP,
				              "FRG", CR_GREY, true);
				hud::DrawText(xOffset + 200, y, hud_scalescoreboard, hud::X_CENTER,
				              hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, "K/D", CR_GREY,
				              true);
			}
		}
		hud::DrawText(xOffset + 232, yOffset + y, hud_scalescoreboard,
			hud::X_CENTER, hud::Y_MIDDLE,
			hud::X_RIGHT, hud::Y_TOP,
			"PNG", CR_GREY, true);

		color = V_GetTextColor(GetTeamInfo((team_t)i)->TextColor.c_str());

		for (short xi = xOffset; xi < xOffset + 232; xi += 2)
		{
			hud::DrawTranslatedPatch(xi, yOffset + y + 8, hud_scalescoreboard,
				hud::X_CENTER, hud::Y_MIDDLE,
				hud::X_LEFT, hud::Y_TOP,
				sbline, Ranges + color * 256, true);
			hud::DrawTranslatedPatch(xi, yOffset + y + 19, hud_scalescoreboard,
				hud::X_CENTER, hud::Y_MIDDLE,
				hud::X_LEFT, hud::Y_TOP,
				sbline, Ranges + color * 256, true);
		}

		// Team Info
		str = hud::TeamName(color, i);
		hud::DrawText(xOffset + 9, yOffset + y + 11, hud_scalescoreboard,
			hud::X_CENTER, hud::Y_MIDDLE,
			hud::X_LEFT, hud::Y_TOP,
			str.c_str(), color, true);
		if (sv_gametype == GM_CTF)
		{
			if (g_lives)
			{
				str = hud::TeamPoints(color, i);
				hud::DrawText(xOffset + 168, yOffset + y + 11, hud_scalescoreboard,
				              hud::X_CENTER, hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP,
				              str.c_str(), color, true);
				hud::TeamLives(str, color, i);
				hud::DrawText(xOffset + 200, yOffset + y + 11, hud_scalescoreboard,
				              hud::X_CENTER, hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP,
				              str.c_str(), color, true);
			}
			else
			{
				str = hud::TeamPoints(color, i);
				hud::DrawText(xOffset + 168, yOffset + y + 11, hud_scalescoreboard,
				              hud::X_CENTER, hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP,
				              str.c_str(), color, true);
				str = hud::TeamFrags(color, i);
				hud::DrawText(xOffset + 200, yOffset + y + 11, hud_scalescoreboard,
				              hud::X_CENTER, hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP,
				              str.c_str(), color, true);
			}
		}
		else
		{
			if (g_lives)
			{
				str = hud::TeamPoints(color, i);
				hud::DrawText(xOffset + 164, yOffset + y + 11, hud_scalescoreboard,
				              hud::X_CENTER, hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP,
				              str.c_str(), color, true);
				hud::TeamLives(str, color, i);
				hud::DrawText(xOffset + 200, yOffset + y + 11, hud_scalescoreboard,
				              hud::X_CENTER, hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP,
				              str.c_str(), color, true);
			}
			else
			{
				str = hud::TeamPoints(color, i);
				hud::DrawText(xOffset + 164, yOffset + y + 11, hud_scalescoreboard,
				              hud::X_CENTER, hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP,
				              str.c_str(), color, true);
				str = hud::TeamKD(color, i);
				hud::DrawText(xOffset + 200, yOffset + y + 11, hud_scalescoreboard,
				              hud::X_CENTER, hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP,
				              str.c_str(), color, true);
			}
		}
		str = hud::TeamPing(color, i);
		hud::DrawText(xOffset + 232, yOffset + y + 11, hud_scalescoreboard,
			hud::X_CENTER, hud::Y_MIDDLE,
			hud::X_RIGHT, hud::Y_TOP,
			str.c_str(), color, true);

		// Ingame Players
		byte limit = extra_rows + 4;
		hud::EATeamPlayerColors(xOffset, yOffset + y + 22, 7, 7, hud_scalescoreboard,
			hud::X_CENTER, hud::Y_MIDDLE,
			hud::X_LEFT, hud::Y_TOP,
			1, limit, i);
		hud::EATeamPlayerNames(xOffset + 9, yOffset + y + 22, hud_scalescoreboard,
			hud::X_CENTER, hud::Y_MIDDLE,
			hud::X_LEFT, hud::Y_TOP,
			1, limit, i, true);
		if (sv_gametype == GM_CTF)
		{
			if (g_lives)
			{
				hud::EATeamPlayerPoints(xOffset + 168, yOffset + y + 22,
				                        hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
				                        hud::X_RIGHT, hud::Y_TOP, 1, limit, i, true);
				hud::EATeamPlayerLives(xOffset + 200, yOffset + y + 22,
				                       hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
				                       hud::X_RIGHT, hud::Y_TOP, 1, limit, i, true);
			}
			else
			{
				hud::EATeamPlayerPoints(xOffset + 168, yOffset + y + 22,
				                        hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
				                        hud::X_RIGHT, hud::Y_TOP, 1, limit, i, true);
				hud::EATeamPlayerFrags(xOffset + 200, yOffset + y + 22,
				                       hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
				                       hud::X_RIGHT, hud::Y_TOP, 1, limit, i, true);
			}
		}
		else
		{
			if (g_lives)
			{
				hud::EATeamPlayerFrags(xOffset + 164, yOffset + y + 22,
				                       hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
				                       hud::X_RIGHT, hud::Y_TOP, 1, limit, i, true);
				hud::EATeamPlayerLives(xOffset + 200, yOffset + y + 22,
				                       hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
				                       hud::X_RIGHT, hud::Y_TOP, 1, limit, i, true);
			}
			else
			{
				hud::EATeamPlayerFrags(xOffset + 164, yOffset + y + 22,
				                       hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
				                       hud::X_RIGHT, hud::Y_TOP, 1, limit, i, true);
				hud::EATeamPlayerKD(xOffset + 200, yOffset + y + 22, hud_scalescoreboard,
				                    hud::X_CENTER, hud::Y_MIDDLE, hud::X_RIGHT,
				                    hud::Y_TOP, 1, limit, i, true);
			}
		}
		hud::EATeamPlayerPings(xOffset + 232, yOffset + y + 22, hud_scalescoreboard,
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

static int GetExtraPlayerTeamRowsQuad()
{
	int max = 0;
	for (int i = 0; i < sv_teamsinplay; i++)
	{
		int count = hud::CountTeamPlayers(i);
		if (count > max)
			max = count;
	}

	if (max > 4)
		return max - 4;

	return 0;
}

static int GetExtraPlayerTeamRowsList()
{
	int extra = 0;
	for (int i = 0; i < sv_teamsinplay; i++)
	{
		int count = hud::CountTeamPlayers(i);
		if (count > 4)
			extra += count - 4;
	}

	return extra;
}

static void ClampToScreenTopLeft(int& y, int& height)
{
	int maxHeight = hud::YSize(hud_scalescoreboard);
	if (height > maxHeight)
	{
		y = -(maxHeight / 2);
		height = maxHeight;
		if (maxHeight % 2 != 0)
			height--;
	}
	if (abs(y) > maxHeight / 2)
	{
		if (y < 0)
			y = -maxHeight / 2;
		else
			y = maxHeight / 2;
	}
}

// [AM] Draw the scoreboard
void Scoreboard(player_t *player)
{
	// Calculate height
	int height, y;
	int extra_spec_rows = 0;
	int extra_player_rows = 0;

	// Reset playerID to self if ever happening to spy.
	if (gamestate == GS_INTERMISSION)
		displayplayer_id = consoleplayer_id;

	int extraQuadRows = 0;

	if (G_IsTeamGame())
	{
		height = 100;

		// Team scoreboard was designed for 4 players on a team. If there are more, increase the height.
		extra_player_rows += GetExtraPlayerTeamRowsQuad();

		if (sv_teamsinplay > 2)
		{
			int teams = sv_teamsinplay.asInt();
			if (teams % 2 != 0)
				teams++;
			extraQuadRows = teams - 2;
			height += extraQuadRows / 2 * DefaultTeamHeight;
			height += extraQuadRows * 16;
		}
	} 
	else
	{
		height = 92;

		// Normal scoreboard was designed for 4 players. If there are more, increase the height.
		int inGamePlayers = P_NumPlayersInGame();
		if (inGamePlayers > 4)
			extra_player_rows += inGamePlayers - 4;
	}

	// If there are more than 3 spectators, increase the height.
	int specs = hud::CountSpectators();
	if (specs > 3)
		extra_spec_rows += ((specs + 2) / 3) - 1;

	height += extra_player_rows * 8;
	height += extra_spec_rows * 8;

	// Starting Y position, measured from the center up.
	y = -(height / 2);
	if (y > -96)
	{
		// Scoreboard starts slightly off-center, then grows to be centered.
		// -96 makes it even with where the default lowres TDM begins.
		// For the record, the lowres DM scoreboard begins at -54.
		y = -96;
	}

	ClampToScreenTopLeft(y, height);
	hud::Dim(0, y, HiResolutionWidth, height, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE, hud::X_CENTER, hud::Y_TOP);

	hud::drawHeader(player, y + 4);
	if (G_IsTeamGame())
	{
		// Teams after 2 require extra player count in header
		int teams = sv_teamsinplay - 2;
		if (teams % 2 != 0)
			teams++;
		y += 31 + (teams * 8);
		hud::drawTeamScores(player, y, extra_player_rows);
	} 
	else
	{
		y += 31;
		hud::drawScores(player, y, extra_player_rows);
		y += extra_player_rows * 8 + 48;
	}

	hud::drawSpectators(player, y, extra_spec_rows);
}

// [AM] Draw the low-resolution scoreboard header.
void drawLowHeader(player_t *player, int y) {
	std::string str = G_GametypeName();

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
	if (sv_gametype == GM_COOP)
	{
		if (g_lives)
		{
			hud::DrawText(62, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "KIL", CR_GREY, true);
			hud::DrawText(90, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "LIV", CR_GREY, true);
		}
		else
		{
			hud::DrawText(62, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "KIL", CR_GREY, true);
			hud::DrawText(90, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "DTH", CR_GREY, true);
		}
	}
	else
	{
		if (g_lives)
		{
			hud::DrawText(22, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "WIN", CR_GREY, true);
			hud::DrawText(50, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "LIV", CR_GREY, true);
			hud::DrawText(90, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "FRG", CR_GREY, true);
		}
		else
		{
			hud::DrawText(22, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "FRG", CR_GREY, true);
			hud::DrawText(50, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "DTH", CR_GREY, true);
			hud::DrawText(90, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "K/D", CR_GREY, true);
		}
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
	if (sv_gametype == GM_COOP)
	{
		if (g_lives)
		{
			hud::EAPlayerKills(62, y + 11, hud_scalescoreboard, hud::X_CENTER,
			                   hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, 1, limit, true);
			hud::EAPlayerLives(90, y + 11, hud_scalescoreboard, hud::X_CENTER,
			                   hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, 1, limit, true);
		}
		else
		{
			hud::EAPlayerKills(62, y + 11, hud_scalescoreboard, hud::X_CENTER,
			                   hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, 1, limit, true);
			hud::EAPlayerDeaths(90, y + 11, hud_scalescoreboard, hud::X_CENTER,
			                    hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, 1, limit, true);
		}
	}
	else
	{
		if (g_lives)
		{
			hud::EAPlayerRoundWins(22, y + 11, hud_scalescoreboard, hud::X_CENTER,
			                       hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, 1, limit,
			                       true);
			hud::EAPlayerLives(50, y + 11, hud_scalescoreboard, hud::X_CENTER,
			                   hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, 1, limit, true);
			hud::EAPlayerFrags(90, y + 11, hud_scalescoreboard, hud::X_CENTER,
			                   hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, 1, limit, true);
		}
		else
		{
			hud::EAPlayerFrags(22, y + 11, hud_scalescoreboard, hud::X_CENTER,
			                   hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, 1, limit, true);
			hud::EAPlayerDeaths(50, y + 11, hud_scalescoreboard, hud::X_CENTER,
			                    hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, 1, limit, true);
			hud::EAPlayerKD(90, y + 11, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			                hud::X_RIGHT, hud::Y_TOP, 1, limit, true);
		}
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

	if (sv_gametype == GM_CTF)
	{
		hud::DrawText(34, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_RIGHT, hud::Y_TOP, "PPL", CR_GREY, true);
		if (g_lives)
		{
			hud::DrawText(62, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "PTS", CR_GREY, true);
			hud::DrawText(90, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "LIV", CR_GREY, true);
		}
		else
		{
			hud::DrawText(62, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "PTS", CR_GREY, true);
			hud::DrawText(90, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "FRG", CR_GREY, true);
		}
	}
	else
	{
		hud::DrawText(22, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_RIGHT, hud::Y_TOP, "PPL", CR_GREY, true);
		if (g_lives)
		{
			hud::DrawText(50, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "FRG", CR_GREY, true);
			hud::DrawText(90, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "LIV", CR_GREY, true);
		}
		else
		{
			hud::DrawText(50, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "FRG", CR_GREY, true);
			hud::DrawText(90, y, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE,
			              hud::X_RIGHT, hud::Y_TOP, "K/D", CR_GREY, true);
		}
	}
	hud::DrawText(118, y, hud_scalescoreboard,
	              hud::X_CENTER, hud::Y_MIDDLE,
	              hud::X_RIGHT, hud::Y_TOP,
	              "MIN", CR_GREY, true);
	hud::DrawText(146, y, hud_scalescoreboard,
	              hud::X_CENTER, hud::Y_MIDDLE,
	              hud::X_RIGHT, hud::Y_TOP,
	              "PNG", CR_GREY, true);

	int yOffset = 8;

	for (int i = 0; i < sv_teamsinplay; i++)
	{
		color = V_GetTextColor(GetTeamInfo((team_t)i)->TextColor.c_str());

		for (short xi = -146 + 1;xi < 146;xi += 2)
		{
			hud::DrawTranslatedPatch(xi, y + yOffset, hud_scalescoreboard,
			                         hud::X_CENTER, hud::Y_MIDDLE,
			                         hud::X_CENTER, hud::Y_TOP,
			                         sbline, Ranges + color * 256, true);
			hud::DrawTranslatedPatch(xi, y + yOffset + 11, hud_scalescoreboard,
			                         hud::X_CENTER, hud::Y_MIDDLE,
			                         hud::X_CENTER, hud::Y_TOP,
			                         sbline, Ranges + color * 256, true);
		}

		// Team Info
		str = hud::TeamName(color, i);
		hud::DrawText(-137, y + yOffset + 3, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_LEFT, hud::Y_TOP,
		              str.c_str(), color, true);

		if (sv_gametype == GM_CTF)
		{
			str = hud::TeamPlayers(color, i);
			hud::DrawText(34, y + yOffset + 3, hud_scalescoreboard, hud::X_CENTER,
			              hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, str.c_str(), color,
			              true);

			if (g_lives)
			{
				str = hud::TeamPoints(color, i);
				hud::DrawText(62, y + yOffset + 3, hud_scalescoreboard, hud::X_CENTER,
				              hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, str.c_str(), color,
				              true);
				hud::TeamLives(str, color, i);
				hud::DrawText(90, y + yOffset + 3, hud_scalescoreboard, hud::X_CENTER,
				              hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, str.c_str(), color,
				              true);
			}
			else
			{
				str = hud::TeamPoints(color, i);
				hud::DrawText(62, y + yOffset + 3, hud_scalescoreboard, hud::X_CENTER,
				              hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, str.c_str(), color,
				              true);
				str = hud::TeamFrags(color, i);
				hud::DrawText(90, y + yOffset + 3, hud_scalescoreboard, hud::X_CENTER,
				              hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, str.c_str(), color,
				              true);
			}
		}
		else
		{
			str = hud::TeamPlayers(color, i);
			hud::DrawText(22, y + yOffset + 3, hud_scalescoreboard, hud::X_CENTER,
			              hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, str.c_str(), color,
			              true);

			if (g_lives)
			{
				str = hud::TeamPoints(color, i);
				hud::DrawText(50, y + yOffset + 3, hud_scalescoreboard, hud::X_CENTER,
				              hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, str.c_str(), color,
				              true);
				hud::TeamLives(str, color, i);
				hud::DrawText(90, y + yOffset + 3, hud_scalescoreboard, hud::X_CENTER,
				              hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, str.c_str(), color,
				              true);
			}
			else
			{
				str = hud::TeamPoints(color, i);
				hud::DrawText(50, y + yOffset + 3, hud_scalescoreboard, hud::X_CENTER,
				              hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, str.c_str(), color,
				              true);
				str = hud::TeamKD(color, i);
				hud::DrawText(90, y + yOffset + 3, hud_scalescoreboard, hud::X_CENTER,
				              hud::Y_MIDDLE, hud::X_RIGHT, hud::Y_TOP, str.c_str(), color,
				              true);
			}
		}

		str = hud::TeamPing(color, i);
		hud::DrawText(146, y + yOffset + 3, hud_scalescoreboard,
		              hud::X_CENTER, hud::Y_MIDDLE,
		              hud::X_RIGHT, hud::Y_TOP,
		              str.c_str(), color, true);

		// Ingame Players
		byte limit = extra_rows + 4;
		hud::EATeamPlayerColors(-146, y + yOffset + 14, 7, 7, hud_scalescoreboard,
		                        hud::X_CENTER, hud::Y_MIDDLE,
		                        hud::X_LEFT, hud::Y_TOP,
		                        1, limit, i);
		hud::EATeamPlayerNames(-137, y + yOffset + 14, hud_scalescoreboard,
		                       hud::X_CENTER, hud::Y_MIDDLE,
		                       hud::X_LEFT, hud::Y_TOP,
		                       1, limit, i, true);

		if (sv_gametype == GM_CTF)
		{
			if (g_lives)
			{
				hud::EATeamPlayerPoints(62, y + yOffset + 14, hud_scalescoreboard,
				                        hud::X_CENTER, hud::Y_MIDDLE, hud::X_RIGHT,
				                        hud::Y_TOP, 1, limit, i, true);
				hud::EATeamPlayerLives(90, y + yOffset + 14, hud_scalescoreboard,
				                       hud::X_CENTER, hud::Y_MIDDLE, hud::X_RIGHT,
				                       hud::Y_TOP, 1, limit, i, true);
			}
			else
			{
				hud::EATeamPlayerPoints(62, y + yOffset + 14, hud_scalescoreboard,
				                        hud::X_CENTER, hud::Y_MIDDLE, hud::X_RIGHT,
				                        hud::Y_TOP, 1, limit, i, true);
				hud::EATeamPlayerFrags(90, y + yOffset + 14, hud_scalescoreboard,
				                       hud::X_CENTER, hud::Y_MIDDLE, hud::X_RIGHT,
				                       hud::Y_TOP, 1, limit, i, true);
			}
		}
		else
		{
			if (g_lives)
			{
				hud::EATeamPlayerFrags(50, y + yOffset + 14, hud_scalescoreboard,
				                       hud::X_CENTER, hud::Y_MIDDLE, hud::X_RIGHT,
				                       hud::Y_TOP, 1, limit, i, true);
				hud::EATeamPlayerLives(90, y + yOffset + 14, hud_scalescoreboard,
				                       hud::X_CENTER, hud::Y_MIDDLE, hud::X_RIGHT,
				                       hud::Y_TOP, 1, limit, i, true);
			}
			else
			{
				hud::EATeamPlayerFrags(50, y + yOffset + 14, hud_scalescoreboard,
				                       hud::X_CENTER, hud::Y_MIDDLE, hud::X_RIGHT,
				                       hud::Y_TOP, 1, limit, i, true);
				hud::EATeamPlayerKD(90, y + yOffset + 14, hud_scalescoreboard,
				                    hud::X_CENTER, hud::Y_MIDDLE, hud::X_RIGHT,
				                    hud::Y_TOP, 1, limit, i, true);
			}
		}

		hud::EATeamPlayerTimes(118, y + yOffset + 14, hud_scalescoreboard,
		                       hud::X_CENTER, hud::Y_MIDDLE,
		                       hud::X_RIGHT, hud::Y_TOP,
		                       1, limit, i, true);
		hud::EATeamPlayerPings(146, y + yOffset + 14, hud_scalescoreboard,
		                       hud::X_CENTER, hud::Y_MIDDLE,
		                       hud::X_RIGHT, hud::Y_TOP,
		                       1, limit, i, true);

		int count = MAX(hud::CountTeamPlayers(i), 4);
		yOffset += 14 + count * 8;
	}

	y += yOffset;
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
void LowScoreboard(player_t *player)
{
	int height, y;
	byte extra_spec_rows = 0;
	byte extra_player_rows = 0;

	if (G_IsTeamGame())
	{
		height = 129;
		// Team scoreboard was designed for 4 players on a team.  If
		// there are more (per team), increase the height.
		extra_player_rows = GetExtraPlayerTeamRowsList();
	}
	else
	{
		height = 72;

		// Normal scoreboard was designed for 4 players.  If there are more, increase the height.
		int players = P_NumPlayersInGame();
		if (players > 4)
			extra_player_rows += players - 4;
	}

	// If there are more than 2 spectators, increase the height.
	int specs = hud::CountSpectators();
	if (specs > 2)
		extra_spec_rows += ((specs + 1) / 2) - 1;

	height += (extra_player_rows + extra_spec_rows) * 8;
	if (sv_teamsinplay > 2)
		height += (sv_teamsinplay.asInt() / 2) * 46;

	// Starting Y position, measured from the center up.
	y = -(height / 2);
	// Scoreboard starts slightly off-center, then grows to be centered.
	// -96 makes it even with where the default lowres TDM begins.
	// For the record, the lowres DM scoreboard begins at -54.
	if (y > -64)
		y = -64;

	ClampToScreenTopLeft(y, height);

	// Dim the background.
	hud::Dim(0, y, 300, height, hud_scalescoreboard, hud::X_CENTER, hud::Y_MIDDLE, hud::X_CENTER, hud::Y_TOP);

	hud::drawLowHeader(player, y + 4);

	if (G_IsTeamGame())
		hud::drawLowTeamScores(player, y + 15, extra_player_rows);
	else
		hud::drawLowScores(player, y + 15, extra_player_rows);

	hud::drawLowSpectators(player, y + (height - 14 - (extra_spec_rows * 8)),
	                       extra_spec_rows);
}

}

void HU_DrawScores(player_t *player)
{
	if (hud::XSize(hud_scalescoreboard) >= HiResolutionWidth)
		hud::Scoreboard(player); 
	else 
		hud::LowScoreboard(player);
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
		if (!G_IsDuelGame() && G_IsRoundsGame())
		{
			return arg2->totalpoints < arg1->totalpoints;
		}

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
		if (G_IsRoundsGame())
		{
			return arg2->totalpoints < arg1->totalpoints;
		}

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

	int frags = 0;
	int deaths = 0;

	if (G_IsRoundsGame() && !G_IsDuelGame())
	{	
		frags = player->totalpoints;
		deaths = player->totaldeaths;
	}
	else
	{
		frags = player->fragcount;
		deaths = player->deathcount;
	}


	if (frags <= 0)	// Copied from HU_DMScores1.
		return 0.0f;
	else if (frags >= 1 && deaths == 0)
		return float(frags);
	else
		return float(frags) / float(deaths);
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

	int points, deaths;

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

		for (int team_num = 0; team_num < sv_teamsinplay; team_num++)
		{
			if (team_num == TEAM_BLUE)
				Printf_Bold("\n-----------------------------BLUE TEAM\n");
			else if (team_num == TEAM_RED)
				Printf_Bold("\n------------------------------RED TEAM\n");
			else if (team_num == TEAM_GREEN)
				Printf_Bold("\n----------------------------GREEN TEAM\n");
			else		// shouldn't happen
				Printf_Bold("\n--------------------------UNKNOWN TEAM\n");

			Printf_Bold("Name            Points Caps Frags Time\n");
			Printf_Bold("--------------------------------------\n");

			for (PlayerPtrList::const_iterator it = sortedplayers.begin(); it != sortedplayers.end(); ++it)
			{
				const player_t* itplayer = *it;
				if (itplayer->userinfo.team == team_num)
				{

					if (G_IsRoundsGame())
					{
						points = itplayer->totalpoints;
					}

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

		for (int team_num = 0; team_num < sv_teamsinplay; team_num++)
		{
			if (team_num == TEAM_BLUE)
				Printf_Bold("\n-----------------------------BLUE TEAM\n");
			else if (team_num == TEAM_RED)
				Printf_Bold("\n------------------------------RED TEAM\n");
			else if (team_num == TEAM_GREEN)
				Printf_Bold("\n----------------------------GREEN TEAM\n");
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
