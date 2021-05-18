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
#include "cl_vote.h"
#include "g_levelstate.h"
#include "g_gametype.h"
#include "c_bind.h"
#include "gui_element.h"

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
static const patch_t* medi[ARRAY_LENGTH(::medipatches)];
static const patch_t* armors[ARRAY_LENGTH(::armorpatches)];
static const patch_t* ammos[ARRAY_LENGTH(::ammopatches)];
static const patch_t* bigammos[ARRAY_LENGTH(::bigammopatches)];
static const patch_t* flagiconteam;
static const patch_t* flagiconteamoffense;
static const patch_t* flagiconteamdefense;
static const patch_t* line_leftempty;
static const patch_t* line_leftfull;
static const patch_t* line_centerempty;
static const patch_t* line_centerleft;
static const patch_t* line_centerright;
static const patch_t* line_centerfull;
static const patch_t* line_rightempty;
static const patch_t* line_rightfull;
static const patch_t* FlagIconHome[NUMTEAMS];
static const patch_t* FlagIconReturn[NUMTEAMS];
static const patch_t* FlagIconTaken[NUMTEAMS];
static const patch_t* FlagIconDropped[NUMTEAMS];
static const patch_t* LivesIcon[NUMTEAMS];

static int		NameUp = -1;

extern patch_t	*sttminus;
extern patch_t	*tallnum[10];
extern patch_t	*faces[];
extern int		st_faceindex;
extern patch_t	*keys[NUMCARDS+NUMCARDS/2];
extern byte		*Ranges;

extern NetDemo netdemo;

typedef std::vector<const patch_t**> PathFreeList;

/**
 * @brief Stores pointers to status bar objects that should be freed on shutdown. 
 */
PathFreeList freelist;

int V_TextScaleXAmount();
int V_TextScaleYAmount();

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

/**
 * @brief Caches a patch, while also adding to a freelist, where it can be
 *        cleaned up in one fell swoop. 
 * 
 * @param patch Destination pointer to write to.
 * @param lump Lump name to cache.
 */
static void CacheHUDPatch(const patch_t** patch, const char* lump)
{
	*patch = W_CachePatch(lump, PU_STATIC);
	::freelist.push_back(patch);
}

/**
 * @brief Cache a patch from a sprite, which needs a special lookup.
 * 
 * @param patch Destination pointer to write to.
 * @param lump Lump name to cache.
 */
static void CacheHUDSprite(const patch_t** patch, const char* lump)
{
	*patch = W_CachePatch(W_GetNumForName(lump, ns_sprites), PU_STATIC);
	::freelist.push_back(patch);
}

/**
 * @brief In addition to unloading our patches we need to ensure the pointers
 *        are NULL, so we don't dereference a stale patch on WAD change by
 *        accident.
 */
void ST_unloadNew()
{
	PathFreeList::iterator it = ::freelist.begin();
	for (; it != ::freelist.end(); ++it)
	{
		if (**it != NULL)
			Z_ChangeTag(**it, PU_CACHE);
		**it = NULL;
	}
	::freelist.clear();
}

void ST_initNew()
{
	int widest = 0;

	// denis - todo - security - these patches have unchecked dimensions
	// ie, if a patch has a 0 width/height, it may cause a divide by zero
	// somewhere else in the code. we download wads, so this is an issue!

	for (size_t i = 0; i < ARRAY_LENGTH(::tallnum); i++)
	{
		if (::tallnum[i]->width() > widest)
			widest = ::tallnum[i]->width();
	}

	for (size_t i = 0; i < ARRAY_LENGTH(::medipatches); i++)
		CacheHUDSprite(&::medi[i], ::medipatches[i]);

	for (size_t i = 0; i < ARRAY_LENGTH(::armorpatches); i++)
		CacheHUDSprite(&::armors[i], ::armorpatches[i]);

	for (size_t i = 0; i < ARRAY_LENGTH(::ammopatches); i++)
	{
		CacheHUDSprite(&::ammos[i], ::ammopatches[i]);
		CacheHUDSprite(&::bigammos[i], ::bigammopatches[i]);
	}

	for (size_t i = 0; i < NUMTEAMS; i++)
	{
		CacheHUDPatch(&::FlagIconHome[i], ::flaghomepatches[i]);
		CacheHUDPatch(&::FlagIconTaken[i], ::flagtakenpatches[i]);
		CacheHUDPatch(&::FlagIconReturn[i], ::flagreturnpatches[i]);
		CacheHUDPatch(&::FlagIconDropped[i], ::flagdroppatches[i]);
		CacheHUDPatch(&::LivesIcon[i], ::livespatches[i]);
	}

	::widest_num = widest;
	::num_height = ::tallnum[0]->height();

	if (multiplayer && (sv_gametype == GM_COOP || demoplayback) && level.time)
		NameUp = level.time + 2 * TICRATE;

	CacheHUDPatch(&::flagiconteam, "FLAGIT");
	CacheHUDPatch(&::flagiconteamoffense, "FLAGITO");
	CacheHUDPatch(&::flagiconteamdefense, "FLAGITD");

	CacheHUDPatch(&::line_leftempty, "ODABARLE");
	CacheHUDPatch(&::line_leftfull, "ODABARLF");
	CacheHUDPatch(&::line_centerempty, "ODABARCE");
	CacheHUDPatch(&::line_centerleft, "ODABARCL");
	CacheHUDPatch(&::line_centerright, "ODABARCR");
	CacheHUDPatch(&::line_centerfull, "ODABARCF");
	CacheHUDPatch(&::line_rightempty, "ODABARRE");
	CacheHUDPatch(&::line_rightfull, "ODABARRF");
}

void ST_DrawNum (int x, int y, DCanvas *scrn, int num)
{
	char digits[11], *d;

	if (num < 0)
	{
		if (hud_scale)
		{
			scrn->DrawLucentPatchCleanNoMove (sttminus, x, y);
			x += CleanXfac * sttminus->width();
		}
		else
		{
			scrn->DrawLucentPatch (sttminus, x, y);
			x += sttminus->width();
		}
		num = -num;
	}

	sprintf (digits, "%d", num);

	d = digits;
	while (*d)
	{
		if (*d >= '0' && *d <= '9')
		{
			if (hud_scale)
			{
				scrn->DrawLucentPatchCleanNoMove (tallnum[*d - '0'], x, y);
				x += CleanXfac * tallnum[*d - '0']->width();
			}
			else
			{
				scrn->DrawLucentPatch (tallnum[*d - '0'], x, y);
				x += tallnum[*d - '0']->width();
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
		x -= tallnum[d%10]->width() * xscale;
	} while (d /= 10);

	if (num < 0)
		x -= sttminus->width() * xscale;

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
					linepatch = line_leftempty;
				} else {
					linepatch = line_leftfull;
				}
			} else if (i == bar_width - 1 && !cutright) {
				if (bar_filled == bar_width) {
					linepatch = line_rightfull;
				} else {
					linepatch = line_rightempty;
				}
			} else {
				if (i == bar_filled - 1) {
					linepatch = line_centerleft;
				} else if (i < bar_filled) {
					linepatch = line_centerfull;
				} else {
					linepatch = line_centerempty;
				}
			}
		} else {
			if (i == 0 && !cutleft) {
				if (bar_filled == bar_width) {
					linepatch = line_leftfull;
				} else {
					linepatch = line_leftempty;
				}
			} else if (i == bar_width - 1 && !cutright) {
				if (bar_filled == 0) {
					linepatch = line_rightempty;
				} else {
					linepatch = line_rightfull;
				}
			} else {
				if (i == (bar_width - bar_filled)) {
					linepatch = line_centerright;
				} else if (i >= (bar_width - bar_filled)) {
					linepatch = line_centerfull;
				} else {
					linepatch = line_centerempty;
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

#include "c_console.h"

/**
 * @brief Draw the chat window.
 */
void HU_ChatWindow()
{
	const Chatlog& log = C_GetChatLog();

	OFont* font = V_GetFont(FONT_DIGFONT);
	FontParams params(*font);
	params.color(CR_GREY);
	OGUIContext ctx;

	DGUIDim dim(ctx, "00 00 00", 0.75);
	dim.contain(LAY_FLEX | LAY_COLUMN);
	dim.size(300, 0);

	for (size_t i = 0; i < 10; i++)
	{
		if (i >= log.size())
			break;

		DGUIParagraph* text = new DGUIParagraph(ctx, log.at(i).line, params);
		dim.push_back(text);
	}
	dim.layout();
	lay_run_context(ctx.layoutAddr());
	dim.render();
	lay_reset_context(ctx.layoutAddr());
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

			const patch_t* drawPatch = ::FlagIconTaken[i];

			if (sv_gametype == GM_CTF && G_IsDefendingTeam(teamInfo->Team))
			{
				switch (teamInfo->FlagData.state)
				{
				case flag_home:
					drawPatch = ::FlagIconHome[i];
					break;
				case flag_carried:
					if (idplayer(teamInfo->FlagData.flagger).userinfo.team == i)
						drawPatch = ::FlagIconReturn[i];
					else
						drawPatch = ::FlagIconTaken[i];
					break;
				case flag_dropped:
					drawPatch = ::FlagIconDropped[i];
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
				const patch_t* itpatch = ::flagiconteam;
				if (G_IsSidesGame())
				{
					// Sides games show offense/defense.
					if (G_IsDefendingTeam(consoleplayer().userinfo.team))
						itpatch = ::flagiconteamdefense;
					else
						itpatch = ::flagiconteamoffense;
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
			               hud::Y_BOTTOM, hud::X_RIGHT, hud::Y_BOTTOM, ::LivesIcon[i]);

			StrFormat(buffer, "%d", teamInfo->LivesPool());
			int color = (i % 2) ? CR_GOLD : CR_GREY;
			hud::DrawText(SCREEN_BORDER + 12, patchPosY + 3, hud_scale, hud::X_RIGHT,
			              hud::Y_BOTTOM, hud::X_RIGHT, hud::Y_BOTTOM, buffer.c_str(),
			              color);
		}
	}
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

	// Draw demo elapsed time
	hud::DrawText(2, 47, hud_scale,
	              hud::X_LEFT, hud::Y_BOTTOM,
	              hud::X_LEFT, hud::Y_BOTTOM,
	              hud::NetdemoElapsed().c_str(), CR_GREY);

	// Draw map number/total
	hud::DrawText(74, 47, hud_scale,
	              hud::X_LEFT, hud::Y_BOTTOM,
	              hud::X_RIGHT, hud::Y_BOTTOM,
	              hud::NetdemoMaps().c_str(), CR_BRICK);

	// Draw the bar.
	// TODO: Once status bar notches have been implemented, put map
	//       change times in as notches.
	int color = CR_GOLD;
	if (netdemo.isPlaying()) {
		color = CR_GREEN;
	}
	ST_DrawBar(color, netdemo.calculateTimeElapsed(), netdemo.calculateTotalTime(),
	           2 * xscale, I_GetSurfaceHeight() - 46 * yscale, 72 * xscale);
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
		const patch_t *current_armor = armors[1];
		if (plyr->armortype == 1) {
			current_armor = armors[0];
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
	hud::DrawPatchScaled(48 + 2 + 10, 2, 20, 20, hud_scale,
	                     hud::X_LEFT, hud::Y_BOTTOM,
	                     hud::X_CENTER, hud::Y_BOTTOM,
	                     faces[st_faceindex]);
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
			ammopatch = bigammos[ammotype];
		} else {
			ammopatch = ammos[ammotype];
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
			V_SetFont(FONT_BIGFONT);

		hud::DrawText(0, 4, hud_scale, hud::X_CENTER, hud::Y_BOTTOM, hud::X_CENTER,
		              hud::Y_BOTTOM, hud::Timer().c_str(), CR_GREY);
		iy += V_LineHeight() + 1;

		if (::hud_bigfont)
			V_SetFont(FONT_SMALLFONT);
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
		V_SetFont(FONT_BIGFONT);

	hud::DrawText(4, 24 + V_LineHeight() + 1, hud_scale, hud::X_RIGHT, hud::Y_BOTTOM,
	              hud::X_RIGHT, hud::Y_BOTTOM, hud::PersonalSpread().c_str(),
	              CR_UNTRANSLATED);
	hud::DrawText(4, 24, hud_scale, hud::X_RIGHT, hud::Y_BOTTOM, hud::X_RIGHT,
	              hud::Y_BOTTOM, hud::PersonalScore().c_str(), CR_UNTRANSLATED);

	if (::hud_bigfont)
		V_SetFont(FONT_SMALLFONT);

	// Draw keys in coop
	if (sv_gametype == GM_COOP) {
		for (byte i = 0;i < NUMCARDS;i++) {
			if (plyr->cards[i]) {
				hud::DrawPatch(4 + (i * 10), 24, hud_scale,
				               hud::X_RIGHT, hud::Y_BOTTOM,
				               hud::X_RIGHT, hud::Y_BOTTOM,
				               keys[i]);
			}
		}
	}

	// Draw the chat window.
	HU_ChatWindow();

	// Draw gametype scoreboard
	hud::drawGametype();
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

	// First line...FONT_BIGFONT.
	std::string str;
	switch (::levelstate.getState())
	{
	case LevelState::WARMUP:
		if (consoleplayer().spectator)
		{
			str = "";
			break;
		}

		str = "Warmup";
		break;
	case LevelState::WARMUP_COUNTDOWN:
	case LevelState::WARMUP_FORCED_COUNTDOWN:
		str = G_GametypeName();
		break;
	case LevelState::PREROUND_COUNTDOWN:
		StrFormat(str, "Round %d\n", ::levelstate.getRound());
		break;
	case LevelState::INGAME:
		if (G_CanShowFightMessage())
		{
			if (G_IsSidesGame())
			{
				if (G_IsDefendingTeam(consoleplayer().userinfo.team))
				{
					str = "DEFEND!\n";
				}
				else
				{
					str = "CAPTURE!\n";
				}
			}
			else
			{
				str = "FIGHT!\n";
			}
		}
		else
		{
			str = "";
		}
		break;
	case LevelState::ENDROUND_COUNTDOWN:
		StrFormat(str, "Round %d complete\n", ::levelstate.getRound());
		break;
	case LevelState::ENDGAME_COUNTDOWN:
		StrFormat(str, "Match complete\n");
		break;
	default:
		str = "";
		break;
	}

	V_SetFont(FONT_BIGFONT);

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

	V_SetFont(FONT_SMALLFONT);

	// Second line...FONT_SMALLFONT.
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
			V_SetFont(FONT_BIGFONT);

		hud::DrawText(0, iy, hud_scale, hud::X_CENTER, hud::Y_BOTTOM, hud::X_CENTER,
		              hud::Y_BOTTOM, hud::Timer().c_str(), CR_GREY);
		iy += V_LineHeight() + 1;

		if (::hud_bigfont)
			V_SetFont(FONT_SMALLFONT);
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
		const patch_t *curr_powerup = medi[0];
		int xPos = 20;
		int yPos = 2;

		if (plyr->powers[pw_strength])
		{
			curr_powerup = medi[1];
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
		const patch_t *current_armor = armors[1];
		if(plyr->armortype == 1)
			current_armor = armors[0];

		if (current_armor)
		{
			if (hud_scale)
				screen->DrawLucentPatchCleanNoMove (current_armor, 20 * CleanXfac, y - 4*CleanYfac);
			else
				screen->DrawLucentPatch (current_armor, 20, y - 4);
		}
		ST_DrawNum (40*xscale, y - (armors[0]->height()+3)*yscale,
					 screen, plyr->armorpoints);
	}

	// Draw ammo
	if (ammotype < NUMAMMO)
	{
		const patch_t *ammopatch = ammos[weaponinfo[plyr->readyweapon].ammotype];

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
				if (hud_scale)
					screen->DrawLucentPatchCleanNoMove(keys[i], I_GetSurfaceWidth() - 10*CleanXfac, y);
				else
					screen->DrawLucentPatch(keys[i], I_GetSurfaceWidth() - 10, y);

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

VERSION_CONTROL (st_new_cpp, "$Id$")
