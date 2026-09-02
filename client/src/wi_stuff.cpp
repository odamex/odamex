// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2026 by The Odamex Team.
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
//		Intermission screens.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <ctype.h>
#include <array>

#include "z_zone.h"
#include "m_random.h"
#include "i_video.h"
#include "w_wad.h"
#include "g_game.h"
#include "r_local.h"
#include "s_sound.h"
#include "s_sndseq.h"
#include "v_video.h"
#include "wi_stuff.h"
#include "c_console.h"
#include "hu_stuff.h"
#include "v_palette.h"
#include "c_dispatch.h"
#include "v_text.h"
#include "gi.h"
#include "v_textcolors.h"
#include "wi_interlevel.h"

extern byte* Ranges;

void WI_unloadData(void);
size_t P_NumPlayersInGame();

//
// Data needed to add patches to full screen intermission pics.
// Patches are statistics messages, and animations.
// Loads of by-pixel layout and placement, offsets etc.
//

// GLOBAL LOCATIONS
constexpr int WI_TITLEY = 2;
constexpr int WI_SPACINGY = 33;

// Halfway across the 320 unit wide screen the intermission is laid out on.
constexpr int WI_CENTERX = 160;

// Single Player
constexpr int SP_STATSX = 50;
constexpr int SP_STATSY = 50;
constexpr int SP_TIMEX = 16;
constexpr int SP_TIMEY = 168;

// NET GAME STUFF
constexpr int NG_STATSY = 50;

#define NG_STATSX				(32 + pStar->width()/2 + 32*!dofrags)

constexpr int NG_SPACINGX = 64;

//
// GENERAL DATA
//

//
// Locally used stuff.
//

// in seconds
#define SHOWNEXTLOCDELAY		4

namespace
{

// used to accelerate or skip a stage
bool				acceleratestage;

// wbs->pnum
unsigned			me;

 // specifies current state
stateenum_t		state;

// contains information passed into intermission
wbstartstruct_t* wbs;

std::vector<wbplayerstruct_t> plrs;	// = wbs->plyr
std::vector<int> cnt_kills_c;	// = cnt_kills
std::vector<int> cnt_items_c;	// = cnt_items
std::vector<int> cnt_secret_c;	// = cnt_secret
std::vector<int> cnt_frags_c;	// = cnt_frags
std::array<lumpHandle_t, 4>	faceclassic;
int dofrags;
int ng_state;

// used for general timing
int				cnt;

// used for timing of background animation
int				bcnt;

// Since classic is used for singleplayer only...
int			cnt_kills;
int			cnt_items;
int			cnt_secret;
int			cnt_time;
int			cnt_par;
int			cnt_pause;

int			inter_width;
int			inter_height;


//
//		GRAPHICS
//

// %, : graphics
lumpHandle_t		percent;
lumpHandle_t		colon;

// 0-9 graphic
constexpr size_t WI_NUMDIGITS = 10;
std::array<lumpHandle_t, WI_NUMDIGITS>	num;

// minus sign
lumpHandle_t		wiminus;

// "Finished!" graphics
lumpHandle_t		finished; //(Removed) Dan - Causes GUI Issues |FIX-ME|
// [Nes] Re-added for singleplayer

// "Entering" graphic
lumpHandle_t		entering;

 // "Kills", "Items", "Secrets"
lumpHandle_t		kills;
lumpHandle_t		secret;
lumpHandle_t		items;
lumpHandle_t		frags;
lumpHandle_t		scrt;

// Time sucks.
lumpHandle_t		timepatch;
lumpHandle_t		par;
lumpHandle_t		sucks;

// "Total", your face, your dead face
lumpHandle_t		total;
lumpHandle_t		star;
lumpHandle_t		bstar;

lumpHandle_t		p; // [RH] Only one

 // Name graphics of each level (centered)
std::array<lumpHandle_t, 2>	lnames;

// [RH] Info to dynamically generate the level name graphics
std::array<int, 2>			lnamewidths;
std::array<const char*, 2>	lnametexts;

// Map authors, empty when there is none or the title patch already shows it
std::array<std::string, 2>	lnameauthors;

IWindowSurface*	background_surface;

IWindowSurface*	anim_surface;

interlevel_t* enteranim;
interlevel_t* exitanim;

} // namespace

EXTERN_CVAR (sv_maxplayers)
EXTERN_CVAR (wi_oldintermission)
EXTERN_CVAR (cl_autoscreenshot)

//
// ID24 STUFF - largely based on the implementation from Woof, with some bits from Rum and Raisin
//

struct wi_animationstate_t
{
	std::vector<interlevelframe_t> frames;
	const int xpos;
	const int ypos;
	int frame_index;
	bool frame_start;
	int duration_left;

	wi_animationstate_t(std::vector<interlevelframe_t> f = {}, int x = 0, int y = 0, int fi = 0, bool fs = false, int dl = 0) :
		frames(f), xpos(x), ypos(y), frame_index(fi), frame_start(fs), duration_left(dl) {}
};

struct wi_animation_t
{
	std::vector<wi_animationstate_t> exiting_states;
	std::vector<wi_animationstate_t> entering_states;

	std::vector<wi_animationstate_t>* states;
};

static wi_animation_t animation;

//
// CODE
//

static bool WI_checkConditions(const std::vector<interlevelcond_t>& conditions,
							   bool enteringcondition)
{
	bool conditionsmet = true;

	LevelInfos& levels = getLevelInfos();
	const level_pwad_info_t& exitinglevel = levels.findByName(wbs->current);
	const level_pwad_info_t& enteringlevel = levels.findByName(wbs->next);
	const level_pwad_info_t& currentlevel = enteringcondition ? enteringlevel : exitinglevel;
	const int mapnum = currentlevel.mapnum;
	const OLumpName& mapname = currentlevel.mapname;

	for (const auto& cond : conditions)
	{
		switch (cond.condition)
		{
			case animcondition_t::CurrMapGreater:
				conditionsmet = conditionsmet && (mapnum > cond.param);
				break;

			case animcondition_t::CurrMapEqual:
				if (cond.isZDoom)
					conditionsmet = conditionsmet && (mapname == cond.mapname1);
				else
					conditionsmet = conditionsmet && (mapnum == cond.param);
				break;

			case animcondition_t::CurrMapNotEqual:
				if (cond.isZDoom)
					conditionsmet = conditionsmet && (mapname != cond.mapname1);
				else
					conditionsmet = conditionsmet && (mapnum != cond.param);
				break;

			case animcondition_t::MapVisited:
				if (cond.isZDoom)
					conditionsmet = conditionsmet && levels.findByName(cond.mapname1).flags & LEVEL_VISITED;
				else
				{
					bool res = false;
					for (size_t i = 0; i < levels.size(); i++)
					{
						const level_pwad_info_t& level = levels.at(i);
						if ((level.mapnum == cond.param) && (level.flags & LEVEL_VISITED))
						{
							res = true;
							break;
						}
					}
					conditionsmet = conditionsmet && res;
				}
				break;

			case animcondition_t::MapNotVisited:
				if (cond.isZDoom)
					conditionsmet = conditionsmet && !(levels.findByName(cond.mapname1).flags & LEVEL_VISITED);
				else
				{
					bool res = true;
					for (size_t i = 0; i < levels.size(); i++)
					{
						const level_pwad_info_t& level = levels.at(i);
						if ((level.mapnum == cond.param) && (level.flags & LEVEL_VISITED))
						{
							res = false;
							break;
						}
					}
					conditionsmet = conditionsmet && res;
				}
				break;

			case animcondition_t::CurrMapNotSecret:
				conditionsmet = conditionsmet && !(currentlevel.flags & LEVEL_SECRET);
				break;

			case animcondition_t::AnySecretVisited:
				conditionsmet = conditionsmet && wbs->didsecret;
				break;

			case animcondition_t::OnFinishedScreen:
				conditionsmet = conditionsmet && !enteringcondition;
				break;

			case animcondition_t::OnEnteringScreen:
				conditionsmet = conditionsmet && enteringcondition;
				break;

			case animcondition_t::TravelingBetween:
				conditionsmet = conditionsmet && (exitinglevel.mapname == cond.mapname1) && (enteringlevel.mapname == cond.mapname2);
				break;

			case animcondition_t::NotTravelingBetween:
				conditionsmet = conditionsmet && !((exitinglevel.mapname == cond.mapname1) && (enteringlevel.mapname == cond.mapname2));
				break;

			default:
				break;
		}
	}
	return conditionsmet;
}

static void WI_updateAnimationStates(std::vector<wi_animationstate_t>& states)
{
	for (auto& state : states)
	{
		interlevelframe_t& frame = state.frames.at(state.frame_index);

		if (state.duration_left == 0)
		{
			int tics = 1;
			switch (frame.type & 0xF)
			{
				case interlevelframe_t::DurationInf:
					continue;
				case interlevelframe_t::DurationFixed:
					if (state.frame_start && (frame.type & interlevelframe_t::RandomStart))
					{
						int maxtics = frame.duration;
						tics = M_Random() % maxtics;
						break;
					}
					tics = frame.duration;
					break;

				case interlevelframe_t::DurationRand:
					{
						int maxtics = frame.maxduration;
						int mintics = frame.duration;
						tics = M_Random() % maxtics;
						tics = std::clamp(tics, mintics, maxtics);
					}
					break;

				default:
					break;
			}

			state.duration_left = std::max(tics, 1);

			if (!state.frame_start)
			{
				state.frame_index++;
				if (state.frame_index == state.frames.size())
				{
					state.frame_index = 0;
				}
			}
		}

		state.duration_left--;
		state.frame_start = false;
	}
}

static void WI_updateAnimation(bool enteringcondition)
{
	animation.states = nullptr;

	if (!enteringcondition && exitanim)
	{
		animation.states = &animation.exiting_states;
	}
	else if (enteringcondition && enteranim)
	{
		animation.states = &animation.entering_states;
	}

	if (!animation.states)
		return;

	WI_updateAnimationStates(*animation.states);
}

static void WI_drawAnimation(void)
{
	if (!animation.states)
	{
		return;
	}

	int scaled_x = (inter_width - 320) / 2;
	DCanvas* canvas = anim_surface->getDefaultCanvas();
	for (const auto& state : *animation.states)
	{
		const interlevelframe_t& frame = state.frames.at(state.frame_index);
		patch_t* patch = W_CachePatch(frame.imagelumpnum);
		if (!frame.altimagelump.empty())
		{
			int left = state.xpos - patch->leftoffset();
			int top = state.ypos - patch->topoffset();
			int right = left + patch->width();
			int bottom = top + patch->height();

			if (!(left >= 0 && right < 320 && top >= 0 && bottom < 200))
			{
				patch = W_CachePatch(frame.altimagelumpnum);
			}
		}

		canvas->DrawPatch(patch, state.xpos + scaled_x, state.ypos);
	}
}

static void WI_initAnimationStates(std::vector<wi_animationstate_t>& out,
								   const std::vector<interlevellayer_t>& layers,
								   bool enteringcondition)
{
	for (const auto& layer : layers)
	{
		if (!WI_checkConditions(layer.conditions, enteringcondition))
		{
			continue;
		}

		for (const auto& anim : layer.anims)
		{
			if (!WI_checkConditions(anim.conditions, enteringcondition))
			{
				continue;
			}

			out.emplace_back(anim.frames, anim.xpos, anim.ypos, 0, true, 0);
		}
	}
}

static void WI_initAnimation(void)
{
	if (exitanim)
	{
		WI_initAnimationStates(animation.exiting_states, exitanim->layers, false);
	}

	if (enteranim)
	{
		WI_initAnimationStates(animation.entering_states, enteranim->layers, true);
	}

	return;
}

//
// WI_GetWidth
//
// Returns the width of the area that the intermission screen will be
// drawn to. The intermisison screen should be 4:3, except in 320x200 mode.
//
static int WI_GetWidth()
{
	const int surface_width = I_GetPrimarySurface()->getWidth();
	const int surface_height = I_GetPrimarySurface()->getHeight();

	// Using widescreen assets? It may go off screen.
	// Preserve the aspect ratio and make the box big
	// Maybe too big? (it will be cropped if so)
	if (inter_width > 320)
	{
		return I_GetAspectCorrectWidth(surface_height, inter_height, inter_width);
	}

	if (I_IsProtectedResolution(I_GetVideoWidth(), I_GetVideoHeight()))
		return surface_width;

	if (surface_width * 3 >= surface_height * 4)
		return surface_height * 4 / 3;
	else
		return surface_width;
}


//
// WI_GetHeight
//
// Returns the height of the area that the intermission screen will be
// drawn to. The intermisison screen should be 4:3, except in 320x200 mode.
//
static int WI_GetHeight()
{
	const int surface_width = I_GetPrimarySurface()->getWidth();
	const int surface_height = I_GetPrimarySurface()->getHeight();

	if (I_IsProtectedResolution(I_GetVideoWidth(), I_GetVideoHeight()))
		return surface_height;

	if (surface_width * 3 >= surface_height * 4)
		return surface_height;
	else
		return surface_width * 3 / 4;
}

// slam background
void WI_slamBackground()
{
	IWindowSurface* primary_surface = I_GetPrimarySurface();
	const int destw = WI_GetWidth(), desth = WI_GetHeight();
	primary_surface->clear();		// ensure black background in matted modes
	anim_surface->clear();

	background_surface->lock();
	anim_surface->lock();

	anim_surface->blitcrop(background_surface, 0, 0, background_surface->getWidth(), background_surface->getHeight(),
						   0, 0, anim_surface->getWidth(), anim_surface->getHeight());

	WI_drawAnimation();

	primary_surface->blitcrop(anim_surface, 0, 0, anim_surface->getWidth(), anim_surface->getHeight(),
							  (primary_surface->getWidth() - destw) / 2, (primary_surface->getHeight() - desth) / 2,
							  destw, desth);

	background_surface->unlock();
	anim_surface->unlock();
}
namespace
{

int WI_BigNameHeight()
{
	const patch_t* p = W_CachePatch("FONTB39");
	return p->height() - p->topoffset();
}

int WI_DrawName (const char *str, int x, int y)
{
	patch_t *p = NULL;

	::V_ColorMap = translationref_t(::Ranges + CR_GREY * 256);
	while (*str)
	{
		// Recolor on a color escape code instead of drawing it.
		if (str[0] == TEXTCOLOR_ESCAPE && str[1] != '\0')
		{
			int new_color = V_GetTextColor(str);
			if (new_color == -1)
				new_color = CR_GREY;

			::V_ColorMap =
			    translationref_t(::Ranges + (static_cast<ptrdiff_t>(new_color) * 256));
			str += 2;
			continue;
		}

		int lump = W_CheckNumForName(fmt::format("FONTB{:02d}", toupper(*str) - 32));

		if (lump != -1)
		{
			p = W_CachePatch (lump);
			screen->DrawTranslatedPatchClean(p, x, y);
			x += p->width() - 1;
		}
		else
		{
			x += 12;
		}
		str++;
	}

	const int height = WI_BigNameHeight();
	return height + (height / 4);
}

constexpr int WI_SMALLNAMEBLANK = 4;

OLumpName WI_SmallNameChar(char c)
{
	return fmt::format("STCFN{:03d}", HU_FONTSTART + (toupper(c) - 32) - 1);
}

int WI_SmallNameHeight()
{
	const patch_t* p = W_CachePatch(WI_SmallNameChar('M'));
	return p->height() - p->topoffset();
}

int WI_DrawSmallName(const char* str, int x, int y)
{
	patch_t* p = NULL;

	while (*str)
	{
		// This font is drawn untranslated, so color codes are only skipped.
		if (str[0] == TEXTCOLOR_ESCAPE && str[1] != '\0')
		{
			str += 2;
			continue;
		}

		const int lump = W_CheckNumForName(WI_SmallNameChar(*str));

		if (lump != -1)
		{
			p = W_CachePatch(lump);
			screen->DrawPatchClean(p, x, y);
			x += p->width() - 1;
		}
		else
		{
			x += WI_SMALLNAMEBLANK;
		}
		str++;
	}

	const int height = WI_SmallNameHeight();
	return height + (height / 4);
}

// Width of a string drawn by WI_DrawSmallName.
int WI_CalcSmallWidth(const char* str)
{
	int w = 0;

	while (*str)
	{
		// Color escape codes take up no space.
		if (str[0] == TEXTCOLOR_ESCAPE && str[1] != '\0')
		{
			str += 2;
			continue;
		}

		const int lump = W_CheckNumForName(WI_SmallNameChar(*str));

		if (lump != -1)
			w += W_CachePatch(lump)->width() - 1;
		else
			w += WI_SMALLNAMEBLANK;

		str++;
	}

	return w;
}

// Draws the author centered under a level name of the given height, and returns
// how far down the drawing position moves as a result.
//
// The level name already left a quarter of its height as a gap, so the author
// is drawn straight into it and the same gap is left below.
int WI_DrawAuthorName(const char* author, int y, int nameheight)
{
	WI_DrawSmallName(author, WI_CENTERX - (WI_CalcSmallWidth(author) / 2), y);

	return WI_SmallNameHeight() + (nameheight / 4);
}

} // namespace

//Draws "<Levelname> Finished!"
void WI_drawLF()
{
	if (lnames[0].empty() && !lnamewidths[0])
		return;

	int y = WI_TITLEY;
	int nameheight;

	if (!lnames[0].empty())
	{
		// draw <LevelName>
		patch_t* lnames0 = W_ResolvePatchHandle(lnames[0]);
		screen->DrawPatchClean(lnames0, (320 - lnames0->width()) / 2, y);
		nameheight = lnames0->height();
		y += nameheight + (nameheight / 4);
	}
	else
	{
		// [RH] draw a dynamic title string
		nameheight = WI_BigNameHeight();
		y += WI_DrawName (lnametexts[0], WI_CENTERX - (lnamewidths[0] / 2), y);
	}

	// draw the author underneath, if the map names one
	if (!lnameauthors[0].empty())
		y += WI_DrawAuthorName(lnameauthors[0].c_str(), y, nameheight);

	// draw "Finished!"
	//if (!multiplayer || sv_maxplayers <= 1)
	patch_t* fin = W_ResolvePatchHandle(finished);

	// (Removed) Dan - Causes GUI Issues |FIX-ME|
	screen->DrawPatchClean(fin, (320 - fin->width()) / 2, y);
}



// Draws "Entering <LevelName>"
void WI_drawEL()
{
	if (lnames[1].empty() && !lnamewidths[1])
		return;

	int y = WI_TITLEY;

	patch_t* ent = W_ResolvePatchHandle(entering);
	patch_t* lnames1 = W_ResolvePatchHandle(lnames[1]);

	// draw "Entering"
	screen->DrawPatchClean(ent, (320 - ent->width()) / 2, y);

	// [RH] Changed to adjust by height of entering patch instead of title
	if (lnames1->height() < 200)
		y += ent->height() + (ent->height() / 4);

	int nameheight;

	if (!lnames[1].empty())
	{
		// draw level
		screen->DrawPatchClean(lnames1, (320 - lnames1->width()) / 2, y);
		nameheight = lnames1->height();
		y += nameheight + (nameheight / 4);
	}
	else
	{
		// [RH] draw a dynamic title string
		nameheight = WI_BigNameHeight();
		y += WI_DrawName (lnametexts[1], WI_CENTERX - (lnamewidths[1] / 2), y);
	}

	// draw the author underneath, if the map names one
	if (!lnameauthors[1].empty())
		WI_DrawAuthorName(lnameauthors[1].c_str(), y, nameheight);
}

void WI_drawAnimatedBack()
{
	WI_slamBackground();
}

int WI_drawNum(int n, int x, int y, int digits)
{
	if (digits < 0)
	{
		if (n == 0)
		{
			// make variable-length zeros 1 digit long
			digits = 1;
		}
		else
		{
			// figure out # of digits in #
			digits = 0;
			int temp = n;

			while (temp)
			{
				temp /= 10;
				digits++;
			}
		}
	}

	const bool neg = n < 0;
	if (neg)
		n = -n;

	// if non-number, do not draw it
	if (n == 1994)
		return 0;

	const int fontwidth = W_ResolvePatchHandle(num[0])->width();

	// draw the new number
	while (digits--)
	{
		x -= fontwidth;
		screen->DrawPatchClean(W_ResolvePatchHandle(num[n % 10]), x, y);
		n /= 10;
	}

	// draw a minus sign if necessary
	if (neg)
		screen->DrawPatchClean(W_ResolvePatchHandle(wiminus), x -= 8, y);

	return x;
}

void WI_drawPercent (int p, int x, int y, int b = 0)
{
	if (p < 0)
		return;

	screen->DrawPatchClean(W_ResolvePatchHandle(percent), x, y);
	if (b == 0)
		WI_drawNum(p, x, y, -1);
	else
		WI_drawNum(p * 100 / b, x, y, -1);
}

void WI_drawTime (int t, int x, int y)
{
	if (t < 0)
		return;

	if (t <= 61 * 59)
	{
		int div = 1;

		patch_t* col = W_ResolvePatchHandle(colon);

		do
		{
			const int n = (t / div) % 60;
			x = WI_drawNum(n, x, y, 2) - col->width();
			div *= 60;

			// draw
			if (div==60 || t / div)
				screen->DrawPatchClean(col, x, y);

		} while (t / div);
	}
	else
	{
		patch_t* suk = W_ResolvePatchHandle(sucks);

		// "sucks"
		screen->DrawPatchClean(suk, x - suk->width(), y);
	}
}

void WI_End()
{
	WI_unloadData();

	I_FreeSurface(background_surface);

	I_FreeSurface(anim_surface);
}

void WI_initNoState()
{
	state = NoState;
	acceleratestage = 0;
	cnt = 10;
}

void WI_updateNoState()
{
	// denis - let the server decide when to load the next map
	if (serverside)
	{
		if (!--cnt)
		{
			WI_End();
			G_WorldDone();
		}
	}

	WI_updateAnimation(state != StatCount);
}

void WI_initShowNextLoc()
{
	state = ShowNextLoc;
	acceleratestage = 0;
	cnt = SHOWNEXTLOCDELAY * TICRATE;
}

void WI_updateShowNextLoc()
{
	if(serverside)
	{
		if (!--cnt || acceleratestage)
			WI_initNoState();
	}
	WI_updateAnimation(state != StatCount);
}

void WI_drawShowNextLoc()
{
	// draw animated background
	WI_drawAnimatedBack();
	// draws which level you are entering..
	WI_drawEL();
}

void WI_drawNoState()
{
	WI_drawShowNextLoc();
}

int WI_fragSum (const player_t &player)
{
	return player.fragcount;
}

void WI_drawDeathmatchStats()
{
	// draw animated background
	WI_drawAnimatedBack();
	WI_drawLF();

	// [RH] Draw heads-up scores display
	HU_DrawScores(&idplayer(me));
}

void WI_initNetgameStats()
{
	state = StatCount;
	acceleratestage = 0;
	ng_state = 1;

	cnt_pause = TICRATE;

	cnt_kills_c.clear();
	cnt_items_c.clear();
	cnt_secret_c.clear();
	cnt_frags_c.clear();

	for (const auto& player : players)
	{
		if (!(player.ingame()))
			continue;

		cnt_kills_c.push_back(0);
		cnt_items_c.push_back(0);
		cnt_secret_c.push_back(0);
		cnt_frags_c.push_back(0);

		dofrags += WI_fragSum(player);
	}

	dofrags = !!dofrags;
}

void WI_updateNetgameStats()
{
	unsigned int i;
	bool stillticking;

	if (acceleratestage && ng_state != 10)
	{
		acceleratestage = 0;

		i = 0;
		for (auto it = players.begin();it != players.end();++it,++i)
		{
			if (!(it->ingame()))
				continue;

			cnt_kills_c[i] = plrs[i].skills;
			cnt_items_c[i] = plrs[i].sitems;
			cnt_secret_c[i] = plrs[i].ssecret;

			if (dofrags)
				cnt_frags_c[i] = WI_fragSum(*it);
		}
		S_Sound (CHAN_INTERFACE, "intermission/nextstage", 1, ATTN_NONE);
		ng_state = 10;
	}
	if (ng_state == 2)
	{
		if (!(bcnt&3))
			S_Sound (CHAN_INTERFACE, "intermission/tick", 1, ATTN_NONE);

		stillticking = false;

		i = 0;
		for (auto it = players.begin();it != players.end();++it,++i)
		{
			if (!(it->ingame()))
				continue;

			cnt_kills_c[i] += 2;

			if (cnt_kills_c[i] > plrs[i].skills)
				cnt_kills_c[i] = plrs[i].skills;
			else
				stillticking = true;
		}

		if (!stillticking)
		{
			S_Sound (CHAN_INTERFACE, "intermission/nextstage", 1, ATTN_NONE);
			ng_state++;
		}
	}
	else if (ng_state == 4)
	{
		if (!(bcnt&3))
			S_Sound (CHAN_INTERFACE, "intermission/tick", 1, ATTN_NONE);

		stillticking = false;

		i = 0;
		for (Players::iterator it = players.begin();it != players.end();++it,++i)
		{
			if (!(it->ingame()))
				continue;

			cnt_items_c[i] += 2;
			if (cnt_items_c[i] > plrs[i].sitems)
				cnt_items_c[i] = plrs[i].sitems;
			else
				stillticking = true;
		}
		if (!stillticking)
		{
			S_Sound (CHAN_INTERFACE, "intermission/nextstage", 1, ATTN_NONE);
			ng_state++;
		}
	}
	else if (ng_state == 6)
	{
		if (!(bcnt&3))
			S_Sound (CHAN_INTERFACE, "intermission/tick", 1, ATTN_NONE);

		stillticking = false;

		i = 0;
		for (Players::iterator it = players.begin();it != players.end();++it,++i)
		{
			if (!(it->ingame()))
				continue;

			cnt_secret_c[i] += 2;

			if (cnt_secret_c[i] > plrs[i].ssecret)
				cnt_secret_c[i] = plrs[i].ssecret;
			else
				stillticking = true;
		}

		if (!stillticking)
		{
			S_Sound (CHAN_INTERFACE, "intermission/nextstage", 1, ATTN_NONE);
			ng_state += 1 + 2*!dofrags;
		}
	}
	else if (ng_state == 8)
	{
		int fsum;
		if (!(bcnt&3))
			S_Sound (CHAN_INTERFACE, "intermission/tick", 1, ATTN_NONE);

		stillticking = false;

		i = 0;
		for (Players::iterator it = players.begin();it != players.end();++it,++i)
		{
			if (!(it->ingame()))
				continue;

			cnt_frags_c[i] += 1;

			if (cnt_frags_c[i] >= (fsum = WI_fragSum(*it)))
				cnt_frags_c[i] = fsum;
			else
				stillticking = true;
		}

		if (!stillticking)
		{
			S_Sound (CHAN_INTERFACE, "intermission/cooptotal", 1, ATTN_NONE);
			ng_state++;
		}
	}
	else if (ng_state == 10)
	{
		if (acceleratestage)
		{
			if (dofrags)
				S_Sound (CHAN_INTERFACE, "intermission/pastdmstats", 1, ATTN_NONE);
			else
				S_Sound (CHAN_INTERFACE, "intermission/pastcoopstats", 1, ATTN_NONE);
			if ((gameinfo.flags & GI_MAPxx) && (enteranim == nullptr || demoplayback))
				WI_initNoState();
			else
				WI_initShowNextLoc();
		}
	}
	else if (ng_state & 1)
	{
		if (!--cnt_pause)
		{
			ng_state++;
			cnt_pause = TICRATE;
		}
	}

	WI_updateAnimation(state != StatCount);
}

void WI_drawNetgameStats()
{
	unsigned int nbPlayers = 0;

	const patch_t* pPercent = W_ResolvePatchHandle(::percent);
	const patch_t* pKills = W_ResolvePatchHandle(::kills);
	const patch_t* pItems = W_ResolvePatchHandle(::items);
	const patch_t* pScrt = W_ResolvePatchHandle(::scrt);
	const patch_t* pFrags = W_ResolvePatchHandle(::frags);
	const patch_t* pStar = W_ResolvePatchHandle(::star);
	const patch_t* pP = W_ResolvePatchHandle(::p);

	const short pwidth = pPercent->width();

	// draw animated background
	WI_drawAnimatedBack();

	WI_drawLF();

	// draw stat titles (top line)
	screen->DrawPatchClean(pKills, NG_STATSX + NG_SPACINGX - pKills->width(), NG_STATSY);
	screen->DrawPatchClean(pItems, NG_STATSX + 2 * NG_SPACINGX - pItems->width(),
	                       NG_STATSY);
	screen->DrawPatchClean(pScrt, NG_STATSX + 3 * NG_SPACINGX - pScrt->width(),
	                       NG_STATSY);

	if (::dofrags)
	{
		screen->DrawPatchClean(pFrags, NG_STATSX + 4 * NG_SPACINGX - pFrags->width(),
		                       NG_STATSY);
	}

	// draw stats
	unsigned int y = NG_STATSY + pKills->height();

	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		// Make sure while demoplaybacking that we're not exceeding the hardlimit of 4 players.
		if (demoplayback && it->id > 4)
			break;

		// Break it anyway if we count more than 4 ACTIVE players in our session.
		if (!demoplayback && nbPlayers > 4)
			break;

		const byte i = (it->id) - 1;

		if (!it->ingame())
			continue;

		unsigned int x = NG_STATSX;
		// [RH] Only use one graphic for the face backgrounds
		//enaiel: Fix incorrect player background when showing old intermission
		V_ColorMap = translationref_t(translationtables + it->id * 256, it->id);

		screen->DrawTranslatedPatchClean(pP, x - pP->width(), y);
		// classic face background colour
		//screen->DrawTranslatedPatchClean (faceclassic[i], x-p->width(), y);

		//enaiel: Draw displayplayer face instead of consoleplayer in vanilla oldintermission screen
		if (i == (displayplayer_id -1))
			screen->DrawPatchClean(pStar, x - pP->width(), y);

		// Display player names online!
		if (!demoplayback)
		{
			WI_DrawSmallName(it->userinfo.netname.c_str(), x+10, y+24);
		}

		x += NG_SPACINGX;

		WI_drawPercent (cnt_kills_c[i], x-pwidth, y+10, wbs->maxkills);	x += NG_SPACINGX;
		WI_drawPercent (cnt_items_c[i], x-pwidth, y+10, wbs->maxitems);	x += NG_SPACINGX;
		WI_drawPercent (cnt_secret_c[i], x-pwidth, y+10, wbs->maxsecret); x += NG_SPACINGX;

		if (dofrags)
			WI_drawNum(cnt_frags_c[i], x, y+10, -1);

		y += WI_SPACINGY+4;
		nbPlayers++;
	}
}

static int sp_state;

void WI_initStats()
{
    state = StatCount;
    acceleratestage = 0;
    sp_state = 1;
    cnt_kills = cnt_items = cnt_secret = -1;
    cnt_time = cnt_par = -1;
    cnt_pause = TICRATE;
}

static int StatPercent(int statValue, int statValueMax)
{
	if (statValueMax > 0)
	{
		return (statValue * 100) / statValueMax;
	}
	return std::max(1, statValue) * 100;    // Report the case where value == max == 0 as a full success.
}

void WI_updateStats()
{
	const int finalKillPercent   = StatPercent(level.killed_monsters, wminfo.maxkills);
	const int finalItemPercent   = StatPercent(level.found_items,     wminfo.maxitems);
	const int finalSecretPercent = StatPercent(level.found_secrets,   wminfo.maxsecret);

	if (acceleratestage && sp_state != 10)
	{
		acceleratestage = 0;

		cnt_kills  = finalKillPercent;
		cnt_items  = finalItemPercent;
		cnt_secret = finalSecretPercent;
		cnt_time   = (plrs[me].stime) ? plrs[me].stime / TICRATE : level.time / TICRATE;
		cnt_par    = wminfo.partime / TICRATE;

		S_Sound (CHAN_INTERFACE, "intermission/nextstage", 1, ATTN_NONE);
		sp_state = 10;
	}
	if (sp_state == 2)
	{
		cnt_kills += 2;

		if (!(bcnt&3))
			S_Sound (CHAN_INTERFACE, "intermission/tick", 1, ATTN_NONE);

		if (!gameinfo.intermissionCounter || cnt_kills >= finalKillPercent)
		{
			cnt_kills = finalKillPercent;
			S_Sound (CHAN_INTERFACE, "intermission/nextstage", 1, ATTN_NONE);
			sp_state++;
		}
	}
	else if (sp_state == 4)
	{
		cnt_items += 2;

		if (!(bcnt&3))
			S_Sound (CHAN_INTERFACE, "intermission/tick", 1, ATTN_NONE);

		if (!gameinfo.intermissionCounter || cnt_items >= finalItemPercent)
		{
			cnt_items = finalItemPercent;
			S_Sound (CHAN_INTERFACE, "intermission/nextstage", 1, ATTN_NONE);
			sp_state++;
		}
	}
	else if (sp_state == 6)
	{
		cnt_secret += 2;

		if (!(bcnt&3))
			S_Sound (CHAN_INTERFACE, "intermission/tick", 1, ATTN_NONE);

		if (!gameinfo.intermissionCounter || cnt_secret >= finalSecretPercent)
		{
			cnt_secret = finalSecretPercent;
			S_Sound (CHAN_INTERFACE, "intermission/nextstage", 1, ATTN_NONE);
			sp_state++;
		}
	}
	else if (sp_state == 8)
	{
		if (!(bcnt&3))
			S_Sound (CHAN_INTERFACE, "intermission/tick", 1, ATTN_NONE);

		cnt_time += 3;

		if (cnt_time >= plrs[me].stime / TICRATE)
			cnt_time = plrs[me].stime / TICRATE;

		cnt_par += 3;

		if (!gameinfo.intermissionCounter || cnt_par >= wminfo.partime / TICRATE)
		{
			cnt_par = wminfo.partime / TICRATE;

			if (cnt_time >= plrs[me].stime / TICRATE)
			{
			S_Sound (CHAN_INTERFACE, "intermission/nextstage", 1, ATTN_NONE);
			sp_state++;
			}
		}
	}
	else if (sp_state == 10)
	{
		if (acceleratestage)
		{
			level_pwad_info_t& nextlevel = getLevelInfos().findByName(wbs->next);
			OLumpName enterpic = nextlevel.enterpic;

			if (!enterpic.empty() || enteranim != nullptr)
			{
				if (enteranim != nullptr && !enteranim->musiclump.empty())
					S_ChangeMusic(enteranim->musiclump.c_str(), true);
				else if (!nextlevel.zintermusic.empty())
					S_ChangeMusic(nextlevel.zintermusic.c_str(), true);
				else
					S_ChangeMusic(gameinfo.intermissionMusic.c_str(), true);
				// background
				const OLumpName& bg_lump = enteranim == nullptr ? enterpic : enteranim->backgroundlump;
				const patch_t* bg_patch = W_CachePatch(W_CheckWidescreenPatch(bg_lump));

				inter_width = bg_patch->width();
				inter_height = bg_patch->height() + (bg_patch->height() / 5);

				background_surface =
				    I_AllocateSurface(bg_patch->width(), bg_patch->height(), 8);
				anim_surface =
				    I_AllocateSurface(bg_patch->width(), bg_patch->height(), 8);
				const DCanvas* canvas = background_surface->getDefaultCanvas();

				background_surface->lock();
				canvas->DrawPatch(bg_patch, 0, 0);
				background_surface->unlock();
			}

			S_Sound (CHAN_INTERFACE, "intermission/paststats", 1, ATTN_NONE);

			if (gameinfo.flags & GI_MAPxx && (enteranim == nullptr || demoplayback))
				WI_initNoState();
			else
				WI_initShowNextLoc();
		}
	}
	else if (sp_state & 1)
	{
		if (!--cnt_pause)
		{
			sp_state++;
			cnt_pause = TICRATE;
		}
	}

	WI_updateAnimation(state != StatCount);
}

void WI_drawStats()
{
	const patch_t* pKills = W_ResolvePatchHandle(::kills);
	const patch_t* pItems = W_ResolvePatchHandle(::items);
	const patch_t* pSecret = W_ResolvePatchHandle(::secret);
	const patch_t* pTimepatch = W_ResolvePatchHandle(::timepatch);
	const patch_t* pPar = W_ResolvePatchHandle(::par);

	// line height
	const int lh = (3 * W_ResolvePatchHandle(::num[0])->height()) / 2;

	// draw animated background
	WI_drawAnimatedBack();
	WI_drawLF();

	screen->DrawPatchClean(pKills, SP_STATSX, SP_STATSY);
	WI_drawPercent(cnt_kills, 320 - SP_STATSX, SP_STATSY);

	screen->DrawPatchClean(pItems, SP_STATSX, SP_STATSY + lh);
	WI_drawPercent(cnt_items, 320 - SP_STATSX, SP_STATSY + lh);

	screen->DrawPatchClean(pSecret, SP_STATSX, SP_STATSY + 2 * lh);
	WI_drawPercent(cnt_secret, 320 - SP_STATSX, SP_STATSY + 2 * lh);

	screen->DrawPatchClean(pTimepatch, SP_TIMEX, SP_TIMEY);
	WI_drawTime(cnt_time, WI_CENTERX - SP_TIMEX, SP_TIMEY);

	if ((gameinfo.flags & GI_MAPxx) || wbs->epsd < 3)
	{
		screen->DrawPatchClean(pPar, SP_TIMEX + WI_CENTERX, SP_TIMEY);
		WI_drawTime(cnt_par, 320 - SP_TIMEX, SP_TIMEY);
	}
}

void WI_checkForAccelerate()
{
	if (!serverside)
		return;

	// check for button presses to skip delays
	for (auto& player : players)
	{
		if (player.ingame())
		{
			if (player.cmd.buttons & BT_ATTACK)
			{
				if (!player.attackdown)
					acceleratestage = 1;
				player.attackdown = true;
			}
			else
				player.attackdown = false;
			if (player.cmd.buttons & BT_USE)
			{
				if (!player.usedown)
					acceleratestage = 1;
				player.usedown = true;
			}
			else
				player.usedown = false;
		}
	}
}



// Updates stuff each tick
void WI_Ticker()
{
	// counter for general background animation
	bcnt++;

	if (bcnt == 1)
	{
		level_pwad_info_t& currentlevel = getLevelInfos().findByName(wbs->current);

		// intermission music
		if (exitanim != nullptr && !exitanim->musiclump.empty())
			S_ChangeMusic (exitanim->musiclump.c_str(), true);
		else if (W_CheckNumForName(wbs->winner ? "D_OWIN" : "D_OLOSE") != -1)
			S_ChangeMusic (wbs->winner ? "D_OWIN" : "D_OLOSE", true);
		else if (W_CheckNumForName(wbs->winner ? "D_STWIN" : "D_STLOSE") != -1)
			S_ChangeMusic (wbs->winner ? "D_STWIN" : "D_STLOSE", true);
		else if (!currentlevel.zintermusic.empty())
			S_ChangeMusic (currentlevel.zintermusic.c_str(), true);
		else
			S_ChangeMusic (gameinfo.intermissionMusic.c_str(), true);
	}

    WI_checkForAccelerate();

	switch (state)
	{
		case StatCount:
			if (multiplayer)
			{
			    if (demoplayback)
			    {
				    WI_updateNetgameStats();
				}
			    else
			    {
				    if (sv_gametype == 0 && wi_oldintermission && P_NumPlayersInGame() < 5)
					    WI_updateNetgameStats();
				    else
					    WI_updateNoState();
				}
			}
			else
				WI_updateStats();
			break;

		case ShowNextLoc:
			WI_updateShowNextLoc();
			break;

		case NoState:
			WI_updateNoState();
			break;
	}

	// [ML] If cl_autoscreenshot is on, take a screenshot 3 seconds
	//		after the level end. (Multiplayer only)
	if (cl_autoscreenshot && multiplayer && bcnt == (3 * TICRATE))
	{
		AddCommandString("screenshot");
	}
}

static int WI_CalcWidth (const char *str)
{
	if (!str)
		return 0;

	int w = 0;

	while (*str)
	{
		// Color escape codes take up no space.
		if (str[0] == TEXTCOLOR_ESCAPE && str[1] != '\0')
		{
			str += 2;
			continue;
		}

		const OLumpName charname = fmt::format("FONTB{:02d}", toupper(*str) - 32);
		int lump = W_CheckNumForName(charname);

		if (lump != -1)
		{
			const patch_t* p = W_CachePatch(lump);
			w += p->width() - 1;
		} else {
			w += 12;
		}
		str++;
	}

	return w;
}

void WI_loadData()
{
	exitanim = enteranim = nullptr;
	LevelInfos& levels = getLevelInfos();
	level_pwad_info_t& currentlevel = levels.findByName(wbs->current);
	level_pwad_info_t& nextlevel = levels.findByName(wbs->next);

	OLumpName winanim;
	OLumpName winpic;
	if (W_CheckNumForName(wbs->winner ? "WINANIM" : "LOSEANIM") != -1)
		winanim = wbs->winner ? "WINANIM" : "LOSEANIM";
	else if (W_CheckNumForName(wbs->winner ? "WINERPIC" : "LOSERPIC") != -1)
		winpic = wbs->winner ? "WINERPIC" : "LOSERPIC";

	animation = wi_animation_t();

	if (!winanim.empty())
		exitanim = WI_GetInterlevel(winanim);
	else if (!currentlevel.exitanim.empty())
		exitanim = WI_GetInterlevel(currentlevel.exitanim);
	else if (!currentlevel.exitscript.empty())
		exitanim = WI_GetIntermissionScript(currentlevel.exitscript);

	if (!nextlevel.enteranim.empty())
		enteranim = WI_GetInterlevel(nextlevel.enteranim);
	else if (!nextlevel.enterscript.empty())
		enteranim = WI_GetIntermissionScript(nextlevel.enterscript);

	WI_initAnimation();

	OLumpName name;

	if (exitanim != nullptr)
		name = exitanim->backgroundlump;
	else if (!winpic.empty())
		name = winpic;
	else if (!currentlevel.exitpic.empty())
		name = currentlevel.exitpic;
	else
		name = "INTERPIC";

	// background
	const patch_t* bg_patch = W_CachePatch(W_CheckWidescreenPatch(name));

	inter_width = bg_patch->width();
	inter_height = bg_patch->height() + (bg_patch->height() / 5);

	background_surface = I_AllocateSurface(bg_patch->width(), bg_patch->height(), 8);
	anim_surface = I_AllocateSurface(bg_patch->width(), bg_patch->height(), 8);
	const DCanvas* canvas = background_surface->getDefaultCanvas();

	background_surface->lock();
	canvas->DrawPatch(bg_patch, bg_patch->leftoffset(), bg_patch->topoffset());
	background_surface->unlock();

	for (int i = 0, j; i < 2; i++)
	{
		const level_pwad_info_t& linfo =
		    levels.findByName(i == 0 ? wbs->current : wbs->next);
		const OLumpName& lname = (i == 0 ? wbs->lname0 : wbs->lname1);

		if (!lname.empty())
			j = W_CheckNumForName (lname);
		else
			j = -1;

		if (j >= 0)
		{
			lnames[i] = W_CachePatchHandle(j, PU_STATIC);
		}
		else
		{
			lnames[i].clear();
			lnametexts[i] = linfo.level_name.c_str();
			lnamewidths[i] = WI_CalcWidth (lnametexts[i]);
		}

		// Determine if we should display the map author.
		// MAPINFO can just straight up ask for it to be disabled.
		const bool patchshowsauthor =
		    !lnames[i].empty() && (linfo.flags2 & LEVEL2_HIDEAUTHORNAME);

		// A UMAPINFO map that brings its own title graphic has drawn that graphic
		// to suit itself, so an author line under it is not wanted.
		const bool umapinfoshowsauthor = (linfo.flags2 & LEVEL2_FROMUMAPINFO) &&
		                                    !linfo.pname.empty() &&
		                                    W_IsLumpFromPWAD(linfo.pname);

		lnameauthors[i] = (patchshowsauthor || umapinfoshowsauthor) ? "" : linfo.author;
	}

	for (int i = 0; i < 10; i++)
	{
		// numbers 0-9
		name = fmt::format("WINUM{}", i);
		num[i] = W_CachePatchHandle(name.c_str(), PU_STATIC);
	}

	wiminus = W_CachePatchHandle("WIMINUS", PU_STATIC);

	// percent sign
	percent = W_CachePatchHandle("WIPCNT", PU_STATIC);

	// ":"
	colon = W_CachePatchHandle("WICOLON", PU_STATIC);

	// "finished"
	// (Removed) Dan - Causes GUI Issues |FIX-ME|
	finished = W_CachePatchHandle("WIF", PU_STATIC);

	// "entering"
	entering = W_CachePatchHandle("WIENTER", PU_STATIC);

	// "kills"
	kills = W_CachePatchHandle("WIOSTK", PU_STATIC);

	// "items"
	items = W_CachePatchHandle("WIOSTI", PU_STATIC);

	// "scrt"
	scrt = W_CachePatchHandle("WIOSTS", PU_STATIC);

	// "secret"
	secret = W_CachePatchHandle("WISCRT2", PU_STATIC);

	// "frgs"
	frags = W_CachePatchHandle("WIFRGS", PU_STATIC);

	// "time"
	timepatch = W_CachePatchHandle("WITIME", PU_STATIC);

	// "sucks"
	sucks = W_CachePatchHandle("WISUCKS", PU_STATIC);

	// "par"
	par = W_CachePatchHandle("WIPAR", PU_STATIC);

	// "total"
	total = W_CachePatchHandle("WIMSTT", PU_STATIC);

	// your face
	star = W_CachePatchHandle("STFST01", PU_STATIC);

	// dead face
	bstar = W_CachePatchHandle("STFDEAD0", PU_STATIC);

	p = W_CachePatchHandle("STPBANY", PU_STATIC);

	if (exitanim != nullptr)
	{
		for (const auto& layer : exitanim->layers)
		{
			for (const auto& anim : layer.anims)
			{
				for (const auto& frame : anim.frames)
				{
					W_CachePatch(frame.imagelumpnum);
				}
			}
		}
	}
	if (enteranim != nullptr)
	{
		for (const auto& layer : enteranim->layers)
		{
			for (const auto& anim : layer.anims)
			{
				for (const auto& frame : anim.frames)
				{
					W_CachePatch(frame.imagelumpnum);
				}
			}
		}
	}

	// [Nes] Classic vanilla lifebars.
	for (int i = 0; i < 4; i++)
	{
		name = fmt::format("STPB{}", i);
		faceclassic[i] = W_CachePatchHandle(name, PU_STATIC);
	}
}

void WI_unloadData()
{
	for (int i = 0; i < 10; i++)
		num[i].clear();

	wiminus.clear();
	percent.clear();
	colon.clear();
	kills.clear();
	secret.clear();
	frags.clear();
	items.clear();
	finished.clear();
	entering.clear();
	timepatch.clear();
	sucks.clear();
	par.clear();
	total.clear();
	p.clear();

	for (int i = 0; i < 4; i++)
		faceclassic[i].clear();
}

void WI_Drawer()
{
	C_MidPrint(NULL);	// Don't midprint anything during intermission

	// If the background screen has been freed, then we really shouldn't
	// be in here. (But it happens anyway.)
	if (background_surface && anim_surface)
	{
		switch (state)
		{
		case StatCount:
			if (multiplayer)
			{
				if (demoplayback)
				{
					WI_drawNetgameStats();
				}
				else
				{
					if (sv_gametype == 0 && wi_oldintermission && P_NumPlayersInGame() < 5)
						WI_drawNetgameStats();
					else
						WI_drawDeathmatchStats();
				}
			}
			else
				WI_drawStats();
			break;

		case ShowNextLoc:
			WI_drawShowNextLoc();
			break;

		default:
			WI_drawNoState();
			break;
		}
	}
}


void WI_initVariables (wbstartstruct_t *wbstartstruct)
{
	wbs = wbstartstruct;

	acceleratestage = 0;
	cnt = bcnt = 0;
	me = wbs->pnum;
	plrs = wbs->plyr;
}

void WI_Start (wbstartstruct_t *wbstartstruct)
{
	WI_initVariables (wbstartstruct);
	WI_loadData ();
	WI_initStats();
	WI_initNetgameStats();

	S_StopAllChannels ();
	SN_StopAllSequences ();
}

void WI_Shutdown()
{
	WI_ClearInterlevels();
}

VERSION_CONTROL (wi_stuff_cpp, "$Id$")
