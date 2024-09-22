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
//		Intermission screens.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <ctype.h>

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
#define WI_TITLEY				2
#define WI_SPACINGY 			33

// Single Player
#define SP_STATSX		50
#define SP_STATSY		50
#define SP_TIMEX		16
#define SP_TIMEY		168

// NET GAME STUFF
#define NG_STATSY				50
#define NG_STATSX				(32 + pStar->width()/2 + 32*!dofrags)

#define NG_SPACINGX 			64

//
// GENERAL DATA
//

//
// Locally used stuff.
//

// in seconds
#define SHOWNEXTLOCDELAY		4

// used to accelerate or skip a stage
static bool				acceleratestage;

// wbs->pnum
static unsigned			me;

 // specifies current state
static stateenum_t		state;

// contains information passed into intermission
static wbstartstruct_t* wbs;

static std::vector<wbplayerstruct_t> plrs;	// = wbs->plyr
static std::vector<int> cnt_kills_c;	// = cnt_kills
static std::vector<int> cnt_items_c;	// = cnt_items
static std::vector<int> cnt_secret_c;	// = cnt_secret
static std::vector<int> cnt_frags_c;	// = cnt_frags
static lumpHandle_t		faceclassic[4];
static int dofrags;
static int ng_state;

// used for general timing
static int				cnt;

// used for timing of background animation
static int				bcnt;

// Since classic is used for singleplayer only...
static int			cnt_kills;
static int			cnt_items;
static int			cnt_secret;
static int			cnt_time;
static int			cnt_par;
static int			cnt_pause;

static int			inter_width;
static int			inter_height;


//
//		GRAPHICS
//

// %, : graphics
static lumpHandle_t		percent;
static lumpHandle_t		colon;

// 0-9 graphic
static lumpHandle_t		num[10];

// minus sign
static lumpHandle_t		wiminus;

// "Finished!" graphics
static lumpHandle_t		finished; //(Removed) Dan - Causes GUI Issues |FIX-ME|
// [Nes] Re-added for singleplayer

// "Entering" graphic
static lumpHandle_t		entering;

 // "Kills", "Items", "Secrets"
static lumpHandle_t		kills;
static lumpHandle_t		secret;
static lumpHandle_t		items;
static lumpHandle_t		frags;
static lumpHandle_t		scrt;

// Time sucks.
static lumpHandle_t		timepatch;
static lumpHandle_t		par;
static lumpHandle_t		sucks;

// "Total", your face, your dead face
static lumpHandle_t		total;
static lumpHandle_t		star;
static lumpHandle_t		bstar;

static lumpHandle_t		p; // [RH] Only one

 // Name graphics of each level (centered)
static lumpHandle_t		lnames[2];

// [RH] Info to dynamically generate the level name graphics
static int				lnamewidths[2];
static const char*		lnametexts[2];

static IWindowSurface*	background_surface;

static IWindowSurface*	anim_surface;

static interlevel_t* enteranim;
static interlevel_t* exitanim;

EXTERN_CVAR (sv_maxplayers)
EXTERN_CVAR (wi_oldintermission)
EXTERN_CVAR (cl_autoscreenshot)

//
// ID24 STUFF - largely based on the implementation from Woof, with some bits from Rum and Raisin
//

typedef struct
{
	std::vector<interlevelframe_t> frames;
	const int xpos;
	const int ypos;
	int frame_index;
	bool frame_start;
	int duration_left;
} wi_animationstate_t;

typedef struct
{
	std::vector<wi_animationstate_t> exiting_states;
	std::vector<wi_animationstate_t> entering_states;

	std::vector<wi_animationstate_t>* states;
} wi_animation_t;

static wi_animation_t* animation;

//
// CODE
//

static bool WI_checkConditions(const std::vector<interlevelcond_t>& conditions,
							   bool enteringcondition)
{
	bool conditionsmet = true;

	LevelInfos& levels = getLevelInfos();
	level_pwad_info_t& currentlevel = enteringcondition ? levels.findByName(wbs->next) : levels.findByName(wbs->current);
	int map_number = currentlevel.levelnum;

	for (const auto& cond : conditions)
	{
		switch (cond.condition)
		{
			case animcondition_t::CurrMapGreater:
				conditionsmet = conditionsmet && (map_number > cond.param);
				break;

			case animcondition_t::CurrMapEqual:
				conditionsmet = conditionsmet && (map_number == cond.param);
				break;

			case animcondition_t::MapVisited:
				conditionsmet = conditionsmet && levels.findByNum(cond.param).flags & LEVEL_VISITED;
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
						tics = clamp(tics, mintics, maxtics);
					}
					break;

				default:
					break;
			}

			state.duration_left = MAX(tics, 1);

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
	if (!animation)
	{
		return;
	}

	animation->states = nullptr;

	if (!enteringcondition && exitanim)
	{
		animation->states = &animation->exiting_states;
	}
	else if (enteranim)
	{
		animation->states = &animation->entering_states;
	}

	if (!animation->states)
		return;

	WI_updateAnimationStates(*animation->states);
}

static void WI_drawAnimation(void)
{
	if (!animation || !animation->states)
	{
		return;
	}

	int scaled_x = (inter_width - 320) / 2;
	DCanvas* canvas = anim_surface->getDefaultCanvas();
	for (const auto& state : *animation->states)
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

			wi_animationstate_t state = { anim.frames, anim.xpos, anim.ypos, 0, true, 0 };
			out.push_back(state);
		}
	}
}

static void WI_initAnimation(void)
{
	if (!animation)
	{
		return;
	}

	if (exitanim)
	{
		WI_initAnimationStates(animation->exiting_states, exitanim->layers, false);
	}

	if (enteranim)
	{
		WI_initAnimationStates(animation->entering_states, enteranim->layers, true);
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

static int WI_DrawName (const char *str, int x, int y)
{
	patch_t *p = NULL;

	::V_ColorMap = translationref_t(::Ranges + CR_GREY * 256);
	while (*str)
	{
		char charname[9];
		sprintf (charname, "FONTB%02u", toupper(*str) - 32);
		int lump = W_CheckNumForName(charname);

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

	p = W_CachePatch ("FONTB39");
	return (5*(p->height()-p->topoffset()))/4;
}

static int WI_DrawSmallName(const char* str, int x, int y)
{
	patch_t* p = NULL;

	while (*str)
	{
		char charname[9];
		sprintf(charname, "STCFN%.3d", HU_FONTSTART + (toupper(*str) - 32) - 1);
		int lump = W_CheckNumForName(charname);

		if (lump != -1)
		{
			p = W_CachePatch(lump);
			screen->DrawPatchClean(p, x, y);
			x += p->width() - 1;
		}
		else
		{
			x += 12;
		}
		str++;
	}

	p = W_CachePatch("FONTB39");
	return (5 * (p->height() - p->topoffset())) / 4;
}

//Draws "<Levelname> Finished!"
void WI_drawLF()
{
	if (lnames[0].empty() && !lnamewidths[0])
		return;

	int y = WI_TITLEY;

	if (!lnames[0].empty())
	{
		// draw <LevelName>
		patch_t* lnames0 = W_ResolvePatchHandle(lnames[0]);
		screen->DrawPatchClean(lnames0, (320 - lnames0->width()) / 2, y);
		y += (5 * lnames0->height()) / 4;
	}
	else
	{
		// [RH] draw a dynamic title string
		y += WI_DrawName (lnametexts[0], 160 - lnamewidths[0] / 2, y);
	}

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
	y += (5 * ent->height()) / 4;

	if (!lnames[1].empty())
	{
		// draw level
		screen->DrawPatchClean(lnames1, (320 - lnames1->width()) / 2, y);
	}
	else
	{
		// [RH] draw a dynamic title string
		WI_DrawName (lnametexts[1], 160 - lnamewidths[1] / 2, y);
	}
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

int WI_fragSum (player_t &player)
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

	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		if (!(it->ingame()))
			continue;

		cnt_kills_c.push_back(0);
		cnt_items_c.push_back(0);
		cnt_secret_c.push_back(0);
		cnt_frags_c.push_back(0);

		dofrags += WI_fragSum(*it);
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
		for (Players::iterator it = players.begin();it != players.end();++it,++i)
		{
			if (!(it->ingame()))
				continue;

			cnt_kills_c[i] = plrs[i].skills;
			cnt_items_c[i] = plrs[i].sitems;
			cnt_secret_c[i] = plrs[i].ssecret;

			if (dofrags)
				cnt_frags_c[i] = WI_fragSum(*it);
		}
		S_Sound (CHAN_INTERFACE, "weapons/rocklx", 1, ATTN_NONE);
		ng_state = 10;
	}
	if (ng_state == 2)
	{
		if (!(bcnt&3))
			S_Sound (CHAN_INTERFACE, "weapons/pistol", 1, ATTN_NONE);

		stillticking = false;

		i = 0;
		for (Players::iterator it = players.begin();it != players.end();++it,++i)
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
			S_Sound (CHAN_INTERFACE, "weapons/rocklx", 1, ATTN_NONE);
			ng_state++;
		}
	}
	else if (ng_state == 4)
	{
		if (!(bcnt&3))
			S_Sound (CHAN_INTERFACE, "weapons/pistol", 1, ATTN_NONE);

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
			S_Sound (CHAN_INTERFACE, "weapons/rocklx", 1, ATTN_NONE);
			ng_state++;
		}
	}
	else if (ng_state == 6)
	{
		if (!(bcnt&3))
			S_Sound (CHAN_INTERFACE, "weapons/pistol", 1, ATTN_NONE);

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
			S_Sound (CHAN_INTERFACE, "weapons/rocklx", 1, ATTN_NONE);
			ng_state += 1 + 2*!dofrags;
		}
	}
	else if (ng_state == 8)
	{
		int fsum;
		if (!(bcnt&3))
			S_Sound (CHAN_INTERFACE, "weapons/pistol", 1, ATTN_NONE);

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
			S_Sound (CHAN_INTERFACE, "player/male/death1", 1, ATTN_NONE);
			ng_state++;
		}
	}
	else if (ng_state == 10)
	{
		if (acceleratestage)
		{
			S_Sound (CHAN_INTERFACE, "weapons/shotgr", 1, ATTN_NONE);
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
			std::string str;
			StrFormat(str, "%s", it->userinfo.netname.c_str());
			WI_DrawSmallName(str.c_str(), x+10, y+24);
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

void WI_updateStats()
{
    if (acceleratestage && sp_state != 10)
    {
		acceleratestage = 0;
		cnt_kills = (wminfo.maxkills) ? (level.killed_monsters * 100) / wminfo.maxkills : 0;
		cnt_items = (wminfo.maxitems) ? (level.found_items * 100) / wminfo.maxitems : 0;
		cnt_secret = (wminfo.maxsecret) ? (level.found_secrets * 100) / wminfo.maxsecret : 0;
		cnt_time = (plrs[me].stime) ? plrs[me].stime / TICRATE : level.time / TICRATE;
		cnt_par = wminfo.partime / TICRATE;
		S_Sound (CHAN_INTERFACE, "world/barrelx", 1, ATTN_NONE);
		sp_state = 10;
    }
    if (sp_state == 2)
    {
		cnt_kills += 2;

		if (!(bcnt&3))
		    S_Sound (CHAN_INTERFACE, "weapons/pistol", 1, ATTN_NONE);

		if (!wminfo.maxkills || cnt_kills >= (level.killed_monsters * 100) / wminfo.maxkills)
		{
		    cnt_kills = (wminfo.maxkills) ? (level.killed_monsters * 100) / wminfo.maxkills : 0;
		    S_Sound (CHAN_INTERFACE, "world/barrelx", 1, ATTN_NONE);
		    sp_state++;
		}
    }
    else if (sp_state == 4)
    {
		cnt_items += 2;

		if (!(bcnt&3))
		    S_Sound (CHAN_INTERFACE, "weapons/pistol", 1, ATTN_NONE);

		if (!wminfo.maxitems || cnt_items >= (level.found_items * 100) / wminfo.maxitems)
		{
		    cnt_items = (wminfo.maxitems) ? (level.found_items * 100) / wminfo.maxitems : 0;
		    S_Sound (CHAN_INTERFACE, "world/barrelx", 1, ATTN_NONE);
		    sp_state++;
		}
    }
    else if (sp_state == 6)
    {
		cnt_secret += 2;

		if (!(bcnt&3))
		    S_Sound (CHAN_INTERFACE, "weapons/pistol", 1, ATTN_NONE);

		if (!wminfo.maxsecret || cnt_secret >= (level.found_secrets * 100) / wminfo.maxsecret)
		{
		    cnt_secret = (wminfo.maxsecret) ? (level.found_secrets * 100) / wminfo.maxsecret : 0;
		    S_Sound (CHAN_INTERFACE, "world/barrelx", 1, ATTN_NONE);
		    sp_state++;
		}
    }
    else if (sp_state == 8)
    {
		if (!(bcnt&3))
		    S_Sound (CHAN_INTERFACE, "weapons/pistol", 1, ATTN_NONE);

		cnt_time += 3;

		if (cnt_time >= plrs[me].stime / TICRATE)
		    cnt_time = plrs[me].stime / TICRATE;

		cnt_par += 3;

		if (cnt_par >= wminfo.partime / TICRATE)
		{
		    cnt_par = wminfo.partime / TICRATE;

		    if (cnt_time >= plrs[me].stime / TICRATE)
		    {
			S_Sound (CHAN_INTERFACE, "world/barrelx", 1, ATTN_NONE);
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
				if (enteranim != nullptr)
					S_ChangeMusic(enteranim->musiclump.c_str(), true);
				// background
				const char* bg_lump = enteranim == nullptr ? enterpic.c_str() : enteranim->backgroundlump.c_str();
				const patch_t* bg_patch = W_CachePatch(bg_lump);

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

		    S_Sound (CHAN_INTERFACE, "weapons/shotgr", 1, ATTN_NONE);

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
	WI_drawTime(cnt_time, 160 - SP_TIMEX, SP_TIMEY);

	if ((gameinfo.flags & GI_MAPxx) || wbs->epsd < 3)
	{
		screen->DrawPatchClean(pPar, SP_TIMEX + 160, SP_TIMEY);
		WI_drawTime(cnt_par, 320 - SP_TIMEX, SP_TIMEY);
	}
}

void WI_checkForAccelerate()
{
	if (!serverside)
		return;

	// check for button presses to skip delays
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		if (it->ingame())
		{
			player_t *player = &*it;

			if (player->cmd.buttons & BT_ATTACK)
			{
				if (!player->attackdown)
					acceleratestage = 1;
				player->attackdown = true;
			}
			else
				player->attackdown = false;
			if (player->cmd.buttons & BT_USE)
			{
				if (!player->usedown)
					acceleratestage = 1;
				player->usedown = true;
			}
			else
				player->usedown = false;
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
		// intermission music
		if (exitanim != nullptr)
			S_ChangeMusic (exitanim->musiclump.c_str(), true);
		else if ((gameinfo.flags & GI_MAPxx))
			S_ChangeMusic ("d_dm2int", true);
		else
			S_ChangeMusic ("d_inter", true);
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
		char charname[9];
		sprintf (charname, "FONTB%02u", toupper(*str) - 32);
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
	LevelInfos& levels = getLevelInfos();
	level_pwad_info_t& currentlevel = levels.findByName(wbs->current);
	level_pwad_info_t& nextlevel = levels.findByName(wbs->next);

	animation = new wi_animation_t();

	if (!currentlevel.exitanim.empty())
	{
		exitanim = WI_GetInterlevel(currentlevel.exitanim.c_str());
	}
	if (!nextlevel.enteranim.empty())
	{
		enteranim = WI_GetInterlevel(nextlevel.enteranim.c_str());
	}
	WI_initAnimation();

	char name[17];

	if (exitanim != nullptr)
		strcpy(name, exitanim->backgroundlump.c_str());
	else if (currentlevel.exitpic[0] != '\0')
		strcpy(name, currentlevel.exitpic.c_str());
	else if ((gameinfo.flags & GI_MAPxx) || ((gameinfo.flags & GI_MENUHACK_RETAIL) && wbs->epsd >= 3))
		strcpy(name, "INTERPIC");
	else
		sprintf(name, "WIMAP%d", wbs->epsd);

	// background
	const patch_t* bg_patch = W_CachePatch(name);

	inter_width = bg_patch->width();
	inter_height = bg_patch->height() + (bg_patch->height() / 5);

	background_surface = I_AllocateSurface(bg_patch->width(), bg_patch->height(), 8);
	anim_surface = I_AllocateSurface(bg_patch->width(), bg_patch->height(), 8);
	const DCanvas* canvas = background_surface->getDefaultCanvas();

	background_surface->lock();
	canvas->DrawPatch(bg_patch, 0, 0);
	background_surface->unlock();

	for (int i = 0, j; i < 2; i++)
	{
		char *lname = (i == 0 ? wbs->lname0 : wbs->lname1);

		if (lname)
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
			lnametexts[i] = levels.findByName(i == 0 ? wbs->current : wbs->next).level_name.c_str();
			lnamewidths[i] = WI_CalcWidth (lnametexts[i]);
		}
	}

	for (int i = 0; i < 10; i++)
	{
		// numbers 0-9
		sprintf(name, "WINUM%d", i);
			num[i] = W_CachePatchHandle(name, PU_STATIC);
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
		sprintf(name, "STPB%d", i);
		faceclassic[i] = W_CachePatchHandle(name, PU_STATIC);
	}
}

void WI_unloadData()
{
	exitanim = enteranim = nullptr;
	delete animation;

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
		faceclassic[i ].clear();
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
