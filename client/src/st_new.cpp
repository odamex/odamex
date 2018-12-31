// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
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
//	Additional status bar stuff
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>

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
#include "m_swap.h"
#include "st_stuff.h"
#include "hu_drawers.h"
#include "hu_elements.h"
#include "hu_stuff.h"
#include "c_cvars.h"
#include "p_ctf.h"
#include "cl_vote.h"

static int		widestnum, numheight;
static const patch_t	*medi;
static const patch_t	*armors[2];
static const patch_t	*ammos[4];
static const patch_t	*bigammos[4];
static const patch_t	*flagiconteam;
static const patch_t	*flagiconbhome;
static const patch_t	*flagiconrhome;
static const patch_t	*flagiconbtakenbyb;
static const patch_t	*flagiconbtakenbyr;
static const patch_t	*flagiconrtakenbyb;
static const patch_t	*flagiconrtakenbyr;
static const patch_t	*flagicongtakenbyb;
static const patch_t	*flagicongtakenbyr;
static const patch_t	*flagiconbdropped;
static const patch_t	*flagiconrdropped;
static const patch_t *line_leftempty;
static const patch_t *line_leftfull;
static const patch_t *line_centerempty;
static const patch_t *line_centerleft;
static const patch_t *line_centerright;
static const patch_t *line_centerfull;
static const patch_t *line_rightempty;
static const patch_t *line_rightfull;
static const char ammopatches[4][8] = {"CLIPA0", "SHELA0", "CELLA0", "ROCKA0"};
static const char bigammopatches[4][8] = {"AMMOA0", "SBOXA0", "CELPA0", "BROKA0"};
static int		NameUp = -1;

extern patch_t	*sttminus;
extern patch_t	*tallnum[10];
extern patch_t	*faces[];
extern int		st_faceindex;
extern patch_t	*keys[NUMCARDS+NUMCARDS/2];
extern byte		*Ranges;
extern flagdata CTFdata[NUMFLAGS];

extern NetDemo netdemo;

int V_TextScaleXAmount();
int V_TextScaleYAmount();

EXTERN_CVAR (hud_scale)
EXTERN_CVAR (hud_timer)
EXTERN_CVAR (hud_targetcount)
EXTERN_CVAR (sv_fraglimit)

void ST_unloadNew (void)
{
	int i;

	Z_ChangeTag (medi, PU_CACHE);

	Z_ChangeTag (flagiconteam, PU_CACHE);
	Z_ChangeTag (flagiconbhome, PU_CACHE);
	Z_ChangeTag (flagiconrhome, PU_CACHE);
	Z_ChangeTag (flagiconbtakenbyb, PU_CACHE);
	Z_ChangeTag (flagiconbtakenbyr, PU_CACHE);
	Z_ChangeTag (flagiconrtakenbyb, PU_CACHE);
	Z_ChangeTag (flagiconrtakenbyr, PU_CACHE);
	Z_ChangeTag (flagicongtakenbyb, PU_CACHE);
	Z_ChangeTag (flagicongtakenbyr, PU_CACHE);
	Z_ChangeTag (flagiconbdropped, PU_CACHE);
	Z_ChangeTag (flagiconrdropped, PU_CACHE);
	Z_ChangeTag (line_leftempty, PU_CACHE);
	Z_ChangeTag (line_leftfull, PU_CACHE);
	Z_ChangeTag (line_centerempty, PU_CACHE);
	Z_ChangeTag (line_centerleft, PU_CACHE);
	Z_ChangeTag (line_centerright, PU_CACHE);
	Z_ChangeTag (line_centerfull, PU_CACHE);
	Z_ChangeTag (line_rightempty, PU_CACHE);
	Z_ChangeTag (line_rightfull, PU_CACHE);

	for (i = 0; i < 2; i++)
		Z_ChangeTag (armors[i], PU_CACHE);

	for (i = 0; i < 4; i++)
		Z_ChangeTag (ammos[i], PU_CACHE);
}

void ST_initNew (void)
{
	int i;
	int widest = 0;
	char name[8];
	int lump;

	// denis - todo - security - these patches have unchecked dimensions
	// ie, if a patch has a 0 width/height, it may cause a divide by zero
	// somewhere else in the code. we download wads, so this is an issue!

	for (i = 0; i < 10; i++) {
		if (tallnum[i]->width() > widest)
			widest = tallnum[i]->width();
	}

	strcpy (name, "ARM1A0");
	for (i = 0; i < 2; i++) {
		name[3] = i + '1';
		if ((lump = W_CheckNumForName (name, ns_sprites)) != -1)
			armors[i] = W_CachePatch (lump, PU_STATIC);
	}

	for (i = 0; i < 4; i++) {
		if ((lump = W_CheckNumForName (ammopatches[i], ns_sprites)) != -1)
			ammos[i] = W_CachePatch (lump, PU_STATIC);
		if ((lump = W_CheckNumForName (bigammopatches[i], ns_sprites)) != -1)
			bigammos[i] = W_CachePatch (lump, PU_STATIC);
	}

	if ((lump = W_CheckNumForName ("MEDIA0", ns_sprites)) != -1)
		medi = W_CachePatch (lump, PU_STATIC);

	flagiconteam = W_CachePatch ("FLAGIT", PU_STATIC);
	flagiconbhome = W_CachePatch ("FLAGIC2B", PU_STATIC);
	flagiconrhome = W_CachePatch ("FLAGIC2R", PU_STATIC);
	flagiconbtakenbyb = W_CachePatch ("FLAGI3BB", PU_STATIC);
	flagiconbtakenbyr = W_CachePatch ("FLAGI3BR", PU_STATIC);
	flagiconrtakenbyb = W_CachePatch ("FLAGI3RB", PU_STATIC);
	flagiconrtakenbyr = W_CachePatch ("FLAGI3RR", PU_STATIC);
	flagiconbdropped = W_CachePatch ("FLAGIC4B", PU_STATIC);
	flagiconrdropped = W_CachePatch ("FLAGIC4R", PU_STATIC);

	widestnum = widest;
	numheight = tallnum[0]->height();

	if (multiplayer && (sv_gametype == GM_COOP || demoplayback) && level.time)
		NameUp = level.time + 2*TICRATE;

	line_leftempty = W_CachePatch ("ODABARLE", PU_STATIC);
	line_leftfull = W_CachePatch ("ODABARLF", PU_STATIC);
	line_centerempty = W_CachePatch ("ODABARCE", PU_STATIC);
	line_centerleft = W_CachePatch ("ODABARCL", PU_STATIC);
	line_centerright = W_CachePatch ("ODABARCR", PU_STATIC);
	line_centerfull = W_CachePatch ("ODABARCF", PU_STATIC);
	line_rightempty = W_CachePatch ("ODABARRE", PU_STATIC);
	line_rightfull = W_CachePatch ("ODABARRF", PU_STATIC);
}

void ST_DrawNum (int x, int y, DCanvas *scrn, int num)
{
	char digits[8], *d;

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

namespace hud {

// [AM] Draw CTF scoreboard
void drawCTF() {
	if (sv_gametype != GM_CTF) {
		return;
	}

	player_t *plyr = &consoleplayer();
	int xscale = hud_scale ? CleanXfac : 1;
	int yscale = hud_scale ? CleanYfac : 1;
	const patch_t *flagbluepatch = flagiconbhome;
	const patch_t *flagredpatch = flagiconrhome;

	switch (CTFdata[it_blueflag].state) {
		case flag_carried:
			if (CTFdata[it_blueflag].flagger) {
				player_t &player = idplayer(CTFdata[it_blueflag].flagger);
				if (player.userinfo.team == TEAM_BLUE) {
					flagbluepatch = flagiconbtakenbyb;
				} else if (player.userinfo.team == TEAM_RED) {
					flagbluepatch = flagiconbtakenbyr;
				}
			}
			break;
		case flag_dropped:
			flagbluepatch = flagiconbdropped;
			break;
		default:
			break;
	}

	switch (CTFdata[it_redflag].state) {
		case flag_carried:
			if (CTFdata[it_redflag].flagger) {
				player_t &player = idplayer(CTFdata[it_redflag].flagger);
				if (player.userinfo.team == TEAM_BLUE) {
					flagredpatch = flagiconrtakenbyb;
				} else if (player.userinfo.team == TEAM_RED) {
					flagredpatch = flagiconrtakenbyr;
				}
			}
			break;
		case flag_dropped:
			flagredpatch = flagiconrdropped;
			break;
		default:
			break;
	}

	// Draw base flag patches
	hud::DrawPatch(4, 61, hud_scale,
	               hud::X_RIGHT, hud::Y_BOTTOM,
	               hud::X_RIGHT, hud::Y_BOTTOM,
	               flagbluepatch);
	hud::DrawPatch(4, 43, hud_scale,
	               hud::X_RIGHT, hud::Y_BOTTOM,
	               hud::X_RIGHT, hud::Y_BOTTOM,
	               flagredpatch);

	// Draw team border
	switch (plyr->userinfo.team) {
		case TEAM_BLUE:
			hud::DrawPatch(4, 61, hud_scale,
			               hud::X_RIGHT, hud::Y_BOTTOM,
			               hud::X_RIGHT, hud::Y_BOTTOM,
			               flagiconteam);
			break;
		case TEAM_RED:
			hud::DrawPatch(4, 43, hud_scale,
			               hud::X_RIGHT, hud::Y_BOTTOM,
			               hud::X_RIGHT, hud::Y_BOTTOM,
			               flagiconteam);
			break;
		default:
			break;
	}

	// Draw team scores
	ST_DrawNumRight(I_GetSurfaceWidth() - 24 * xscale, I_GetSurfaceHeight() - (62 + 16) * yscale,
	                screen, TEAMpoints[TEAM_BLUE]);
	ST_DrawNumRight(I_GetSurfaceWidth() - 24 * xscale, I_GetSurfaceHeight() - (44 + 16) * yscale,
	                screen, TEAMpoints[TEAM_RED]);
}

// [AM] Draw netdemo state
// TODO: This is ripe for commonizing, but I _need_ to get this done soon.
void drawNetdemo() {
	if (!(netdemo.isPlaying() || netdemo.isPaused())) {
		return;
	}

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
	player_t *plyr = &displayplayer();

	// TODO: I can probably get rid of these invocations once I put a
	//       copy of ST_DrawNumRight into the hud namespace. -AM
	unsigned int y, xscale, yscale;
	xscale = hud_scale ? CleanXfac : 1;
	yscale = hud_scale ? CleanYfac : 1;
	y = I_GetSurfaceHeight() - (numheight + 4) * yscale;

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

	// Draw warmup state or timer
	str = hud::Warmup(color);
	if (!str.empty())
	{
		hud::DrawText(0, 4, hud_scale,
		              hud::X_CENTER, hud::Y_BOTTOM,
		              hud::X_CENTER, hud::Y_BOTTOM,
		              str.c_str(), color);
	}
	else if (hud_timer)
	{
		str = hud::Timer(color);
		hud::DrawText(0, 4, hud_scale,
		              hud::X_CENTER, hud::Y_BOTTOM,
		              hud::X_CENTER, hud::Y_BOTTOM,
		              str.c_str(), color);
	}

	// Draw other player name, if spying
	str = hud::SpyPlayerName(color);
	hud::DrawText(0, 12, hud_scale,
	              hud::X_CENTER, hud::Y_BOTTOM,
	              hud::X_CENTER, hud::Y_BOTTOM,
	              str.c_str(), color);

	// Draw targeted player names.
	hud::EATargets(0, 20, hud_scale,
	               hud::X_CENTER, hud::Y_BOTTOM,
	               hud::X_CENTER, hud::Y_BOTTOM,
	               1, hud_targetcount);

	// Draw stat lines.  Vertically aligned with the bottom of the armor
	// number on the other side of the screen.
	str = hud::PersonalSpread(color);
	hud::DrawText(4, 32, hud_scale,
	              hud::X_RIGHT, hud::Y_BOTTOM,
	              hud::X_RIGHT, hud::Y_BOTTOM,
	              str.c_str(), color);
	str = hud::PersonalScore(color);
	hud::DrawText(4, 24, hud_scale,
	              hud::X_RIGHT, hud::Y_BOTTOM,
	              hud::X_RIGHT, hud::Y_BOTTOM,
	              str.c_str(), color);

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

	// Draw CTF scoreboard
	hud::drawCTF();
}

// [AM] Spectator HUD.
void SpectatorHUD() {
	int color;
	std::string str;

	// Draw warmup state or timer
	str = hud::Warmup(color);
	if (!str.empty()) {
		hud::DrawText(0, 4, hud_scale,
		              hud::X_CENTER, hud::Y_BOTTOM,
		              hud::X_CENTER, hud::Y_BOTTOM,
		              str.c_str(), color);
	} else if (hud_timer) {
		str = hud::Timer(color);
		hud::DrawText(0, 4, hud_scale,
		              hud::X_CENTER, hud::Y_BOTTOM,
		              hud::X_CENTER, hud::Y_BOTTOM,
		              str.c_str(), color);
	}

	// Draw other player name, if spying
	str = hud::SpyPlayerName(color);
	hud::DrawText(0, 12, hud_scale,
	              hud::X_CENTER, hud::Y_BOTTOM,
	              hud::X_CENTER, hud::Y_BOTTOM,
	              str.c_str(), color);

	// Draw help text if there's no other player name
	if (str.empty()) {
		hud::DrawText(0, 12, hud_scale,
		              hud::X_CENTER, hud::Y_BOTTOM,
		              hud::X_CENTER, hud::Y_BOTTOM,
		              hud::HelpText().c_str(), CR_GREY);
	}

	// Draw targeted player names.
	hud::EATargets(0, 20, hud_scale,
	               hud::X_CENTER, hud::Y_BOTTOM,
	               hud::X_CENTER, hud::Y_BOTTOM,
	               1, 0);

	// Draw CTF scoreboard
	hud::drawCTF();
}

// [AM] Original ZDoom HUD
void ZDoomHUD() {
	player_t *plyr = &displayplayer();
	int y, i;
	ammotype_t ammotype = weaponinfo[plyr->readyweapon].ammotype;
	int xscale = hud_scale ? CleanXfac : 1;
	int yscale = hud_scale ? CleanYfac : 1;

	y = I_GetSurfaceHeight() - (numheight + 4) * yscale;

	// Draw health
	if (hud_scale)
		screen->DrawLucentPatchCleanNoMove (medi, 20 * CleanXfac,
									  I_GetSurfaceHeight() - 2*CleanYfac);
	else
		screen->DrawLucentPatch (medi, 20, I_GetSurfaceHeight() - 2);
	ST_DrawNum (40 * xscale, y, screen, plyr->health);

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
    if (sv_gametype == GM_CTF)
    {
		hud::drawCTF();
    }
	else if (sv_gametype != GM_COOP)
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
	str = hud::Warmup(color);
	if (!str.empty()) {
		hud::DrawText(0, 4, hud_scale,
		              hud::X_CENTER, hud::Y_BOTTOM,
		              hud::X_CENTER, hud::Y_BOTTOM,
		              str.c_str(), color);
	} else if (hud_timer) {
		str = hud::Timer(color);
		hud::DrawText(0, 4, hud_scale,
		              hud::X_CENTER, hud::Y_BOTTOM,
		              hud::X_CENTER, hud::Y_BOTTOM,
		              str.c_str(), color);
	}

	// Draw other player name, if spying
	str = hud::SpyPlayerName(color);
	hud::DrawText(0, 12, hud_scale,
	              hud::X_CENTER, hud::Y_BOTTOM,
	              hud::X_CENTER, hud::Y_BOTTOM,
	              str.c_str(), color);

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

	int color;
	std::string str;

	// Draw warmup state or timer
	str = hud::Warmup(color);
	if (!str.empty())
	{
		hud::DrawText(0, st_y + 4, hud_scale,
		              hud::X_CENTER, hud::Y_BOTTOM,
		              hud::X_CENTER, hud::Y_BOTTOM,
		              str.c_str(), color);
	}
	else if (hud_timer)
	{
		str = hud::Timer(color);
		hud::DrawText(0, st_y + 4, hud_scale,
		              hud::X_CENTER, hud::Y_BOTTOM,
		              hud::X_CENTER, hud::Y_BOTTOM,
		              str.c_str(), color);
	}

	// Draw other player name, if spying
	str = hud::SpyPlayerName(color);
	hud::DrawText(0, st_y + 12, hud_scale,
	              hud::X_CENTER, hud::Y_BOTTOM,
	              hud::X_CENTER, hud::Y_BOTTOM,
	              str.c_str(), color);

	// Draw targeted player names.
	hud::EATargets(0, st_y + 20, hud_scale,
	               hud::X_CENTER, hud::Y_BOTTOM,
	               hud::X_CENTER, hud::Y_BOTTOM,
	               1, hud_targetcount);

	// Draw CTF scoreboard
	hud::drawCTF();
}

}

VERSION_CONTROL (st_new_cpp, "$Id$")
