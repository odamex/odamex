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

#include <algorithm>

#include <ctype.h>
#include <array>

#include "z_zone.h"
#include "m_random.h"
#include "i_video.h"
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
#include "resources/res_texture.h"
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

#define NG_STATSX				(32 + star->mWidth / 2 + 32*!dofrags)

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
std::array<const Texture*, 4>	faceclassic;
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
const Texture*		percent;
const Texture*		colon;

// 0-9 graphic
constexpr size_t WI_NUMDIGITS = 10;
std::array<const Texture*, WI_NUMDIGITS>	num;

// minus sign
const Texture*		wiminus;

// "Finished!" graphics
const Texture*		finished; //(Removed) Dan - Causes GUI Issues |FIX-ME|
// [Nes] Re-added for singleplayer

// "Entering" graphic
const Texture*		entering;

 // "Kills", "Items", "Secrets"
const Texture*		kills;
const Texture*		secret;
const Texture*		items;
const Texture*		frags;
const Texture*		scrt;

// Time sucks.
const Texture*		timepatch;
const Texture*		par;
const Texture*		sucks;

// "Total", your face, your dead face
const Texture*		total;
const Texture*		star;
const Texture*		bstar;

const Texture*		p; // [RH] Only one

 // Name graphics of each level (centered)
std::array<const Texture*, 2>	lnames;

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
		const Texture*  patch = Res_CacheTexture(frame.imageresourceid);
		if (!frame.altimagelump.empty())
		{
			int left = state.xpos - patch->leftoffset();
			int top = state.ypos - patch->topoffset();
			int right = left + patch->width();
			int bottom = top + patch->height();

			if (!(left >= 0 && right < 320 && top >= 0 && bottom < 200))
			{
				patch = Res_CacheTexture(frame.altimageresourceid);
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
	const Texture* p = W_CachePatch("FONTB39");
	return p->height() - p->topoffset();
}

int WI_DrawName (const char *str, int x, int y)
{
	char charname[9];

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

		const ResourceId lump = Res_GetTextureResourceId(
		    OStringToUpper(fmt::format("FONTB{:02d}", toupper(*str) - 32)), GRAPHICS, false);

		if (Res_CheckResource(lump))
		{
			const Texture* texture = Res_CacheTexture(lump, PU_CACHE);
			screen->DrawTextureClean(texture, x, y);
			x += texture->mWidth - 1;
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
	const Texture* p = W_CachePatch(WI_SmallNameChar('M'));
	return p->height() - p->topoffset();
}

int WI_DrawSmallName(const char* str, int x, int y)
{
	while (*str)
	{
		// This font is drawn untranslated, so color codes are only skipped.
		if (str[0] == TEXTCOLOR_ESCAPE && str[1] != '\0')
		{
			str += 2;
			continue;
		}

		const ResourceId lump =
		    Res_GetTextureResourceId(OStringToUpper(WI_SmallNameChar(*str).c_str()),
		                             GRAPHICS, false);

		if (Res_CheckResource(lump))
		{
			const Texture* texture = Res_CacheTexture(lump, PU_CACHE);
			screen->DrawTextureClean(texture, x, y);
			x += texture->mWidth - 1;
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

		const ResourceId lump =
		    Res_GetTextureResourceId(OStringToUpper(WI_SmallNameChar(*str).c_str()),
		                             GRAPHICS, false);

		if (Res_CheckResource(lump))
			w += Res_CacheTexture(lump, PU_CACHE)->width() - 1;
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
	if (!lnames[0] && !lnamewidths[0])
		return;

	int y = WI_TITLEY;
	int nameheight;

	if (lnames[0])
	{
		// draw <LevelName>
		screen->DrawTextureClean(lnames[0], (320 - lnames[0]->mWidth) / 2, y);
		nameheight = lnames[0]->mHeight;
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
		screen->DrawTextureClean(finished, (320 - finished->mWidth)/2, y);  // (Removed) Dan - Causes GUI Issues |FIX-ME|
}



// Draws "Entering <LevelName>"
void WI_drawEL()
{
	if (!lnames[1] && !lnamewidths[1])
		return;

	int y = WI_TITLEY;

	// draw "Entering"
	screen->DrawTextureClean(entering, (320 - entering->mWidth)/2, y);

	// [RH] Changed to adjust by height of entering patch instead of title
	if (entering->mHeight < 200)
		y += entering->mHeight + (entering->mHeight / 4);

	int nameheight;

	if (lnames[1])
	{
		// draw level
		screen->DrawTextureClean(lnames[1], (320 - lnames[1]->mWidth) / 2, y);
		nameheight = lnames[1]->mHeight;
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
    int		fontwidth = num[0]->mWidth;
    int		temp;

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

	// draw the new number
	while (digits--)
	{
		x -= fontwidth;
		screen->DrawTextureClean(num[ n % 10 ], x, y);
		n /= 10;
	}

	// draw a minus sign if necessary
	if (neg)
		screen->DrawTextureClean(wiminus, x -= 8, y);

	return x;
}

#include "hu_stuff.h"

void WI_drawPercent (int p, int x, int y, int b = 0)
{
	if (p < 0)
		return;

	screen->DrawTextureClean(percent, x, y);
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

	do
	{
	    const int n = (t / div) % 60;
	    x = WI_drawNum(n, x, y, 2) - colon->mWidth;
	    div *= 60;

	    // draw
	    if (div==60 || t / div)
		screen->DrawTextureClean(colon, x, y);

		} while (t / div);
	}
	else
	{
		const Texture*  suk = W_ResolvePatchHandle(sucks);

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

	unsigned int x, y;
	const short pwidth = percent->mWidth;

	// draw animated background
	WI_drawAnimatedBack();

	WI_drawLF();

	// draw stat titles (top line)
	screen->DrawTextureClean(::kills, NG_STATSX+NG_SPACINGX-::kills->mWidth, NG_STATSY);

	screen->DrawTextureClean(::items, NG_STATSX+2*NG_SPACINGX-::items->mWidth, NG_STATSY);

	screen->DrawTextureClean(::scrt, NG_STATSX+3*NG_SPACINGX-::scrt->mWidth, NG_STATSY);

	if (::dofrags)
	{
		screen->DrawTextureClean(::frags, NG_STATSX + 4 * NG_SPACINGX - ::frags->mWidth, NG_STATSY);
	}

	// draw stats
	y = NG_STATSY + ::kills->mHeight;

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

		screen->DrawTranslatedTextureClean(::p, x - ::p->mWidth, y);
		// classic face background colour
		//screen->DrawTranslatedPatchClean (faceclassic[i], x-p->width(), y);

		//enaiel: Draw displayplayer face instead of consoleplayer in vanilla oldintermission screen
		if (i == (displayplayer_id -1))
			screen->DrawTextureClean(::star, x-p->mWidth, y);

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
				const Texture* bg_patch = W_CachePatch(W_CheckWidescreenPatch(bg_lump));

				inter_width = bg_patch->width();
				inter_height = bg_patch->height() + (bg_patch->height() / 5);

				background_surface =
				    I_AllocateSurface(bg_patch->width(), bg_patch->height(), 8);
				anim_surface =
				    I_AllocateSurface(bg_patch->width(), bg_patch->height(), 8);
				const DCanvas* canvas = background_surface->getDefaultCanvas();

				background_surface->lock();
				canvas->DrawTexture(bg_patch, 0, 0);
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
	// line height
	const int lh = (3 * ::num[0]->mHeight) / 2;

	// draw animated background
	WI_drawAnimatedBack();
	WI_drawLF();

    screen->DrawTextureClean(kills, SP_STATSX, SP_STATSY);
    WI_drawPercent(cnt_kills, 320 - SP_STATSX, SP_STATSY);

    screen->DrawTextureClean(items, SP_STATSX, SP_STATSY+lh);
    WI_drawPercent(cnt_items, 320 - SP_STATSX, SP_STATSY+lh);

    screen->DrawTextureClean(secret, SP_STATSX, SP_STATSY+2*lh);
    WI_drawPercent(cnt_secret, 320 - SP_STATSX, SP_STATSY+2*lh);

	screen->DrawTextureClean(timepatch, SP_TIMEX, SP_TIMEY);
	WI_drawTime(cnt_time, WI_CENTERX - SP_TIMEX, SP_TIMEY);

	if ((gameinfo.flags & GI_MAPxx) || wbs->epsd < 3)
	{
		screen->DrawTextureClean(par, SP_TIMEX + WI_CENTERX, SP_TIMEY);
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
		else if (Res_CheckResource(Res_GetResourceId(wbs->winner ? "D_OWIN" : "D_OLOSE", NS_GLOBAL)))
			S_ChangeMusic (wbs->winner ? "D_OWIN" : "D_OLOSE", true);
		else if (Res_CheckResource(Res_GetResourceId(wbs->winner ? "D_STWIN" : "D_STLOSE", NS_GLOBAL)))
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

static int WI_CalcWidth(const char *str)
{
	int w = 0;
	char lump_name[9];

	if (!str)
		return 0;

	while (*str)
	{
		// Color escape codes take up no space.
		if (str[0] == TEXTCOLOR_ESCAPE && str[1] != '\0')
		{
			str += 2;
			continue;
		}

		const OLumpName charname = fmt::format("FONTB{:02d}", toupper(*str) - 32);
		const ResourceId lump = Res_GetTextureResourceId(OStringToUpper(charname.c_str()), GRAPHICS, false);

		if (Res_CheckResource(lump))
		{
			const Texture* p = W_CachePatch(lump);
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
	if (Res_CheckResource(Res_GetResourceId(wbs->winner ? "WINANIM" : "LOSEANIM", NS_GLOBAL)))
		winanim = wbs->winner ? "WINANIM" : "LOSEANIM";
	else if (Res_CheckResource(Res_GetTextureResourceId(wbs->winner ? "WINERPIC" : "LOSERPIC", GRAPHICS, false)))
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
	const Texture* bg_patch = W_CachePatch(W_CheckWidescreenPatch(name));

	inter_width = bg_patch->width();
	inter_height = bg_patch->height() + (bg_patch->height() / 5);

	background_surface = I_AllocateSurface(bg_patch->width(), bg_patch->height(), 8);
	anim_surface = I_AllocateSurface(bg_patch->width(), bg_patch->height(), 8);
	const DCanvas* canvas = background_surface->getDefaultCanvas();

	background_surface->lock();
	canvas->DrawPatch(bg_patch, bg_patch->leftoffset(), bg_patch->topoffset());
	background_surface->unlock();

	for (int i = 0; i < 2; i++)
	{
		const level_pwad_info_t& linfo =
		    levels.findByName(i == 0 ? wbs->current : wbs->next);
		const OLumpName& lname = (i == 0 ? wbs->lname0 : wbs->lname1);

		ResourceId lname_res_id = ResourceId::INVALID_ID;
		if (!lname.empty())
			lname_res_id = Res_GetTextureResourceId(OStringToUpper(lname.c_str()), GRAPHICS, false);

		if (Res_CheckResource(lname_res_id))
		{
			lnames[i] = Res_CacheTexture(lname_res_id, PU_STATIC);
		}
		else
		{
			lnames[i] = NULL;
			lnametexts[i] = linfo.level_name.c_str();
			lnamewidths[i] = WI_CalcWidth (lnametexts[i]);
		}

		// Determine if we should display the map author.
		// MAPINFO can just straight up ask for it to be disabled.
		const bool patchshowsauthor =
		    lnames[i] != NULL && (linfo.flags2 & LEVEL2_HIDEAUTHORNAME);

		// A UMAPINFO map that brings its own title graphic has drawn that graphic
		// to suit itself, so an author line under it is not wanted.
		const bool umapinfoshowsauthor = (linfo.metadataflags & metadataflags_t::AUTHORFROMUMAPINFO) &&
		                                    !linfo.pname.empty() &&
		                                    Res_IsResourceFromPWAD(Res_GetResourceId(
		                                        linfo.pname.c_str(), NS_GLOBAL));

		lnameauthors[i] = (patchshowsauthor || umapinfoshowsauthor) ? "" : linfo.author;
	}

	for (int i = 0; i < 10; i++)
	{
		// numbers 0-9
		name = fmt::format("WINUM{}", i);
		num[i] = W_CachePatch(name.c_str(), PU_STATIC);
	}

    wiminus = Res_CacheTexture("WIMINUS", PATCH, PU_STATIC);

	// percent sign
    percent = Res_CacheTexture("WIPCNT", PATCH, PU_STATIC);

	// ":"
    colon = Res_CacheTexture("WICOLON", PATCH, PU_STATIC);

	// "finished"
	// (Removed) Dan - Causes GUI Issues |FIX-ME|
	finished = W_CachePatch("WIF", PU_STATIC);

	// "entering"
	entering = Res_CacheTexture("WIENTER", PATCH, PU_STATIC);

	// "kills"
    kills = Res_CacheTexture("WIOSTK", PATCH, PU_STATIC);

	// "items"
    items = Res_CacheTexture("WIOSTI", PATCH, PU_STATIC);

    // "scrt"
    scrt = Res_CacheTexture("WIOSTS", PATCH, PU_STATIC);

	// "secret"
    secret = Res_CacheTexture("WISCRT2", PATCH, PU_STATIC);

	// "frgs"
	frags = Res_CacheTexture("WIFRGS", PATCH, PU_STATIC);

	// "time"
    timepatch = Res_CacheTexture("WITIME", PATCH, PU_STATIC);

    // "sucks"
    sucks = Res_CacheTexture("WISUCKS", PATCH, PU_STATIC);

    // "par"
    par = Res_CacheTexture("WIPAR", PATCH, PU_STATIC);

	// "total"
	total = Res_CacheTexture("WIMSTT", PATCH, PU_STATIC);

	// your face
	star = Res_CacheTexture("STFST01", PATCH, PU_STATIC);

	// dead face
	bstar = Res_CacheTexture("STFDEAD0", PATCH, PU_STATIC);

	p = Res_CacheTexture("STPBANY", GRAPHICS, PU_STATIC);

	if (exitanim != nullptr)
	{
		for (const auto& layer : exitanim->layers)
		{
			for (const auto& anim : layer.anims)
			{
				for (const auto& frame : anim.frames)
				{
					Res_CacheTexture(frame.imageresourceid);
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
					Res_CacheTexture(frame.imageresourceid);
				}
			}
		}
	}

	// [Nes] Classic vanilla lifebars.
	for (int i = 0; i < 4; i++)
	{
		name = fmt::format("STPB{}", i);
		faceclassic[i] = W_CachePatch(name, PU_STATIC);
	}
}

void WI_unloadData()
{
	for (int i = 0; i < 10; i++)
		num[i] = NULL;

	wiminus = NULL;
	percent = NULL;
	colon = NULL;
	kills = NULL;
	secret = NULL;
	frags = NULL;
	items = NULL;
	finished = NULL;
	entering = NULL;
	timepatch = NULL;
	sucks = NULL;
	par = NULL;
	total = NULL;
	p = NULL;

	for (int i = 0; i < 4; i++)
		faceclassic[i] = NULL;
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
