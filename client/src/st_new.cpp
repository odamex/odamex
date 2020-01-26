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
#include <sstream>

#include "doomtype.h"
#include "doomdef.h"
#include "doomstat.h"
#include "cl_demo.h"
#include "d_items.h"
#include "i_video.h"
#include "v_video.h"
#include "v_text.h"
#include "z_zone.h"
#include "i_system.h"
#include "st_stuff.h"
#include "hu_drawers.h"
#include "hu_elements.h"
#include "c_cvars.h"
#include "p_ctf.h"
#include "cl_vote.h"
#include "resources/res_main.h"
#include "resources/res_texture.h"

static int		widestnum, numheight;
static const Texture* medi[2];
static const Texture* armors[2];
static const Texture* ammos[4];
static const Texture* bigammos[4];
static const Texture* flagiconteam;
static const Texture* flagiconbhome;
static const Texture* flagiconrhome;
static const Texture* flagiconbtakenbyb;
static const Texture* flagiconbtakenbyr;
static const Texture* flagiconrtakenbyb;
static const Texture* flagiconrtakenbyr;
static const Texture* flagicongtakenbyb;
static const Texture* flagicongtakenbyr;
static const Texture* flagiconbdropped;
static const Texture* flagiconrdropped;
static const Texture* line_leftempty;
static const Texture* line_leftfull;
static const Texture* line_centerempty;
static const Texture* line_centerleft;
static const Texture* line_centerright;
static const Texture* line_centerfull;
static const Texture* line_rightempty;
static const Texture* line_rightfull;

static const char medipatches[2][8] = { "MEDIA0", "PSTRA0" };
static const char ammopatches[4][8] = {"CLIPA0", "SHELA0", "CELLA0", "ROCKA0"};
static const char bigammopatches[4][8] = {"AMMOA0", "SBOXA0", "CELPA0", "BROKA0"};

static int		NameUp = -1;

extern const Texture*	sttminus;
extern const Texture*	tallnum[10];
extern const Texture*	faces[];
extern int		st_faceindex;
extern const Texture*	keys[NUMCARDS+NUMCARDS/2];
extern byte		*Ranges;
extern flagdata CTFdata[NUMFLAGS];

extern NetDemo netdemo;

int V_TextScaleXAmount();
int V_TextScaleYAmount();

EXTERN_CVAR (hud_scale)
EXTERN_CVAR (hud_timer)
EXTERN_CVAR (hud_targetcount)
EXTERN_CVAR (hud_demobar)
EXTERN_CVAR (sv_fraglimit)

void ST_unloadNew (void)
{
	int i;

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
	{
		Z_ChangeTag(medi[i], PU_CACHE);
		Z_ChangeTag(armors[i], PU_CACHE);
	}

	for (i = 0; i < 4; i++)
		Z_ChangeTag (ammos[i], PU_CACHE);
}

void ST_initNew (void)
{
	int i;
	int widest = 0;
	char name[8];

	// denis - todo - security - these patches have unchecked dimensions
	// ie, if a patch has a 0 width/height, it may cause a divide by zero
	// somewhere else in the code. we download wads, so this is an issue!

	for (i = 0; i < 10; i++) {
		if (tallnum[i]->mWidth > widest)
			widest = tallnum[i]->mWidth;
	}

	strcpy (name, "ARM1A0");
	for (i = 0; i < 2; i++) {
		name[3] = i + '1';
		if (Res_CheckResource(name, sprites_directory_name))
			armors[i] = Res_CacheTexture(name, SPRITE, PU_STATIC);
	}

	for (i = 0; i < 4; i++) {
		if (Res_CheckResource(ammopatches[i], sprites_directory_name))
			ammos[i] = Res_CacheTexture(ammopatches[i], sprites_directory_name, PU_STATIC);
		if (Res_CheckResource(bigammopatches[i], sprites_directory_name))
			bigammos[i] = Res_CacheTexture(bigammopatches[i], SPRITE, PU_STATIC);
	}

	for (i = 0; i < 2; i++)
	{
		if (Res_CheckResource(medipatches[i], sprites_directory_name))
			medi[i] = Res_CacheTexture(medipatches[i], SPRITE, PU_STATIC);
	}

	flagiconteam = Res_CacheTexture("FLAGIT", SPRITE, PU_STATIC);
	flagiconbhome = Res_CacheTexture("FLAGIC2B", SPRITE, PU_STATIC);
	flagiconrhome = Res_CacheTexture("FLAGIC2R", SPRITE, PU_STATIC);
	flagiconbtakenbyb = Res_CacheTexture("FLAGI3BB", SPRITE, PU_STATIC);
	flagiconbtakenbyr = Res_CacheTexture("FLAGI3BR", SPRITE, PU_STATIC);
	flagiconrtakenbyb = Res_CacheTexture("FLAGI3RB", SPRITE, PU_STATIC);
	flagiconrtakenbyr = Res_CacheTexture("FLAGI3RR", SPRITE, PU_STATIC);
	flagiconbdropped = Res_CacheTexture("FLAGIC4B", SPRITE, PU_STATIC);
	flagiconrdropped = Res_CacheTexture("FLAGIC4R", SPRITE, PU_STATIC);

	widestnum = widest;
	numheight = tallnum[0]->mHeight;

	if (multiplayer && (sv_gametype == GM_COOP || demoplayback) && level.time)
		NameUp = level.time + 2*TICRATE;

	line_leftempty = Res_CacheTexture("ODABARLE", SPRITE, PU_STATIC);
	line_leftfull = Res_CacheTexture("ODABARLF", SPRITE, PU_STATIC);
	line_centerempty = Res_CacheTexture("ODABARCE", SPRITE, PU_STATIC);
	line_centerleft = Res_CacheTexture("ODABARCL", SPRITE, PU_STATIC);
	line_centerright = Res_CacheTexture("ODABARCR", SPRITE, PU_STATIC);
	line_centerfull = Res_CacheTexture("ODABARCF", SPRITE, PU_STATIC);
	line_rightempty = Res_CacheTexture("ODABARRE", SPRITE, PU_STATIC);
	line_rightfull = Res_CacheTexture("ODABARRF", SPRITE, PU_STATIC);
}

void ST_DrawNum (int x, int y, DCanvas *scrn, int num)
{
	char digits[8], *d;

	if (num < 0)
	{
		if (hud_scale)
		{
			scrn->DrawLucentTextureCleanNoMove(sttminus, x, y);
			x += CleanXfac * sttminus->mWidth;
		}
		else
		{
			scrn->DrawLucentTexture(sttminus, x, y);
			x += sttminus->mWidth;
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
				scrn->DrawLucentTextureCleanNoMove(tallnum[*d - '0'], x, y);
				x += CleanXfac * tallnum[*d - '0']->mWidth;
			}
			else
			{
				scrn->DrawLucentTexture(tallnum[*d - '0'], x, y);
				x += tallnum[*d - '0']->mWidth;
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
		x -= tallnum[d%10]->mWidth * xscale;
	} while (d /= 10);

	if (num < 0)
		x -= sttminus->mWidth * xscale;

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
		const Texture* texture = NULL;
		if (!reverse) {
			if (i == 0 && !cutleft) {
				if (bar_filled == 0) {
					texture = line_leftempty;
				} else {
					texture = line_leftfull;
				}
			} else if (i == bar_width - 1 && !cutright) {
				if (bar_filled == bar_width) {
					texture = line_rightfull;
				} else {
					texture = line_rightempty;
				}
			} else {
				if (i == bar_filled - 1) {
					texture = line_centerleft;
				} else if (i < bar_filled) {
					texture = line_centerfull;
				} else {
					texture = line_centerempty;
				}
			}
		} else {
			if (i == 0 && !cutleft) {
				if (bar_filled == bar_width) {
					texture = line_leftfull;
				} else {
					texture = line_leftempty;
				}
			} else if (i == bar_width - 1 && !cutright) {
				if (bar_filled == 0) {
					texture = line_rightempty;
				} else {
					texture = line_rightfull;
				}
			} else {
				if (i == (bar_width - bar_filled)) {
					texture = line_centerright;
				} else if (i >= (bar_width - bar_filled)) {
					texture = line_centerfull;
				} else {
					texture = line_centerempty;
				}
			}
		}

		int xi = x + (i * xscale * 2);
		if (hud_scale) {
			screen->DrawTranslatedTextureCleanNoMove(texture, xi, y);
		} else {
			screen->DrawTranslatedTexture(texture, xi, y);
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
	const Texture* flagbluepatch = flagiconbhome;
	const Texture* flagredpatch = flagiconrhome;

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
	hud::DrawTexture(4, 61, hud_scale,
	               hud::X_RIGHT, hud::Y_BOTTOM,
	               hud::X_RIGHT, hud::Y_BOTTOM,
	               flagbluepatch);
	hud::DrawTexture(4, 43, hud_scale,
	               hud::X_RIGHT, hud::Y_BOTTOM,
	               hud::X_RIGHT, hud::Y_BOTTOM,
	               flagredpatch);

	// Draw team border
	switch (plyr->userinfo.team) {
		case TEAM_BLUE:
			hud::DrawTexture(4, 61, hud_scale,
			               hud::X_RIGHT, hud::Y_BOTTOM,
			               hud::X_RIGHT, hud::Y_BOTTOM,
			               flagiconteam);
			break;
		case TEAM_RED:
			hud::DrawTexture(4, 43, hud_scale,
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
	player_t *plyr = &displayplayer();

	// TODO: I can probably get rid of these invocations once I put a
	//       copy of ST_DrawNumRight into the hud namespace. -AM
	unsigned int y, xscale, yscale;
	xscale = hud_scale ? CleanXfac : 1;
	yscale = hud_scale ? CleanYfac : 1;
	y = I_GetSurfaceHeight() - (numheight + 4) * yscale;

	// Draw Armor if the player has any
	if (plyr->armortype && plyr->armorpoints) {
		const Texture* current_armor = armors[1];
		if (plyr->armortype == 1) {
			current_armor = armors[0];
		}

		if (current_armor) {
			// Draw Armor type.  Vertically centered against armor number.
			hud::DrawTextureScaled(48 + 2 + 10, 32, 20, 20, hud_scale,
			                     hud::X_LEFT, hud::Y_BOTTOM,
			                     hud::X_CENTER, hud::Y_MIDDLE,
			                     current_armor);
		}
		ST_DrawNumRight(48 * xscale, y - 20 * yscale, screen, plyr->armorpoints);
	}

	// Draw Doomguy.  Vertically scaled to an area two pixels above and
	// below the health number, and horizontally centered below the armor.
	hud::DrawTextureScaled(48 + 2 + 10, 2, 20, 20, hud_scale,
	                     hud::X_LEFT, hud::Y_BOTTOM,
	                     hud::X_CENTER, hud::Y_BOTTOM,
	                     faces[st_faceindex]);
	ST_DrawNumRight(48 * xscale, y, screen, plyr->health);

	// Draw Ammo
	ammotype_t ammotype = weaponinfo[plyr->readyweapon].ammotype;
	if (ammotype < NUMAMMO) {
		const Texture* ammopatch;
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
		if (ammopatch->mWidth > 16 || ammopatch->mHeight > 16) {
			hud::DrawTextureScaled(12, 12, 16, 16, hud_scale,
			                     hud::X_RIGHT, hud::Y_BOTTOM,
			                     hud::X_CENTER, hud::Y_MIDDLE,
			                     ammopatch);
		} else {
			hud::DrawTexture(12, 12, hud_scale,
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
				hud::DrawTexture(4 + (i * 10), 24, hud_scale,
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
	{
		const Texture* curr_powerup = medi[0];
		int xPos = 20;
		int yPos = 2;

		if (plyr->powers[pw_strength])
		{
			curr_powerup = medi[1];
			xPos -= 1;	// the x position of the Berzerk is 1 pixel to the right compared to the Medikit.
			yPos += 4;	// the y position of the Berzerk is slightly lowered by 4. So make it the same y position as the medikit.
		}

		if (hud_scale)
			screen->DrawLucentTextureCleanNoMove(curr_powerup, xPos * CleanXfac, I_GetSurfaceHeight() - yPos * CleanYfac);
		else
			screen->DrawLucentTexture(curr_powerup, xPos, I_GetSurfaceHeight() - yPos);
		ST_DrawNum(40 * xscale, y, screen, plyr->health);
	}

	// Draw armor
	if (plyr->armortype && plyr->armorpoints)
	{
		const Texture* current_armor = armors[1];
		if (plyr->armortype == 1)
			current_armor = armors[0];

		if (current_armor)
		{
			if (hud_scale)
				screen->DrawLucentTextureCleanNoMove (current_armor, 20 * CleanXfac, y - 4*CleanYfac);
			else
				screen->DrawLucentTexture (current_armor, 20, y - 4);
		}
		ST_DrawNum (40*xscale, y - (armors[0]->mHeight + 3) * yscale, screen, plyr->armorpoints);
	}

	// Draw ammo
	if (ammotype < NUMAMMO)
	{
		const Texture* ammopatch = ammos[weaponinfo[plyr->readyweapon].ammotype];

		if (hud_scale)
			screen->DrawLucentTextureCleanNoMove(ammopatch, I_GetSurfaceWidth() - 14 * CleanXfac, I_GetSurfaceHeight() - 4 * CleanYfac);
		else
			screen->DrawLucentTexture(ammopatch, I_GetSurfaceWidth() - 14, I_GetSurfaceHeight() - 4);

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
					screen->DrawLucentTextureCleanNoMove(keys[i], I_GetSurfaceWidth() - 10*CleanXfac, y);
				else
					screen->DrawLucentTexture(keys[i], I_GetSurfaceWidth() - 10, y);

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
