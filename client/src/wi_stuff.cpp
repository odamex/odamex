// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
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
//		Intermission screens.
//
//-----------------------------------------------------------------------------


#include <ctype.h>
#include <stdio.h>

#include "z_zone.h"
#include "m_random.h"
#include "m_swap.h"
#include "i_system.h"
#include "w_wad.h"
#include "g_game.h"
#include "g_level.h"
#include "r_local.h"
#include "s_sound.h"
#include "doomstat.h"
#include "v_video.h"
#include "wi_stuff.h"
#include "c_console.h"
#include "hu_stuff.h"
#include "v_palette.h"

CVAR (wi_percents, "1", CVAR_ARCHIVE)

void WI_unloadData(void);

//
// Data needed to add patches to full screen intermission pics.
// Patches are statistics messages, and animations.
// Loads of by-pixel layout and placement, offsets etc.
//


//
// Different vetween registered DOOM (1994) and
//	Ultimate DOOM - Final edition (retail, 1995?).
// This is supposedly ignored for commercial
//	release (aka DOOM II), which had 32 maps
//	in one episode. So there.
#define NUMEPISODES 	4
#define NUMMAPS 		9


// GLOBAL LOCATIONS
#define WI_TITLEY				2
#define WI_SPACINGY 			33


typedef enum
{
	ANIM_ALWAYS,
    ANIM_RANDOM,
	ANIM_LEVEL
} animenum_t;

typedef struct
{
    int		x;
    int		y;

} point_t;

//
// Animation.
// There is another anim_t used in p_spec.
//
typedef struct
{
    animenum_t	type;

    // period in tics between animations
    int		period;

    // number of animation frames
    int		nanims;

    // location of animation
    point_t	loc;

    // ALWAYS: n/a,
    // RANDOM: period deviation (<256),
    // LEVEL: level
    int		data1;

    // ALWAYS: n/a,
    // RANDOM: random base period,
    // LEVEL: n/a
    int		data2;

    // actual graphics for frames of animations
    patch_t*	p[3];

    // following must be initialized to zero before use!

    // next value of bcnt (used in conjunction with period)
    int		nexttic;

    // last drawn animation frame
    int		lastdrawn;

    // next frame number to animate
    int		ctr;

    // used by RANDOM and LEVEL when animating
    int		state;

} anim_t;

static point_t lnodes[NUMEPISODES][NUMMAPS] =
{
    // Episode 0 World Map
    {
	{ 185, 164 },	// location of level 0 (CJ)
	{ 148, 143 },	// location of level 1 (CJ)
	{ 69, 122 },	// location of level 2 (CJ)
	{ 209, 102 },	// location of level 3 (CJ)
	{ 116, 89 },	// location of level 4 (CJ)
	{ 166, 55 },	// location of level 5 (CJ)
	{ 71, 56 },	// location of level 6 (CJ)
	{ 135, 29 },	// location of level 7 (CJ)
	{ 71, 24 }	// location of level 8 (CJ)
    },

    // Episode 1 World Map should go here
    {
	{ 254, 25 },	// location of level 0 (CJ)
	{ 97, 50 },	// location of level 1 (CJ)
	{ 188, 64 },	// location of level 2 (CJ)
	{ 128, 78 },	// location of level 3 (CJ)
	{ 214, 92 },	// location of level 4 (CJ)
	{ 133, 130 },	// location of level 5 (CJ)
	{ 208, 136 },	// location of level 6 (CJ)
	{ 148, 140 },	// location of level 7 (CJ)
	{ 235, 158 }	// location of level 8 (CJ)
    },

    // Episode 2 World Map should go here
    {
	{ 156, 168 },	// location of level 0 (CJ)
	{ 48, 154 },	// location of level 1 (CJ)
	{ 174, 95 },	// location of level 2 (CJ)
	{ 265, 75 },	// location of level 3 (CJ)
	{ 130, 48 },	// location of level 4 (CJ)
	{ 279, 23 },	// location of level 5 (CJ)
	{ 198, 48 },	// location of level 6 (CJ)
	{ 140, 25 },	// location of level 7 (CJ)
	{ 281, 136 }	// location of level 8 (CJ)
    }

};

//
// Animation locations for episode 0 (1).
// Using patches saves a lot of space,
//  as they replace 320x200 full screen frames.
//
static anim_t epsd0animinfo[] =
{
    { ANIM_ALWAYS, TICRATE/3, 3, { 224, 104 }, 0, 0, { NULL, NULL, NULL }, 0, 0, 0, 0 },
    { ANIM_ALWAYS, TICRATE/3, 3, { 184, 160 }, 0, 0, { NULL, NULL, NULL }, 0, 0, 0, 0 },
    { ANIM_ALWAYS, TICRATE/3, 3, { 112, 136 }, 0, 0, { NULL, NULL, NULL }, 0, 0, 0, 0 },
    { ANIM_ALWAYS, TICRATE/3, 3, { 72, 112 }, 0, 0, { NULL, NULL, NULL }, 0, 0, 0, 0 },
    { ANIM_ALWAYS, TICRATE/3, 3, { 88, 96 }, 0, 0, { NULL, NULL, NULL }, 0, 0, 0, 0 },
    { ANIM_ALWAYS, TICRATE/3, 3, { 64, 48 }, 0, 0, { NULL, NULL, NULL }, 0, 0, 0, 0 },
    { ANIM_ALWAYS, TICRATE/3, 3, { 192, 40 }, 0, 0, { NULL, NULL, NULL }, 0, 0, 0, 0 },
    { ANIM_ALWAYS, TICRATE/3, 3, { 136, 16 }, 0, 0, { NULL, NULL, NULL }, 0, 0, 0, 0 },
    { ANIM_ALWAYS, TICRATE/3, 3, { 80, 16 }, 0, 0, { NULL, NULL, NULL }, 0, 0, 0, 0 },
    { ANIM_ALWAYS, TICRATE/3, 3, { 64, 24 }, 0, 0, { NULL, NULL, NULL }, 0, 0, 0, 0 }
};

static anim_t epsd1animinfo[] =
{
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 1, 0, { NULL, NULL, NULL }, 0, 0, 0, 0  },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 2, 0, { NULL, NULL, NULL }, 0, 0, 0, 0  },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 3, 0, { NULL, NULL, NULL }, 0, 0, 0, 0  },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 4, 0, { NULL, NULL, NULL }, 0, 0, 0, 0  },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 5, 0, { NULL, NULL, NULL }, 0, 0, 0, 0  },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 6, 0, { NULL, NULL, NULL }, 0, 0, 0, 0  },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 7, 0, { NULL, NULL, NULL }, 0, 0, 0, 0  },
    { ANIM_LEVEL, TICRATE/3, 3, { 192, 144 }, 8, 0, { NULL, NULL, NULL }, 0, 0, 0, 0  },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 8, 0, { NULL, NULL, NULL }, 0, 0, 0, 0  }
};

static anim_t epsd2animinfo[] =
{
    { ANIM_ALWAYS, TICRATE/3, 3, { 104, 168 }, 0, 0, { NULL, NULL, NULL }, 0, 0, 0, 0 },
    { ANIM_ALWAYS, TICRATE/3, 3, { 40, 136 }, 0, 0, { NULL, NULL, NULL }, 0, 0, 0, 0 },
    { ANIM_ALWAYS, TICRATE/3, 3, { 160, 96 }, 0, 0, { NULL, NULL, NULL }, 0, 0, 0, 0 },
    { ANIM_ALWAYS, TICRATE/3, 3, { 104, 80 }, 0, 0, { NULL, NULL, NULL }, 0, 0, 0, 0 },
    { ANIM_ALWAYS, TICRATE/3, 3, { 120, 32 }, 0, 0, { NULL, NULL, NULL }, 0, 0, 0, 0 },
    { ANIM_ALWAYS, TICRATE/4, 3, { 40, 0 }, 0, 0, { NULL, NULL, NULL }, 0, 0, 0, 0 }
};

static int NUMANIMS[NUMEPISODES] =
{
	sizeof(epsd0animinfo)/sizeof(anim_t),
	sizeof(epsd1animinfo)/sizeof(anim_t),
	sizeof(epsd2animinfo)/sizeof(anim_t)
};

static anim_t *anims[NUMEPISODES] =
{
	epsd0animinfo,
	epsd1animinfo,
	epsd2animinfo
};

// [RH] Map name -> index mapping
static char names[NUMEPISODES][NUMMAPS][8] = {
	{ "E1M1", "E1M2", "E1M3", "E1M4", "E1M5", "E1M6", "E1M7", "E1M8", "E1M9" },
	{ "E2M1", "E2M2", "E2M3", "E2M4", "E2M5", "E2M6", "E2M7", "E2M8", "E2M9" },
	{ "E3M1", "E3M2", "E3M3", "E3M4", "E3M5", "E3M6", "E3M7", "E3M8", "E3M9" }
};

//
// GENERAL DATA
//

//
// Locally used stuff.
//
#define FB (screen)


// in seconds
#define SHOWNEXTLOCDELAY		4
//#define SHOWLASTLOCDELAY		SHOWNEXTLOCDELAY


// used to accelerate or skip a stage
static int				acceleratestage;

// wbs->pnum
static unsigned			me;

 // specifies current state
static stateenum_t		state;

// contains information passed into intermission
static wbstartstruct_t* wbs;

static std::vector<wbplayerstruct_t> plrs;	// = wbs->plyr

// used for general timing
static int				cnt;

// used for timing of background animation
static int				bcnt;

struct cnt_t
{
	int cnt_kills, cnt_items, cnt_secret, cnt_frags;
};

static std::vector<cnt_t> stats;
static std::vector<int> dm_totals;


//
//		GRAPHICS
//

// You Are Here graphic
static patch_t* 		yah[2];

// splat
static patch_t* 		splat;

// 0-9 graphic
static patch_t* 		num[10];

// minus sign
static patch_t* 		wiminus;

// "Finished!" graphics
static patch_t* 		finished;

// "Entering" graphic
static patch_t* 		entering;

static patch_t* 		p;		// [RH] Only one

 // Name graphics of each level (centered)
static patch_t*			lnames[2];

// [RH] Info to dynamically generate the level name graphics
static int				lnamewidths[2];
static char				*lnametexts[2];

static DCanvas			*background;

//
// CODE
//

// slam background
// UNUSED static unsigned char *background=0;


void WI_slamBackground (void)
{
	background->Blit (0, 0, background->width, background->height,
		FB, 0, 0, FB->width, FB->height);
}

static int WI_DrawName (char *str, int x, int y)
{
	int lump;
	patch_t *p = NULL;
	char charname[9];

	while (*str)
	{
		sprintf (charname, "FONTB%02u", toupper(*str) - 32);
		lump = W_CheckNumForName (charname);
		if (lump != -1)
		{
			p = (patch_t *)W_CacheLumpNum (lump, PU_CACHE);
			FB->DrawPatchClean (p, x, y);
			x += p->width() - 1;
		}
		else
		{
			x += 12;
		}
		str++;
	}

	p = (patch_t *)W_CacheLumpName ("FONTB39", PU_CACHE);
	return (5*(p->height()-p->topoffset()))/4;
}


// Draws "<Levelname> Finished!"
void WI_drawLF (void)
{
	int y;

	if (!lnames[0] && !lnamewidths[0])
		return;

	y = WI_TITLEY;

	if (lnames[0])
	{
		// draw <LevelName>
		FB->DrawPatchClean (lnames[0], (320 - lnames[0]->width())/2, y);
		y += (5*lnames[0]->height())/4;
	}
	else
	{
		// [RH] draw a dynamic title string
		y += WI_DrawName (lnametexts[0], 160 - lnamewidths[0] / 2, y);
	}

	// draw "Finished!"
	FB->DrawPatchClean (finished, (320 - finished->width())/2, y);
}



// Draws "Entering <LevelName>"
void WI_drawEL (void)
{
	int y = WI_TITLEY;

	if (!lnames[1] && !lnamewidths[1])
		return;

	y = WI_TITLEY;

	// draw "Entering"
	FB->DrawPatchClean (entering, (320 - entering->width())/2, y);

	// [RH] Changed to adjust by height of entering patch instead of title
	y += (5*entering->height())/4;

	if (lnames[1])
	{
		// draw level
		FB->DrawPatchClean (lnames[1], (320 - lnames[1]->width())/2, y);
	}
	else
	{
		// [RH] draw a dynamic title string
		WI_DrawName (lnametexts[1], 160 - lnamewidths[1] / 2, y);
	}
}

int WI_MapToIndex (char *map)
{
	int i;

	for (i = 0; i < NUMMAPS; i++)
	{
		if (!strnicmp (names[wbs->epsd][i], map, 8))
			break;
	}
	return i;
}

void WI_drawOnLnode (int n, patch_t *c[])
{

	int 	i;
	int 	left;
	int 	top;
	int 	right;
	int 	bottom;
	BOOL 	fits = false;

	i = 0;
	do
	{
		left = lnodes[wbs->epsd][n].x - c[i]->leftoffset();
		top = lnodes[wbs->epsd][n].y - c[i]->topoffset();
		right = left + c[i]->width();
		bottom = top + c[i]->height();

		if (left >= 0 && right < 320 && top >= 0 && bottom < 200)
		{
			fits = true;
		}
		else
		{
			i++;
		}
	} while (!fits && i != 2);

	if (fits && i<2)
	{
		FB->DrawPatchIndirect (c[i], lnodes[wbs->epsd][n].x, lnodes[wbs->epsd][n].y);
	}
	else
	{
		// DEBUG
		DPrintf ("Could not place patch on level %d", n+1);
	}
}



void WI_initAnimatedBack (void)
{
	int i;
	anim_t *a;

	if (gamemode == commercial || wbs->epsd > 2)
		return;

	for (i = 0; i < NUMANIMS[wbs->epsd]; i++)
	{
		a = &anims[wbs->epsd][i];

		// init variables
		a->ctr = -1;

		// specify the next time to draw it
		if (a->type == ANIM_ALWAYS)
			a->nexttic = bcnt + 1 + (M_Random()%a->period);
		else if (a->type == ANIM_LEVEL)
			a->nexttic = bcnt + 1;
	}

}

void WI_updateAnimatedBack (void)
{
	int i;
	anim_t *a;

	if (gamemode == commercial || wbs->epsd > 2)
		return;

	for (i = 0; i < NUMANIMS[wbs->epsd]; i++)
	{
		a = &anims[wbs->epsd][i];

		if (bcnt == a->nexttic)
		{
			switch (a->type)
			{
			  case ANIM_ALWAYS:
				if (++a->ctr >= a->nanims)
					a->ctr = 0;
				a->nexttic = bcnt + a->period;
				break;

			  case ANIM_RANDOM:
				  a->ctr++;
				  if (a->ctr == a->nanims)
				  {
					  a->ctr = -1;
					  a->nexttic = bcnt+a->data2+(M_Random()%a->data1);
				  }
					  else a->nexttic = bcnt + a->period;
				  break;

			  case ANIM_LEVEL:
				// gawd-awful hack for level anims

				if (!(state == StatCount && i == 7)
					&& (WI_MapToIndex (wbs->next) + 1) == a->data1)
				{
					a->ctr++;
					if (a->ctr == a->nanims)
						a->ctr--;
					a->nexttic = bcnt + a->period;
				}

				break;
			}
		}

	}

}

void WI_drawAnimatedBack (void)
{
	int i;
	anim_t *a;

	if (gamemode != commercial && wbs->epsd <= 2)
	{
		background->Lock ();
		for (i = 0; i < NUMANIMS[wbs->epsd]; i++)
		{
			a = &anims[wbs->epsd][i];

			if (a->ctr >= 0)
				background->DrawPatch (a->p[a->ctr], a->loc.x, a->loc.y);
		}
		background->Unlock ();
	}

	WI_slamBackground ();
}

#include "hu_stuff.h"
extern patch_t *hu_font[HU_FONTSIZE];

void WI_End (void)
{
	WI_unloadData();
	delete background;
	background = NULL;
}

void WI_initNoState (void)
{
	state = NoState;
	acceleratestage = 0;
	cnt = 10;
}

void WI_updateNoState (void)
{
	WI_updateAnimatedBack();

	// denis - let the server decide when to load the next map
	/*
	if (!--cnt)
	{
		WI_End();
		G_WorldDone();
	}
	*/
}

static BOOL snl_pointeron = false;

void WI_initShowNextLoc (void)
{
	state = ShowNextLoc;
	acceleratestage = 0;
	cnt = SHOWNEXTLOCDELAY * TICRATE;

	WI_initAnimatedBack();
}

void WI_updateShowNextLoc (void)
{
	WI_updateAnimatedBack();

	if (!--cnt || acceleratestage)
		WI_initNoState();
	else
		snl_pointeron = (cnt & 31) < 20;
}

void WI_drawShowNextLoc (void)
{
	int i;

	// draw animated background
	WI_drawAnimatedBack ();

	if (gamemode != commercial)
	{
		if (wbs->epsd > 2)
		{
			WI_drawEL();
			return;
		}

		// draw a splat on taken cities.
		for (i=0; i < NUMMAPS; i++) {
			if (FindLevelInfo (names[wbs->epsd][i])->flags & LEVEL_VISITED)
				WI_drawOnLnode(i, &splat);
		}

		// draw flashing ptr
		if (snl_pointeron)
			WI_drawOnLnode(WI_MapToIndex (wbs->next), yah);
	}

	// draws which level you are entering..
	WI_drawEL();

}

void WI_drawNoState (void)
{
	snl_pointeron = true;
	WI_drawShowNextLoc();
}

int WI_fragSum (player_t &player)
{
	return player.fragcount;
}

void WI_drawNetgameStats (void)
{
	// draw animated background
	WI_drawAnimatedBack();
	WI_drawLF();

	// [RH] Draw heads-up scores display
	HU_DrawScores (&players[me]);
}

// Updates stuff each tick
void WI_Ticker (void)
{
	// counter for general background animation
	bcnt++;

	if (bcnt == 1)
	{
		// intermission music
		if (gamemode == commercial)
			S_ChangeMusic ("d_dm2int", true);
		else
			S_ChangeMusic ("d_inter", true);
	}

	switch (state)
	{
		case StatCount:
			WI_updateAnimatedBack();
			break;

		case ShowNextLoc:
			WI_updateShowNextLoc();
			break;

		case NoState:
			WI_updateNoState();
			break;
	}
}

static int WI_CalcWidth (char *str)
{
	int w = 0;
	int lump;
	patch_t *p;
	char charname[9];

	if (!str)
		return 0;

	while (*str) {
		sprintf (charname, "FONTB%02u", toupper(*str) - 32);
		lump = W_CheckNumForName (charname);
		if (lump != -1) {
			p = (patch_t *)W_CacheLumpNum (lump, PU_CACHE);
			w += p->width() - 1;
		} else {
			w += 12;
		}
		str++;
	}

	return w;
}

void WI_loadData (void)
{
	int i, j;
	char name[9];
	anim_t *a;
	patch_t *bg;

	if ((gamemode == commercial) ||
		(gamemode == retail && wbs->epsd >= 3))
		strcpy (name, "INTERPIC");
	else
		sprintf (name, "WIMAP%d", wbs->epsd);

	// background
	bg = (patch_t *)W_CacheLumpName (name, PU_CACHE);
	background = new DCanvas (bg->width(), bg->height(), 8);
	background->Lock ();
	background->DrawPatch (bg, 0, 0);
	background->Unlock ();

	for (i = 0; i < 2; i++)
	{
		char *lname = (i == 0 ? wbs->lname0 : wbs->lname1);

		if (lname)
			j = W_CheckNumForName (lname);
		else
			j = -1;

		if (j >= 0)
		{
			lnames[i] = (patch_t *)W_CacheLumpNum (j, PU_STATIC);
		}
		else
		{
			lnames[i] = NULL;
			lnametexts[i] = FindLevelInfo (i == 0 ? wbs->current : wbs->next)->level_name;
			lnamewidths[i] = WI_CalcWidth (lnametexts[i]);
		}
	}

	if (gamemode != commercial)
	{
		// you are here
		yah[0] = (patch_t *)W_CacheLumpName ("WIURH0", PU_STATIC);

		// you are here (alt.)
		yah[1] = (patch_t *)W_CacheLumpName ("WIURH1", PU_STATIC);

		// splat
		splat = (patch_t *)W_CacheLumpName ("WISPLAT", PU_STATIC);

		if (wbs->epsd < 3)
		{
			for (j=0;j<NUMANIMS[wbs->epsd];j++)
			{
				a = &anims[wbs->epsd][j];
				for (i=0;i<a->nanims;i++)
				{
					// MONDO HACK!
					if (wbs->epsd != 1 || j != 8)
					{
						// animations
						sprintf (name, "WIA%d%.2d%.2d", wbs->epsd, j, i);
						a->p[i] = (patch_t *)W_CacheLumpName (name, PU_STATIC);
					}
					else
					{
						// HACK ALERT!
						a->p[i] = anims[1][4].p[i];
					}
				}
			}
		}
	}

	// "finished"
	finished = (patch_t *)W_CacheLumpName ("WIF", PU_STATIC);

	// "entering"
	entering = (patch_t *)W_CacheLumpName ("WIENTER", PU_STATIC);

	p = (patch_t *)W_CacheLumpName ("STPBANY", PU_STATIC);
}

void WI_unloadData (void)
{
	int i, j;

	Z_ChangeTag (wiminus, PU_CACHE);

	for (i = 0; i < 10; i++)
		Z_ChangeTag (num[i], PU_CACHE);

	for (i = 0; i < 2; i++) {
		if (lnames[i]) {
			Z_ChangeTag (lnames[i], PU_CACHE);
			lnames[i] = NULL;
		}
	}

	if (gamemode != commercial)
	{
		Z_ChangeTag (yah[0], PU_CACHE);
		Z_ChangeTag (yah[1], PU_CACHE);

		Z_ChangeTag (splat, PU_CACHE);

		if (wbs->epsd < 3)
		{
			for (j=0;j<NUMANIMS[wbs->epsd];j++)
			{
				if (wbs->epsd != 1 || j != 8)
					for (i=0;i<anims[wbs->epsd][j].nanims;i++)
						Z_ChangeTag (anims[wbs->epsd][j].p[i], PU_CACHE);
			}
		}
	}

	Z_ChangeTag (finished, PU_CACHE);
	Z_ChangeTag (entering, PU_CACHE);

	Z_ChangeTag (p, PU_CACHE);
}

void WI_Drawer (void)
{
	// If the background screen has been freed, then we really shouldn't
	// be in here. (But it happens anyway.)
	if (background)
	{
		switch (state)
		{
		case StatCount:
			WI_drawNetgameStats();
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

	WI_initAnimatedBack();
	V_SetBlend (0,0,0,0);
	S_StopAllChannels ();
// 	SN_StopAllSequences ();
}

VERSION_CONTROL (wi_stuff_cpp, "$Id:$")

