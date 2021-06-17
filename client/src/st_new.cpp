// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
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
//	Additional status bar stuff
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <sstream>

#include "cmdlib.h"
#include "doomtype.h"
#include "doomdef.h"
#include "doomstat.h"
#include "cl_demo.h"
#include "cl_main.h"
#include "d_items.h"
#include "i_video.h"
#include "v_video.h"
#include "v_text.h"
#include "w_wad.h"
#include "z_zone.h"
#include "i_system.h"
#include "st_stuff.h"
#include "hu_drawers.h"
#include "hu_elements.h"
#include "c_cvars.h"
#include "p_ctf.h"
#include "cl_parse.h"
#include "cl_vote.h"
#include "g_levelstate.h"
#include "g_gametype.h"
#include "c_bind.h"
#include "c_dispatch.h"

static const char* medipatches[] = {"MEDIA0", "PSTRA0"};
static const char* armorpatches[] = {"ARM1A0", "ARM2A0"};
static const char* ammopatches[] = {"CLIPA0", "SHELA0", "CELLA0", "ROCKA0"};
static const char* bigammopatches[] = {"AMMOA0", "SBOXA0", "CELPA0", "BROKA0"};
static const char* flaghomepatches[NUMTEAMS] = {"FLAGIC2B", "FLAGIC2R", "FLAGIC2G"};
static const char* flagtakenpatches[NUMTEAMS] = {"FLAGIC3B", "FLAGIC3R", "FLAGIC3G"};
static const char* flagreturnpatches[NUMTEAMS] = {"FLAGIC4B", "FLAGIC4R", "FLAGIC4G"};
static const char* flagdroppatches[NUMTEAMS] = {"FLAGIC5B", "FLAGIC5R", "FLAGIC5G"};
static const char* livespatches[NUMTEAMS] = {"ODALIVEB", "ODALIVER", "ODALIVEG"};

static int widest_num, num_height;
static lumpHandle_t medi[ARRAY_LENGTH(::medipatches)];
static lumpHandle_t armors[ARRAY_LENGTH(::armorpatches)];
static lumpHandle_t ammos[ARRAY_LENGTH(::ammopatches)];
static lumpHandle_t bigammos[ARRAY_LENGTH(::bigammopatches)];
static lumpHandle_t flagiconteam;
static lumpHandle_t flagiconteamoffense;
static lumpHandle_t flagiconteamdefense;
static lumpHandle_t line_leftempty;
static lumpHandle_t line_leftfull;
static lumpHandle_t line_centerempty;
static lumpHandle_t line_centerleft;
static lumpHandle_t line_centerright;
static lumpHandle_t line_centerfull;
static lumpHandle_t line_rightempty;
static lumpHandle_t line_rightfull;
static lumpHandle_t FlagIconHome[NUMTEAMS];
static lumpHandle_t FlagIconReturn[NUMTEAMS];
static lumpHandle_t FlagIconTaken[NUMTEAMS];
static lumpHandle_t FlagIconDropped[NUMTEAMS];
static lumpHandle_t LivesIcon[NUMTEAMS];
static lumpHandle_t ToastIcon[NUMMODS];

static int		NameUp = -1;

extern lumpHandle_t sttminus;
extern lumpHandle_t tallnum[10];
extern lumpHandle_t faces[];
extern int st_faceindex;
extern lumpHandle_t keys[NUMCARDS + NUMCARDS / 2];
extern byte* Ranges;

extern NetDemo netdemo;

typedef std::vector<const patch_t**> PathFreeList;

/**
 * @brief Stores pointers to status bar objects that should be freed on shutdown. 
 */
PathFreeList freelist;

int V_TextScaleXAmount();
int V_TextScaleYAmount();

EXTERN_CVAR(hud_demoprotos)
EXTERN_CVAR(hud_scale)
EXTERN_CVAR(hud_bigfont)
EXTERN_CVAR(hud_timer)
EXTERN_CVAR(hud_targetcount)
EXTERN_CVAR(hud_transparency)
EXTERN_CVAR(hud_demobar)
EXTERN_CVAR(sv_fraglimit)
EXTERN_CVAR(sv_teamsinplay)
EXTERN_CVAR(g_lives)
EXTERN_CVAR(sv_scorelimit);
EXTERN_CVAR(sv_warmup)
EXTERN_CVAR(hud_feedobits)

void ST_unloadNew()
{
	// Do nothing - let our handles stale on their own.
}

void ST_initNew()
{
	int widest = 0;

	// denis - todo - security - these patches have unchecked dimensions
	// ie, if a patch has a 0 width/height, it may cause a divide by zero
	// somewhere else in the code. we download wads, so this is an issue!

	for (size_t i = 0; i < ARRAY_LENGTH(::tallnum); i++)
	{
		patch_t* talli = W_ResolvePatchHandle(::tallnum[i]);
		if (talli->width() > widest)
			widest = talli->width();
	}

	for (size_t i = 0; i < ARRAY_LENGTH(::medipatches); i++)
		::medi[i] = W_CachePatchHandle(::medipatches[i], PU_STATIC, ns_sprites);

	for (size_t i = 0; i < ARRAY_LENGTH(::armorpatches); i++)
		::armors[i] = W_CachePatchHandle(::armorpatches[i], PU_STATIC, ns_sprites);

	for (size_t i = 0; i < ARRAY_LENGTH(::ammopatches); i++)
	{
		::ammos[i] = W_CachePatchHandle(::ammopatches[i], PU_STATIC, ns_sprites);
		::bigammos[i] = W_CachePatchHandle(::bigammopatches[i], PU_STATIC, ns_sprites);
	}

	for (size_t i = 0; i < NUMTEAMS; i++)
	{
		::FlagIconHome[i] = W_CachePatchHandle(::flaghomepatches[i], PU_STATIC);
		::FlagIconTaken[i] = W_CachePatchHandle(::flagtakenpatches[i], PU_STATIC);
		::FlagIconReturn[i] = W_CachePatchHandle(::flagreturnpatches[i], PU_STATIC);
		::FlagIconDropped[i] = W_CachePatchHandle(::flagdroppatches[i], PU_STATIC);
		::LivesIcon[i] = W_CachePatchHandle(::livespatches[i], PU_STATIC);
	}

	::widest_num = widest;
	::num_height = W_ResolvePatchHandle(::tallnum[0])->height();

	if (multiplayer && (sv_gametype == GM_COOP || demoplayback) && level.time)
		NameUp = level.time + 2 * TICRATE;

	::flagiconteam = W_CachePatchHandle("FLAGIT", PU_STATIC);
	::flagiconteamoffense = W_CachePatchHandle("FLAGITO", PU_STATIC);
	::flagiconteamdefense = W_CachePatchHandle("FLAGITD", PU_STATIC);

	::line_leftempty = W_CachePatchHandle("ODABARLE", PU_STATIC);
	::line_leftfull = W_CachePatchHandle("ODABARLF", PU_STATIC);
	::line_centerempty = W_CachePatchHandle("ODABARCE", PU_STATIC);
	::line_centerleft = W_CachePatchHandle("ODABARCL", PU_STATIC);
	::line_centerright = W_CachePatchHandle("ODABARCR", PU_STATIC);
	::line_centerfull = W_CachePatchHandle("ODABARCF", PU_STATIC);
	::line_rightempty = W_CachePatchHandle("ODABARRE", PU_STATIC);
	::line_rightfull = W_CachePatchHandle("ODABARRF", PU_STATIC);

	std::string buffer;
	for (size_t i = 0; i < NUMMODS; i++)
	{
		StrFormat(buffer, "ODAMOD%d", i);
		::ToastIcon[i] = W_CachePatchHandle(buffer.c_str(), PU_STATIC);
	}
}

void ST_DrawNum (int x, int y, DCanvas *scrn, int num)
{
	char digits[11], *d;
	patch_t* minus = W_ResolvePatchHandle(sttminus);

	if (num < 0)
	{
		if (hud_scale)
		{
			scrn->DrawLucentPatchCleanNoMove(minus, x, y);
			x += CleanXfac * minus->width();
		}
		else
		{
			scrn->DrawLucentPatch(minus, x, y);
			x += minus->width();
		}
		num = -num;
	}

	sprintf (digits, "%d", num);

	d = digits;
	while (*d)
	{
		if (*d >= '0' && *d <= '9')
		{
			patch_t* numpatch = W_ResolvePatchHandle(tallnum[*d - '0']);
			if (hud_scale)
			{
				scrn->DrawLucentPatchCleanNoMove(numpatch, x, y);
				x += CleanXfac * numpatch->width();
			}
			else
			{
				scrn->DrawLucentPatch(numpatch, x, y);
				x += numpatch->width();
			}
		}
		d++;
	}
}

void ST_DrawNumRight (int x, int y, DCanvas *scrn, int num)
{
	int d = abs(num);
	int xscale = hud_scale ? CleanXfac : 1;

	do {
		x -= W_ResolvePatchHandle(tallnum[d % 10])->width() * xscale;
	} while (d /= 10);

	if (num < 0)
		x -= W_ResolvePatchHandle(sttminus)->width() * xscale;

	ST_DrawNum (x, y, scrn, num);
}

/**
 * Draw a bar on the screen.
 *
 * @param normalcolor Bar color.  Uses text colors (i.e. CR_RED).
 * @param value Value of the bar.
 * @param total Maximum value of the bar.
 * @param x Unscaled leftmost X position to draw from.
 * @param y Unscaled topmost Y position to draw from.
 * @param width Width of the bar in unscaled pixels.
 * @param reverse If true, the bar is drawn with the 'baseline' on the right.
 * @param cutleft True if you want the left end of the bar to not have a cap.
 * @param cutright True if you want the right end of the bar to not have a cap.
 */
void ST_DrawBar (int normalcolor, unsigned int value, unsigned int total,
				 int x, int y, int width, bool reverse = false,
				 bool cutleft = false, bool cutright = false) {
	int xscale = hud_scale ? CleanXfac : 1;

	if (normalcolor > NUM_TEXT_COLORS || normalcolor == CR_GREY) {
		normalcolor = CR_RED;
	}

	if (width < (4 * xscale)) {
		width = 4 * xscale;
	}
	width -= (width % (2 * xscale));

	int bar_width = width / (2 * xscale);

	int bar_filled;
	if (value == 0) {
		// Bar is forced empty.
		bar_filled = 0;
	} else if (value >= total) {
		// Bar is forced full.
		bar_filled = bar_width;
	} else {
		bar_filled = (value * bar_width) / total;
		if (bar_filled == 0) {
			// Bar is prevented from being empty.
			bar_filled = 1;
		} else if (bar_filled >= bar_width) {
			// Bar is prevented from being full.
			bar_filled = bar_width - 1;
		}
	}

	V_ColorMap = translationref_t(Ranges + normalcolor * 256);
	for (int i = 0;i < bar_width;i++) {
		const patch_t* linepatch;
		if (!reverse) {
			if (i == 0 && !cutleft) {
				if (bar_filled == 0) {
					linepatch = W_ResolvePatchHandle(line_leftempty);
				} else {
					linepatch = W_ResolvePatchHandle(line_leftfull);
				}
			} else if (i == bar_width - 1 && !cutright) {
				if (bar_filled == bar_width) {
					linepatch = W_ResolvePatchHandle(line_rightfull);
				} else {
					linepatch = W_ResolvePatchHandle(line_rightempty);
				}
			} else {
				if (i == bar_filled - 1) {
					linepatch = W_ResolvePatchHandle(line_centerleft);
				} else if (i < bar_filled) {
					linepatch = W_ResolvePatchHandle(line_centerfull);
				} else {
					linepatch = W_ResolvePatchHandle(line_centerempty);
				}
			}
		} else {
			if (i == 0 && !cutleft) {
				if (bar_filled == bar_width) {
					linepatch = W_ResolvePatchHandle(line_leftfull);
				} else {
					linepatch = W_ResolvePatchHandle(line_leftempty);
				}
			} else if (i == bar_width - 1 && !cutright) {
				if (bar_filled == 0) {
					linepatch = W_ResolvePatchHandle(line_rightempty);
				} else {
					linepatch = W_ResolvePatchHandle(line_rightfull);
				}
			} else {
				if (i == (bar_width - bar_filled)) {
					linepatch = W_ResolvePatchHandle(line_centerright);
				} else if (i >= (bar_width - bar_filled)) {
					linepatch = W_ResolvePatchHandle(line_centerfull);
				} else {
					linepatch = W_ResolvePatchHandle(line_centerempty);
				}
			}
		}

		int xi = x + (i * xscale * 2);
		if (hud_scale) {
			screen->DrawTranslatedPatchCleanNoMove(linepatch, xi, y);
		} else {
			screen->DrawTranslatedPatch(linepatch, xi, y);
		}
	}
}

// [AM] Draw the state of voting
void ST_voteDraw (int y) {
	vote_state_t vote_state;
	if (!VoteState::instance().get(vote_state)) {
		return;
	}

	int xscale = hud_scale ? CleanXfac : 1;
	int yscale = hud_scale ? CleanYfac : 1;

	// Vote Result/Countdown
	std::ostringstream buffer;
	std::string result_string;
	EColorRange result_color;

	switch (vote_state.result) {
	case VOTE_YES:
		result_string = "VOTE PASSED";
		result_color = CR_GREEN;
		break;
	case VOTE_NO:
		result_string = "VOTE FAILED";
		result_color = CR_RED;
		break;
	case VOTE_INTERRUPT:
		result_string = "VOTE INTERRUPTED";
		result_color = CR_TAN;
		break;
	case VOTE_ABANDON:
		result_string = "VOTE ABANDONED";
		result_color = CR_TAN;
		break;
	case VOTE_UNDEC:
		buffer << "VOTE NOW: " << vote_state.countdown;
		result_string = buffer.str();
		if (vote_state.countdown <= 5 && (I_MSTime() % 1000) < 500) {
			result_color = CR_BRICK;
		} else {
			result_color = CR_GOLD;
		}
		break;
	default:
		return;
	}

	size_t x1, x2;
	x1 = (I_GetSurfaceWidth() - V_StringWidth(result_string.c_str()) * xscale) >> 1;
	if (hud_scale) {
		screen->DrawTextClean(result_color, x1, y, result_string.c_str());
	} else {
		screen->DrawText(result_color, x1, y, result_string.c_str());
	}

	// Votestring - Break lines
	brokenlines_t *votestring = V_BreakLines(320, vote_state.votestring.c_str());
	for (byte i = 0;i < 4;i++) {
		if (votestring[i].width == -1) {
			break;
		}

		x2 = (I_GetSurfaceWidth() - votestring[i].width * xscale) >> 1;
		y += yscale * 8;

		if (hud_scale) {
			screen->DrawTextClean(CR_GREY, x2, y, votestring[i].string);
		} else {
			screen->DrawText(CR_GREY, x2, y, votestring[i].string);
		}
	}
	V_FreeBrokenLines(votestring);

	if (vote_state.result == VOTE_ABANDON) {
		return;
	}

	// Voting Bar
	y += yscale * 8;

	ST_DrawBar(CR_RED, vote_state.no, vote_state.no_needed,
			   (I_GetSurfaceWidth() >> 1) - xscale * 40, y, xscale * 40,
			   true, false, true);
	ST_DrawBar(CR_GREEN, vote_state.yes, vote_state.yes_needed,
			   (I_GetSurfaceWidth() >> 1), y, xscale * 40, false, true);
}

namespace hud {

/**
 * @brief Sometimes we want the HUD to show round wins and not current round points.
 */
static bool TeamHUDShowsRoundWins()
{
	// If it's not a rounds game, obviously don't show it.
	if (!G_IsRoundsGame())
		return false;

	// If score is a wincon and it's 1, we don't want to display the in-round
	// score since it's always 0-0 except past the end.
	if (G_UsesScorelimit() && ::sv_scorelimit.asInt() == 1)
		return true;

	// In TLMS if there's no fraglimit there's no reason to display frags
	// since team frags are usually an alterantive win condition for when
	// time runs out.
	if (G_IsLivesGame() && G_UsesFraglimit() && ::sv_fraglimit.asInt() == 0)
		return true;

	return false;
}

/**
 * @brief Draw gametype-specific scoreboard, such as flags and lives.
 */
static void drawGametype()
{
	const int SCREEN_BORDER = 4;
	const int FLAG_ICON_HEIGHT = 18;
	const int LIVES_HEIGHT = 12;

	if (!G_IsTeamGame())
	{
		return;
	}

	std::string buffer;
	player_t* plyr = &consoleplayer();
	int xscale = hud_scale ? CleanXfac : 1;
	int yscale = hud_scale ? CleanYfac : 1;

	int patchPosY = ::hud_bigfont ? 53 : 43;

	bool shouldShowScores = G_IsTeamGame();
	bool shouldShowLives = G_IsLivesGame();

	if (shouldShowScores)
	{
		patchPosY += sv_teamsinplay.asInt() * FLAG_ICON_HEIGHT;
	}
	if (shouldShowLives)
	{
		patchPosY += sv_teamsinplay.asInt() * LIVES_HEIGHT;
	}

	for (int i = 0; i < sv_teamsinplay; i++)
	{
		TeamInfo* teamInfo = GetTeamInfo((team_t)i);
		if (shouldShowScores)
		{
			patchPosY -= FLAG_ICON_HEIGHT;

			const patch_t* drawPatch = W_ResolvePatchHandle(::FlagIconTaken[i]);

			if (sv_gametype == GM_CTF && G_IsDefendingTeam(teamInfo->Team))
			{
				switch (teamInfo->FlagData.state)
				{
				case flag_home:
					drawPatch = W_ResolvePatchHandle(::FlagIconHome[i]);
					break;
				case flag_carried:
					if (idplayer(teamInfo->FlagData.flagger).userinfo.team == i)
						drawPatch = W_ResolvePatchHandle(::FlagIconReturn[i]);
					else
						drawPatch = W_ResolvePatchHandle(::FlagIconTaken[i]);
					break;
				case flag_dropped:
					drawPatch = W_ResolvePatchHandle(::FlagIconDropped[i]);
					break;
				default:
					break;
				}
			}

			if (drawPatch != NULL)
			{
				hud::DrawPatch(SCREEN_BORDER, patchPosY, hud_scale, hud::X_RIGHT,
				               hud::Y_BOTTOM, hud::X_RIGHT, hud::Y_BOTTOM, drawPatch);
			}

			if (plyr->userinfo.team == i)
			{
				const patch_t* itpatch = W_ResolvePatchHandle(::flagiconteam);
				if (G_IsSidesGame())
				{
					// Sides games show offense/defense.
					if (G_IsDefendingTeam(consoleplayer().userinfo.team))
						itpatch = W_ResolvePatchHandle(::flagiconteamdefense);
					else
						itpatch = W_ResolvePatchHandle(::flagiconteamoffense);
				}
				hud::DrawPatch(SCREEN_BORDER, patchPosY, hud_scale, hud::X_RIGHT,
				               hud::Y_BOTTOM, hud::X_RIGHT, hud::Y_BOTTOM, itpatch);
			}

			if (TeamHUDShowsRoundWins())
			{
				ST_DrawNumRight(I_GetSurfaceWidth() - 24 * xscale,
				                I_GetSurfaceHeight() - (patchPosY + 17) * yscale,
				                ::screen, teamInfo->RoundWins);
			}
			else
			{
				ST_DrawNumRight(I_GetSurfaceWidth() - 24 * xscale,
				                I_GetSurfaceHeight() - (patchPosY + 17) * yscale,
				                ::screen, teamInfo->Points);
			}
		}

		if (shouldShowLives)
		{
			patchPosY -= LIVES_HEIGHT;
			hud::DrawPatch(SCREEN_BORDER, patchPosY, hud_scale, hud::X_RIGHT,
			               hud::Y_BOTTOM, hud::X_RIGHT, hud::Y_BOTTOM,
			               W_ResolvePatchHandle(::LivesIcon[i]));

			StrFormat(buffer, "%d", teamInfo->LivesPool());
			int color = (i % 2) ? CR_GOLD : CR_GREY;
			hud::DrawText(SCREEN_BORDER + 12, patchPosY + 3, hud_scale, hud::X_RIGHT,
			              hud::Y_BOTTOM, hud::X_RIGHT, hud::Y_BOTTOM, buffer.c_str(),
			              color);
		}
	}
}

size_t proto_selected;

/**
 * @brief Draw protocol buffer packets
 */
void drawProtos()
{
	const Protos& protos = CL_GetTicProtos();
	if (protos.size() == 0)
		return;

	V_SetFont("DIGFONT");

	proto_selected = clamp(proto_selected, (size_t)0, protos.size() - 1);

	// Starting y is five rows from the top.
	int y = 7 * 5;

	const double scale = 0.75;
	const int indent = V_StringWidth(" >");

	for (Protos::const_iterator it = protos.begin(); it != protos.end(); ++it)
	{
		bool selected = proto_selected == (it - protos.begin());

		if (selected)
		{
			// Draw arrow
			hud::DrawText(0, y, scale, hud::X_LEFT, hud::Y_TOP, hud::X_LEFT, hud::Y_TOP,
			              " >", CR_GOLD, true);
		}

		// Give each protocol header its own unique color.
		int rowColor = it->header % (NUM_TEXT_COLORS - 2);
		if (rowColor >= CR_WHITE)
			rowColor++;
		if (rowColor >= CR_UNTRANSLATED)
			rowColor++;

		// Draw name
		hud::DrawText(indent, y, scale, hud::X_LEFT, hud::Y_TOP, hud::X_LEFT, hud::Y_TOP,
		              it->name.c_str(), rowColor, true);
		y += V_StringHeight(it->name.c_str());

		if (selected)
		{
			// Draw data
			hud::DrawText(indent, y, 0.75, hud::X_LEFT, hud::Y_TOP, hud::X_LEFT,
			              hud::Y_TOP, it->data.c_str(), CR_WHITE, true);
			y += V_StringHeight(it->data.c_str());
		}
	}

	V_SetFont("SMALLFONT");
}

// [AM] Draw netdemo state
// TODO: This is ripe for commonizing, but I _need_ to get this done soon.
void drawNetdemo() {
	if (!(netdemo.isPlaying() || netdemo.isPaused())) {
		return;
	}

	if (!hud_demobar)
		return;

	int xscale = hud_scale ? CleanXfac : 1;
	int yscale = hud_scale ? CleanYfac : 1;

	V_SetFont("DIGFONT");

	int color = CR_GOLD;
	if (netdemo.isPlaying())
	{
		color = CR_GREEN;
	}

	// Dim the background.
	hud::Dim(1, 41, 74, 13, ::hud_scale,
		hud::X_LEFT, hud::Y_BOTTOM,
		hud::X_LEFT, hud::Y_BOTTOM);

	// Draw demo elapsed time
	hud::DrawText(2, 47, hud_scale,
	              hud::X_LEFT, hud::Y_BOTTOM,
	              hud::X_LEFT, hud::Y_BOTTOM,
	              hud::NetdemoElapsed().c_str(), color);

	// Draw map number/total
	hud::DrawText(74, 47, hud_scale,
	              hud::X_LEFT, hud::Y_BOTTOM,
	              hud::X_RIGHT, hud::Y_BOTTOM,
	              hud::NetdemoMaps().c_str(), CR_BRICK);

	V_SetFont("SMALLFONT");

	// Draw the bar.
	// TODO: Once status bar notches have been implemented, put map
	//       change times in as notches.
	ST_DrawBar(color, netdemo.calculateTimeElapsed(), netdemo.calculateTotalTime(),
	           2 * xscale, I_GetSurfaceHeight() - 46 * yscale, 72 * xscale);

	if (netdemo.isPaused() && ::hud_demoprotos)
	{
		drawProtos();
	}
}

// [ML] 9/29/2011: New fullscreen HUD, based on Ralphis's work
void OdamexHUD() {
	std::string buf;
	player_t *plyr = &displayplayer();

	// TODO: I can probably get rid of these invocations once I put a
	//       copy of ST_DrawNumRight into the hud namespace. -AM
	unsigned int y, xscale, yscale;
	xscale = hud_scale ? CleanXfac : 1;
	yscale = hud_scale ? CleanYfac : 1;
	y = I_GetSurfaceHeight() - (num_height + 4) * yscale;

	// Draw Armor if the player has any
	if (plyr->armortype && plyr->armorpoints) {
		const patch_t* current_armor = W_ResolvePatchHandle(armors[1]);
		if (plyr->armortype == 1) {
			current_armor = W_ResolvePatchHandle(armors[0]);
		}

		if (current_armor) {
			// Draw Armor type.  Vertically centered against armor number.
			hud::DrawPatchScaled(48 + 2 + 10, 32, 20, 20, hud_scale,
			                     hud::X_LEFT, hud::Y_BOTTOM,
			                     hud::X_CENTER, hud::Y_MIDDLE,
			                     current_armor);
		}
		ST_DrawNumRight(48 * xscale, y - 20 * yscale, screen, plyr->armorpoints);
	}

	// Draw Doomguy.  Vertically scaled to an area two pixels above and
	// below the health number, and horizontally centered below the armor.
	hud::DrawPatchScaled(48 + 2 + 10, 2, 20, 20, hud_scale, hud::X_LEFT, hud::Y_BOTTOM,
	                     hud::X_CENTER, hud::Y_BOTTOM,
	                     W_ResolvePatchHandle(faces[st_faceindex]));
	ST_DrawNumRight(48 * xscale, y, screen, plyr->health);

	if (g_lives)
	{
		// Lives are next to doomguy.  Supposed to be vertically-centered with his head.
		int lives_color = CR_GREY;
		if (plyr->lives <= 0)
			lives_color = CR_DARKGREY;

		StrFormat(buf, "x%d", plyr->lives);
		hud::DrawText(48 + 2 + 20 + 2, 10 + 2, hud_scale, hud::X_LEFT, hud::Y_BOTTOM,
		              hud::X_LEFT, hud::Y_MIDDLE, buf.c_str(), lives_color, false);
	}

	// Draw Ammo
	ammotype_t ammotype = weaponinfo[plyr->readyweapon].ammotype;
	if (ammotype < NUMAMMO) {
		const patch_t *ammopatch;
		// Use big ammo if the player has a backpack.
		if (plyr->backpack) {
			ammopatch = W_ResolvePatchHandle(bigammos[ammotype]);
		} else {
			ammopatch = W_ResolvePatchHandle(ammos[ammotype]);
		}

		// Draw ammo.  We have a 16x16 box to the right of the ammo where the
		// ammo type is drawn.
		// TODO: This "scale only if bigger than bounding box" can
		//       probably be commonized, along with "scale only if
		//       smaller than bounding box".
		if (ammopatch->width() > 16 || ammopatch->height() > 16) {
			hud::DrawPatchScaled(12, 12, 16, 16, hud_scale,
			                     hud::X_RIGHT, hud::Y_BOTTOM,
			                     hud::X_CENTER, hud::Y_MIDDLE,
			                     ammopatch);
		} else {
			hud::DrawPatch(12, 12, hud_scale,
			               hud::X_RIGHT, hud::Y_BOTTOM,
			               hud::X_CENTER, hud::Y_MIDDLE,
			               ammopatch);
		}
		ST_DrawNumRight(I_GetSurfaceWidth() - 24 * xscale, y, screen, plyr->ammo[ammotype]);
	}

	int color;
	std::string str;
	int iy = 4;

	if (::hud_timer)
	{
		if (::hud_bigfont)
			V_SetFont("BIGFONT");

		hud::DrawText(0, 4, hud_scale, hud::X_CENTER, hud::Y_BOTTOM, hud::X_CENTER,
		              hud::Y_BOTTOM, hud::Timer().c_str(), CR_GREY);
		iy += V_LineHeight() + 1;

		if (::hud_bigfont)
			V_SetFont("SMALLFONT");
	}

	// Draw other player name, if spying
	hud::DrawText(0, iy, hud_scale, hud::X_CENTER, hud::Y_BOTTOM, hud::X_CENTER,
	              hud::Y_BOTTOM, hud::SpyPlayerName().c_str(), CR_GREY);
	iy += V_LineHeight() + 1;

	// Draw targeted player names.
	hud::EATargets(0, iy, hud_scale, hud::X_CENTER, hud::Y_BOTTOM, hud::X_CENTER,
	               hud::Y_BOTTOM, 1, hud_targetcount);
	iy += V_LineHeight() + 1;

	// Draw stat lines.  Vertically aligned with the bottom of the armor
	// number on the other side of the screen.
	if (::hud_bigfont)
		V_SetFont("BIGFONT");

	hud::DrawText(4, 24 + V_LineHeight() + 1, hud_scale, hud::X_RIGHT, hud::Y_BOTTOM,
	              hud::X_RIGHT, hud::Y_BOTTOM, hud::PersonalSpread().c_str(),
	              CR_UNTRANSLATED);
	hud::DrawText(4, 24, hud_scale, hud::X_RIGHT, hud::Y_BOTTOM, hud::X_RIGHT,
	              hud::Y_BOTTOM, hud::PersonalScore().c_str(), CR_UNTRANSLATED);

	if (::hud_bigfont)
		V_SetFont("SMALLFONT");

	// Draw keys in coop
	if (sv_gametype == GM_COOP) {
		for (byte i = 0;i < NUMCARDS;i++) {
			if (plyr->cards[i]) {
				hud::DrawPatch(4 + (i * 10), 24, hud_scale, hud::X_RIGHT, hud::Y_BOTTOM,
				               hud::X_RIGHT, hud::Y_BOTTOM,
				               W_ResolvePatchHandle(keys[i]));
			}
		}
	}

	// Draw gametype scoreboard
	hud::drawGametype();
}

struct drawToast_t
{
	int tic;
	int pid_highlight;
	std::string left;
	lumpHandle_t icon;
	std::string right;
};

typedef std::vector<drawToast_t> drawToasts_t;

drawToasts_t g_Toasts;

void DrawToasts()
{
	if (!hud_feedobits)
		return;

	V_SetFont("DIGFONT");

	std::string buffer;
	int y = 1;

	const float oldtrans = ::hud_transparency;
	for (drawToasts_t::const_iterator it = g_Toasts.begin(); it != g_Toasts.end(); ++it)
	{
		// Only render the message if it's less than 2 seconds in.
		int tics = ::gametic - it->tic;
		if (tics < TICRATE * 2)
		{
			::hud_transparency.ForceSet(1.0);
		}
		else if (tics < TICRATE * 3)
		{
			tics %= TICRATE;
			float trans = static_cast<float>(TICRATE - tics) / TICRATE;
			::hud_transparency.ForceSet(trans);
		}
		else
		{
			::hud_transparency.ForceSet(0.0);
		}

		int x = 1;

		// Right-hand side.
		hud::DrawText(0, y, hud_scale, hud::X_RIGHT, hud::Y_TOP, hud::X_RIGHT, hud::Y_TOP,
		              it->right.c_str(), CR_GREY);
		x += V_StringWidth(it->right.c_str()) + 1;

		// Icon
		patch_t* icon = W_ResolvePatchHandle(it->icon);
		hud::DrawPatch(x, y, hud_scale, hud::X_RIGHT, hud::Y_TOP, hud::X_RIGHT,
		               hud::Y_TOP, icon, false, true);
		x += icon->width() + 1;

		// Left-hand side.
		hud::DrawText(x, y, hud_scale, hud::X_RIGHT, hud::Y_TOP, hud::X_RIGHT, hud::Y_TOP,
		              it->left.c_str(), CR_GREY);

		y += MAX(V_LineHeight(), static_cast<int>(icon->height()));
	}
	::hud_transparency.ForceSet(oldtrans);

	V_SetFont("SMALLFONT");
}

void ToastTicker()
{
	// Remove stale toasts in a loop.
	drawToasts_t::iterator it = g_Toasts.begin();
	while (it != g_Toasts.end())
	{
		int tics = ::gametic - it->tic;

		if (tics >= TICRATE * 3)
		{
			it = g_Toasts.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void PushToast(const toast_t& toast)
{
	std::string buffer;

	drawToast_t drawToast;
	drawToast.tic = ::gametic;
	drawToast.pid_highlight = -1;

	if (toast.flags & toast_t::LEFT)
	{
		buffer += toast.left + " ";
	}
	if (toast.flags & toast_t::LEFT_PLUS)
	{
		buffer += TEXTCOLOR_GOLD "+ " + toast.left_plus + " ";
	}
	if (!buffer.empty())
	{
		buffer.resize(buffer.size() - 1);
		drawToast.left = buffer;
	}

	if (toast.flags & toast_t::ICON)
	{
		drawToast.icon = ::ToastIcon[toast.icon];
	}

	buffer.clear();
	if (toast.flags & toast_t::RIGHT)
	{
		buffer += toast.right + " ";
	}
	if (toast.flags & toast_t::RIGHT_PLUS)
	{
		buffer += TEXTCOLOR_GOLD "+ " + toast.right_plus + " ";
	}
	if (!buffer.empty())
	{
		buffer.resize(buffer.size() - 1);
		drawToast.right = buffer;
	}

	g_Toasts.push_back(drawToast);
}

static std::string WinToColorString(const WinInfo& win)
{
	std::string buf;
	if (win.type == WinInfo::WIN_PLAYER)
	{
		const player_t& pl = idplayer(win.id);
		if (pl.userinfo.netname.empty())
		{
			StrFormat(buf, TEXTCOLOR_GREEN "???" TEXTCOLOR_NORMAL);
		}
		else
		{
			StrFormat(buf, TEXTCOLOR_GREEN "%s" TEXTCOLOR_NORMAL,
			          pl.userinfo.netname.c_str());
		}
		return buf;
	}
	else if (win.type == WinInfo::WIN_TEAM)
	{
		const TeamInfo& tm = *GetTeamInfo((team_t)win.id);
		if (tm.Team == TEAM_NONE)
		{
			StrFormat(buf, TEXTCOLOR_GREEN "???" TEXTCOLOR_NORMAL);
		}
		else
		{
			StrFormat(buf, "%s%s" TEXTCOLOR_NORMAL, tm.TextColor.c_str(),
			          tm.ColorString.c_str());
		}
		return buf;
	}

	StrFormat(buf, TEXTCOLOR_GREEN "???" TEXTCOLOR_NORMAL);
	return buf;
}

void LevelStateHUD()
{
	// Don't bother with levelstate information in lobbies.
	if (::level.flags & LEVEL_LOBBYSPECIAL)
	{
		return;
	}

	// First line...BIGFONT.
	std::string str;
	switch (::levelstate.getState())
	{
	case LevelState::WARMUP:
		if (consoleplayer().spectator)
		{
			str = "";
			break;
		}

		str = TEXTCOLOR_YELLOW "Warmup";
		break;
	case LevelState::WARMUP_COUNTDOWN:
	case LevelState::WARMUP_FORCED_COUNTDOWN:
		str = TEXTCOLOR_YELLOW + G_GametypeName();
		break;
	case LevelState::PREROUND_COUNTDOWN:
		StrFormat(str, TEXTCOLOR_YELLOW "Round " TEXTCOLOR_YELLOW " %d\n",
		          ::levelstate.getRound());
		break;
	case LevelState::INGAME:
		if (G_CanShowFightMessage())
		{
			if (G_IsSidesGame())
			{
				if (G_IsDefendingTeam(consoleplayer().userinfo.team))
				{
					str = TEXTCOLOR_YELLOW "DEFEND!\n";
				}
				else
				{
					str = TEXTCOLOR_YELLOW "CAPTURE!\n";
				}
			}
			else
			{
				str = TEXTCOLOR_YELLOW "FIGHT!\n";
			}
		}
		else
		{
			str = "";
		}
		break;
	case LevelState::ENDROUND_COUNTDOWN:
		StrFormat(str,
		          TEXTCOLOR_YELLOW "Round " TEXTCOLOR_GOLD "%d" TEXTCOLOR_YELLOW
		                           " complete\n",
		          ::levelstate.getRound());
		break;
	case LevelState::ENDGAME_COUNTDOWN:
		StrFormat(str, TEXTCOLOR_YELLOW "Match complete\n");
		break;
	default:
		str = "";
		break;
	}

	V_SetFont("BIGFONT");

	int surface_width = I_GetSurfaceWidth(), surface_height = I_GetSurfaceHeight();
	int w = V_StringWidth(str.c_str()) * CleanYfac;
	int h = 12 * CleanYfac;

	float oldtrans = ::hud_transparency;
	if (::levelstate.getState() == LevelState::INGAME)
	{
		// Only render the "FIGHT" message if it's less than 2 seconds in.
		int tics = ::level.time - ::levelstate.getIngameStartTime();
		if (tics < TICRATE * 2)
		{
			::hud_transparency.ForceSet(1.0);
		}
		else if (tics < TICRATE * 3)
		{
			tics %= TICRATE;
			float trans = static_cast<float>(TICRATE - tics) / TICRATE;
			::hud_transparency.ForceSet(trans);
		}
		else
		{
			::hud_transparency.ForceSet(0.0);
		}
	}
	else
	{
		::hud_transparency.ForceSet(1.0);
	}
	if (::hud_transparency > 0.0f)
	{
		screen->DrawTextStretchedLuc(CR_GREY, surface_width / 2 - w / 2,
		                             surface_height / 4 - h / 2, str.c_str(), CleanYfac,
		                             CleanYfac);
	}

	V_SetFont("SMALLFONT");

	// Second line...SMALLFONT.
	str = "";
	switch (::levelstate.getState())
	{
	case LevelState::WARMUP:
		if (consoleplayer().spectator)
			break;

		if (sv_warmup)
		{
			if (consoleplayer().ready)
			{
				StrFormat(str, "Waiting for other players to ready up...");
			}
			else
			{
				StrFormat(str,
				          "Press " TEXTCOLOR_GOLD "%s" TEXTCOLOR_NORMAL
				          " when ready to play",
				          ::Bindings.GetKeynameFromCommand("ready").c_str());
			}
		}
		else
		{
			StrFormat(str, "Waiting for other players to join...");
		}

		break;
	case LevelState::WARMUP_COUNTDOWN:
	case LevelState::WARMUP_FORCED_COUNTDOWN:
		StrFormat(str, "Match begins in " TEXTCOLOR_GREEN "%d",
		          ::levelstate.getCountdown());
		break;
	case LevelState::PREROUND_COUNTDOWN:
		StrFormat(str, "Weapons unlocked in " TEXTCOLOR_GREEN "%d",
		          ::levelstate.getCountdown());
		break;
	case LevelState::INGAME:
		if (G_CanShowFightMessage())
		{
			if (G_IsSidesGame())
			{
				if (G_IsDefendingTeam(consoleplayer().userinfo.team))
				{
					str = TEXTCOLOR_YELLOW "Defend the flag!\n";
				}
				else
				{
					str = TEXTCOLOR_GREEN "Capture the flag!\n";
				}
			}
		}
		else
		{
			str = "";
		}
		break;
	case LevelState::ENDROUND_COUNTDOWN: {
		WinInfo win = ::levelstate.getWinInfo();
		if (win.type == WinInfo::WIN_DRAW)
			StrFormat(str, "Tied at the end of the round");
		else if (win.type == WinInfo::WIN_PLAYER)
			StrFormat(str, "%s wins the round", WinToColorString(win).c_str());
		else if (win.type == WinInfo::WIN_TEAM)
			StrFormat(str, "%s team wins the round", WinToColorString(win).c_str());
		else
			StrFormat(str, "Next round in " TEXTCOLOR_GREEN "%d",
			          ::levelstate.getCountdown());
		break;
	}
	case LevelState::ENDGAME_COUNTDOWN: {
		WinInfo win = ::levelstate.getWinInfo();
		if (win.type == WinInfo::WIN_DRAW)
			StrFormat(str, "The game ends in a tie");
		else if (win.type == WinInfo::WIN_PLAYER)
			StrFormat(str, "%s wins!", WinToColorString(win).c_str());
		else if (win.type == WinInfo::WIN_TEAM)
			StrFormat(str, "%s team wins!", WinToColorString(win).c_str());
		else
			StrFormat(str, "Intermission in " TEXTCOLOR_GREEN "%d",
			          ::levelstate.getCountdown());
		break;
	}
	}

	w = V_StringWidth(str.c_str()) * CleanYfac;
	h = 8 * CleanYfac;
	if (::hud_transparency > 0.0f)
	{
		screen->DrawTextStretchedLuc(CR_GREY, surface_width / 2 - w / 2,
		                             (surface_height / 4 - h / 2) + (12 * CleanYfac),
		                             str.c_str(), CleanYfac, CleanYfac);
	}

	::hud_transparency.ForceSet(oldtrans);
}

// [AM] Spectator HUD.
void SpectatorHUD()
{
	int color;
	int iy = 4;

	// Draw warmup state or timer
	if (::hud_timer)
	{
		if (::hud_bigfont)
		{
			V_SetFont("BIGFONT");
		}

		hud::DrawText(0, iy, hud_scale, hud::X_CENTER, hud::Y_BOTTOM, hud::X_CENTER,
		              hud::Y_BOTTOM, hud::Timer().c_str(), CR_GREY);
		iy += V_LineHeight() + 1;

		if (::hud_bigfont)
			V_SetFont("SMALLFONT");
	}

	// Draw help text - spy player name is handled elsewhere.
	hud::DrawText(0, iy, hud_scale, hud::X_CENTER, hud::Y_BOTTOM, hud::X_CENTER,
	              hud::Y_BOTTOM, hud::HelpText().c_str(), CR_GREY);
	iy += V_LineHeight() + 1;

	// Draw targeted player names.
	hud::EATargets(0, iy, hud_scale, hud::X_CENTER, hud::Y_BOTTOM, hud::X_CENTER,
	               hud::Y_BOTTOM, 1, 0);

	// Draw gametype scoreboard
	hud::drawGametype();
}

// [AM] Original ZDoom HUD
void ZDoomHUD() {
	player_t *plyr = &displayplayer();
	int y, i;
	ammotype_t ammotype = weaponinfo[plyr->readyweapon].ammotype;
	int xscale = hud_scale ? CleanXfac : 1;
	int yscale = hud_scale ? CleanYfac : 1;

	y = I_GetSurfaceHeight() - (num_height + 4) * yscale;

	// Draw health
	{
		const patch_t* curr_powerup = W_ResolvePatchHandle(medi[0]);
		int xPos = 20;
		int yPos = 2;

		if (plyr->powers[pw_strength])
		{
			curr_powerup = W_ResolvePatchHandle(medi[1]);
			xPos -= 1;	// the x position of the Berzerk is 1 pixel to the right compared to the Medikit.
			yPos += 4;	// the y position of the Berzerk is slightly lowered by 4. So make it the same y position as the medikit.
		}

		if (hud_scale)
			screen->DrawLucentPatchCleanNoMove(curr_powerup, xPos * CleanXfac,
				I_GetSurfaceHeight() - yPos * CleanYfac);
		else
			screen->DrawLucentPatch(curr_powerup, xPos, I_GetSurfaceHeight() - yPos);
		ST_DrawNum(40 * xscale, y, screen, plyr->health);
	}

	// Draw armor
	if (plyr->armortype && plyr->armorpoints)
	{
		const patch_t* current_armor = W_ResolvePatchHandle(armors[1]);
		if(plyr->armortype == 1)
			current_armor = W_ResolvePatchHandle(armors[0]);

		if (current_armor)
		{
			if (hud_scale)
				screen->DrawLucentPatchCleanNoMove (current_armor, 20 * CleanXfac, y - 4*CleanYfac);
			else
				screen->DrawLucentPatch (current_armor, 20, y - 4);
		}
		ST_DrawNum(40 * xscale,
		           y - (W_ResolvePatchHandle(armors[0])->height() + 3) * yscale, screen,
		           plyr->armorpoints);
	}

	// Draw ammo
	if (ammotype < NUMAMMO)
	{
		const patch_t* ammopatch =
		    W_ResolvePatchHandle(ammos[weaponinfo[plyr->readyweapon].ammotype]);

		if (hud_scale)
			screen->DrawLucentPatchCleanNoMove(ammopatch,
							I_GetSurfaceWidth() - 14 * CleanXfac, I_GetSurfaceHeight() - 4 * CleanYfac);
		else
			screen->DrawLucentPatch(ammopatch, I_GetSurfaceWidth() - 14, I_GetSurfaceHeight() - 4);

		ST_DrawNumRight(I_GetSurfaceWidth() - 25 * xscale, y, screen, plyr->ammo[ammotype]);
	}

	// Draw top-right info. (Keys/Frags/Score)
	hud::drawGametype();

	if (sv_gametype != GM_COOP)
	{
		// Draw frags (in DM)
		ST_DrawNumRight(I_GetSurfaceWidth() - (2 * xscale), 2 * yscale, screen, plyr->fragcount);
	}
	else
	{
		// Draw keys (not DM)
		y = CleanYfac;
		for (i = 0; i < 6; i++)
		{
			if (plyr->cards[i])
			{
				patch_t* keysi = W_ResolvePatchHandle(keys[i]);

				if (hud_scale)
				{
					screen->DrawLucentPatchCleanNoMove(
					    keysi, I_GetSurfaceWidth() - 10 * CleanXfac, y);
				}
				else
				{
					screen->DrawLucentPatch(keysi, I_GetSurfaceWidth() - 10, y);
				}

				y += (8 + (i < 3 ? 0 : 2)) * yscale;
			}
		}
	}

	int color;
	std::string str;

	// Draw warmup state or timer
	if (hud_timer) {
		hud::DrawText(0, 4, hud_scale, hud::X_CENTER, hud::Y_BOTTOM, hud::X_CENTER,
		              hud::Y_BOTTOM, hud::Timer().c_str(), CR_GREY);
	}

	if (g_lives)
	{
		// Lives are next to doomguy.  Supposed to be vertically-centered with his head.
		int lives_color = CR_GREY;
		if (plyr->lives <= 0)
			lives_color = CR_DARKGREY;
		
		std::string buf;
		StrFormat(buf, "x%d", plyr->lives);
		hud::DrawText(60 + 2 + 20 + 2, 10 + 2, hud_scale, hud::X_LEFT, hud::Y_BOTTOM,
		              hud::X_LEFT, hud::Y_MIDDLE, buf.c_str(), lives_color, false);
	}

	// Draw other player name, if spying
	hud::DrawText(0, 12, hud_scale, hud::X_CENTER, hud::Y_BOTTOM, hud::X_CENTER,
	              hud::Y_BOTTOM, hud::SpyPlayerName().c_str(), CR_GREY);

	// Draw targeted player names.
	hud::EATargets(0, 20, hud_scale,
	               hud::X_CENTER, hud::Y_BOTTOM,
	               hud::X_CENTER, hud::Y_BOTTOM,
	               1, 0);
}

// [AM] HUD drawn with the Doom Status Bar.
void DoomHUD()
{
	int surface_width = I_GetSurfaceWidth(), surface_height = I_GetSurfaceHeight();

	// ST_Y is the number of pixels of viewable space, taking into account the
	// status bar.  We need to convert this into scaled pixels as best we can.
	int st_y = surface_height - ST_StatusBarY(surface_width, surface_height);
	if (hud_scale)
		st_y /= CleanYfac;

	// Draw warmup state or timer
	if (hud_timer)
	{
		hud::DrawText(0, st_y + 4, hud_scale, hud::X_CENTER, hud::Y_BOTTOM, hud::X_CENTER,
		              hud::Y_BOTTOM, hud::Timer().c_str(), CR_UNTRANSLATED);
	}

	// Draw other player name, if spying
	hud::DrawText(0, st_y + 12, hud_scale, hud::X_CENTER, hud::Y_BOTTOM, hud::X_CENTER,
	              hud::Y_BOTTOM, hud::SpyPlayerName().c_str(), CR_UNTRANSLATED);

	// Draw targeted player names.
	hud::EATargets(0, st_y + 20, hud_scale,
	               hud::X_CENTER, hud::Y_BOTTOM,
	               hud::X_CENTER, hud::Y_BOTTOM,
	               1, hud_targetcount);

	// Draw gametype scoreboard
	hud::drawGametype();
}

}

BEGIN_COMMAND(netprotoup)
{
	if (hud::proto_selected > 0)
		hud::proto_selected -= 1;
}
END_COMMAND(netprotoup)

BEGIN_COMMAND(netprotodown)
{
	// Rely on clamp in drawer
	hud::proto_selected += 1;
}
END_COMMAND(netprotodown)

VERSION_CONTROL (st_new_cpp, "$Id$")
