// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2012 by The Odamex Team.
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
//  AutoMap module.
//
//-----------------------------------------------------------------------------


#include <stdio.h>

#include "doomdef.h"
#include "g_level.h"
#include "z_zone.h"
#include "doomdef.h"
#include "st_stuff.h"
#include "p_local.h"
#include "p_lnspec.h"
#include "w_wad.h"
#include "v_palette.h"

#include "m_cheat.h"
#include "i_system.h"
#include "c_dispatch.h"
#include "p_ctf.h"
#include "cl_demo.h"

// Needs access to LFB.
#include "v_video.h"

#include "v_text.h"

extern patch_t *hu_font[];

// State.
#include "doomstat.h"
#include "r_state.h"

// Data.
#include "gstrings.h"

#include "am_map.h"

int CL_GetPlayerColor(player_t*);

static int Background, YourColor, WallColor, TSWallColor,
		   FDWallColor, CDWallColor, ThingColor,
		   SecretWallColor, GridColor, XHairColor,
		   NotSeenColor,
		   LockedColor,
		   AlmostBackground,
		   TeleportColor, ExitColor;

static int lockglow = 0;

EXTERN_CVAR (am_rotate)
EXTERN_CVAR (am_overlay)
EXTERN_CVAR (am_showsecrets)
EXTERN_CVAR (am_showmonsters)
EXTERN_CVAR (am_showtime)
EXTERN_CVAR (am_classicmapstring)
EXTERN_CVAR (am_usecustomcolors)
EXTERN_CVAR (am_ovshare)

EXTERN_CVAR (am_backcolor)
EXTERN_CVAR (am_yourcolor)
EXTERN_CVAR (am_wallcolor)
EXTERN_CVAR (am_tswallcolor)
EXTERN_CVAR (am_fdwallcolor)
EXTERN_CVAR (am_cdwallcolor)
EXTERN_CVAR (am_thingcolor)
EXTERN_CVAR (am_gridcolor)
EXTERN_CVAR (am_xhaircolor)
EXTERN_CVAR (am_notseencolor)
EXTERN_CVAR (am_lockedcolor)
EXTERN_CVAR (am_exitcolor)
EXTERN_CVAR (am_teleportcolor)

EXTERN_CVAR (am_ovyourcolor)
EXTERN_CVAR (am_ovwallcolor)
EXTERN_CVAR (am_ovtswallcolor)
EXTERN_CVAR (am_ovfdwallcolor)
EXTERN_CVAR (am_ovcdwallcolor)
EXTERN_CVAR (am_ovthingcolor)
EXTERN_CVAR (am_ovgridcolor)
EXTERN_CVAR (am_ovxhaircolor)
EXTERN_CVAR (am_ovnotseencolor)
EXTERN_CVAR (am_ovlockedcolor)
EXTERN_CVAR (am_ovexitcolor)
EXTERN_CVAR (am_ovteleportcolor)

BEGIN_COMMAND (resetcustomcolors)
{
    am_backcolor = "00 00 3a";
    am_yourcolor = "fc e8 d8";
    am_wallcolor = "00 8b ff";
    am_tswallcolor = "10 32 7e";
    am_fdwallcolor = "1a 1a 8a";
    am_cdwallcolor = "00 00 5a";
    am_thingcolor = "9f d3 ff";
    am_gridcolor = "44 44 88";
    am_xhaircolor = "80 80 80";
    am_notseencolor = "00 22 6e";
    am_lockedcolor = "bb bb bb";
    am_exitcolor = "ff ff 00";
    am_teleportcolor = "ff a3 00";

    am_ovyourcolor = "fc e8 d8";
    am_ovwallcolor = "00 8b ff";
    am_ovtswallcolor = "10 32 7e";
    am_ovfdwallcolor = "1a 1a 8a";
    am_ovcdwallcolor = "00 00 5a";
    am_ovthingcolor = "9f d3 ff";
    am_ovgridcolor = "44 44 88";
    am_ovxhaircolor = "80 80 80";
    am_ovnotseencolor = "00 22 6e";
    am_ovlockedcolor = "bb bb bb";
    am_ovexitcolor = "ff ff 00";
    am_ovteleportcolor = "ff a3 00";
    Printf (PRINT_HIGH, "Custom automap colors reset to default.\n");
}
END_COMMAND (resetcustomcolors)

EXTERN_CVAR		(screenblocks)

// drawing stuff
#define	FB		(screen)

#define AM_PANDOWNKEY	KEY_DOWNARROW
#define AM_PANUPKEY		KEY_UPARROW
#define AM_PANRIGHTKEY	KEY_RIGHTARROW
#define AM_PANLEFTKEY	KEY_LEFTARROW
#define AM_ZOOMINKEY	KEY_EQUALS
#define AM_ZOOMINKEY2	0x4e	// DIK_ADD
#define AM_ZOOMOUTKEY	KEY_MINUS
#define AM_ZOOMOUTKEY2	0x4a	// DIK_SUBTRACT
#define AM_GOBIGKEY		0x0b	// DIK_0
#define AM_FOLLOWKEY	'f'
#define AM_GRIDKEY		'g'
#define AM_MARKKEY		'm'
#define AM_CLEARMARKKEY	'c'

#define AM_NUMMARKPOINTS 10

// scale on entry
#define INITSCALEMTOF (.2*FRACUNIT)
// how much the automap moves window per tic in frame-buffer coordinates
// moves 140 pixels in 1 second
#define F_PANINC	(140/TICRATE)
// how much zoom-in per tic
// goes to 2x in 1 second
#define M_ZOOMIN        ((int) (1.02*FRACUNIT))
// how much zoom-out per tic
// pulls out to 0.5x in 1 second
#define M_ZOOMOUT       ((int) (FRACUNIT/1.02))

// translates between frame-buffer and map distances
#define FTOM(x) FixedMul(((x)<<16),scale_ftom)
#define MTOF(x) (FixedMul((x),scale_mtof)>>16)
// translates between frame-buffer and map coordinates
#define CXMTOF(x)  (MTOF((x)-m_x)/* - f_x*/)
#define CYMTOF(y)  (f_h - MTOF((y)-m_y)/* + f_y*/)

typedef struct {
	int x, y;
} fpoint_t;

typedef struct {
	fpoint_t a, b;
} fline_t;

typedef struct {
	fixed_t x,y;
} mpoint_t;

typedef struct {
	mpoint_t a, b;
} mline_t;

typedef struct {
	fixed_t slp, islp;
} islope_t;



//
// The vector graphics for the automap.
//  A line drawing of the player pointing right,
//   starting from the middle.
//
#define R ((8*PLAYERRADIUS)/7)
mline_t player_arrow[] = {
	{ { -R+R/8, 0 }, { R, 0 } }, // -----
	{ { R, 0 }, { R-R/2, R/4 } },  // ----->
	{ { R, 0 }, { R-R/2, -R/4 } },
	{ { -R+R/8, 0 }, { -R-R/8, R/4 } }, // >---->
	{ { -R+R/8, 0 }, { -R-R/8, -R/4 } },
	{ { -R+3*R/8, 0 }, { -R+R/8, R/4 } }, // >>--->
	{ { -R+3*R/8, 0 }, { -R+R/8, -R/4 } }
};
#undef R
#define NUMPLYRLINES (sizeof(player_arrow)/sizeof(mline_t))

#define R ((8*PLAYERRADIUS)/7)
mline_t cheat_player_arrow[] = {
	{ { -R+R/8, 0 }, { R, 0 } }, // -----
	{ { R, 0 }, { R-R/2, R/6 } },  // ----->
	{ { R, 0 }, { R-R/2, -R/6 } },
	{ { -R+R/8, 0 }, { -R-R/8, R/6 } }, // >----->
	{ { -R+R/8, 0 }, { -R-R/8, -R/6 } },
	{ { -R+3*R/8, 0 }, { -R+R/8, R/6 } }, // >>----->
	{ { -R+3*R/8, 0 }, { -R+R/8, -R/6 } },
	{ { -R/2, 0 }, { -R/2, -R/6 } }, // >>-d--->
	{ { -R/2, -R/6 }, { -R/2+R/6, -R/6 } },
	{ { -R/2+R/6, -R/6 }, { -R/2+R/6, R/4 } },
	{ { -R/6, 0 }, { -R/6, -R/6 } }, // >>-dd-->
	{ { -R/6, -R/6 }, { 0, -R/6 } },
	{ { 0, -R/6 }, { 0, R/4 } },
	{ { R/6, R/4 }, { R/6, -R/7 } }, // >>-ddt->
	{ { R/6, -R/7 }, { R/6+R/32, -R/7-R/32 } },
	{ { R/6+R/32, -R/7-R/32 }, { R/6+R/10, -R/7 } }
};
#undef R
#define NUMCHEATPLYRLINES (sizeof(cheat_player_arrow)/sizeof(mline_t))

#define R (FRACUNIT)
// [RH] Avoid lots of warnings without compiler-specific #pragmas
#define L(a,b,c,d) { {(fixed_t)((a)*R),(fixed_t)((b)*R)}, {(fixed_t)((c)*R),(fixed_t)((d)*R)} }
mline_t triangle_guy[] = {
	L (-.867,-.5, .867,-.5),
	L (.867,-.5, 0,1),
	L (0,1, -.867,-.5)
};
#define NUMTRIANGLEGUYLINES (sizeof(triangle_guy)/sizeof(mline_t))

mline_t thintriangle_guy[] = {
	L (-.5,-.7, 1,0),
	L (1,0, -.5,.7),
	L (-.5,.7, -.5,-.7)
};
#undef L
#undef R
#define NUMTHINTRIANGLEGUYLINES (sizeof(thintriangle_guy)/sizeof(mline_t))




static int 	cheating = 0;
static int 	grid = 0;

static int 	leveljuststarted = 1; 	// kluge until AM_LevelInit() is called

BOOL		automapactive = false;

// location of window on screen
static int	f_x;
static int	f_y;

// size of window on screen
static int	f_w;
static int	f_h;
static int	f_p;				// [RH] # of bytes from start of a line to start of next

static byte *fb;				// pseudo-frame buffer
static int	amclock;

static mpoint_t	m_paninc;		// how far the window pans each tic (map coords)
static fixed_t	mtof_zoommul;	// how far the window zooms in each tic (map coords)
static fixed_t	ftom_zoommul;	// how far the window zooms in each tic (fb coords)

static fixed_t	m_x, m_y;		// LL x,y where the window is on the map (map coords)
static fixed_t	m_x2, m_y2;		// UR x,y where the window is on the map (map coords)

//
// width/height of window on map (map coords)
//
static fixed_t	m_w;
static fixed_t	m_h;

// based on level size
static fixed_t	min_x;
static fixed_t	min_y;
static fixed_t	max_x;
static fixed_t	max_y;

static fixed_t	max_w; // max_x-min_x,
static fixed_t	max_h; // max_y-min_y

// based on player size
static fixed_t	min_w;
static fixed_t	min_h;


static fixed_t	min_scale_mtof; // used to tell when to stop zooming out
static fixed_t	max_scale_mtof; // used to tell when to stop zooming in

// old stuff for recovery later
static fixed_t old_m_w, old_m_h;
static fixed_t old_m_x, old_m_y;

// old location used by the Follower routine
static mpoint_t f_oldloc;

// used by MTOF to scale from map-to-frame-buffer coords
static fixed_t scale_mtof = (fixed_t)INITSCALEMTOF;
// used by FTOM to scale from frame-buffer-to-map coords (=1/scale_mtof)
static fixed_t scale_ftom;

static patch_t *marknums[10]; // numbers used for marking by the automap
static mpoint_t markpoints[AM_NUMMARKPOINTS]; // where the points are
static int markpointnum = 0; // next point to be assigned

static bool followplayer = true; // specifies whether to follow the player around

// [RH] Not static so that the DeHackEd code can reach it.
extern byte cheat_amap_seq[5];
cheatseq_t cheat_amap = { cheat_amap_seq, 0 };

static BOOL stopped = true;

extern NetDemo netdemo;

#define NUMALIASES		3
#define WALLCOLORS		-1
#define FDWALLCOLORS	-2
#define CDWALLCOLORS	-3

#define WEIGHTBITS		6
#define WEIGHTSHIFT		(FRACBITS-WEIGHTBITS)
#define NUMWEIGHTS		(1<<WEIGHTBITS)
#define WEIGHTMASK		(NUMWEIGHTS-1)


void AM_rotatePoint (fixed_t *x, fixed_t *y);

//
//
//
void AM_activateNewScale(void)
{
	m_x += m_w/2;
	m_y += m_h/2;
	m_w = FTOM(f_w);
	m_h = FTOM(f_h);
	m_x -= m_w/2;
	m_y -= m_h/2;
	m_x2 = m_x + m_w;
	m_y2 = m_y + m_h;
}

//
//
//
void AM_saveScaleAndLoc(void)
{
	old_m_x = m_x;
	old_m_y = m_y;
	old_m_w = m_w;
	old_m_h = m_h;
}

//
//
//
void AM_restoreScaleAndLoc(void)
{
	m_w = old_m_w;
	m_h = old_m_h;
	if (!followplayer)
	{
		m_x = old_m_x;
		m_y = old_m_y;
    }
	else
	{
		m_x = displayplayer().camera->x - m_w/2;
		m_y = displayplayer().camera->y - m_h/2;
    }
	m_x2 = m_x + m_w;
	m_y2 = m_y + m_h;

	// Change the scaling multipliers
	scale_mtof = FixedDiv(f_w<<FRACBITS, m_w);
	scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}

//
// adds a marker at the current location
//
void AM_addMark(void)
{
	markpoints[markpointnum].x = m_x + m_w/2;
	markpoints[markpointnum].y = m_y + m_h/2;
	markpointnum = (markpointnum + 1) % AM_NUMMARKPOINTS;
}

//
// Determines bounding box of all vertices,
// sets global variables controlling zoom range.
//
void AM_findMinMaxBoundaries(void)
{
	int i;
	fixed_t a;
	fixed_t b;

	min_x = min_y =  MAXINT;
	max_x = max_y = -MAXINT;

	for (i=0;i<numvertexes;i++) {
		if (vertexes[i].x < min_x)
			min_x = vertexes[i].x;
		else if (vertexes[i].x > max_x)
			max_x = vertexes[i].x;

		if (vertexes[i].y < min_y)
			min_y = vertexes[i].y;
		else if (vertexes[i].y > max_y)
			max_y = vertexes[i].y;
	}

	max_w = max_x - min_x;
	max_h = max_y - min_y;

	min_w = 2*PLAYERRADIUS; // const? never changed?
	min_h = 2*PLAYERRADIUS;

	a = FixedDiv((screen->width)<<FRACBITS, max_w);
	b = FixedDiv((screen->height)<<FRACBITS, max_h);

	min_scale_mtof = a < b ? a : b;
	max_scale_mtof = FixedDiv((screen->height)<<FRACBITS, 2*PLAYERRADIUS);
}


//
//
//
void AM_changeWindowLoc(void)
{
	if (m_paninc.x || m_paninc.y) {
		followplayer = 0;
		f_oldloc.x = MAXINT;
	}

	m_x += m_paninc.x;
	m_y += m_paninc.y;

	if (m_x + m_w/2 > max_x)
		m_x = max_x - m_w/2;
	else if (m_x + m_w/2 < min_x)
		m_x = min_x - m_w/2;

	if (m_y + m_h/2 > max_y)
		m_y = max_y - m_h/2;
	else if (m_y + m_h/2 < min_y)
		m_y = min_y - m_h/2;

	m_x2 = m_x + m_w;
	m_y2 = m_y + m_h;
}


//
//
//
void AM_initVariables(void)
{
	static event_t st_notify = { ev_keyup, AM_MSGENTERED, 0, 0 };

	automapactive = true;

	f_oldloc.x = MAXINT;
	amclock = 0;

	m_paninc.x = m_paninc.y = 0;
	ftom_zoommul = FRACUNIT;
	mtof_zoommul = FRACUNIT;

	m_w = FTOM(screen->width);
	m_h = FTOM(screen->height);

	// find player to center on initially
	player_t *pl = &displayplayer();
	if (!pl->ingame())
	{
		for (size_t pnum = 0; pnum < players.size(); pnum++)
		{
			if (players[pnum].ingame())
			{
				pl = &players[pnum];
				break;
			}
		}
	}

	if(!pl->camera)
		return;

	m_x = pl->camera->x - m_w/2;
	m_y = pl->camera->y - m_h/2;
	AM_changeWindowLoc();

	// for saving & restoring
	old_m_x = m_x;
	old_m_y = m_y;
	old_m_w = m_w;
	old_m_h = m_h;

	// inform the status bar of the change
	ST_Responder (&st_notify);
}

static void GetComponents (int color, DWORD *palette, float &r, float &g, float &b)
{
	if (palette)
		color = palette[color];

	r = (float)RPART(color);
	g = (float)GPART(color);
	b = (float)BPART(color);
}

void AM_initColors (BOOL overlayed)
{
	DWORD *palette;

	palette = GetDefaultPalette()->colors;

	if (overlayed && !am_ovshare)
	{
		YourColor = V_GetColorFromString (palette, am_ovyourcolor.cstring());
		SecretWallColor =
			WallColor = V_GetColorFromString (palette, am_ovwallcolor.cstring());
		TSWallColor = V_GetColorFromString (palette, am_ovtswallcolor.cstring());
		FDWallColor = V_GetColorFromString (palette, am_ovfdwallcolor.cstring());
		CDWallColor = V_GetColorFromString (palette, am_ovcdwallcolor.cstring());
		ThingColor = V_GetColorFromString (palette, am_ovthingcolor.cstring());
        GridColor = V_GetColorFromString (palette, am_ovgridcolor.cstring());
        XHairColor = V_GetColorFromString (palette, am_ovxhaircolor.cstring());
		NotSeenColor = V_GetColorFromString (palette, am_ovnotseencolor.cstring());
		LockedColor = V_GetColorFromString (NULL, am_ovlockedcolor.cstring());
		ExitColor = V_GetColorFromString (palette, am_ovexitcolor.cstring());
		TeleportColor = V_GetColorFromString (palette, am_ovteleportcolor.cstring());
	}
	else if (am_usecustomcolors || (overlayed && am_ovshare))
	{
		/* Use the custom colors in the am_* cvars */
		Background = V_GetColorFromString (palette, am_backcolor.cstring());
		YourColor = V_GetColorFromString (palette, am_yourcolor.cstring());
		SecretWallColor =
			WallColor = V_GetColorFromString (palette, am_wallcolor.cstring());
		TSWallColor = V_GetColorFromString (palette, am_tswallcolor.cstring());
		FDWallColor = V_GetColorFromString (palette, am_fdwallcolor.cstring());
		CDWallColor = V_GetColorFromString (palette, am_cdwallcolor.cstring());
		ThingColor = V_GetColorFromString (palette, am_thingcolor.cstring());
		GridColor = V_GetColorFromString (palette, am_gridcolor.cstring());
		XHairColor = V_GetColorFromString (palette, am_xhaircolor.cstring());
		NotSeenColor = V_GetColorFromString (palette, am_notseencolor.cstring());
		LockedColor = V_GetColorFromString (NULL, am_lockedcolor.cstring());
		ExitColor = V_GetColorFromString (palette, am_exitcolor.cstring());
		TeleportColor = V_GetColorFromString (palette, am_teleportcolor.cstring());
		{
			unsigned int ba = V_GetColorFromString (NULL, am_backcolor.cstring());
			int r = RPART(ba) - 16;
			int g = GPART(ba) - 16;
			int b = BPART(ba) - 16;
			if (r < 0)
				r += 32;
			if (g < 0)
				g += 32;
			if (b < 0)
				b += 32;

			palette_t *pal = GetDefaultPalette();
			if (screen->is8bit())
				AlmostBackground = BestColor(pal->basecolors,
										r, g , b, pal->numcolors);
			else
				AlmostBackground = MAKERGB(r,g,b);
		}
	}
	else
	{
		/* Use colors corresponding to the original Doom's */
		Background = V_GetColorFromString (palette, "00 00 00");
		YourColor = V_GetColorFromString (palette, "FF FF FF");
		AlmostBackground = V_GetColorFromString (palette, "10 10 10");
		SecretWallColor =
			WallColor = V_GetColorFromString (palette, "fc 00 00");
		TSWallColor = V_GetColorFromString (palette, "80 80 80");
		FDWallColor = V_GetColorFromString (palette, "bc 78 48");
		LockedColor = V_GetColorFromString (NULL, "fc fc 00");
        CDWallColor = V_GetColorFromString (palette, "fc fc 00");
		ThingColor = V_GetColorFromString (palette, "74 fc 6c");
		GridColor = V_GetColorFromString (palette, "4c 4c 4c");
		XHairColor = V_GetColorFromString (palette, "80 80 80");
		NotSeenColor = V_GetColorFromString (palette, "6c 6c 6c");
	}

	float backRed, backGreen, backBlue;

	GetComponents (Background, palette, backRed, backGreen, backBlue);
}

//
//
//
void AM_loadPics(void)
{
	int i;
	char namebuf[9];

	for (i = 0; i < 10; i++)
	{
		sprintf(namebuf, "AMMNUM%d", i);
		marknums[i] = W_CachePatch (namebuf, PU_STATIC);
	}
}

void AM_unloadPics(void)
{
	int i;

	for (i = 0; i < 10; i++)
	{
		if (marknums[i])
		{
			Z_ChangeTag (marknums[i], PU_CACHE);
			marknums[i] = NULL;
		}
	}
}

void AM_clearMarks(void)
{
	int i;

	for (i = AM_NUMMARKPOINTS-1; i >= 0; i--)
		markpoints[i].x = -1; // means empty
	markpointnum = 0;
}

//
// should be called at the start of every level
// right now, i figure it out myself
//
void AM_LevelInit(void)
{
	leveljuststarted = 0;

	AM_clearMarks();

	AM_findMinMaxBoundaries();
	scale_mtof = FixedDiv(min_scale_mtof, (int) (0.7*FRACUNIT));
	if (scale_mtof > max_scale_mtof)
		scale_mtof = min_scale_mtof;
	scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}




//
//
//
void AM_Stop (void)
{
	static event_t st_notify = { ev_keyup, AM_MSGEXITED, 0, 0 };

    if (!automapactive)
    {
        return;
    }

	AM_unloadPics ();
	automapactive = false;
	ST_Responder (&st_notify);
	stopped = true;
	viewactive = true;
}

//
//
//
void AM_Start (void)
{
	static char lastmap[8] = "";

	if (!stopped) AM_Stop();
	stopped = false;
	if (strncmp (lastmap, level.mapname, 8))
	{
		AM_LevelInit();
		strncpy (lastmap, level.mapname, 8);
	}
	AM_initVariables();
	AM_loadPics();
}

//
// set the window scale to the maximum size
//
void AM_minOutWindowScale(void)
{
	scale_mtof = min_scale_mtof;
	scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}

//
// set the window scale to the minimum size
//
void AM_maxOutWindowScale(void)
{
	scale_mtof = max_scale_mtof;
	scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}


BEGIN_COMMAND (togglemap)
{
	if (gamestate != GS_LEVEL)
		return;

	if (!automapactive)
	{
		AM_Start ();
		if (am_overlay)
			viewactive = true;
		else
			viewactive = false;
	}
	else
	{
		if (am_overlay > 0 && am_overlay < 3 && viewactive)
		{
			viewactive = false;
		}
		else
		{
			AM_Stop ();
		}
	}

	if (automapactive)
		AM_initColors (viewactive);
}
END_COMMAND (togglemap)

//
// Handle events (user inputs) in automap mode
//
BOOL AM_Responder (event_t *ev)
{
	int rc;
	static int bigstate = 0;

	rc = false;

	if (automapactive && ev->type == ev_keydown)
	{
		rc = true;
		switch(ev->data1)
		{
		case AM_PANRIGHTKEY: // pan right
			if (!followplayer)
				m_paninc.x = FTOM(F_PANINC);
			else
				rc = false;
			break;
		case AM_PANLEFTKEY: // pan left
			if (!followplayer)
				m_paninc.x = -FTOM(F_PANINC);
			else
				rc = false;
			break;
		case AM_PANUPKEY: // pan up
			if (!followplayer)
				m_paninc.y = FTOM(F_PANINC);
			else
				rc = false;
			break;
		case AM_PANDOWNKEY: // pan down
			if (!followplayer)
				m_paninc.y = -FTOM(F_PANINC);
			else
				rc = false;
			break;
		case AM_ZOOMOUTKEY: // zoom out
		case AM_ZOOMOUTKEY2:
			mtof_zoommul = M_ZOOMOUT;
			ftom_zoommul = M_ZOOMIN;
			break;
		case AM_ZOOMINKEY: // zoom in
		case AM_ZOOMINKEY2:
			mtof_zoommul = M_ZOOMIN;
			ftom_zoommul = M_ZOOMOUT;
			break;
		case AM_GOBIGKEY:
			bigstate = !bigstate;
			if (bigstate)
			{
				AM_saveScaleAndLoc();
				AM_minOutWindowScale();
			}
			else
				AM_restoreScaleAndLoc();
			break;
		default:
			switch (ev->data2)
			{
			case AM_FOLLOWKEY:
				followplayer = !followplayer;
				f_oldloc.x = MAXINT;
				Printf (PRINT_HIGH, "%s\n", followplayer ? GStrings(AMSTR_FOLLOWON) : GStrings(AMSTR_FOLLOWOFF));
				break;
			case AM_GRIDKEY:
				grid = !grid;
				Printf (PRINT_HIGH, "%s\n", grid ? GStrings(AMSTR_GRIDON) : GStrings(AMSTR_GRIDOFF));
				break;
			case AM_MARKKEY:
				Printf (PRINT_HIGH, "%s %d\n",  GStrings(AMSTR_MARKEDSPOT), markpointnum);
				AM_addMark();
				break;
			case AM_CLEARMARKKEY:
				AM_clearMarks();
				Printf (PRINT_HIGH, "%s\n", GStrings(AMSTR_MARKSCLEARED));
				break;
			default:
				rc = false;
			}
		}
		if (sv_gametype == GM_COOP && cht_CheckCheat(&cheat_amap, (char)ev->data2))
		{
			rc = true;	// [RH] Eat last keypress of cheat sequence
			cheating = (cheating+1) % 3;
		}
	}
	else if (ev->type == ev_keyup)
	{
		rc = false;
		switch (ev->data1)
		{
		case AM_PANRIGHTKEY:
			if (!followplayer) m_paninc.x = 0;
			break;
		case AM_PANLEFTKEY:
			if (!followplayer) m_paninc.x = 0;
			break;
		case AM_PANUPKEY:
			if (!followplayer) m_paninc.y = 0;
			break;
		case AM_PANDOWNKEY:
			if (!followplayer) m_paninc.y = 0;
			break;
		case AM_ZOOMOUTKEY:
		case AM_ZOOMOUTKEY2:
		case AM_ZOOMINKEY:
		case AM_ZOOMINKEY2:
			mtof_zoommul = FRACUNIT;
			ftom_zoommul = FRACUNIT;
			break;
		}
	}

	return rc;
}


//
// Zooming
//
void AM_changeWindowScale (void)
{
	// Change the scaling multipliers
	scale_mtof = FixedMul(scale_mtof, mtof_zoommul);
	scale_ftom = FixedDiv(FRACUNIT, scale_mtof);

	if (scale_mtof < min_scale_mtof)
		AM_minOutWindowScale();
	else if (scale_mtof > max_scale_mtof)
		AM_maxOutWindowScale();
}


//
//
//
void AM_doFollowPlayer(void)
{
	player_t &p = displayplayer();

    if (f_oldloc.x != p.camera->x ||
		f_oldloc.y != p.camera->y)
	{
		m_x = FTOM(MTOF(p.camera->x)) - m_w/2;
		m_y = FTOM(MTOF(p.camera->y)) - m_h/2;
		m_x2 = m_x + m_w;
		m_y2 = m_y + m_h;
		f_oldloc.x = p.camera->x;
		f_oldloc.y = p.camera->y;
	}
}

//
// Updates on Game Tick
//
void AM_Ticker (void)
{
	if (!automapactive)
		return;

	amclock++;

	if (followplayer)
		AM_doFollowPlayer();

	// Change the zoom if necessary
	if (ftom_zoommul != FRACUNIT)
		AM_changeWindowScale();

	// Change x,y location
	if (m_paninc.x || m_paninc.y)
		AM_changeWindowLoc();

    // NES - Glowing effect on locked doors.
    if (lockglow < 90)
        lockglow++;
    else
        lockglow = 0;

}


//
// Clear automap frame buffer.
//
void AM_clearFB (int color)
{
	int y;

	if (screen->is8bit()) {
		if (f_w == f_p)
			memset (fb, color, f_w*f_h);
		else
			for (y = 0; y < f_h; y++)
				memset (fb + y * f_p, color, f_w);
	} else {
		int x;
		int *line;

		line = (int *)(fb);
		for (y = 0; y < f_h; y++) {
			for (x = 0; x < f_w; x++) {
				line[x] = color;
			}
			line += f_p >> 2;
		}
	}
}


//
// Automap clipping of lines.
//
// Based on Cohen-Sutherland clipping algorithm but with a slightly
// faster reject and precalculated slopes.  If the speed is needed,
// use a hash algorithm to handle  the common cases.
//
BOOL AM_clipMline (mline_t *ml, fline_t *fl)
{
	enum {
		LEFT	=1,
		RIGHT	=2,
		BOTTOM	=4,
		TOP	=8
	};

	register int outcode1 = 0;
	register int outcode2 = 0;
	register int outside;

	fpoint_t tmp = {0, 0};
	int dx;
	int dy;


#define DOOUTCODE(oc, mx, my) \
	(oc) = 0; \
	if ((my) < 0) (oc) |= TOP; \
	else if ((my) >= f_h) (oc) |= BOTTOM; \
	if ((mx) < 0) (oc) |= LEFT; \
	else if ((mx) >= f_w) (oc) |= RIGHT;

	// do trivial rejects and outcodes
	if (ml->a.y > m_y2)
		outcode1 = TOP;
	else if (ml->a.y < m_y)
		outcode1 = BOTTOM;

	if (ml->b.y > m_y2)
		outcode2 = TOP;
	else if (ml->b.y < m_y)
		outcode2 = BOTTOM;

	if (outcode1 & outcode2)
		return false; // trivially outside

	if (ml->a.x < m_x)
		outcode1 |= LEFT;
	else if (ml->a.x > m_x2)
		outcode1 |= RIGHT;

	if (ml->b.x < m_x)
		outcode2 |= LEFT;
	else if (ml->b.x > m_x2)
		outcode2 |= RIGHT;

	if (outcode1 & outcode2)
		return false; // trivially outside

	// transform to frame-buffer coordinates.
	fl->a.x = CXMTOF(ml->a.x);
	fl->a.y = CYMTOF(ml->a.y);
	fl->b.x = CXMTOF(ml->b.x);
	fl->b.y = CYMTOF(ml->b.y);

	DOOUTCODE(outcode1, fl->a.x, fl->a.y);
	DOOUTCODE(outcode2, fl->b.x, fl->b.y);

	if (outcode1 & outcode2)
		return false;

	while (outcode1 | outcode2) {
		// may be partially inside box
		// find an outside point
		if (outcode1)
			outside = outcode1;
		else
			outside = outcode2;

		// clip to each side
		if (outside & TOP) {
			dy = fl->a.y - fl->b.y;
			dx = fl->b.x - fl->a.x;
			tmp.x = fl->a.x + (dx*(fl->a.y))/dy;
			tmp.y = 0;
		} else if (outside & BOTTOM) {
			dy = fl->a.y - fl->b.y;
			dx = fl->b.x - fl->a.x;
			tmp.x = fl->a.x + (dx*(fl->a.y-f_h))/dy;
			tmp.y = f_h-1;
		} else if (outside & RIGHT) {
			dy = fl->b.y - fl->a.y;
			dx = fl->b.x - fl->a.x;
			tmp.y = fl->a.y + (dy*(f_w-1 - fl->a.x))/dx;
			tmp.x = f_w-1;
		} else if (outside & LEFT) {
			dy = fl->b.y - fl->a.y;
			dx = fl->b.x - fl->a.x;
			tmp.y = fl->a.y + (dy*(-fl->a.x))/dx;
			tmp.x = 0;
		}

		if (outside == outcode1) {
			fl->a = tmp;
			DOOUTCODE(outcode1, fl->a.x, fl->a.y);
		} else {
			fl->b = tmp;
			DOOUTCODE(outcode2, fl->b.x, fl->b.y);
		}

		if (outcode1 & outcode2)
			return false; // trivially outside
	}

	return true;
}
#undef DOOUTCODE


//
// Classic Bresenham w/ whatever optimizations needed for speed
//
void
AM_drawFline
( fline_t*	fl,
  int		color )
{
			register int x;
			register int y;
			register int dx;
			register int dy;
			register int sx;
			register int sy;
			register int ax;
			register int ay;
			register int d;

#define PUTDOTP(xx,yy,cc) fb[(yy)*f_p+(xx)]=(cc)
#define PUTDOTD(xx,yy,cc) *((int *)(fb+(yy)*f_p+((xx)<<2)))=(cc)

			fl->a.x += f_x;
			fl->b.x += f_x;
			fl->a.y += f_y;
			fl->b.y += f_y;

			dx = fl->b.x - fl->a.x;
			ax = 2 * (dx<0 ? -dx : dx);
			sx = dx<0 ? -1 : 1;

			dy = fl->b.y - fl->a.y;
			ay = 2 * (dy<0 ? -dy : dy);
			sy = dy<0 ? -1 : 1;

			x = fl->a.x;
			y = fl->a.y;

			if (ax > ay) {
				d = ay - ax/2;

					while (1) {
						PUTDOTP(x,y,(byte)color);
						if (x == fl->b.x)
							return;
						if (d>=0) {
							y += sy;
							d -= ax;
						}
						x += sx;
						d += ay;
					}

			}
			else {
				d = ax - ay/2;
					while (1) {
						PUTDOTP(x, y, (byte)color);
						if (y == fl->b.y)
							return;
						if (d >= 0) {
							x += sx;
							d -= ay;
						}
						y += sy;
						d += ax;
					}

				}
}


//
// Clip lines, draw visible part sof lines.
//
void
AM_drawMline
( mline_t*	ml,
  int		color )
{
    static fline_t fl;

    if (AM_clipMline(ml, &fl))
	AM_drawFline(&fl, color); // draws it on frame buffer using fb coords
}



//
// Draws flat (floor/ceiling tile) aligned grid lines.
//
void AM_drawGrid(int color)
{
	fixed_t x, y;
	fixed_t start, end;
	mline_t ml;

	// Figure out start of vertical gridlines
	start = m_x;
	if ((start-bmaporgx)%(MAPBLOCKUNITS<<FRACBITS))
		start += (MAPBLOCKUNITS<<FRACBITS)
			- ((start-bmaporgx)%(MAPBLOCKUNITS<<FRACBITS));
	end = m_x + m_w;

	// draw vertical gridlines
	ml.a.y = m_y;
	ml.b.y = m_y+m_h;
	for (x=start; x<end; x+=(MAPBLOCKUNITS<<FRACBITS)) {
		ml.a.x = x;
		ml.b.x = x;
		if (am_rotate) {
			AM_rotatePoint (&ml.a.x, &ml.a.y);
			AM_rotatePoint (&ml.b.x, &ml.b.y);
		}
		AM_drawMline(&ml, color);
	}

	// Figure out start of horizontal gridlines
	start = m_y;
	if ((start-bmaporgy)%(MAPBLOCKUNITS<<FRACBITS))
		start += (MAPBLOCKUNITS<<FRACBITS)
			- ((start-bmaporgy)%(MAPBLOCKUNITS<<FRACBITS));
	end = m_y + m_h;

	// draw horizontal gridlines
	ml.a.x = m_x;
	ml.b.x = m_x + m_w;
	for (y=start; y<end; y+=(MAPBLOCKUNITS<<FRACBITS)) {
		ml.a.y = y;
		ml.b.y = y;
		if (am_rotate) {
			AM_rotatePoint (&ml.a.x, &ml.a.y);
			AM_rotatePoint (&ml.b.x, &ml.b.y);
		}
		AM_drawMline(&ml, color);
	}
}

//
// Determines visible lines, draws them.
// This is LineDef based, not LineSeg based.
//
void AM_drawWalls(void)
{
	int i, r, g, b;
	static mline_t l;
	float rdif, gdif, bdif;
	palette_t *pal = GetDefaultPalette();


	for (i=0;i<numlines;i++) {
		l.a.x = lines[i].v1->x;
		l.a.y = lines[i].v1->y;
		l.b.x = lines[i].v2->x;
		l.b.y = lines[i].v2->y;

		if (am_rotate) {
			AM_rotatePoint (&l.a.x, &l.a.y);
			AM_rotatePoint (&l.b.x, &l.b.y);
		}

		if (cheating || (lines[i].flags & ML_MAPPED))
		{
			if ((lines[i].flags & ML_DONTDRAW) && !cheating)
				continue;
            if (!lines[i].backsector &&
                (((am_usecustomcolors || viewactive) &&
                lines[i].special != Exit_Normal &&
                lines[i].special != Exit_Secret) ||
                (!am_usecustomcolors && !viewactive)))
            {
				AM_drawMline(&l, WallColor);
			}
			else
			{
				if ((lines[i].special == Teleport ||
					lines[i].special == Teleport_NoFog ||
					lines[i].special == Teleport_Line) &&
					(am_usecustomcolors || viewactive))
				{ // teleporters
					AM_drawMline(&l, TeleportColor);
				}
				else if ((lines[i].special == Teleport_NewMap ||
						 lines[i].special == Teleport_EndGame ||
						 lines[i].special == Exit_Normal ||
						 lines[i].special == Exit_Secret) &&
						 (am_usecustomcolors || viewactive))
				{ // exit
					AM_drawMline(&l, ExitColor);
				}
				else if (lines[i].flags & ML_SECRET)
				{ // secret door
					if (cheating)
						AM_drawMline(&l, SecretWallColor);
				    else
						AM_drawMline(&l, WallColor);
				}
				else if (lines[i].special == Door_LockedRaise)
				{
				    // NES - Locked doors glow from a predefined color to either blue, yellow, or red.
                    r = RPART(LockedColor), g = GPART(LockedColor), b = BPART(LockedColor);

                    if (am_usecustomcolors) {
                        if (lines[i].args[3] == (BCard | CardIsSkull)) {
                            rdif = (0 - r)/30;
                            gdif = (0 - g)/30;
                            bdif = (255 - b)/30;
                        } else if (lines[i].args[3] == (YCard | CardIsSkull)) {
                            rdif = (255 - r)/30;
                            gdif = (255 - g)/30;
                            bdif = (0 - b)/30;
                        } else {
                            rdif = (255 - r)/30;
                            gdif = (0 - g)/30;
                            bdif = (0 - b)/30;
                        }

                        if (lockglow < 30) {
                            AM_drawMline (&l, BestColor(pal->basecolors, r + ((int)rdif*lockglow),
                                          g + ((int)gdif*lockglow), b + ((int)bdif*lockglow),
                                          pal->numcolors));
                        } else if (lockglow < 60) {
                            AM_drawMline (&l, BestColor(pal->basecolors, r + ((int)rdif*(60-lockglow)),
                                          g + ((int)gdif*(60-lockglow)), b + ((int)bdif*(60-lockglow)),
                                          pal->numcolors));
                        } else {
                            AM_drawMline (&l, BestColor(pal->basecolors, r, g, b,
                                          pal->numcolors));
                        }
				    } else {
                        AM_drawMline (&l, BestColor(pal->basecolors, r, g, b,
                                      pal->numcolors));
                    }
                }
				else if (lines[i].backsector->floorheight
					  != lines[i].frontsector->floorheight)
				{
					AM_drawMline(&l, FDWallColor); // floor level change
				}
				else if (lines[i].backsector->ceilingheight
					  != lines[i].frontsector->ceilingheight)
				{
					AM_drawMline(&l, CDWallColor); // ceiling level change
				}
				else if (cheating)
				{
					AM_drawMline(&l, TSWallColor);
				}
			}
		}
		else if (consoleplayer().powers[pw_allmap])
		{
			if (!(lines[i].flags & ML_DONTDRAW))
				AM_drawMline(&l, NotSeenColor);
		}
    }
}


//
// Rotation in 2D.
// Used to rotate player arrow line character.
//
void
AM_rotate
( fixed_t*	x,
  fixed_t*	y,
  angle_t	a )
{
    fixed_t tmpx;

    tmpx =
	FixedMul(*x,finecosine[a>>ANGLETOFINESHIFT])
	- FixedMul(*y,finesine[a>>ANGLETOFINESHIFT]);

    *y   =
	FixedMul(*x,finesine[a>>ANGLETOFINESHIFT])
	+ FixedMul(*y,finecosine[a>>ANGLETOFINESHIFT]);

    *x = tmpx;
}

void AM_rotatePoint (fixed_t *x, fixed_t *y)
{
	player_t &p = displayplayer();

	*x -= p.camera->x;
	*y -= p.camera->y;
	AM_rotate (x, y, ANG90 - p.camera->angle);
	*x += p.camera->x;
	*y += p.camera->y;
}

void
AM_drawLineCharacter
( mline_t*	lineguy,
  int		lineguylines,
  fixed_t	scale,
  angle_t	angle,
  int		color,
  fixed_t	x,
  fixed_t	y )
{
	int		i;
	mline_t	l;

	for (i=0;i<lineguylines;i++) {
		l.a.x = lineguy[i].a.x;
		l.a.y = lineguy[i].a.y;

		if (scale) {
			l.a.x = FixedMul(scale, l.a.x);
			l.a.y = FixedMul(scale, l.a.y);
		}

		if (angle)
			AM_rotate(&l.a.x, &l.a.y, angle);

		l.a.x += x;
		l.a.y += y;

		l.b.x = lineguy[i].b.x;
		l.b.y = lineguy[i].b.y;

		if (scale) {
			l.b.x = FixedMul(scale, l.b.x);
			l.b.y = FixedMul(scale, l.b.y);
		}

		if (angle)
			AM_rotate(&l.b.x, &l.b.y, angle);

		l.b.x += x;
		l.b.y += y;

		AM_drawMline(&l, color);
	}
}

void AM_drawPlayers(void)
{
	angle_t angle;
	size_t i;
	player_t &conplayer = displayplayer();
	DWORD *palette;
	palette = GetDefaultPalette()->colors;

	if (!multiplayer)
	{
		if (am_rotate)
			angle = ANG90;
		else
			angle = conplayer.camera->angle;

		if (cheating)
			AM_drawLineCharacter
			(cheat_player_arrow, NUMCHEATPLYRLINES, 0,
			 angle, YourColor, conplayer.camera->x, conplayer.camera->y);
		else
			AM_drawLineCharacter
			(player_arrow, NUMPLYRLINES, 0, angle,
			 YourColor, conplayer.camera->x, conplayer.camera->y);
		return;
	}

	for (i = 0; i < players.size(); i++)
	{
		player_t *p = &players[i];
		int color = 0;
		mpoint_t pt;

		if (!players[i].ingame() || !p->mo ||
			(((sv_gametype == GM_DM && p != &conplayer) ||
			((sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF) && p->userinfo.team != conplayer.userinfo.team))
			&& !(netdemo.isPlaying() || netdemo.isPaused())
			&& !demoplayback && !(conplayer.spectator)) || p->spectator)
		{
			continue;
		}

		if (p->powers[pw_invisibility])
			color = AlmostBackground;
		else if (demoplayback && democlassic) {
			switch (i) {
				case 0: color = V_GetColorFromString (palette, "00 FF 00"); break;
				case 1: color = V_GetColorFromString (palette, "60 60 B0"); break;
				case 2: color = V_GetColorFromString (palette, "B0 B0 30"); break;
				case 3: color = V_GetColorFromString (palette, "C0 00 00"); break;
				default: break;
			}
		} else {
			int playercolor = CL_GetPlayerColor(p);
			color = BestColor (GetDefaultPalette()->basecolors,
							   RPART(playercolor),
							   GPART(playercolor),
							   BPART(playercolor),
							   GetDefaultPalette()->numcolors);
		}

		pt.x = p->mo->x;
		pt.y = p->mo->y;
		angle = p->mo->angle;

		if (am_rotate)
		{
			AM_rotatePoint (&pt.x, &pt.y);
			angle -= conplayer.camera->angle - ANG90;
		}

		AM_drawLineCharacter
			(player_arrow, NUMPLYRLINES, 0, angle,
			 color, pt.x, pt.y);
    }
}

void AM_drawThings (int color)
{
	int		 i;
	AActor*	 t;
	mpoint_t p;
	angle_t	 angle;

	for (i=0;i<numsectors;i++)
	{
		t = sectors[i].thinglist;
		while (t)
		{
			p.x = t->x;
			p.y = t->y;
			angle = t->angle;

			if (am_rotate)
			{
				AM_rotatePoint (&p.x, &p.y);
				angle += ANG90 - displayplayer().camera->angle;
			}

			AM_drawLineCharacter
			(thintriangle_guy, NUMTHINTRIANGLEGUYLINES,
			 16<<FRACBITS, angle, color, p.x, p.y);
			t = t->snext;
		}
	}
}

void AM_drawMarks (void)
{
	int i, fx, fy, w, h;
	mpoint_t pt;

	for (i = 0; i < AM_NUMMARKPOINTS; i++)
	{
		if (markpoints[i].x != -1)
		{
			//      w = LESHORT(marknums[i]->width);
			//      h = LESHORT(marknums[i]->height);
			w = 5; // because something's wrong with the wad, i guess
			h = 6; // because something's wrong with the wad, i guess

			pt.x = markpoints[i].x;
			pt.y = markpoints[i].y;

			if (am_rotate)
				AM_rotatePoint (&pt.x, &pt.y);

			fx = CXMTOF(pt.x);
			fy = CYMTOF(pt.y) - 3;

			if (fx >= f_x && fx <= f_w - w && fy >= f_y && fy <= f_h - h)
				FB->DrawPatchCleanNoMove (marknums[i], fx, fy);
		}
	}
}

void AM_drawCrosshair (int color)
{
	fb[f_p*((f_h+1)/2)+(f_w/2)] = (byte)color; // single point for now
}

void AM_Drawer (void)
{
	if (!automapactive)
		return;

	fb = screen->buffer;
	if (!viewactive)
	{
		// [RH] Set f_? here now to handle automap overlaying
		// and view size adjustments.
		f_x = f_y = 0;
		f_w = screen->width;
		f_h = ST_Y;
		f_p = screen->pitch;

		AM_clearFB(Background);
	}
	else
	{
		f_x = viewwindowx;
		f_y = viewwindowy;
		f_w = realviewwidth;
		f_h = realviewheight;
		f_p = screen->pitch;
	}
	AM_activateNewScale();

	if (grid)
		AM_drawGrid(GridColor);

	AM_drawWalls();
	AM_drawPlayers();
	if (cheating==2)
		AM_drawThings(ThingColor);

	if (!(viewactive && am_overlay < 2))
		AM_drawCrosshair(XHairColor);

	AM_drawMarks();

	if (!(viewactive && am_overlay < 2)) {

		char line[64+10];
		int OV_Y, i, time = level.time / TICRATE, height;

		height = (hu_font[0]->height() + 1) * CleanYfac;
		OV_Y = screen->height - ((32 * screen->height) / 200);

		if (sv_gametype == GM_COOP)
		{
			if (am_showmonsters)
			{
				sprintf (line, TEXTCOLOR_RED "MONSTERS:"
							   TEXTCOLOR_NORMAL " %d / %d",
							   level.killed_monsters, level.total_monsters);
                if (viewactive && screenblocks == 11)
                    FB->DrawTextClean (CR_GREY, screen->width - V_StringWidth (line) * CleanXfac, OV_Y - (height * 4) + 1, line);
                else if (viewactive && screenblocks == 12)
                    FB->DrawTextClean (CR_GREY, 0, screen->height - (height * 2) + 1, line);
                else
                    FB->DrawTextClean (CR_GREY, 0, ST_Y - (height * 2) + 1, line);
			}

			if (am_showsecrets)
			{
				sprintf (line, TEXTCOLOR_RED "SECRETS:"
							   TEXTCOLOR_NORMAL " %d / %d",
							   level.found_secrets, level.total_secrets);
                if (viewactive && screenblocks == 11)
                    FB->DrawTextClean (CR_GREY, screen->width - V_StringWidth (line) * CleanXfac, OV_Y - (height * 3) + 1, line);
                else if (viewactive && screenblocks == 12)
                    FB->DrawTextClean (CR_GREY, screen->width - V_StringWidth (line) * CleanXfac, screen->height - (height * 2) + 1, line);
                else
                    FB->DrawTextClean (CR_GREY, screen->width - V_StringWidth (line) * CleanXfac, ST_Y - (height * 2) + 1, line);
			}
		}

		if (am_classicmapstring)
		{
			int firstmap;
			int mapoffset = 1;
			switch (gamemission)
			{
				case doom2:
				firstmap = HUSTR_1;
				break;
				case pack_plut:
				firstmap = PHUSTR_1;
				break;
				case pack_tnt:
				firstmap = THUSTR_1;
				break;
				default:
				firstmap = HUSTR_E1M1;
				mapoffset = level.cluster; // Episodes skip map numbers.
				break;
			}
			strcpy(line, GStrings(firstmap + level.levelnum - mapoffset));

			if (viewactive && screenblocks == 11)
				FB->DrawTextClean(CR_RED, screen->width - V_StringWidth (line) * CleanXfac, OV_Y - (height * 1) + 1, line);
			else if (viewactive && screenblocks == 12)
				FB->DrawTextClean (CR_RED, 0, screen->height - (height * 1) + 1, line);
			else
				FB->DrawTextClean (CR_RED, 0, ST_Y - (height * 1) + 1, line);
		}
		else
		{
            line[0] = '\x8a';
            line[1] = CR_RED + 'A';
            i = 0;
            while (i < 8 && level.mapname[i]) {
                line[2 + i] = level.mapname[i];
                i++;
            }
            i += 2;
            line[i++] = ':';
            line[i++] = ' ';
            line[i++] = '\x8a';
            line[i++] = '-';
            strcpy (&line[i], level.level_name);
            if (viewactive && screenblocks == 11)
                FB->DrawTextClean (CR_GREY, screen->width - V_StringWidth (line) * CleanXfac, OV_Y - (height * 1) + 1, line);
            else if (viewactive && screenblocks == 12)
                FB->DrawTextClean (CR_GREY, 0, screen->height - (height * 1) + 1, line);
            else
                FB->DrawTextClean (CR_GREY, 0, ST_Y - (height * 1) + 1, line);
		}

		if (am_showtime) {
			sprintf (line, " %02d:%02d:%02d", time/3600, (time%3600)/60, time%60);	// Time
            if (viewactive && screenblocks == 11)
                FB->DrawTextClean (CR_RED, screen->width - V_StringWidth (line) * CleanXfac, OV_Y - (height * 2) + 1, line);
            else if (viewactive && screenblocks == 12)
                FB->DrawTextClean (CR_RED, screen->width - V_StringWidth (line) * CleanXfac, screen->height - (height * 1) + 1, line);
            else
                FB->DrawTextClean (CR_RED, screen->width - V_StringWidth (line) * CleanXfac, ST_Y - (height * 1) + 1, line);
		}

	}
}

VERSION_CONTROL (am_map_cpp, "$Id$")


